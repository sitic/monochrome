#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_stdlib.h"
#include "implot.h"

#include "utils/plot_utils.h"
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

  const std::array<ColorMap, 8> cmaps = {ColorMap::GRAY,      ColorMap::DIFF,   ColorMap::HSV,
                                         ColorMap::BLACKBODY, ColorMap::MAGMA,  ColorMap::DIFF_POS,
                                         ColorMap::DIFF_NEG,  ColorMap::VIRIDIS};
  std::map<ColorMap, GLuint> cmap_texs;
}  // namespace prm

#include "ui_main.h"
#include "ui_recording.h"
#include "ui_export.h"