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
  if (ImGui::Begin("About Monochrome", p_open, ImGuiWindowFlags_AlwaysAutoResize)) {
    auto get_readme = []() -> std::string {
      auto readme = utils::get_rc_text_file("README.md");
      readme = fmt::format("Monochrome version {}\n{}", MONOCHROME_VERSION, readme);
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
    static std::string readme = get_readme();    
    static ImGuiConnector::markdown md;
    md.print(readme.data(), readme.data() + readme.size());
  }
  ImGui::End();
}

void show_main_imgui_window_menubar() {
  static bool show_about = false;
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
    if (ImGui::BeginMenu("Help")) {
      if (ImGui::MenuItem("About")) {
        show_about = !show_about;
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }
  if (show_about) show_about_window(&show_about);
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