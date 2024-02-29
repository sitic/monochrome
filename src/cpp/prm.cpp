#include "prm.h"

namespace prm {
  int main_window_width  = 0;
  int main_window_height = 0;
  int trace_length       = 200;
  int max_trace_length   = 2000;
  int display_fps        = 60;
  double lastframetime   = 0;

  const std::array<ColorMap, 7> cmaps = {ColorMap::GRAY,      ColorMap::DIFF,     ColorMap::HSV,
                                         ColorMap::BLACKBODY, ColorMap::DIFF_POS, ColorMap::DIFF_NEG,
                                         ColorMap::VIRIDIS};
  std::map<ColorMap, GLuint> cmap_texs;

  PlaybackCtrl playbackCtrl = {};

  GLFWwindow *main_window                    = nullptr;
  std::vector<SharedRecordingPtr> recordings = {};
  // Queue Element: [child, parent, as_flow]
  std::queue<std::tuple<SharedRecordingPtr, SharedRecordingPtr, bool>> merge_queue;
}  // namespace prm