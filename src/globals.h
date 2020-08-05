#pragma once

#include <string>
#include <optional>
#include <utility>
#include <memory>
#include <variant>
#include <vector>

#include <fmt/format.h>

#include "colormap.h"

enum class BitRange : int { U8, U10, U12, U16, FLOAT, DIFF, PHASE, PHASE_DIFF, I8 };
inline const char *BitRangeNames[9] = {"uint8",   "uint10",  "uint12",  "uint16", "[0, 1]",
                                       "[-1, 1]", "[0, 2π]", "[-π, π]", "int8"};

enum class TransferFunction : int { LINEAR, DIFF, DIFF_POS, DIFF_NEG };
inline const char *TransferFunctionNames[4] = {"Linear", "Difference", "Difference Positive",
                                               "Difference Negative"};

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
    std::optional<BitRange> bitrange             = std::nullopt;
    std::optional<ColorMap> cmap                 = std::nullopt;
    std::optional<std::string> parentName        = std::nullopt;
    std::optional<TransferFunction> transfer_fct = std::nullopt;

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
}  // namespace global

namespace utils {
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
      case BitRange::I8:
        return {-125, 125};
        //throw std::logic_error("Don't call with function with auto");
    }
    throw std::logic_error("This line should not be reached");
  }

  template <typename It>
  using ValueType = typename std::iterator_traits<It>::value_type;

  template <typename It>
  std::pair<ValueType<It>, ValueType<It>> minmax_element_skipNaN(It first, It last) {
    // If arguments point to non-floating point values fallback to std::minmax_element(), because
    // only floating point values can have NaN
    if constexpr (!std::is_same<ValueType<It>, float>::value) {
      auto [min, max] = std::minmax_element(first, last);
      return {*min, *max};
    } else {
      It min = first, max = first;
      for (; first < last; first++) {
        if (!std::isnan(*first)) {
          min = first;
          max = first;
          break;
        }
      }
      if (first == last) return {0, 0};  // everything is NaN

      while (++first != last) {
        if (std::isnan(*first)) {
          continue;
        } else if (*first > *max) {
          max = first;
        } else if (*first < *min) {
          min = first;
        }
      }
      return {*min, *max};
    }
  }
  template <typename It>
  std::optional<BitRange> detect_bitrange(It begin, It end) {
    auto [min, max] = minmax_element_skipNaN(begin, end);
    if (min == max) {
      return std::nullopt;
    }
    if (min < 0) {
      if (min >= -1 && max <= 1) {
        return BitRange::DIFF;
      } else if (min >= -M_PI && max <= M_PI) {
        return BitRange::PHASE_DIFF;
      } else {
        return std::nullopt;
      }
    } else if (max <= 1) {
      return BitRange::FLOAT;
    } else if (max <= 2 * M_PI) {
      return BitRange::PHASE;
    } else if (max < (1 << 8)) {
      return BitRange::U8;
    } else if (max < (1 << 10)) {
      return BitRange::U10;
    } else if (max < (1 << 12)) {
      return BitRange::U12;
    } else {
      return BitRange::U16;
    }
  }
}  // namespace utils