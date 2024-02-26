void display_recording_metadata(const SharedRecordingPtr &rec) {
  if (!rec->date().empty()) ImGui::TextWrapped("Date: %s", rec->date().c_str());
  if (!rec->comment().empty()) {
    ImGui::TextWrapped("Comment: %s", rec->comment().c_str());
    if (rec->file()->capabilities()[FileCapabilities::SET_COMMENT]) {
      ImGui::SameLine();
      if (ImGui::SmallButton(ICON_FA_EDIT)) {
        rec->comment_edit_ui.comment = rec->comment();
        rec->comment_edit_ui.show    = true;
      }

      if (rec->comment_edit_ui.show) {
        auto window_name = fmt::format("Edit Comment: {}", rec->name());
        ImGui::Begin(window_name.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::InputText("##comment", &rec->comment_edit_ui.comment);
        if (ImGui::Button("Save")) {
          rec->file()->set_comment(rec->comment_edit_ui.comment);
          rec->comment_edit_ui.show = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
          rec->comment_edit_ui.show    = false;
          rec->comment_edit_ui.comment = rec->comment();
        }
        ImGui::End();
      }
    }
  }
  ImGui::Columns(3);
  ImGui::Text("Frames %d", rec->length());
  ImGui::NextColumn();
  ImGui::Text("Width  %d", rec->Nx());
  ImGui::NextColumn();
  ImGui::Text("Height %d", rec->Ny());
  ImGui::NextColumn();
  if (rec->duration().count() > 0) {
    ImGui::TextWrapped("Duration  %.3fs", rec->duration().count());
    ImGui::NextColumn();
  }
  if (rec->fps() != 0) {
    ImGui::TextWrapped("FPS  %.3f", rec->fps());
    ImGui::NextColumn();
  }
  for (const auto &v : rec->file()->metadata()) {
    ImGui::TextWrapped("%s %s", v.first.c_str(), v.second.c_str());
    ImGui::NextColumn();
  }
  ImGui::Columns(1);
}

bool display_recording_buttons(const SharedRecordingPtr &rec, RecordingWindow *parent) {
  if (!parent) {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Export to video file: ");
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FILE_EXPORT u8" " ICON_FA_VIDEO)) {
      auto &ctrl         = rec->export_ctrl.video;
      ctrl.export_window = true;
      ctrl.assign_auto_filename(rec->path());
      ctrl.tend        = rec->length();
      ctrl.description = rec->comment();
    }
    ImGui::Text("Export as image sequence: ");
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FILE_EXPORT u8" " ICON_FA_FILE_IMAGE)) {
      auto &ctrl         = rec->export_ctrl.png;
      ctrl.export_window = true;
      ctrl.assign_auto_filename(rec->path());
    }
    ImGui::Text("Export as raw video: ");
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FILE_EXPORT u8" raw")) {
      auto &ctrl         = rec->export_ctrl.raw;
      ctrl.export_window = true;
      ctrl.start         = {0, 0};
      ctrl.size          = {rec->Nx(), rec->Ny()};
      ctrl.frames        = {0, rec->length()};
      ctrl.assign_auto_filename(rec->path());
    }
    if (global::recordings.size() > 1) {
      ImGui::SameLine();
      if (ImGui::Button(ICON_FA_LAYER_GROUP)) ImGui::OpenPopup("merge_popup");
      if (ImGui::BeginPopup("merge_popup")) {
        for (auto &r : global::recordings) {
          if (r.get() == rec.get()) continue;
          auto l = fmt::format("Merge into '{}'", r->name());
          if (ImGui::Selectable(l.c_str())) {
            global::merge_queue.push({rec, r, false});
          }
          if (rec->file()->capabilities()[FileCapabilities::AS_FLOW]) {
            auto l2 = l + " as flow"s;
            if (ImGui::Selectable(l2.c_str())) {
              global::merge_queue.push({rec, r, true});
            }
          }
        }
        ImGui::EndPopup();
      }
    }
    return false;
  } else {
    if (ImGui::Button(ICON_FA_TRASH_ALT)) {
      rec->set_context(nullptr);
      // Child will be deleted later, after we have left the loop over all children.
      return true;
    }
  }
  return false;
}

void show_histogram_ui(const SharedRecordingPtr &rec, RecordingWindow *parent) {
  // Histogram and other controls
  ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
  ImGui::PlotHistogram("##histogram", rec->histogram.data.data(), rec->histogram.data.size(), 0,
                       nullptr, 0, rec->histogram.max_value(), ImVec2(0, 100));
  ImGui::SameLine();
  {
    ImGui::BeginGroup();
    ImGui::Text("Histogram");
    ImGui::NewLine();
    // Bitrange switcher
    {
      int item = static_cast<int>(rec->bitrange);
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
      ImGui::Combo("Format", &item, BitRangeNames, IM_ARRAYSIZE(BitRangeNames));
      if (auto br = static_cast<BitRange>(item); rec->bitrange != br) {
        rec->bitrange      = br;
        float &min         = rec->get_min(Transformations::None);
        float &max         = rec->get_max(Transformations::None);
        std::tie(min, max) = utils::bitrange_to_float(br);
      }
    }
    // Colormap switcher
    {
      ColorMap current_cmap = rec->colormap();
      auto itemwidth        = ImGui::GetContentRegionAvail().x * 0.5f;
      ImGui::SetNextItemWidth(itemwidth);
      ImVec2 combo_pos = ImGui::GetCursorScreenPos();
      if (ImGui::BeginCombo("Colormap", "")) {
        for (auto [cmap, tex_id] : prm::cmap_texs) {
          auto l = ColorMapsNames[static_cast<int>(cmap)];
          ImGui::PushID(l);
          bool is_selected = (cmap == current_cmap);
          if (ImGui::Selectable("", is_selected) && (cmap != current_cmap)) {
            rec->colormap(cmap);
          }
          ImGui::SameLine();
          ImGui::Image((void *)(intptr_t)tex_id, ImVec2(80, ImGui::GetTextLineHeight()));
          ImGui::SameLine();
          ImGui::Text("%s", l);
          ImGui::SameLine();
          ImGui::Spacing();
          ImGui::PopID();
        }
        ImGui::EndCombo();
      }
      const auto style = ImGui::GetStyle();
      ImGui::SetCursorScreenPos(
          ImVec2(combo_pos.x + style.FramePadding.x, combo_pos.y + style.FramePadding.y));
      const ImVec2 i_size = {itemwidth - ImGui::GetFrameHeight() - 2.5f * style.FramePadding.x,
                             ImGui::GetTextLineHeight()};
      ImGui::Image((void *)(intptr_t)prm::cmap_texs[current_cmap], i_size);
    }
    if (parent) {
      ImGui::Checkbox("Overlay", &rec->as_overlay);
      if (rec->as_overlay) {
        auto itemwidth = ImGui::GetContentRegionAvail().x * 0.5f;
        ImGui::SetNextItemWidth(itemwidth);
        ImGui::Combo("Overlay Method", &rec->overlay_method, OverlayMethodNames,
                     IM_ARRAYSIZE(OverlayMethodNames));
      }
    }
    ImGui::EndGroup();
  }

  // Controls for min and max values
  bool symmetrize_minmax = false;
  if (ImGui::SliderFloat("min", &rec->get_min(prm::transformation), rec->histogram.min,
                         rec->histogram.max)) {
    symmetrize_minmax =
        (rec->histogram.symmetric || prm::transformation == Transformations::FrameDiff);
  }
  if (rec->get_min(prm::transformation) < 0) {
    ImGui::SameLine();
    if (ImGui::Checkbox("Symmetric", &rec->histogram.symmetric)) {
      symmetrize_minmax = rec->histogram.symmetric;
    }
  }
  if (ImGui::SliderFloat("max", &rec->get_max(prm::transformation), rec->histogram.min,
                         rec->histogram.max)) {
    if (prm::transformation == Transformations::FrameDiff) {
      rec->get_min(prm::transformation) = -rec->get_max(prm::transformation);
    }
  }
  if (symmetrize_minmax) {
    rec->get_max(prm::transformation) = -rec->get_min(prm::transformation);
  }
}

void show_traces_ui(const SharedRecordingPtr &rec) {
  ImGui::BeginTabBar("##traces");

  if (ImGui::BeginTabItem("View")) {
    for (auto &trace : rec->traces) {
      ImGui::PushID(trace.id);

      int size = trace.data.size();
      if (size == 0) {
        ImGui::PopID();
        ImGui::EndTabItem();
        continue;
      }

      auto label = trace.pos.to_string();
      auto data = trace.data.data();
      if (size > prm::trace_length) {
        data += (size - prm::trace_length);
        size = prm::trace_length;
      }

      trace.scale.left  = data - trace.data.data();
      trace.scale.right = trace.data.size();
      trace.scale.scale(data, data + size);
      ImPlot::SetNextAxisLinks(ImAxis_X1, trace.scale.scaleX ? &trace.scale.left : nullptr,
                              trace.scale.scaleX ? &trace.scale.right : nullptr);
      ImPlot::SetNextAxisLinks(ImAxis_Y1, trace.scale.scaleY ? &trace.scale.lower : nullptr,
                              trace.scale.scaleY ? &trace.scale.upper : nullptr);
      auto ptitle = fmt::format("Pos {}###trace", label);
      // auto plot_size = ImVec2(ImGui::GetContentRegionAvail().x * 0.85f, 180);
      auto plot_size = ImVec2(ImGui::GetContentRegionAvail().x, 180);
      if (ImPlot::BeginPlot(ptitle.c_str(), plot_size)) {
        ImPlot::SetupAxes(nullptr, nullptr);
        ImPlot::SetNextLineStyle({trace.color[0], trace.color[1], trace.color[2], trace.color[3]});
        auto title = "###ttrace" + label;
        ImPlot::PlotLine(title.c_str(), trace.data.data(), trace.data.size());
        ImPlotUtils::draw_liney(trace.scale.restarts);
        ImPlot::EndPlot();
      }

      // ImGui::SameLine();
      // ImGui::BeginGroup();
      // ImGui::AlignTextToFramePadding();
      // ImGui::Text("Trace ");
      // ImGui::SameLine();
      // ImGui::ColorEdit3(label.c_str(), trace.color.data(),
      //                   ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
      // ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.95f);
      // if (ImGui::InputInt2("###Trace", trace.pos.data())) {
      //   trace.pos[0] = std::clamp(trace.pos[0], 0, rec->Nx());
      //   trace.pos[1] = std::clamp(trace.pos[1], 0, rec->Ny());
      // }
      // ImGui::EndGroup();
      ImGui::PopID();
    }
    ImGui::EndTabItem();
  }

  if (ImGui::BeginTabItem("Settings")) {
    {
      int trace_width = Trace::width();
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
      if (ImGui::InputInt("ROI width [px]", &trace_width, 2, 5)) {
        Trace::width(trace_width);
      }
    }
    {
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
      ImGui::SliderInt("Trace length [frames]", &prm::trace_length, 10, prm::max_trace_length);
    }

    for (auto &trace : rec->traces) {
      ImGui::PushID(trace.id);
      auto title = "Pos " + trace.pos.to_string();
      if (ImGui::CollapsingHeader(title.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button(u8"Delete " ICON_FA_TRASH_ALT)) {
          rec->remove_trace(trace.pos);
        }

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.3f);
        if (ImGui::InputInt2("Center position", trace.pos.data())) {
          trace.pos[0] = std::clamp(trace.pos[0], 0, rec->Nx());
          trace.pos[1] = std::clamp(trace.pos[1], 0, rec->Ny());
        }

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.3f);
        ImGui::ColorEdit3("Color", trace.color.data());

        ImGui::NewLine();
        if (ImGui::Button("Export video trace as .txt file")) {
          auto &ctrl         = rec->export_ctrl.trace;
          ctrl.export_window = true;
          ctrl.assign_auto_filename(rec->path(), trace.pos, Trace::width());
          ctrl.tend = rec->length();
        }
        if (ImGui::Button("Export ROI as .npy file")) {
          auto &ctrl                      = rec->export_ctrl.raw;
          ctrl.export_window              = true;
          std::tie(ctrl.start, ctrl.size) = Trace::clamp(trace.pos, {rec->Nx(), rec->Ny()});
          ctrl.frames                     = {0, rec->length()};
          ctrl.assign_auto_filename(rec->path());
        }
        ImGui::NewLine();
        if (ImGui::Button("Reset data")) {
          trace.data.clear();
        }
        ImGui::NewLine();
        ImGui::Checkbox("Auto scale x-axis", &trace.scale.scaleX);
        ImGui::Checkbox("Auto scale y-axis", &trace.scale.scaleY);
      }
    }
    ImGui::PopID();
    ImGui::EndTabItem();
  }
  ImGui::EndTabBar();
}

void show_flow_ui(const SharedRecordingPtr &rec) {
  for (auto &flow : rec->flows) {
    ImGui::PushID(flow.data.get());
    ImGui::Text("Flow %s", flow.data->name().c_str());
    ImGui::SameLine();
    ImGui::ColorEdit4("", flow.color.data(),
                      ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    ImGui::SameLine();
    if (flow.show)
      flow.show = !ImGui::Button(ICON_FA_EYE_SLASH);
    else
      flow.show = ImGui::Button(ICON_FA_EYE);
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_TRASH_ALT)) {
      flow.data = nullptr;
    }
    ImGui::PopID();
  }
  // Actually remove deleted flows
  rec->flows.erase(
      std::remove_if(rec->flows.begin(), rec->flows.end(), [](const auto &f) { return !f.data; }),
      rec->flows.end());
  ImGui::Columns(2);
  ImGui::SliderInt("flow skip", &FlowData::skip, 1, 25);
  ImGui::NextColumn();
  ImGui::SliderFloat("flow point size", &FlowData::pointsize, 0, 10);
  ImGui::Columns(1);
}

void show_points_ui(const SharedRecordingPtr &rec) {
  for (const auto &vid : rec->points_videos) {
    ImGui::PushID(vid.get());
    ImGui::Text("%s", vid->name.c_str());
    ImGui::SliderFloat("point size", &vid->point_size, 0, 10);
    ImGui::SameLine();
    if (vid->show)
      vid->show = !ImGui::Button(ICON_FA_EYE_SLASH);
    else
      vid->show = ImGui::Button(ICON_FA_EYE);
    ImGui::SameLine();
    ImGui::ColorEdit4("", vid->color.data(),
                      ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    ImGui::Separator();
    ImGui::PopID();
  }
}

void show_transformations_tab() {
  auto selectable_factory = [](auto &p, auto default_val) {
    return [&p, default_val](const char *label, auto e) {
      bool is_active = p == e;
      if (ImGui::Selectable(label, is_active)) {

        p = is_active ? default_val : e;

        global::do_forall_recordings([](auto &r) { r->reset_traces(); });
      }

      return is_active;
    };
  };

  auto kernel_size_select = [](unsigned int &val, auto reset_fn) {
    ImGui::Indent(10);
    const int step = 2;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
    if (ImGui::InputScalar("Kernel size", ImGuiDataType_U32, &val, &step, nullptr, "%d")) {
      global::do_forall_recordings([&reset_fn](auto &r) { reset_fn(r.get()); });
    }
    ImGui::Unindent(10);
  };

  ImGui::Columns(3);
  ImGui::SetNextItemOpen(true, ImGuiCond_Once);
  if (ImGui::TreeNode("Pre Filters")) {
    auto selectable = selectable_factory(prm::prefilter, Filters::None);
    if (selectable("Gauss", Filters::Gauss)) {
      ImGui::Indent(10);
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
      float sigma = Transformation::GaussFilter::get_sigma();
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 1.f);
      if (ImGui::DragFloat("##sigma", &sigma, 0.01, 0, 5, u8"σ = %.2f")) {
        Transformation::GaussFilter::set_sigma(sigma);
      }
      ImGui::Unindent(10);
    }
    if (selectable("Mean", Filters::Mean)) {
      kernel_size_select(Transformation::MeanFilter::kernel_size, [](RecordingWindow *r) {
        auto transform = r->transformationArena.create_if_needed(Filters::Mean, 0);
        auto c         = dynamic_cast<Transformation::MeanFilter *>(transform);
        assert(c);
        c->reset();
      });
    }
    if (selectable("Median", Filters::Median)) {
      kernel_size_select(Transformation::MedianFilter::kernel_size, [](RecordingWindow *r) {
        auto transform = r->transformationArena.create_if_needed(Filters::Median, 0);
        auto c         = dynamic_cast<Transformation::MedianFilter *>(transform);
        assert(c);
        c->reset();
      });
    }
    ImGui::TreePop();
  }

  ImGui::NextColumn();

  ImGui::SetNextItemOpen(true, ImGuiCond_Once);
  if (ImGui::TreeNode("Transformations")) {
    auto selectable = selectable_factory(prm::transformation, Transformations::None);
    if (selectable("Frame Difference", Transformations::FrameDiff)) {
      ImGui::Indent(10);
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
      ImGui::SliderInt("Frames", &Transformation::FrameDiff::n_frame_diff, 1, 100);
      if (ImGui::Button("Add As Overlays")) {
        std::vector<std::pair<SharedRecordingPtr, SharedRecordingPtr>> new_recordings;
        for (auto rec : global::recordings) {
          auto r = std::make_shared<FixedTransformRecordingWindow>(
              rec, prm::prefilter, prm::transformation, prm::postfilter, "Frame Difference");
          new_recordings.push_back({r, rec});
        }
        for (auto [r, rec] : new_recordings) {
          global::recordings.push_back(r);
          r->open_window();
          global::merge_queue.push({r, rec, false});
        }
        prm::transformation = Transformations::None;
        prm::prefilter      = Filters::None;
        prm::postfilter     = Filters::None;
      }
      ImGui::Unindent(10);
    }
    if (selectable("Contrast Enhancement", Transformations::ContrastEnhancement)) {
      ImGui::Indent(10);
      const int step = 2;
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
      if (ImGui::InputScalar("Kernel size", ImGuiDataType_U32,
                            &Transformation::ContrastEnhancement::kernel_size, &step, nullptr,
                            "%d")) {
        global::do_forall_recordings([](auto &r) {
          auto transform =
              r->transformationArena.create_if_needed(Transformations::ContrastEnhancement, 0);
          auto c = dynamic_cast<Transformation::ContrastEnhancement *>(transform);
          assert(c);
          c->reset();
        });
      }
//      ImGui::SliderInt("Mask", &Transformation::ContrastEnhancement::maskVersion, 0, 2);
      ImGui::Unindent(10);
    }
    ImGui::TreePop();
  }

  ImGui::NextColumn();

  ImGui::SetNextItemOpen(true, ImGuiCond_Once);
  if (ImGui::TreeNode("Post Filters")) {
    auto selectable = selectable_factory(prm::postfilter, Filters::None);
    if (selectable("Gauss", Filters::Gauss)) {
      ImGui::Indent(10);
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
      float sigma = Transformation::GaussFilter::get_sigma();
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 1.f);
      if (ImGui::DragFloat("##sigma", &sigma, 0.01, 0, 5, u8"σ = %.2f")) {
        Transformation::GaussFilter::set_sigma(sigma);
      }
      ImGui::Unindent(10);
    }
    if (selectable("Mean", Filters::Mean)) {
      kernel_size_select(Transformation::MeanFilter::kernel_size, [](RecordingWindow *r) {
        auto transform = r->transformationArena.create_if_needed(Filters::Mean, 1);
        auto c         = dynamic_cast<Transformation::MeanFilter *>(transform);
        assert(c);
        c->reset();
      });
    }
    if (selectable("Median", Filters::Median)) {
      kernel_size_select(Transformation::MedianFilter::kernel_size, [](RecordingWindow *r) {
        auto transform = r->transformationArena.create_if_needed(Filters::Median, 1);
        auto c         = dynamic_cast<Transformation::MedianFilter *>(transform);
        assert(c);
        c->reset();
      });
    }
    ImGui::TreePop();
  }
  ImGui::Columns(1);
}

int show_recording_ui(const SharedRecordingPtr &rec, int rec_nr, RecordingWindow *parent = nullptr) {
  auto name = fmt::format("{}###{}", rec->name(), static_cast<void *>(rec.get()));
  if (ImGui::CollapsingHeader(name.c_str(),
                              ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth)) {
    if (!rec->active) {
      // layer was just activated
      rec->active   = true;
      if (!parent) {
        glfwShowWindow(rec->window);
      } else {
        rec->playback = parent->playback;
      }
    }
  } else {
    rec->active = false;
    if (!parent) {
      glfwHideWindow(rec->window);
    }
    return rec_nr;
  }
  ImGui::PushID(rec.get());

  int t = rec->current_frame();
  ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogram));
  ImGui::SetNextItemWidth(-1);
  if (ImGui::SliderInt("##progress", &t, 0, rec->length() - 1, "Frame %d")) {
    rec->playback.set_next(t);
    for (auto &c : rec->children) {
      c->playback.set_next(t);
    }
  }
  ImGui::PopStyleColor(1);

  if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_AutoSelectNewTabs)) {
    if (ImGui::BeginTabItem("Histogram")) {
      show_histogram_ui(rec, parent);
      ImGui::EndTabItem();
    }

    // Controls for flows
    if (!rec->flows.empty() && ImGui::BeginTabItem("Flows")) {
      show_flow_ui(rec);
      ImGui::EndTabItem();
    }

    if (!rec->points_videos.empty() && ImGui::BeginTabItem("Points")) {
      show_points_ui(rec);
      ImGui::EndTabItem();
    }

    // Plot traces and show their controls
    if (!rec->traces.empty() && ImGui::BeginTabItem("Traces")) {
      show_traces_ui(rec);
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Metadata & Controls")) {
      auto rec_name = rec->name();
      if (ImGui::InputText("Name", &rec_name)) {
        rec->set_name(rec_name);
      }
      ImGui::Checkbox("Show", &rec->active);

      ImGui::Separator();

      display_recording_metadata(rec);

      ImGui::Separator();

      if (display_recording_buttons(rec, parent)) {
        ImGui::Columns(1);
        ImGui::PopID();
        ImGui::EndTabItem();
        ImGui::EndTabBar();
        return rec_nr;
      }

      ImGui::Separator();
      show_transformations_tab();

      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }

  ImGui::PopID();
  for (const auto &crec : rec->children) {
    rec_nr = show_recording_ui(crec, rec_nr, rec.get());
  }
  // Actually delete children which have been selected for deletionq
  rec->children.erase(std::remove_if(rec->children.begin(), rec->children.end(),
                                     [](const auto &r) { return r->glcontext == r->window; }),
                      rec->children.end());
  return rec_nr + 1;
}
