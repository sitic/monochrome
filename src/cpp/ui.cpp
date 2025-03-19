#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"

#include "fonts/IconsFontAwesome5.h"
#include "fonts/IconsMaterialDesignIcons.h"

#include "globals.h"
#include "prm.h"

#include "ui/main.h"
#include "ui/recording.h"
#include "ui/export.h"

namespace {
  bool about_window_open = false;
}

void show_about_window() {
  auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_HorizontalScrollbar;
  ImGui::SetNextWindowSizeConstraints(ImVec2(2 * ImGui::GetContentRegionAvail().x / 3, 0), ImVec2(FLT_MAX, FLT_MAX));
  if (ImGui::Begin("About Monochrome", &about_window_open, flags)) {
    auto get_readme = []() -> std::string {
      auto readme = utils::get_rc_text_file("README.md");
      // If line starts with an image ("[!" or "![") remove it
      std::stringstream ss(readme);
      std::string line;
      std::string result;
      while (std::getline(ss, line)) {
        if (!(line.size() >= 2 && line.substr(0, 2) == "[!" ||
              line.size() >= 2 && line.substr(0, 2) == "![")) {
          result += line + "\n";
        }
      }
      return result;
    };
    ImGui::PushFont(ImGuiConnector::font_bold);
    ImGui::TextUnformatted("Monochrome version " MONOCHROME_VERSION);
    ImGui::PopFont();
    ImGui::Spacing();
    ImGui::TextUnformatted("By Jan Lebert and all Monochrome contributors.");
    ImGui::TextUnformatted("Contact:");
    ImGui::SameLine();
    ImGui::TextLinkOpenURL("mail@janlebert.com", "mailto:mail@janlebert.com");
    ImGui::TextUnformatted("Website:");
    ImGui::SameLine();
    ImGui::TextLinkOpenURL("https://github.com/sitic/monochrome");

    static std::string readme = get_readme();
    static ImGuiConnector::markdown md;
    md.print(readme.data(), readme.data() + readme.size());
  }
  ImGui::End();
}

void show_display_settings() {
  auto pos_x = ImGui::CalcTextSize("Display Frame Rate").x + 2 * ImGui::GetStyle().ItemSpacing.x;

  ImGui::AlignTextToFramePadding();
  ImGui::Text("Media Rotation");
  ImGui::SameLine(pos_x);
  if (ImGui::Button(ICON_MDI_ROTATE_LEFT)) RecordingWindow::add_rotation(-90);
  ImGui::SameLine();
  if (ImGui::Button(ICON_MDI_ROTATE_RIGHT)) RecordingWindow::add_rotation(90);
  ImGui::SameLine();
  if (ImGui::Button("Reset##rotation_reset")) RecordingWindow::set_rotation(0);


  ImGui::AlignTextToFramePadding();
  ImGui::Text("Media Image Flip");
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
  ImGui::Text("Display Frame Rate");
  ImGui::SameLine(pos_x);
  ImGui::SetNextItemWidth(120);
  int max_display_fps = prm::display_fps;
  ImGui::SetNextItemWidth(100);
  if (ImGui::InputInt("Hz##dfps", &max_display_fps, 10, 30)) {
    if (max_display_fps > 0) {
      prm::display_fps   = max_display_fps;
      prm::lastframetime = glfwGetTime();
    }
  }
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("Set the maximum frame rate for video display");

  ImGui::SameLine();
  if (ImGui::Button("Reset##fps_reset")) {
    prm::display_fps   = 60;
    prm::lastframetime = glfwGetTime();
  }

  ImGui::Indent(pos_x);
  ImGui::Text("Current: %.1f FPS", ImGui::GetIO().Framerate);
  ImGui::Unindent(pos_x);

  ImGui::Spacing();
  ImGui::Spacing();
  if (ImGui::Button(ICON_FA_SAVE u8" Save All Settings to Config File",
                    ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
    auto filepath = settings::save_current_settings();
    global::new_ui_message("Settings saved to {}", filepath);
  }
}

void show_main_imgui_window_menubar() {
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Open File", "Ctrl+O")) {
        utils::load_file_filepicker();
      }
      if (ImGui::MenuItem("Open Folder", "Ctrl+Shift+O")) {
        utils::load_folder_filepicker();
      }

      if (ImGui::BeginMenu("Recent Files")) {
        auto recent_files = settings::get_recent_files();
        bool no_recent    = recent_files.empty();
        if (no_recent) {
          ImGui::MenuItem("(No recent files)", nullptr, false, false);
        } else {
          for (const auto& file_path : recent_files) {
            std::string filename   = file_path.filename().string();
            std::string menu_label = filename;

            // Truncate very long filenames for display
            if (menu_label.length() > 40) {
              menu_label = menu_label.substr(0, 37) + "...";
            }

            if (ImGui::MenuItem(menu_label.c_str())) {
              global::add_file_to_load(file_path.string());
            }
            if (ImGui::IsItemHovered()) {
              ImGui::SetTooltip("%s", file_path.string().c_str());
            }
          }
          ImGui::Separator();
          if (ImGui::MenuItem("Clear Recent Files")) {
            settings::clear_recent_files();
          }
        }
        ImGui::EndMenu();
      }

      ImGui::Separator();
      if (ImGui::MenuItem("Quit", "Ctrl+Q")) {
        glfwSetWindowShouldClose(prm::main_window, GLFW_TRUE);
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Display Settings")) {
      ImGui::BeginChild("Display Settings", ImVec2(300, 200),
                        ImGuiChildFlags_AlwaysUseWindowPadding | ImGuiChildFlags_AutoResizeX |
                            ImGuiChildFlags_AutoResizeY);
      show_display_settings();
      ImGui::EndChild();
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help")) {
      if (ImGui::BeginMenu("Online Resources")) {
        ImGui::TextLinkOpenURL("GitHub", "https://github.com/sitic/monochrome");
        ImGui::TextLinkOpenURL("Documentation", "https://monochrome.readthedocs.io/");
        ImGui::EndMenu();
      }
      if (ImGui::MenuItem("About")) {
        about_window_open = !about_window_open;
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }
  if (about_window_open) show_about_window();
}

void show_main_imgui_window() {
  ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
  ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size, ImGuiCond_Always);
  auto flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus |
               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
               ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing |
               ImGuiWindowFlags_NoNav | ImGuiWindowFlags_MenuBar;
  ImGui::Begin("Monochrome", nullptr, flags);
  show_main_imgui_window_menubar();

  {
    ImGui::PushStyleColor(ImGuiCol_Separator, ImGui::GetStyleColorVec4(ImGuiCol_TabActive));
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();
  }
  show_top_ui();
  ImGui::BeginChild("Media", ImGui::GetContentRegionAvail(), ImGuiChildFlags_None);

  {
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Separator, ImGui::GetStyleColorVec4(ImGuiCol_TabActive));
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();
  }

  if (!prm::recordings.empty()) {
    ImGui::SeparatorText("Media");
    for (const auto& rec : prm::recordings) {
      if (rec->active) rec->display();
      show_recording_ui(rec);
      show_export_recording_ui(rec);
      ImGui::Spacing();
    }
  } else {
    ImGui::BeginChild("Description", ImVec2(400, 0));
    ImGui::PushFont(ImGuiConnector::font_regular_large);
    static auto image_tex = load_icon_image();
    auto image_width = 80;
    ImGui::Image(image_tex, ImVec2(4 * image_width, image_width));
    ImGui::Spacing();
    ImGui::TextWrapped("Monochrome is a lightweight and fast viewer for scientific imaging and video data with a focus on monochromatic data.");
    ImGui::Spacing();

    {
      ImGui::Spacing();
      ImGui::Spacing();
      ImGui::PushFont(ImGuiConnector::font_bold_large);
      ImGui::SeparatorText("Start");
      ImGui::PopFont();
      ImGui::Spacing();

      ImGui::TextUnformatted(u8" – ");
      ImGui::SameLine();
      if (ImGui::Selectable(ICON_FA_FILE_IMPORT " Open Image/Video File")) {
        utils::load_file_filepicker();
      }

      ImGui::TextUnformatted(u8" – ");
      ImGui::SameLine();
      if (ImGui::Selectable(ICON_FA_FOLDER_OPEN " Open Image Folder")) {
        utils::load_folder_filepicker();
      }

      ImGui::TextUnformatted(u8" – ");
      ImGui::SameLine();
      ImGui::TextUnformatted(ICON_FA_ANCHOR " Drag & Drop Files/Folders");
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Drag and drop files or image folders into this window to load them");
      }

      ImGui::TextUnformatted(u8" – ");
      ImGui::SameLine();
      if (ImGui::Selectable(ICON_FA_ROCKET " Open Documentation")) {
        about_window_open = !about_window_open;
      }
    }

    ImGui::Spacing();
    ImGui::Spacing();

    {
      ImGui::PushFont(ImGuiConnector::font_bold_large);
      ImGui::SeparatorText("Recent Files");
      ImGui::PopFont();
      ImGui::Spacing();
      auto recent_files = settings::get_recent_files();
      if (recent_files.empty()) {
        ImGui::TextUnformatted(u8" – ");
        ImGui::SameLine();
        ImGui::TextUnformatted("(No recent files)");
      }
      for (const auto& file_path : recent_files) {
        std::string filename   = file_path.filename().string();
        std::string menu_label = filename;
        if (menu_label.length() > 40) {
          menu_label = menu_label.substr(0, 37) + "...";
        }
        ImGui::TextUnformatted(u8" – ");
        ImGui::SameLine();
        if (ImGui::Selectable(menu_label.c_str())) {
          global::add_file_to_load(file_path.string());
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s", file_path.string().c_str());
        }
      }
    }
    ImGui::PopFont();
    ImGui::EndChild();
  }
  ImGui::EndChild();

  ImGui::End();
}