#include <vector>
#include <string>
#include <optional>
#include "readerwriterqueue.h"

#include "globals.h"
#include "prm.h"

namespace {
  moodycamel::ReaderWriterQueue<std::string> files_to_load(3);
  moodycamel::ReaderWriterQueue<std::shared_ptr<global::RawArray3>> array3_to_load(2);
  moodycamel::ReaderWriterQueue<std::shared_ptr<global::PointsVideo>> pointsvideo_to_load(2);
}  // namespace

namespace global {
  std::vector<Message> messages = {};

  Message::Message(std::string msg) : msg(std::move(msg)) {
    static int _id = -1;
    _id += 1;
    id = _id;
  }

  std::string tcp_host = "127.0.0.1";
  short tcp_port       = 4864;

  void add_file_to_load(const std::string& file) {
    files_to_load.enqueue(file);
  }

  std::optional<std::string> get_file_to_load() {
    std::string tmp;
    return files_to_load.try_dequeue(tmp) ? std::optional<std::string>(tmp) : std::nullopt;
  }

  void add_RawArray3_to_load(std::shared_ptr<global::RawArray3> arr) {
    array3_to_load.enqueue(std::move(arr));
  }

  std::optional<std::shared_ptr<RawArray3>> get_rawarray3_to_load() {
    std::shared_ptr<global::RawArray3> tmp;
    return array3_to_load.try_dequeue(tmp) ? std::optional<std::shared_ptr<global::RawArray3>>(tmp)
                                           : std::nullopt;
  }

  void close_window(const std::string& recording_name) {}
  void close_all_windows() {}
  std::vector<std::pair<std::string, std::vector<Vec2i>>> get_trace_pos() {
    return {};
  }
  void add_PointsVideo_to_load(std::shared_ptr<PointsVideo> obj) {
    pointsvideo_to_load.enqueue(std::move(obj));
  }
  std::optional<std::shared_ptr<PointsVideo>> get_pointsvideo_to_load() {
    std::shared_ptr<PointsVideo> tmp;
    return pointsvideo_to_load.try_dequeue(tmp) ? std::optional<std::shared_ptr<PointsVideo>>(tmp)
                                                : std::nullopt;
  }

  void quit(int) {
    if (prm::main_window) {
      glfwSetWindowShouldClose(prm::main_window, GLFW_TRUE);
    }
  }
}  // namespace global
