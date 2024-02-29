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

void show_main_imgui_window() {
  ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
  ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size, ImGuiCond_Always);
  auto flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus |
               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
               ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing |
               ImGuiWindowFlags_NoNav;
  ImGui::Begin("Monochrome", nullptr, flags);

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
    ImGui::Text("Drag and drop a .npy or .dat file here to load it.");
    ImGui::Text("Or use the python library to load a recording.");
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