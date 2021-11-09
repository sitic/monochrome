int show_recording_ui(const SharedRecordingPtr &rec, int rec_nr, RecordingWindow *parent = nullptr) {
  auto name = fmt::format("{}###{}", rec->name(), static_cast<void *>(rec.get()));
  if (!parent) {
    int x           = std::clamp(rec_nr / 3, 0, prm::main_window_multipier - 1);
    float y         = (rec_nr % 3) * 0.3f + 0.2f * (x == 0);
    auto window_pos = ImVec2(x * prm::main_window_width, y * prm::main_window_height);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(prm::main_window_width, 0), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::SetNextWindowCollapsed(!rec->active, ImGuiCond_Always);
    rec->active = ImGui::Begin(name.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);
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
    if (t < rec->length()) {
      rec->playback.set_next(t);
    }
  }
  ImGui::PopStyleColor(1);

  // Show metadata
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
  } else {
    if (ImGui::Button(ICON_FA_TRASH_ALT)) {
      rec->set_context(nullptr);
      // Child will be deleted later, after we have left the loop over all children.
      ImGui::Columns(1);
      ImGui::PopID();
      return rec_nr;
    }
  }
  ImGui::Columns(1);


  // Histogram and other controls
  ImGui::PushItemWidth(prm::main_window_width * 0.7f);
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
      ImGui::SliderFloat("point size", &vid->point_size, 0, 10);
      ImGui::SameLine();
      ImGui::ColorEdit4("", vid->color.data(),
                        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
      ImGui::Separator();
      ImGui::PopID();
    }
  }

  // Plot traces and show their controls
  for (auto &[trace, pos, color, scale] : rec->traces) {
    int size = trace.size();
    if (size == 0) continue;

    auto label = pos.to_string();
    ImGui::PushID(label.c_str());
    auto data = trace.data();
    if (size > prm::trace_length) {
      data += (size - prm::trace_length);
      size = prm::trace_length;
    }

    scale.left  = data - trace.data();
    scale.right = trace.size();
    scale.scale(data, data + size);
    ImPlot::SetNextAxisLinks(ImAxis_X1, scale.scaleX ? &scale.left : nullptr,
                             scale.scaleX ? &scale.right : nullptr);
    ImPlot::SetNextAxisLinks(ImAxis_Y1, scale.scaleY ? &scale.lower : nullptr,
                             scale.scaleY ? &scale.upper : nullptr);
    auto ptitle = "###trace" + label;
    if (ImPlot::BeginPlot(ptitle.c_str(), nullptr, nullptr,
                          ImVec2(ImGui::GetContentRegionAvail().x * 0.85f, 180),
                          ImPlotFlags_AntiAliased)) {
      ImPlot::SetNextLineStyle({color[0], color[1], color[2], color[3]});
      auto title = "###ttrace" + label;
      ImPlot::PlotLine(title.c_str(), trace.data(), trace.size());
      ImPlotUtils::draw_liney(scale.restarts);
      ImPlot::EndPlot();
    }

    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text("%s", label.c_str());
    ImGui::SameLine();
    ImGui::ColorEdit3(label.c_str(), color.data(),
                      ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    ImGui::Checkbox("Scale X", &scale.scaleX);
    ImGui::Checkbox("Scale Y", &scale.scaleY);
    if (ImGui::Button("Reset")) {
      trace.clear();
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_TRASH_ALT)) {
      rec->remove_trace(pos);
    }
    if (ImGui::Button("Export Trace")) {
      auto &ctrl         = rec->export_ctrl.trace;
      ctrl.export_window = true;
      ctrl.assign_auto_filename(rec->path(), pos, Trace::width());
      ctrl.tend = rec->length();
    }
    if (ImGui::Button("Export ROI")) {
      auto &ctrl                      = rec->export_ctrl.raw;
      ctrl.export_window              = true;
      std::tie(ctrl.start, ctrl.size) = Trace::clamp(pos, {rec->Nx(), rec->Ny()});
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
