#pragma once

#include <string>
#include <optional>
#include <utility>
#include <memory>
#include <variant>
#include <vector>

#include <fmt/format.h>

#include "utils/vectors.h"
#include "utils/definitions.h"

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

    Message(std::string msg);
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

  struct RawArray3MetaData {
    int nx = -1;
    int ny = -1;
    int nt = -1;

    std::string name;
    float duration = 0;  // in seconds
    float fps      = 0;
    std::string date;
    std::string comment;
    std::optional<BitRange> bitrange                          = std::nullopt;
    std::optional<ColorMap> cmap                              = std::nullopt;
    std::optional<std::string> parentName                     = std::nullopt;
    std::optional<TransferFunction> transfer_fct              = std::nullopt;
    std::vector<std::pair<std::string, std::string>> metaData = {};

    bool is_flowfield = false;
  };

  class RawArray3 {
   private:
    RawArray3() = default;

   public:
    RawArray3MetaData meta;
    std::variant<std::vector<float>, std::vector<uint16_t>> data;

    static std::shared_ptr<RawArray3> create_float(RawArray3MetaData metadata_,
                                                   std::size_t data_size) {
      auto a  = std::shared_ptr<RawArray3>(new RawArray3);
      a->meta = std::move(metadata_);
      a->data = std::vector<float>(data_size);
      return a;
    }

    static std::shared_ptr<RawArray3> create_u16(RawArray3MetaData metadata_,
                                                 std::size_t data_size) {
      auto a  = std::shared_ptr<RawArray3>(new RawArray3);
      a->meta = std::move(metadata_);
      a->data = std::vector<uint16_t>(data_size);
      return a;
    }

    std::size_t size() const {
      return std::visit([](auto &v) { return v.size(); }, data);
    }
  };  // namespace global

  void add_RawArray3_to_load(std::shared_ptr<RawArray3> arr);

  std::optional<std::shared_ptr<RawArray3>> get_rawarray3_to_load();

  void close_window(const std::string& recording_name);
  void close_all_windows();

  std::vector<std::pair<std::string, std::vector<Vec2i>>> get_trace_pos();
}  // namespace global