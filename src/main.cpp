#include <CLI/CLI.hpp>
#include <fmt/format.h>

#ifdef _WIN32  // Windows 32 and 64 bit
#include <windows.h>
#endif

#include "ipc.h"
#include "main_window.h"

int main(int argc, char **argv) {
  CLI::App app{"Quick Raw Video Viewer"};
  std::vector<std::string> files;
  bool send_files_over_wire = false;
  bool disable_ipc          = false;
  app.add_option("files", files, "List of files to open")->check(CLI::ExistingFile);
  app.add_option("--scale", RecordingWindow::scale_fct, "Recording window size multiplier")
      ->check(CLI::PositiveNumber);
  app.add_option("--speed", prm::playbackCtrl.val, "Recording playback speed multiplier")
      ->check(CLI::NonNegativeNumber);
  app.add_option("--window-width", prm::main_window_width, "Window width of the main window");
  app.add_option("--window-height", prm::main_window_height, "Window height of the main window");
  app.add_option("--max_trace_length", prm::max_trace_length);
  app.add_flag("--disable-ipc", disable_ipc);
  app.add_flag("--remote-send", send_files_over_wire);
  std::string config_file;
#ifdef _WIN32
  config_file = "%APPDATA%\\quickVidViewer\\quickVidViewer.ini";
#elif defined(unix) || defined(__unix__) || defined(__unix)
  config_file = fmt::format("{}/.config/quickVidViewer.ini", get_user_homedir());
#endif
  bool print_config             = false;
  CLI::Option *print_config_opt = nullptr;
  if (!config_file.empty()) {
    app.set_config("--config", config_file,
                   "Configuration file to load command line arguments from");
    print_config_opt = app.add_flag("--print-config", print_config);
  }

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError &e) {
    return app.exit(e);
  }

  if (print_config) {
    app.remove_option(print_config_opt);
    fmt::print(app.config_to_str(true, true));
    std::exit(EXIT_SUCCESS);
  }

  if (!disable_ipc && !files.empty()) {
    if (ipc::is_another_instance_running()) {
      if (send_files_over_wire) {
        for (const auto &file : files) {
          auto rec       = Recording(fs::path(file));
          auto framesize = rec.Nx() * rec.Ny();
          auto size      = framesize * rec.length();
          std::vector<float> data(size);
          for (long t = 0; t < rec.length(); t++) {
            rec.load_frame(t);
            auto frame = rec.frame.reshaped();
            std::copy(frame.begin(), frame.end(), data.begin() + t * framesize);
          }
          ipc::send_array3(data.data(), rec.Nx(), rec.Ny(), rec.length(), file);
        }
      } else {
        ipc::send_filepaths(files);
      }
      std::exit(EXIT_SUCCESS);
    }
  }

  open_main_window();

  for (const auto &file : files) {
    load_new_file(file);
  }

  if (!disable_ipc) {
    if (!ipc::is_another_instance_running()) {
      ipc::start_server();
    } else {
      fmt::print("Unable to start TCP server, another instance is running!\n");
    }
  }

  display_loop();

  // Cleanup
  ipc::stop_server();
  ImGui_ImplOpenGL2_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(global::main_window);
  recordings.clear();
  glfwTerminate();
  std::exit(EXIT_SUCCESS);
}
