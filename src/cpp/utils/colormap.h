#pragma once

#include <array>

enum class ColorMap : int { GRAY, HSV, BLACKBODY, VIRIDIS, DIFF, DIFF_POS, DIFF_NEG };
inline const char *ColorMapsNames[7] = {"Gray", "HSV",           "Black Body",   "Viridis",
                                        "Diff", "Diff Positive", "Diff Negative"};

inline bool is_diff_colormap(const ColorMap &cmap) {
  return cmap == ColorMap::DIFF || cmap == ColorMap::DIFF_POS || cmap == ColorMap::DIFF_NEG;
}

std::array<float, 3 * 256> get_colormapdata(ColorMap cmap);