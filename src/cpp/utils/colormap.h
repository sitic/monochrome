#pragma once

#include <array>


enum class ColorMap : int { GRAY, HSV, BLACKBODY, VIRIDIS, PRGn, PRGn_POS, PRGn_NEG, RdBu, Tab10 };
inline const char *ColorMapsNames[9] = {"Gray", "HSV",           "Black Body",    "Viridis",
                                        "PRGn", "PRGn Positive", "PRGn Negative", "RdBu",
                                        "tab10"};

inline bool is_diff_colormap(const ColorMap &cmap) {
  return cmap == ColorMap::PRGn || cmap == ColorMap::PRGn_POS || cmap == ColorMap::PRGn_NEG ||
         cmap == ColorMap::RdBu;
}

std::array<float, 3 * 256> get_colormapdata(ColorMap cmap);