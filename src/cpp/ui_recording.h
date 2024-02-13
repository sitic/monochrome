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
    if (ImGui::Button(ICON_FA_FILE_EXPORT u8" raw")) {
      auto &ctrl         = rec->export_ctrl.raw;
      ctrl.export_window = true;
      ctrl.start         = {0, 0};
      ctrl.size          = {rec->Nx(), rec->Ny()};
      ctrl.frames        = {0, rec->length()};
      ctrl.assign_auto_filename(rec->path());
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FILE_EXPORT u8" " ICON_FA_VIDEO)) {
      auto &ctrl         = rec->export_ctrl.video;
      ctrl.export_window = true;
      ctrl.assign_auto_filename(rec->path());
      ctrl.tend        = rec->length();
      ctrl.description = rec->comment();
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FILE_EXPORT u8" " ICON_FA_FILE_IMAGE)) {
      auto &ctrl         = rec->export_ctrl.png;
      ctrl.export_window = true;
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
      ImGui::Columns(1);
      ImGui::PopID();
      return true;
    }
  }
  return false;
}

void display_histogram(const SharedRecordingPtr &rec, RecordingWindow *parent) {
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

int show_recording_ui(const SharedRecordingPtr &rec, int rec_nr, RecordingWindow *parent = nullptr) {
  auto name = fmt::format("{}###{}", rec->name(), static_cast<void *>(rec.get()));
  if (!parent) {
    // TODO: a GUI rewrite should be done to make this more elegant
    int x        = std::clamp(rec_nr / 3, 0, prm::main_window_multipier - 1);
    float y      = (rec_nr % 3) * 0.3f + 0.2f * (x == 0);
    auto vp_size = ImGui::GetMainViewport()->Size;
    vp_size[0] /= static_cast<float>(prm::main_window_multipier);
    auto window_pos    = ImVec2(x * vp_size[0], y * vp_size[1]);
    float window_width = vp_size[0];
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(window_width, 0), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::SetNextWindowCollapsed(!rec->active, ImGuiCond_Always);
    if (ImGui::Begin(name.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      if (!rec->active) {
        // recording was enabled
        rec->active = true;
        glfwShowWindow(rec->window);
      }
    } else {
      if (rec->active) {
        // recording was disabled
        rec->active = false;
        glfwHideWindow(rec->window);
      }
    }
  } else {
    if (ImGui::CollapsingHeader(name.c_str(),
                                ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth)) {
      if (!rec->active) {
        // layer was just activated
        rec->active   = true;
        rec->playback = parent->playback;
      }
    } else {
      rec->active = false;
      return rec_nr;
    }
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

  display_recording_metadata(rec);

  if (display_recording_buttons(rec, parent)) {
    return rec_nr;
  }

  display_histogram(rec, parent);

  // Controls for flows
  if (!rec->flows.empty()) {
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

  if (!rec->points_videos.empty()) {
    ImGui::Separator();
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

  // Plot traces and show their controls
  for (auto &trace : rec->traces) {
    int size = trace.data.size();
    if (size == 0) continue;

    auto label = trace.pos.to_string();
    ImGui::PushID(trace.id);
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
    auto ptitle = "###trace" + label;
    if (ImPlot::BeginPlot(ptitle.c_str(), ImVec2(ImGui::GetContentRegionAvail().x * 0.85f, 180),
                          ImPlotFlags_AntiAliased)) {
      ImPlot::SetupAxes(nullptr, nullptr);
      ImPlot::SetNextLineStyle({trace.color[0], trace.color[1], trace.color[2], trace.color[3]});
      auto title = "###ttrace" + label;
      ImPlot::PlotLine(title.c_str(), trace.data.data(), trace.data.size());
      ImPlotUtils::draw_liney(trace.scale.restarts);
      ImPlot::EndPlot();
    }

    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text("Trace ");
    ImGui::SameLine();
    ImGui::ColorEdit3(label.c_str(), trace.color.data(),
                      ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.95f);
    if (ImGui::InputInt2("###Trace", trace.pos.data())) {
      trace.pos[0] = std::clamp(trace.pos[0], 0, rec->Nx());
      trace.pos[1] = std::clamp(trace.pos[1], 0, rec->Ny());
    }
    ImGui::Checkbox("Scale X", &trace.scale.scaleX);
    ImGui::Checkbox("Scale Y", &trace.scale.scaleY);
    if (ImGui::Button("Reset")) {
      trace.data.clear();
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_TRASH_ALT)) {
      rec->remove_trace(trace.pos);
    }
    if (ImGui::Button("Export Trace")) {
      auto &ctrl         = rec->export_ctrl.trace;
      ctrl.export_window = true;
      ctrl.assign_auto_filename(rec->path(), trace.pos, Trace::width());
      ctrl.tend = rec->length();
    }
    if (ImGui::Button("Export ROI")) {
      auto &ctrl                      = rec->export_ctrl.raw;
      ctrl.export_window              = true;
      std::tie(ctrl.start, ctrl.size) = Trace::clamp(trace.pos, {rec->Nx(), rec->Ny()});
      ctrl.frames                     = {0, rec->length()};
      ctrl.assign_auto_filename(rec->path());
    }
    ImGui::EndGroup();
    ImGui::PopID();
  }
  ImGui::PopID();
  for (const auto &crec : rec->children) {
    rec_nr = show_recording_ui(crec, rec_nr, rec.get());
  }
  // Actually delete children which have been selected for deletionq
  rec->children.erase(std::remove_if(rec->children.begin(), rec->children.end(),
                                     [](const auto &r) { return r->glcontext == r->window; }),
                      rec->children.end());
  if (!parent) {
    ImGui::End();
  }
  return rec_nr + 1;
}
