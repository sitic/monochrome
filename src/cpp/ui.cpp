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

void show_about_window(bool* p_open) {
  auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_HorizontalScrollbar;
  if (ImGui::Begin("About Monochrome", p_open, flags)) {
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
    ImGui::TextUnformatted("By Jan Lebert and all Monochrome contributors");

    static std::string readme = get_readme();    
    static ImGuiConnector::markdown md;
    md.print(readme.data(), readme.data() + readme.size());
  }
  ImGui::End();
}

void show_display_settings_window(bool* p_open) {
  if (ImGui::Begin("Display Settings", p_open, ImGuiWindowFlags_AlwaysAutoResize)) {
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
}
  ImGui::End();
}

void show_main_imgui_window_menubar() {
  static bool show_about = false;
  static bool show_display_settings = false;
  
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Open File", "Ctrl+O")) {
        utils::load_file_filepicker();
      }
      if (ImGui::MenuItem("Open Folder", "Ctrl+Shift+O")) {
        utils::load_folder_filepicker();
      }
      if (ImGui::MenuItem("Quit", "Ctrl+Q")) {
        glfwSetWindowShouldClose(prm::main_window, GLFW_TRUE);
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Display Settings")) {
      if (ImGui::MenuItem("Video Rotation Left", NULL)) RecordingWindow::add_rotation(-90);
      if (ImGui::MenuItem("Video Rotation Right", NULL)) RecordingWindow::add_rotation(90);
      if (ImGui::MenuItem("Reset Rotation", NULL)) RecordingWindow::set_rotation(0);
      
      ImGui::Separator();
      
      if (ImGui::MenuItem("Flip Vertical", NULL)) RecordingWindow::flip_ud();
      if (ImGui::MenuItem("Flip Horizontal", NULL)) RecordingWindow::flip_lr();
      if (ImGui::MenuItem("Reset Flip", NULL)) RecordingWindow::flip_reset();
      
      ImGui::Separator();
      
      if (ImGui::MenuItem("Auto Brightness", NULL, &prm::auto_brightness)) {}
      
      ImGui::Separator();
      
      if (ImGui::MenuItem("All Display Settings...", NULL)) {
        show_display_settings = true;
      }
      
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help")) {
      if (ImGui::BeginMenu("Online Resources")) {
        ImGui::TextLinkOpenURL("GitHub", "https://github.com/sitic/monochrome");
        ImGui::TextLinkOpenURL("Documentation", "https://monochrome.readthedocs.io/");
        ImGui::EndMenu();
      }
      if (ImGui::MenuItem("About")) {
        show_about = !show_about;
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }
  if (show_about) show_about_window(&show_about);
  if (show_display_settings) show_display_settings_window(&show_display_settings);
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

  show_top_ui();
  ImGui::BeginChild("Recordings", ImGui::GetContentRegionAvail(), ImGuiChildFlags_None);

  {
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Separator, ImGui::GetStyleColorVec4(ImGuiCol_TabActive));
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();
    ImGui::SeparatorText("Recordings");
  }

  if (prm::recordings.empty()) {
    ImGui::Text("Drag and drop a file here to load it.");
    ImGui::Text("Or use the python library to load a video or image.");
  } else {
    for (const auto &rec : prm::recordings) {
      if (rec->active) rec->display();
      show_recording_ui(rec);
      show_export_recording_ui(rec);
      ImGui::Spacing();
    }
  }
  ImGui::EndChild();

  ImGui::End();
}