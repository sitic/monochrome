void show_flow_ui(const SharedRecordingPtr &rec) {
  ImGui::SliderInt("Points skip", &FlowData::skip, 1, 25);
  ImGui::SliderFloat("Point size", &FlowData::pointsize, 0, 10);
  for (auto &flow : rec->flows) {
    ImGui::PushID(flow.data.get());

    std::string name = fmt::format("{}###{}", flow.data->name(), static_cast<void *>(flow.data.get()));
    if (ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
      auto flow_name    = flow.data->name();
      if (ImGui::InputText("Name", &flow_name)) {
        flow.data->set_name(flow_name);
      }
      if (flow.show)
        flow.show = !ImGui::Button(u8"Hide " ICON_FA_EYE_SLASH);
      else
        flow.show = ImGui::Button(u8"Show " ICON_FA_EYE);
      if (ImGui::Button(u8"Delete " ICON_FA_TRASH_ALT)) {
        flow.data = nullptr;
      }
      ImGui::ColorEdit4("Color", flow.color.data());

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
      auto rec_name = rec->name();
      if (ImGui::InputText("Name", &rec_name)) {
        rec->set_name(rec_name);
      }
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