#pragma once

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
  if (ImGui::SliderFloat("min", &rec->get_min(rec->transformation), rec->histogram.min,
                         rec->histogram.max)) {
    symmetrize_minmax =
        (rec->histogram.symmetric || rec->transformation == Transformations::FrameDiff);
  }
  if (rec->get_min(rec->transformation) < 0) {
    ImGui::SameLine();
    if (ImGui::Checkbox("Symmetric", &rec->histogram.symmetric)) {
      symmetrize_minmax = rec->histogram.symmetric;
    }
  }
  if (ImGui::SliderFloat("max", &rec->get_max(rec->transformation), rec->histogram.min,
                         rec->histogram.max)) {
    if (rec->transformation == Transformations::FrameDiff) {
      rec->get_min(rec->transformation) = -rec->get_max(rec->transformation);
    }
  }
  if (symmetrize_minmax) {
    rec->get_max(rec->transformation) = -rec->get_min(rec->transformation);
  }
}