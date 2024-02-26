#pragma once

ImGuiWindow* show_main_ui() {
  ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
  ImGui::SetNextWindowSizeConstraints(ImVec2(ImGui::GetMainViewport()->Size[0], 0),
                                      ImVec2(ImGui::GetMainViewport()->Size[0], FLT_MAX));
  auto flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize |
               ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoFocusOnAppearing;
  ImGui::Begin("Drag & drop .dat or .npy files into this window", nullptr, flags);
  
  if (ImGui::BeginTabBar("##TabBar")) {
    if (ImGui::BeginTabItem("Main")) {
      // Speed controls
      {
        if (prm::playbackCtrl.val == 0) {
          if (ImGui::Button(ICON_FA_PLAY)) {
            prm::playbackCtrl.toggle_play_pause();
          }
        } else {
          if (ImGui::Button(ICON_FA_PAUSE)) {
            prm::playbackCtrl.toggle_play_pause();
          }
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_REDO_ALT)) {
          global::do_forall_recordings([](auto &r) { r->playback.restart(); });
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_BACKWARD)) {
          prm::playbackCtrl.deacrease_speed();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.75f);
        ImGui::DragFloat("##speed", &prm::playbackCtrl.val, 0.05, 0, 20, "Playback Speed = %.2f");
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_FORWARD)) {
          prm::playbackCtrl.increase_speed();
        }
      }

      {
        bool resize_windows = false;
        if (ImGui::Button(ICON_FA_SEARCH_MINUS)) {
          RecordingWindow::scale_fct /= 2;
          resize_windows = true;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.75f);
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
        ImGui::Text("Video Flip");
        ImGui::SameLine();
        if (ImGui::Button(ICON_MDI_FLIP_VERTICAL)) RecordingWindow::flip_ud();
        ImGui::SameLine();
        if (ImGui::Button(ICON_MDI_FLIP_HORIZONTAL)) RecordingWindow::flip_lr();
        ImGui::SameLine();
        if (ImGui::Button("Reset")) RecordingWindow::flip_reset();
      }
      ImGui::Columns(1);
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

  auto window = ImGui::GetCurrentWindow();
  ImGui::End();

  return window;
}
