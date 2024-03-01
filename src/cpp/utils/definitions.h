#pragma once

#include <cmath>
#include <stdexcept>
#include <algorithm>

#include "utils/colormap.h"

enum class BitRange : int { NONE, U8, U10, U12, U16, FLOAT, DIFF, PHASE, PHASE_DIFF, I8 };
inline const char *BitRangeNames[] = {"MinMax", "uint8",   "uint10",  "uint12",  "uint16",
                                      "[0, 1]", "[-1, 1]", "[0, 2π]", "[-π, π]", "int8"};

enum class OpacityFunction : int {
  LINEAR,
  LINEAR_R,
  CENTERED,
  FIXED_100,
  FIXED_75,
  FIXED_50,
  FIXED_25,
  FIXED_0
};
inline const char *OpacityFunctionNames[] = {"Linear", "Linear_r", "Centered", "1.0",
                                             "0.75",   "0.5",      "0.25",     "0.0"};

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
      case BitRange::NONE:
        throw std::logic_error("NONE bitrange has no fixed min and max");
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
      if (min >= -1.1 && max <= 1.1) {
        return BitRange::DIFF;
      } else if (min >= -M_PI && max <= M_PI) {
        return BitRange::PHASE_DIFF;
      } else {
        return std::nullopt;
      }
    } else if (max <= 1.5) {
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