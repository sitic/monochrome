#include <vector>
#include <string>
#include <optional>
#include <thread>
#include <mutex>

#include <readerwriterqueue.h>

#include "globals.h"
#include "prm.h"
#include "utils/files.h"

namespace {
  moodycamel::ReaderWriterQueue<std::shared_ptr<global::RemoteCommand>> remote_commands(4);
}  // namespace

namespace global {
  std::string tcp_host = "127.0.0.1";
  short tcp_port       = 4864;
  asio::thread_pool thread_pool(2);
  
  std::vector<Message> messages = {};
  std::vector<std::shared_ptr<Subprocess>> subprocesses = {};

  Message::Message(std::string msg) : msg(std::move(msg)) {
    static int _id = -1;
    _id += 1;
    id = _id;
  }

  void add_remote_command(std::shared_ptr<RemoteCommand> cmd) {
    remote_commands.enqueue(std::move(cmd));
  }

  std::optional<std::shared_ptr<RemoteCommand>> get_remote_command() {
    std::shared_ptr<RemoteCommand> tmp;
    return remote_commands.try_dequeue(tmp) ? std::optional<std::shared_ptr<RemoteCommand>>(tmp)
                                            : std::nullopt;
  }

  void add_file_to_load(const std::string& file) {
    add_remote_command(std::make_shared<LoadFileCommand>(file));
  }

  void quit(int) {
    if (prm::main_window) {
      glfwSetWindowShouldClose(prm::main_window, GLFW_TRUE);
    } else {
      // Monochrome should be in unit test mode, signal the main thread to exit
      global::tcp_port = 0;
    }
  }

  Subprocess::Subprocess(subprocess::RunBuilder _builder,
                         std::string _title,
                         std::string _msg,
                         std::function<void()> _callback) {
    static int _id = -1;
    _id += 1;
    id = _id;

    _builder.cout(subprocess::PipeOption::pipe);
    _builder.cerr(subprocess::PipeOption::cout);

    this->builder = _builder;
    this->title   = _title.empty() ? fmt::format("Subprocess {}", id) : _title;
    this->msg     = _msg;
    this->cmd     = std::accumulate(builder.command.begin(), builder.command.end(), std::string{},
                                    [](std::string a, std::string b) { return a + " " + b; });
    this->callback = std::move(_callback);

    open();
  }

  Subprocess::~Subprocess() {
    running = false;
    if (reader_thread.joinable()) {
      reader_thread.join();
    }
  }

  bool Subprocess::tick() {
    if (!open()) return false;

    std::lock_guard<std::mutex> lock(_mutex);
    if (_cout.length() > cout.length()) {
      cout = _cout;
    }
    return true;
  }

  bool Subprocess::is_running() {
    return running;
  }

  bool Subprocess::open() {
    if (popen) {
      return true;  // already running
    } else if (!running) {
      return true;  // already finished
    } else {  // Check if we need to wait for UV install
      if (builder.command[0] == "uv-placeholder") {
        if (auto uv = utils::get_uv_executable(); uv != "uv-placeholder") {
          // UV install complete
          builder.command[0] = uv;
        } else {
          // UV install is in progress
          return false;
        }
      }
    }

    try {
      this->popen    = std::make_unique<subprocess::Popen>(builder.command, builder.options);

      // Start a thread to continuously read output
      reader_thread = std::thread([this]() {
        try {
          constexpr int buf_size = 8;
          uint8_t buf[buf_size];

          while (running && popen) {
            auto transferred = subprocess::pipe_read(popen->cout, buf, buf_size);
            if (transferred > 0) {
              std::lock_guard<std::mutex> lock(_mutex);
              _cout.insert(_cout.end(), &buf[0], &buf[transferred]);
            } else {
              break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
          }
          subprocess::pipe_close(popen->cout);
          popen->cout = subprocess::kBadPipeValue;
          popen->wait();

          callback();

          std::lock_guard<std::mutex> lock(_mutex);
          this->running = false;
          if (popen->returncode == 0) {
            this->show = false;
          }

        } catch (const std::exception& e) {
          new_ui_message("ERROR: Exception thrown: {}", e.what());
        }
      });
    } catch (const subprocess::CalledProcessError& e) {
      new_ui_message("ERROR: Failed to start subprocess: {}", e.what());
      return false;
    } catch (const std::exception& e) {
      new_ui_message("ERROR: Error during plugin load: {}. Command: {}", e.what(), cmd);
      return false;
    }
    return true;
  }
  
  void add_subprocess(subprocess::RunBuilder process, std::string title, std::string msg, std::function<void()> callback) {
    auto p = std::make_shared<Subprocess>(process, title, msg, std::move(callback));
    global::subprocesses.push_back(p);
  }
}  // namespace global
