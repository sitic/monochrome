#pragma once

#include "imgui_md.h"

#include "globals.h"
#include "utils/settings.h"
#include "utils/ImGuiConnector.h"

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
// Following includes for Windows LinkCallback
#define WIN32_LEAN_AND_MEAN
#include <process.h>
void system_open_url(const std::string &url) {
  ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
}
#elif defined(__APPLE__)
void system_open_url(const std::string &url) {
  std::string cmd = "open " + url;
  system(cmd.c_str());
}
#elif defined(__linux__)
void system_open_url(const std::string &url) {
  std::string cmd = "xdg-open " + url;
  system(cmd.c_str());
}
#else
void system_open_url(const std::string &url) {
  fmt::print("Cannot open url: {}\n", url);
}
#endif

struct markdown_cls : public imgui_md {
  ImFont *get_font() const override {
    if (m_is_table_header) {
      return ImGuiConnector::font_bold_large;
    }

    if (m_is_code) {
      return ImGuiConnector::font_code;
    }

    switch (m_hlevel) {
      case 0:
        return m_is_strong ? ImGuiConnector::font_bold : ImGuiConnector::font_regular;
      case 1:
        return ImGuiConnector::font_bold_large;
      case 2:
        return ImGuiConnector::font_bold_large;
      default:
        return ImGuiConnector::font_bold;
    }
  };

  void open_url() const override { system_open_url(m_href); }

  void SPAN_CODE(bool e) override {
    if (e) {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8, 0.8, 0.8, 1));
    } else {
      ImGui::PopStyleColor();
    }
  }

  void BLOCK_CODE(const MD_BLOCK_CODE_DETAIL *, bool e) override {
    m_is_code = e;
    if (e) {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8, 0.8, 0.8, 1));
      ImGui::Indent();
    } else {
      ImGui::Unindent();
      ImGui::PopStyleColor();
    }
  }

  void BLOCK_QUOTE(bool e) override {
    if (e) {
      ImGui::Indent();
    } else {
      ImGui::Unindent();
    }
  }

  // ignore images
  void SPAN_IMG(const MD_SPAN_IMG_DETAIL *d, bool e) override { m_is_image = e; }
};

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
      if (ImGui::Button(ICON_FA_FAST_BACKWARD)) {
        prm::do_forall_recordings([](auto &r) { r->playback.restart(); });
      }
      ImGui::SameLine();

      float total_w          = ImGui::GetContentRegionMaxAbs().x;
      float size_slider      = total_w / 4;
      float size_firstgroup  = button_w * 2 + 2 * ImGui::GetStyle().ItemSpacing.x;
      float size_secondgroup = button_w * 2 + size_slider + 3 * ImGui::GetStyle().ItemSpacing.x;
      float size_thirdgroup  = button_w * 2 + size_slider + 3 * ImGui::GetStyle().ItemSpacing.x;
      float total_space      = total_w - size_firstgroup - size_secondgroup - size_thirdgroup;

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
          for (const auto &r : prm::recordings) {
            r->resize_window();
          }
        }
      }

      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("Display Settings")) {

      auto pos_x = ImGui::CalcTextSize("Auto Brightness").x + 2 * ImGui::GetStyle().ItemSpacing.x;

      ImGui::AlignTextToFramePadding();
      ImGui::Text("Video Rotation");
      ImGui::SameLine(pos_x);
      if (ImGui::Button(ICON_MDI_ROTATE_LEFT)) RecordingWindow::add_rotation(-90);
      ImGui::SameLine();
      if (ImGui::Button(ICON_MDI_ROTATE_RIGHT)) RecordingWindow::add_rotation(90);
      ImGui::SameLine();
      if (ImGui::Button("Reset##rotation_reset")) RecordingWindow::set_rotation(0);


      ImGui::AlignTextToFramePadding();
      ImGui::Text("Video Flip");
      ImGui::SameLine(pos_x);
      if (ImGui::Button(ICON_MDI_FLIP_VERTICAL)) RecordingWindow::flip_ud();
      ImGui::SameLine();
      if (ImGui::Button(ICON_MDI_FLIP_HORIZONTAL)) RecordingWindow::flip_lr();
      ImGui::SameLine();
      if (ImGui::Button("Reset##flip_reset")) RecordingWindow::flip_reset();

      ImGui::Spacing();
      ImGui::AlignTextToFramePadding();
      ImGui::Text("Auto Brightness");
      ImGui::SameLine(pos_x);
      ImGui::Checkbox("##auto_brightness", &prm::auto_brightness);

      ImGui::Spacing();
      ImGui::AlignTextToFramePadding();
      ImGui::Text("Display FPS");
      ImGui::SameLine(pos_x);
      auto label = fmt::format("(current avg: {:.0f} FPS)###dfps", ImGui::GetIO().Framerate);
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.3f);
      int max_display_fps = prm::display_fps;
      if (ImGui::InputInt(label.c_str(), &max_display_fps)) {
        if (ImGui::IsItemDeactivated() && max_display_fps > 0) {
          prm::display_fps   = max_display_fps;
          prm::lastframetime = glfwGetTime();
        }
      }


      ImGui::Spacing();
      if (ImGui::Button(u8"Save all settings " ICON_FA_SAVE)) {
        auto filepath = save_current_settings();
        global::new_ui_message("Settings saved to {}", filepath);
      }
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("About")) {
      static markdown_cls md;

      static std::string readme = get_rc_text_file("README.md");
      md.print(readme.data(), readme.data() + readme.size());

      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
}
