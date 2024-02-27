#pragma once

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"

#include "globals.h"

namespace global {
  extern GLFWwindow *main_window;
  extern std::vector<SharedRecordingPtr> recordings;
  extern std::queue<std::tuple<SharedRecordingPtr, SharedRecordingPtr, bool>> merge_queue;

  template <typename Func>
  void do_forall_recordings(Func &&f) {
    for (const auto &rec : recordings) {
      f(rec);
      for (const auto &crec : rec->children) {
        f(crec);
      }
    }
  }
}  // namespace global

namespace prm {
  int main_window_width     = 0;
  int main_window_multipier = 1;
  int main_window_height    = 0;
  int trace_length          = 200;
  int max_trace_length      = 2000;
  int display_fps           = 60;
  double lastframetime      = 0;

  Filters prefilter              = Filters::None;
  Transformations transformation = Transformations::None;
  Filters postfilter             = Filters::None;

  const std::array<ColorMap, 7> cmaps = {ColorMap::GRAY,      ColorMap::DIFF,     ColorMap::HSV,
                                         ColorMap::BLACKBODY, ColorMap::DIFF_POS, ColorMap::DIFF_NEG,
                                         ColorMap::VIRIDIS};
  std::map<ColorMap, GLuint> cmap_texs;
}  // namespace prm

#include "ui/main.h"
#include "ui/recording.h"
#include "ui/export.h"

void show_main_imgui_window() {
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size, ImGuiCond_Always);
    auto flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("Monochrome", nullptr, flags);

    show_top_ui();

    {
      ImGui::Spacing();
      ImGui::PushStyleColor(ImGuiCol_Separator, ImGui::GetStyleColorVec4(ImGuiCol_TabActive));
      ImGui::Separator();
      ImGui::PopStyleColor();
      ImGui::Spacing();
      ImGui::SeparatorText("Recordings");
    }


    int rec_nr = 0;
    for (const auto &rec : global::recordings) {
      if (rec->active) rec->display(prm::prefilter, prm::transformation, prm::postfilter);
      rec_nr = show_recording_ui(rec, rec_nr);
      show_export_recording_ui(rec);
      ImGui::Spacing();
    }

    ImGui::End();
}