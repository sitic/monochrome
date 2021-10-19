#pragma once

#include <array>

enum class ColorMap : int { GRAY, DIFF, HSV, BLACKBODY, VIRIDIS, DIFF_POS, DIFF_NEG };
inline const char *ColorMapsNames[7] = {"Gray",    "Diff",          "HSV",          "Black Body",
                                        "Viridis", "Diff Positive", "Diff Negative"};

inline bool is_diff_colormap(const ColorMap &cmap) {
  return cmap == ColorMap::DIFF || cmap == ColorMap::DIFF_POS || cmap == ColorMap::DIFF_NEG;
}

std::array<float, 3 * 256> get_colormapdata(ColorMap cmap);