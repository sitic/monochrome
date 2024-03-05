#include <fstream>

#include <fmt/format.h>

#include "settings.h"
#include "globals.h"
#include "prm.h"
#include "recordingwindow.h"

#include "videorecorder.h"

std::string VideoRecorder::ffmpeg_path = "ffmpeg";

#if defined(__unix__) || defined(__unix) || defined(__APPLE__)
#include <unistd.h>
#include <pwd.h>
namespace {
  std::string get_user_homedir() {
    const char* homedir;

    if ((homedir = getenv("HOME")) == nullptr) {
      homedir = getpwuid(getuid())->pw_dir;
    }
    return homedir;
  }
}  // namespace
#endif

std::string config_file_path() {
#ifdef _WIN32
  char* appdata = getenv("APPDATA");
  return fmt::format("{}\\Monochrome\\Monochrome.ini", appdata);
#elif defined(__unix__) || defined(__unix) || defined(__APPLE__)
  return fmt::format("{}/.config/Monochrome.ini", get_user_homedir());
#else
  return "";
#endif
}

void cli_add_global_options(CLI::App& app) {
  app.add_option("--scale", RecordingWindow::scale_fct, "Recording window size multiplier")
      ->check(CLI::PositiveNumber)
      ->capture_default_str();
  app.add_option("--speed", prm::playbackCtrl.val, "Recording playback speed multiplier")
      ->check(CLI::NonNegativeNumber)
      ->capture_default_str();
  app.add_flag("--auto_brightness", prm::auto_brightness, "Enable automatic brightness adjustment")
      ->default_str(prm::auto_brightness ? "true" : "false");
  app.add_option_function<short>(
         "--rotation", [](const short& rotation) { RecordingWindow::set_rotation(rotation); },
         "Default rotation of videos")
      ->check(CLI::IsMember({0, 90, 180, 270}))
      ->default_str(fmt::format("{}", RecordingWindow::get_rotation()));
  app.add_flag(
         "--fliph",
         [](std::int64_t count) {
           if (count >= 1) RecordingWindow::flip_lr();
         },
         "Flip video horizontally")
      ->default_str(RecordingWindow::get_flip_lr() ? "true" : "false");
  app.add_flag(
         "--flipv",
         [](std::int64_t count) {
           if (count >= 1) RecordingWindow::flip_ud();
         },
         "Flip video vertically")
      ->default_str(RecordingWindow::get_flip_ud() ? "true" : "false");
  app.add_option("--trace_length", prm::trace_length, "Default length (in frames) for traces")
      ->check(CLI::PositiveNumber)
      ->capture_default_str();
  app.add_option("--max_trace_length", prm::max_trace_length,
                 "Maximum length (in frames) for traces")
      ->check(CLI::PositiveNumber)
      ->capture_default_str();
  app.add_option("--display_fps", prm::display_fps, "Default display framerate")
      ->check(CLI::PositiveNumber)
      ->capture_default_str();
  app.add_option("--window-width", prm::main_window_width, "Window width of the main window")
      ->check(CLI::PositiveNumber)
      ->capture_default_str();
  app.add_option("--window-height", prm::main_window_height, "Window height of the main window")
      ->check(CLI::PositiveNumber)
      ->capture_default_str();
  app.add_option("--ffmpeg", VideoRecorder::ffmpeg_path, "Path to the ffmpeg executable")
      ->capture_default_str();
}

std::string save_current_settings() {
  CLI::App app{"Monochrome"};
  cli_add_global_options(app);
  app.set_config("--config");
  app.parse("", false);

  std::string config_path = config_file_path();

  std::string settings = app.config_to_str(true, true);

  std::ofstream file;
  file.open(config_path);
  file << settings;
  file.close();

  return config_path;
}