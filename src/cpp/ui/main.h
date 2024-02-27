#pragma once

void show_top_ui() {
  if (ImGui::BeginTabBar("##TopUiTabs")) {
    if (ImGui::BeginTabItem("Main")) {
      ImGui::Spacing();

      // Speed controls
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
      if (ImGui::Button(ICON_FA_REDO_ALT)) {
        global::do_forall_recordings([](auto &r) { r->playback.restart(); });
      }
      ImGui::SameLine();

      float total_w = ImGui::GetContentRegionMaxAbs().x;
      float size_slider = total_w / 4;
      float size_firstgroup = button_w * 2 + 2 * ImGui::GetStyle().ItemSpacing.x;
      float size_secondgroup = button_w * 2 + size_slider + 3 * ImGui::GetStyle().ItemSpacing.x;
      float size_thirdgroup = button_w * 2 + size_slider + 3 * ImGui::GetStyle().ItemSpacing.x;
      float total_space = total_w - size_firstgroup - size_secondgroup - size_thirdgroup;
      
      ImGui::SameLine(size_firstgroup + total_space / 2);
      {
        if (ImGui::Button(ICON_FA_BACKWARD)) {
          prm::playbackCtrl.deacrease_speed();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(total_w / 4);
        ImGui::DragFloat("##speed", &prm::playbackCtrl.val, 0.05, 0, 20, "Playback Speed = %.2f");
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_FORWARD)) {
          prm::playbackCtrl.increase_speed();
        }
      }

      ImGui::SameLine(total_w - size_secondgroup);
      {
        bool resize_windows = false;
        if (ImGui::Button(ICON_FA_SEARCH_MINUS)) {
          RecordingWindow::scale_fct /= 2;
          resize_windows = true;
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
        if (resize_windows && RecordingWindow::scale_fct != 0.f) {
          for (const auto &r : global::recordings) {
            r->resize_window();
          }
        }
      }

      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Display Settings")) {
      ImGui::Columns(2);
      {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Video Rotation");
        ImGui::SameLine();
        if (ImGui::Button(ICON_MDI_ROTATE_LEFT)) RecordingWindow::add_rotation(-90);
        ImGui::SameLine();
        if (ImGui::Button(ICON_MDI_ROTATE_RIGHT)) RecordingWindow::add_rotation(90);
        ImGui::SameLine();
        if (ImGui::Button("Reset")) RecordingWindow::set_rotation(0);
        // ImGui::SameLine();

        ImGui::NextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Video Flip");
        ImGui::SameLine();
        if (ImGui::Button(ICON_MDI_FLIP_VERTICAL)) RecordingWindow::flip_ud();
        ImGui::SameLine();
        if (ImGui::Button(ICON_MDI_FLIP_HORIZONTAL)) RecordingWindow::flip_lr();
        ImGui::SameLine();
        if (ImGui::Button("Reset")) RecordingWindow::flip_reset();
      }
      ImGui::Columns(1);
      ImGui::Spacing();
      {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Display FPS");
        ImGui::SameLine();
        auto label = fmt::format("(current avg: {:.0f} FPS)###dfps", ImGui::GetIO().Framerate);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.3f);
        int max_display_fps = prm::display_fps;
        if (ImGui::InputInt(label.c_str(), &max_display_fps)) {
          if (ImGui::IsItemDeactivated() && max_display_fps > 0) {
            prm::display_fps   = max_display_fps;
            prm::lastframetime = glfwGetTime();
          }
        }
      }

      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("About")) {
      ImGui::Text("Monochrome is a tool for visualizing monochrome video data.");
      ImGui::NewLine();
      ImGui::Text("For more information, please visit the project's website at: https://github.com/sitic/monochrome");
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
}
