#include <vector>
#include <string>
#include <optional>
#include "readerwriterqueue.h"

#include "globals.h"

namespace {
  moodycamel::ReaderWriterQueue<std::string> files_to_load(3);
  moodycamel::ReaderWriterQueue<std::shared_ptr<global::RawArray3>> array3_to_load(2);
}  // namespace

namespace global {

  std::vector<Message> messages = {};

  std::string tcp_host = "127.0.0.1";
  short tcp_port       = 4864;

  void add_file_to_load(const std::string& file) { files_to_load.enqueue(file); }

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
}  // namespace global
