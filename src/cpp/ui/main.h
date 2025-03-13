#pragma once

#include "imgui_md.h"

#include "globals.h"
#include "utils/settings.h"
#include "utils/ImGuiConnector.h"
#include "utils/files.h"

void show_top_ui() {
  // For spacing
  ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, 0);
  ImGui::Dummy(ImVec2(2, 0));
  ImGui::SameLine();
  ImGui::PopStyleVar();

  if (prm::playbackCtrl.val == 0) {
    if (ImGui::Button(ICON_FA_PLAY)) {
      prm::playbackCtrl.toggle_play_pause();
    }
  } else {
    if (ImGui::Button(ICON_FA_PAUSE)) {
      prm::playbackCtrl.toggle_play_pause();
    }
  }
  float button_w = ImGui::GetItemRectSize().x;
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_FAST_BACKWARD)) {
    prm::do_forall_recordings([](auto &r) { r->playback.restart(); });
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Restart playback");
  }
  ImGui::SameLine();

  // Calculate the space for the playback speed controls based on available space
  float total_w          = (ImGui::GetContentRegionAvail().x + ImGui::GetCursorScreenPos().x - ImGui::GetWindowPos().x);
  float size_slider      = total_w / 4;
  float size_firstgroup  = button_w * 2 + 2 * ImGui::GetStyle().ItemSpacing.x;
  float size_secondgroup = button_w * 2 + size_slider + 3 * ImGui::GetStyle().ItemSpacing.x;
  float size_thirdgroup  = button_w * 2 + size_slider + 3 * ImGui::GetStyle().ItemSpacing.x;
  float total_space      = total_w - size_firstgroup - size_secondgroup - size_thirdgroup;

  ImGui::SameLine(size_firstgroup + total_space / 2);
  {
    if (ImGui::Button(ICON_FA_BACKWARD)) {
      prm::playbackCtrl.decrease_speed();
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Decrease playback speed");
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(total_w / 4);
    ImGui::DragFloat("##speed", &prm::playbackCtrl.val, 0.05, 0, 20, "Playback Speed = %.2f");
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FORWARD)) {
      prm::playbackCtrl.increase_speed();
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Double playback speed");
    }
  }

  ImGui::SameLine(total_w - size_secondgroup);
  {
    bool resize_windows = false;
    if (ImGui::Button(ICON_FA_SEARCH_MINUS)) {
      RecordingWindow::scale_fct /= 2;
      resize_windows = true;
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Decrease media window size");
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(total_w / 4);
    if (ImGui::DragFloat("##scaling", &RecordingWindow::scale_fct, 0.05, 0.5, 10,
                          "Window Scaling = %.1f")) {
      resize_windows = true;
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_SEARCH_PLUS)) {
      RecordingWindow::scale_fct *= 2;
      resize_windows = true;
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Increase media window size");
    }
    if (resize_windows && RecordingWindow::scale_fct != 0.f) {
      for (const auto &r : prm::recordings) {
        r->resize_window();
      }
    }
  }
}
