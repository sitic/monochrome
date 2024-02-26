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