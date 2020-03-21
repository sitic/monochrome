#pragma once

#include <string>
#include <optional>
#include <utility>
#include <memory>

#include <fmt/format.h>

namespace global {
  class Message;
  extern std::vector<Message> messages;

  extern std::string tcp_host;
  extern short tcp_port;

  class Message {
   public:
    bool show = true;
    std::string msg;
    int id = 0;

    Message(std::string msg) : msg(std::move(msg)) {
      static int _id = -1;
      _id += 1;
      id = _id;
    };
  };

  template <typename... Args>
  inline void new_ui_message(const char *fmt, Args &&... args) {
    const std::string msg = fmt::format(fmt, std::forward<Args>(args)...);
    messages.emplace_back(msg);
    fmt::print(msg + "\n");
  }

  template <typename... Args>
  inline void new_ui_message(const std::string &fmt, Args &&... args) {
    return new_ui_message(fmt.c_str(), std::forward<Args>(args)...);
  }

  void add_file_to_load(const std::string &file);

  std::optional<std::string> get_file_to_load();

  struct RawArray3 {
    int nx;
    int ny;
    int nt;
    std::string name;
    std::vector<float> data;

    RawArray3(int nx_, int ny_, int nt_, std::string name_, std::size_t data_size)
        : nx(nx_), ny(ny_), nt(nt_), name(std::move(name_)) {
      data.resize(data_size);
    }
  };

  void add_RawArray3_to_load(std::shared_ptr<RawArray3> arr);

  std::optional<std::shared_ptr<RawArray3>> get_rawarray3_to_load();
}  // namespace global