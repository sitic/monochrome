#include <vector>
#include <string>
#include <optional>
#include <thread>
#include <mutex>

#include <readerwriterqueue.h>

#include "globals.h"
#include "prm.h"

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
    }
  }

  Subprocess::Subprocess(subprocess::RunBuilder _builder, std::string _title, std::string _msg, std::function<void()> _callback) {
    static int _id = -1;
    _id += 1;
    id = _id;

    this->builder = _builder;
    this->title   = _title.empty() ? fmt::format("Subprocess {}", id) : _title;
    this->msg     = _msg;
    this->cmd     = std::accumulate(builder.command.begin(), builder.command.end(), std::string{},
                                    [](std::string a, std::string b) { return a + " " + b; });
    this->popen   = std::make_unique<subprocess::Popen>(_builder.command, _builder.options);
    this->callback = std::move(_callback);

    // Start a thread to continuously read output
    reader_thread = std::thread([this]() {
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
    });
  }
  
  Subprocess::~Subprocess() {
    running = false;
    if (reader_thread.joinable()) {
      reader_thread.join();
    }
  }

  void Subprocess::tick() {
    std::lock_guard<std::mutex> lock(_mutex);
      if (_cout.length() > cout.length()) {
        cout = std::string(_cout);
      }
  }

  bool Subprocess::is_running() {
    return running;
  }
  
  void add_subprocess(subprocess::RunBuilder process, std::string title, std::string msg, std::function<void()> callback) {
    auto p = std::make_shared<Subprocess>(process, title, msg, std::move(callback));
    global::subprocesses.push_back(p);
  }
}  // namespace global
