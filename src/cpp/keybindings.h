#pragma once

#include "recordingwindow.h"

namespace global {
  void common_key_callback(GLFWwindow *window,
                           int key,
                           int scancode,
                           int action,
                           int mods,
                           SharedRecordingPtr callback_rec = nullptr);
}  // namespace global
