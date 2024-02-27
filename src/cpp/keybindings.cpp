#include "keybindings.h"

#include "globals.h"
#include "recordingwindow.h"
#include "prm.h"

namespace global {
  void common_key_callback(GLFWwindow *window,
                           int key,
                           int scancode,
                           int action,
                           int mods,
                           SharedRecordingPtr callback_rec) {
    if (mods == GLFW_MOD_CONTROL && key == GLFW_KEY_Q && action == GLFW_PRESS) {
      // CTRL + Q: Quit monochrome
      glfwSetWindowShouldClose(prm::main_window, GLFW_TRUE);
    } else if (callback_rec && (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) &&
               action == GLFW_PRESS) {
      // ESC or Q: close focused recording
      // Don't call RecordingWindow::close_callback() directly here,
      // causes a segfault in glfw
      glfwSetWindowShouldClose(window, GLFW_TRUE);
    } else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
      // SPACE: toggle play/pause
      if (callback_rec && !callback_rec->active) {
        callback_rec->active = true;
        return;
      }
      prm::playbackCtrl.toggle_play_pause();
    } else if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
      // UP: increase playback speed
      prm::playbackCtrl.increase_speed();
    } else if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
      // DOWN: decrease playback speed
      prm::playbackCtrl.deacrease_speed();
    } else if ((key == GLFW_KEY_R || key == GLFW_KEY_0) && action == GLFW_PRESS) {
      // R: reset
      for (const auto &rec : prm::recordings) {
        rec->playback.set_next(0);
        for (auto &c : rec->children) {
          c->playback.set_next(0);
        }
      }
    } else if ((key == GLFW_KEY_LEFT || key == GLFW_KEY_RIGHT) && action != GLFW_RELEASE) {
      // LEFT or RIGHT: seek 1 frame
      // SHIFT + LEFT or RIGHT: seek 10 frames
      // CTRL + LEFT or RIGHT (+ SHIFT): seek in currently focused recording only
      int steps = (mods & GLFW_MOD_SHIFT) ? 10 : 1;
      if (key == GLFW_KEY_LEFT) {
        steps *= -1;
      }
      bool current_recording_only = (mods & GLFW_MOD_CONTROL);

      auto set_playback = [](auto &rec, int steps) {
        int t = rec->current_frame();
        rec->playback.set_next(t + steps);
        for (auto &c : rec->children) {
          c->playback.set_next(t + steps);
        }
      };

      if (current_recording_only && callback_rec) {
        set_playback(callback_rec, steps);
      } else {
        for (auto &rec : prm::recordings) {
          set_playback(rec, steps);
        }
      }
    } else if (callback_rec && key == GLFW_KEY_P && action == GLFW_PRESS) {
      // P: save screenshot of focused recording
      auto fn = callback_rec->save_snapshot();
      global::new_ui_message("Saved screenshot to {}", fn.string());
    } else if (callback_rec && key == GLFW_KEY_S && action == GLFW_PRESS) {
      // S: sync all recordings playback position to focused recording
      int t = callback_rec->current_frame();
      for (auto &r : prm::recordings) {
        r->playback.set_next(t);
        for (auto &c : r->children) {
          c->playback.set_next(t);
        }
      }
    }
  }
}  // namespace global
