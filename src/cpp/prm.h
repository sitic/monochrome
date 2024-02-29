#include <map>
#include <queue>

#include "transformations.h"
#include "utils/colormap.h"

#include "recordingwindow.h"

namespace prm {
  // Declare global variables and enums
  extern int main_window_width;
  extern int main_window_height;
  extern int trace_length;
  extern int max_trace_length;
  extern int display_fps;
  extern double lastframetime;

  extern const std::array<ColorMap, 7> cmaps;

  extern std::map<ColorMap, GLuint> cmap_texs;

  // Global singletons
  extern PlaybackCtrl playbackCtrl;
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
}  // namespace prm
