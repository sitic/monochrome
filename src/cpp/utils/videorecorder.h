#pragma once

#ifdef _WIN32  // Windows 32 and 64 bit
#include <windows.h>
#endif

#include <stdio.h>
#include <string>
#include <vector>

#include <GLFW/glfw3.h>
#include <fmt/format.h>

#include "utils/utils.h"

class VideoRecorder {

  int width    = 0;
  int height   = 0;
  FILE *ffmpeg = nullptr;
  std::vector<GLubyte> buffer;

  static std::string ffmpeg_encoder_args() {
    const bool nvenc_test_enabled = false;  // disable nvenc for now, needs better testing
    if (nvenc_test_enabled && glfwExtensionSupported("GL_NVX_nvenc_interop")) {
      // if Nvidia NVENC supported, use it instead of cpu encoding
      return "-c:v h264_nvenc -preset slow -profile:v high -rc vbr_hq -qmin:v 19 -qmax:v 21 -b:v 4M "
             "-maxrate:v 10M";
    } else {
      // libx264 encoding preset, one of: ultrafast, superfast, veryfast,
      // faster, fast, medium, slow, slower, veryslow
      std::string preset = "slow";
      // Range is logarithmic 0 (lossless) to 51 (worst quality). Default is 23.
      unsigned quality = 18;
      return fmt::format("-c:v libx264 -preset {preset} -crf {quality:d}",
                         fmt::arg("preset", preset), fmt::arg("quality", quality));
    }
  }

#ifdef _WIN32
  static bool test_ffmpeg() {
    std::string cmd   = fmt::format("\"{ffmpeg_exe}\" -version", fmt::arg("ffmpeg_exe", ffmpeg_path));
    FILE *ffmpeg_test = _popen(cmd.c_str(), "r");
    if (!ffmpeg_test) return false;
    int status = _pclose(ffmpeg_test);
    return status == 0;
  }
#else
  static bool test_ffmpeg() {
    std::string cmd = fmt::format("which \"{ffmpeg_exe}\"", fmt::arg("ffmpeg_exe", ffmpeg_path));
    fmt::print("{}\n", cmd);
    FILE *ffmpeg_test = popen(cmd.c_str(), "r");
    if (!ffmpeg_test) return false;

    // Read from the pipe until the end of the pipe is reached
    char buf[1024];
    while (fgets(buf, sizeof(buf), ffmpeg_test)) {}

    int status = pclose(ffmpeg_test);
    return WIFEXITED(status) && (WEXITSTATUS(status) == 0);
  }
#endif

 public:
  std::string videotitle = "";
  static std::string ffmpeg_path;

  VideoRecorder() = default;

  ~VideoRecorder() { stop_recording(); }

  bool start_recording(const std::string &filename,
                       GLFWwindow *window      = nullptr,
                       int fps                 = 30,
                       std::string description = "") {
    if (ffmpeg) return true;  // if already recording, silently return

    if (fps <= 0) {
      global::new_ui_message("FPS has to be >0");
      return false;
    }

    if (!test_ffmpeg()) {
      global::new_ui_message(
          "ERROR: Unable to find ffmpeg, please install it to create a .mp4 file.");
      return false;
    }

    fmt::print("Starting to record movie\n");

    if (!window) {
      window = glfwGetCurrentContext();
    }
    glfwGetFramebufferSize(window, &width, &height);

    buffer = std::vector<GLubyte>(width * height * 3ul);

    std::string cmd = fmt::format(
        "{ffmpeg_exe} -f rawvideo -pix_fmt rgb24 -framerate {fps:d} "
        "-video_size {width}x{height} -i - -y -threads 0 {encoder_args} -pix_fmt yuv420p "
        // ensure height and width are divisible by 2
        "-vf \"[in]vflip,scale=trunc(iw/2)*2:trunc(ih/2)*2[out]\" "
        "-metadata title=\"{title}\" -metadata description=\"{description}\" "
        "\"{filename}\"",
        fmt::arg("ffmpeg_exe", ffmpeg_path), fmt::arg("fps", fps), fmt::arg("width", width),
        fmt::arg("height", height), fmt::arg("encoder_args", ffmpeg_encoder_args()),
        fmt::arg("title", videotitle), fmt::arg("description", description),
        fmt::arg("filename", filename));

#ifdef _WIN32  // Needs to be "wb" for windows
    ffmpeg = _popen(cmd.c_str(), "wb");
#else
    ffmpeg = popen(cmd.c_str(), "w");
#endif
    if (!ffmpeg) {
      global::new_ui_message("ERROR: Unable to open ffmpeg.");
      return false;
    }
    return true;
  }

  void add_frame() {
    if (!ffmpeg) {
      // VideoRecorder::start_recording has to be called before using ::add_frame
      return;
    }

    glPixelStorei(GL_PACK_ALIGNMENT, 1);  // needed when using GL_RGB
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
    fwrite(buffer.data(), sizeof(GLubyte), buffer.size(), ffmpeg);
  }

  void stop_recording() {
    if (ffmpeg) {
      fmt::print("Stopping movie recording …\n");
#ifdef _WIN32
      _pclose(ffmpeg);
#else
      pclose(ffmpeg);
#endif
      ffmpeg = nullptr;
    }
  }
};