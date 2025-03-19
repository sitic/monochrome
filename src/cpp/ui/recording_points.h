void show_flow_ui(const SharedRecordingPtr &rec) {
  ImGui::SeparatorText("Global Settings");
  ImGui::SliderInt("Points skip", &FlowData::skip, 1, 25);
  ImGui::SliderFloat("Point size", &FlowData::pointsize, 0, 10);
  ImGui::Spacing();
  ImGui::SeparatorText("Individual Settings");
  for (auto &flow : rec->flows) {
    ImGui::PushID(flow.data.get());

    std::string name = fmt::format("{}###{}", flow.data->name(), static_cast<void *>(flow.data.get()));
    if (ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
      auto flow_name    = flow.data->name();
      if (ImGui::InputText("Name", &flow_name)) {
        flow.data->set_name(flow_name);
      }
      ImGui::ColorEdit4("Color", flow.color.data());
      if (flow.show)
        flow.show = !ImGui::Button(u8"Hide " ICON_FA_EYE_SLASH, ImVec2(ImGui::GetItemRectSize().x, 0));
      else
        flow.show = ImGui::Button(u8"Show " ICON_FA_EYE, ImVec2(ImGui::GetItemRectSize().x, 0));
      if (ImGui::Button(u8"Close " ICON_FA_TRASH_ALT, ImVec2(ImGui::GetItemRectSize().x, 0))) {
        flow.data = nullptr;
      }

      ImGui::TreePop();
    }
    ImGui::PopID();
  }
  // Actually remove deleted flows
  rec->flows.erase(
      std::remove_if(rec->flows.begin(), rec->flows.end(), [](const auto &f) { return !f.data; }),
      rec->flows.end());
}

void show_points_ui(const SharedRecordingPtr &rec) {
  for (const auto &vid : rec->points_videos) {
    ImGui::PushID(vid.get());
    std::string name = fmt::format("{}###{}", vid->name, static_cast<void *>(rec.get()));
    if (ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::InputText("Name", &vid->name);
      if (vid->show)
        vid->show = !ImGui::Button(u8"Hide " ICON_FA_EYE_SLASH);
      else
        vid->show = ImGui::Button(u8"Show " ICON_FA_EYE);
      ImGui::ColorEdit4("Color", vid->color.data());
      ImGui::SliderFloat("Point size", &vid->point_size, 0, 10);

      ImGui::TreePop();
    }
    ImGui::PopID();
  }
}