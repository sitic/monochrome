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
        rec->bitrange = br;
        if (br != BitRange::NONE) {
          float &min         = rec->get_min();
          float &max         = rec->get_max();
          std::tie(min, max) = utils::bitrange_to_float(br);
        } else {
          std::tie(rec->get_min(), rec->get_max()) = oportunistic_minmax(rec->file());
          rec->histogram.min = rec->get_min();
          rec->histogram.max = rec->get_max();
        }
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
      auto itemwidth = ImGui::GetContentRegionAvail().x * 0.5f;
      ImGui::SetNextItemWidth(itemwidth);
      int opacity = static_cast<int>(rec->opacity);
      ImGui::Combo("Opacity", &opacity, OpacityFunctionNames, IM_ARRAYSIZE(OpacityFunctionNames));
      rec->opacity = static_cast<OpacityFunction>(opacity);
    }
    ImGui::EndGroup();
  }

  // Controls for min and max values
  bool symmetrize_minmax = rec->histogram.symmetric;
  float old_min          = rec->get_min();
  float old_max          = rec->get_max();
  ImGui::SliderFloat("min", &rec->get_min(), rec->histogram.min, rec->histogram.max);
  if (rec->bitrange != BitRange::NONE) {
    ImGui::SameLine();
    if (ImGui::Button("Auto")) {
      std::tie(rec->get_min(), rec->get_max()) = oportunistic_minmax(rec->file());
    }
  }
  ImGui::SliderFloat("max", &rec->get_max(), rec->histogram.min, rec->histogram.max);
  if (rec->histogram.symmetric || rec->get_min() < 0 || rec->histogram.min < 0) {
    ImGui::SameLine();
    if (ImGui::Checkbox("Symmetric", &rec->histogram.symmetric)) {
      if (rec->histogram.symmetric) { // enabled
        rec->get_min() = -rec->get_max();
      }
    }

    if (rec->histogram.symmetric) {
      if (old_min != rec->get_min())
        rec->get_max() = -rec->get_min();
      else if (old_max != rec->get_max())
        rec->get_min() = -rec->get_max();
    }
  }
}