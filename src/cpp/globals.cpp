#include <vector>
#include <string>
#include <optional>
#include "readerwriterqueue.h"

#include "globals.h"
#include "prm.h"

namespace {
  moodycamel::ReaderWriterQueue<std::shared_ptr<global::RemoteCommand>> remote_commands(4);
}  // namespace

namespace global {
  std::string tcp_host = "127.0.0.1";
  short tcp_port       = 4864;
  
  std::vector<Message> messages = {};

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
}  // namespace global
