#include "prm.h"

namespace prm {
  int main_window_width  = 0;
  int main_window_height = 0;
  bool auto_brightness   = true;
  int display_fps        = 60;
  double lastframetime   = 0;

  std::map<ColorMap, GLuint> cmap_texs;

  PlaybackCtrl playbackCtrl = {};

  GLFWwindow *main_window                    = nullptr;
  std::vector<SharedRecordingPtr> recordings = {};
  // Queue Element: [child, parent]
  std::queue<std::pair<SharedRecordingPtr, SharedRecordingPtr>> merge_queue;
}  // namespace prm