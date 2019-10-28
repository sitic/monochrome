#pragma once

#ifdef _WIN32 // Windows 32 and 64 bit
#include <windows.h>

inline FILE *popen(const char *command, const char *type) {
  return _popen(command, type);
}

inline void pclose(FILE *file) { _pclose(file); }
#endif

#include <stdio.h>
#include <string>
#include <vector>

#include "utils.h"

#include <GLFW/glfw3.h>
#include <fmt/format.h>

class VideoRecorder {

  int width = 0;
  int height = 0;
  FILE *ffmpeg = nullptr;
  std::vector<GLubyte> buffer;

  std::string ffmpeg_encoder_args() {
    if (glfwExtensionSupported("GL_NVX_nvenc_interop")) {
      // if Nvidia NVENC supported, use it instead of cpu encoding
      return "-c:v h264_nvenc -preset slow -profile:v high -rc vbr_hq -qmin:v 19 -qmax:v 21 -b:v 4M -maxrate:v 10M";
    } else {
      // libx264 encoding preset, one of: ultrafast, superfast, veryfast,
      // faster, fast, medium, slow, slower, veryslow
      std::string preset = "slow";
      // Range is logarithmic 0 (lossless) to 51 (worst quality). Default is 23.
      unsigned quality = 18;
      return fmt::format("-c:v libx264 -preset {preset} -crf {quality:d}",
                         fmt::arg("preset", preset),
                         fmt::arg("quality", quality));
    }
  }

public:
  std::string videotitle = "";
  bool recording = false;

  VideoRecorder() = default;

  ~VideoRecorder() { stop_recording(); }

  void start_recording(const std::string &filename,
                       GLFWwindow *window = nullptr, int fps = 30) {
    if (ffmpeg)
      return; // if already recording, silently return

    if (fps <= 0) {
      new_ui_message("FPS has to be >0");
      return;
    }

    fmt::print("Starting to record movie\n");

    if (!window) {
      window = glfwGetCurrentContext();
    }
    glfwGetWindowSize(window, &width, &height);

    buffer = std::vector<GLubyte>(width * height * 4ul);

    std::string cmd = fmt::format(
        "ffmpeg -f rawvideo -pix_fmt rgba -framerate {fps:d} -s "
        "{width}x{height} -i - "
        "-y -threads 0 {encoder_args} -pix_fmt yuv420p "
        // ensure height and width are divisible by 2
        "-vf \"[in]vflip,scale=trunc(iw/2)*2:trunc(ih/2)*2[out]\" "
        "-metadata title=\"{title}\" {filename}",
        fmt::arg("fps", fps), fmt::arg("width", width),
        fmt::arg("height", height),
        fmt::arg("encoder_args", ffmpeg_encoder_args()),
        fmt::arg("title", videotitle), fmt::arg("filename", filename));

    ffmpeg = popen(cmd.c_str(), "w");
    if (!ffmpeg) {
      new_ui_message("unable to open ffmpeg with popen(), can't record output");
    }
  }

  void add_frame() {
    if (!ffmpeg) {
      new_ui_message("VideoRecorder::start_recording has to be called before "
                     "using ::add_frame!");
    }

    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());
    fwrite(buffer.data(), sizeof(GLubyte), buffer.size(), ffmpeg);
  }

  void stop_recording() {
    if (ffmpeg) {
      fmt::print("Stopping movie recording …\n");
      pclose(ffmpeg);
      ffmpeg = nullptr;
    }
  }
};