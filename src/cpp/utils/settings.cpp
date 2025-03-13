#include <fstream>
#include <deque>
#include <mutex>

#include <fmt/format.h>

#include "settings.h"
#include "globals.h"
#include "prm.h"
#include "recordingwindow.h"
#include "utils/files.h"

#include "videorecorder.h"

#if defined(__unix__) || defined(__unix) || defined(__APPLE__)
#include <unistd.h>
#include <pwd.h>
std::string get_user_homedir() {
  const char* homedir;

  if ((homedir = getenv("HOME")) == nullptr) {
    homedir = getpwuid(getuid())->pw_dir;
  }
  return homedir;
}
#endif

std::string VideoRecorder::ffmpeg_path = "ffmpeg";

namespace {
  std::deque<fs::path> _recent_files;
  std::mutex _recent_files_mutex;

  std::string _config_path(std::string filename) {
    #ifdef _WIN32
      char* appdata = getenv("APPDATA");
      return fmt::format("{}\\Monochrome\\{}", appdata, filename);
    #elif defined(__unix__) || defined(__unix) || defined(__APPLE__)
      return fmt::format("{}/.config/{}", get_user_homedir(), filename);
    #else
      return "";
    #endif
  }

  std::string _recent_files_path() {
    return _config_path("recent_files.ini");
  }

  void _load_recent_files_from_file() {
    std::string recent_files_path = _recent_files_path();
    if (!fs::exists(recent_files_path)) {
      return;
    }
    std::ifstream file(recent_files_path);
    if (file.is_open()) {
      std::string line;
      while (std::getline(file, line)) {
        if (!line.empty() && fs::exists(line)) {
          _recent_files.push_back(line);
        }
      }
      file.close();
    }
  }
}  // namespace

namespace settings {

std::string config_file_path() {
  return _config_path("Monochrome.ini");
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
  settings::cli_add_global_options(app);
  app.set_config("--config");
  app.parse("", false);

  std::string config_path = settings::config_file_path();

  std::string settings = app.config_to_str(true, true);

  utils::write_text_file(config_path, settings);

  return config_path;
}

std::vector<fs::path> get_recent_files() {
  std::lock_guard<std::mutex> lock(_recent_files_mutex);
  
  // If the recent files list is empty, try to load from file
  if (_recent_files.empty()) {
    _load_recent_files_from_file();
  }
  return std::vector<fs::path>(_recent_files.begin(), _recent_files.end());
}

void add_recent_file(const fs::path& file) {
  std::lock_guard<std::mutex> lock(_recent_files_mutex);
  if (!fs::exists(file)) {
    return;
  }

  if (_recent_files.empty()) {
    _load_recent_files_from_file();
  }
  
  // Remove file if it already exists in the list
  auto it = std::find(_recent_files.begin(), _recent_files.end(), file);
  if (it != _recent_files.end()) {
    _recent_files.erase(it);
  }
  
  // Add file to the front of the list
  _recent_files.push_front(file);
  
  // Keep only the 10 most recent files
  if (_recent_files.size() > 10) {
    _recent_files.resize(10);
  }
  
  std::string recent_files_path = _recent_files_path();
  std::string content;
  for (const auto& path : _recent_files) {
    content += path.string() + "\n";
  }
  utils::write_text_file(recent_files_path, content);
}

void clear_recent_files() {
  std::lock_guard<std::mutex> lock(_recent_files_mutex);
  _recent_files.clear();
  
  std::string recent_files_path = _recent_files_path();
  utils::write_text_file(recent_files_path, "");
}
} // namespace settings
