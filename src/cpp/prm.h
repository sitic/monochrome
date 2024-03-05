#include <map>
#include <queue>

#include "transformations.h"
#include "utils/colormap.h"

#include "recordingwindow.h"

// Global singletons and variables
namespace prm {
  extern int main_window_width;
  extern int main_window_height;
  extern bool auto_brightness;
  extern int display_fps;
  extern int trace_length;
  extern int max_trace_length;
  extern double lastframetime;

  extern std::map<ColorMap, GLuint> cmap_texs;

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
