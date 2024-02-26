#include <CLI/CLI.hpp>
#include <fmt/format.h>

#ifdef _WIN32  // Windows 32 and 64 bit
#include <windows.h>
#include <shellapi.h>
#endif

#include "ipc.h"
#include "main_window.h"

#ifdef USE_WIN32_MAIN
INT WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPWSTR, INT) {
  UNREFERENCED_PARAMETER(hInst);
  UNREFERENCED_PARAMETER(hPrevInstance);

  int argc;
  char **argv;
  {
    LPWSTR *lpArgv = CommandLineToArgvW(GetCommandLineW(), &argc);
    argv           = (char **)malloc(argc * sizeof(char *));
    int size, i = 0;
    for (; i < argc; ++i) {
      size    = wcslen(lpArgv[i]) + 1;
      argv[i] = (char *)malloc(size);
      wcstombs(argv[i], lpArgv[i], size);
    }
    LocalFree(lpArgv);
  }

#else
int main(int argc, char **argv) {
#endif
  CLI::App app{"Monochrome"};
  std::vector<std::string> files;
  bool send_files_over_wire = false;
  bool disable_ipc          = false;
  float font_scale          = 0;
  app.add_option("files", files, "List of files to open")->check(CLI::ExistingFile);
  app.add_option("--scale", RecordingWindow::scale_fct, "Recording window size multiplier")
      ->check(CLI::PositiveNumber);
  app.add_option("--speed", prm::playbackCtrl.val, "Recording playback speed multiplier")
      ->check(CLI::NonNegativeNumber);
  app.add_option_function<short>(
         "--rotation", [](const short &rotation) { RecordingWindow::set_rotation(rotation); },
         "Default rotation of videos")
      ->check(CLI::IsMember({0, 90, 180, 270}))
      ->default_str("0");
  app.add_flag(
      "--fliph", [](std::int64_t count) { RecordingWindow::flip_lr(); }, "Flip video horizontally");
  app.add_flag(
      "--flipv", [](std::int64_t count) { RecordingWindow::flip_ud(); }, "Flip video vertically");
  app.add_option("--font-scale", font_scale, "Fonts scaling factor");
  app.add_option("--trace_length", prm::trace_length, "Default length (in frames) for traces");
  app.add_option("--max_trace_length", prm::max_trace_length,
                 "Maximum length (in frames) for traces");
  app.add_option("--display_fps", prm::display_fps, "Default display framerate");
  app.add_option("--window-width", prm::main_window_width, "Window width of the main window");
  app.add_option("--window-height", prm::main_window_height, "Window height of the main window");
  app.add_flag(
      "--disable-ipc", disable_ipc,
      "Disable the server process which is used for interprocess-communication with python clients");
  app.add_flag("--remote-send", send_files_over_wire,
               "Test option to send file as array instead of the filename to the main process");
  std::string config_file       = config_file_path();
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

  if (!files.empty()) {
    if (!disable_ipc && ipc::is_another_instance_running()) {
      if (send_files_over_wire) {
        for (const auto &file : files) {
          auto rec              = Recording(fs::path(file));
          std::size_t framesize = rec.Nx() * rec.Ny();
          std::size_t size      = framesize * rec.length();
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
    } else {
      for (const auto &file : files) {
        global::add_file_to_load(file);
      }
    }
  }

  open_main_window(font_scale);

  if (!disable_ipc) {
    if (!ipc::is_another_instance_running()) {
      ipc::start_server();
    } else {
      fmt::print(stderr, "ERROR: Unable to start IPC server, another instance is running!\n");
    }
  }

  display_loop();

  // Cleanup
  ipc::stop_server();
  for (auto [cmap, tex] : prm::cmap_texs) {
    glDeleteTextures(1, &tex);
  }
  ImGuiConnector::Shutdown();

  glfwDestroyWindow(global::main_window);
  global::recordings.clear();
  glfwTerminate();

#ifdef USE_WIN32_MAIN
  {
    int i = 0;
    for (; i < argc; ++i) {
      free(argv[i]);
    }
    free(argv);
  }
#endif
  std::exit(EXIT_SUCCESS);
}
