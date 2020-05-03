#pragma once

#include <string>
#include <optional>
#include <utility>
#include <memory>
#include <vector>

#include <fmt/format.h>

#include "colormap.h"

enum class BitRange : int { U8, U10, U12, U16, FLOAT, DIFF, PHASE, PHASE_DIFF };
inline const char *BitRangeNames[8] = {"uint8",  "uint10",  "uint12",  "uint16",
                                       "[0, 1]", "[-1, 1]", "[0, 2π]", "[-π, π]"};

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
    int nx;
    int ny;
    int nt;

    std::string name;
    float duration                   = 0;  // in seconds
    float fps                        = 0;
    std::string date                 = "";
    std::string comment              = "";
    std::optional<BitRange> bitrange = std::nullopt;
    std::optional<ColorMap> cmap     = std::nullopt;
  };

  struct RawArray3 {
    RawArray3MetaData meta;
    std::vector<float> data;

    RawArray3(RawArray3MetaData metadata_, std::size_t data_size) : meta(std::move(metadata_)) {
      data.resize(data_size);
    }
  };

  void add_RawArray3_to_load(std::shared_ptr<RawArray3> arr);

  std::optional<std::shared_ptr<RawArray3>> get_rawarray3_to_load();
}  // namespace global

constexpr std::pair<float, float> bitrange_to_float(BitRange br) {
  switch (br) {
    case BitRange::FLOAT:
      return {0, 1};
    case BitRange::U8:
      return {0, (1 << 8) - 1};
    case BitRange::U10:
      return {0, (1 << 10) - 1};
    case BitRange::U12:
      return {0, (1 << 12) - 1};
    case BitRange::U16:
      return {0, (1 << 16) - 1};
    case BitRange::DIFF:
      return {-1, 1};
    case BitRange::PHASE:
      return {0, 2 * M_PI};
    case BitRange::PHASE_DIFF:
      return {-M_PI, M_PI};
  }
  throw std::logic_error("This line should not be reached");
}

template <typename It>
std::optional<BitRange> detect_bitrange(It begin, It end) {
  auto [min, max] = std::minmax_element(begin, end);
  if (*min == *max) {
    return std::nullopt;
  }
  if (*min < 0) {
    if (*min >= -1 && *max <= 1) {
      return BitRange::DIFF;
    } else if (*min >= -M_PI && *max <= M_PI) {
      return BitRange::PHASE_DIFF;
    } else {
      return std::nullopt;
    }
  } else if (*max <= 1) {
    return BitRange::FLOAT;
  } else if (*max <= 2 * M_PI) {
    return BitRange::PHASE;
  } else if (*max < (1 << 8)) {
    return BitRange::U8;
  } else if (*max < (1 << 10)) {
    return BitRange::U10;
  } else if (*max < (1 << 12)) {
    return BitRange::U12;
  } else {
    return BitRange::U16;
  }
}