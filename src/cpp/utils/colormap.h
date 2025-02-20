#pragma once

#include <array>

// All ColorMaps, only append to this list and copy it to the IPC schema!
enum class ColorMap : int { GRAY, HSV, BLACKBODY, VIRIDIS, PRGn, PRGn_POS, PRGn_NEG, RdBu, Tab10, Turbo, CMOCEAN_PHASE };
// Names of the ColorMaps for UI
inline const char *ColorMapsNames[11] = {"Gray",  "HSV",           "Black Body",    "Viridis",
                                         "PRGn",  "PRGn Positive", "PRGn Negative", "RdBu",
                                         "Tab10", "Turbo", "cmocean:phase"};
// Order of the ColorMaps in UI
inline const ColorMap ColorMapDisplayOrder[11] = {
    ColorMap::GRAY, ColorMap::BLACKBODY,     ColorMap::VIRIDIS,  ColorMap::Turbo,
    ColorMap::RdBu, ColorMap::PRGn,          ColorMap::PRGn_POS, ColorMap::PRGn_NEG,
    ColorMap::HSV,  ColorMap::CMOCEAN_PHASE, ColorMap::Tab10};

inline bool is_diff_colormap(const ColorMap &cmap) {
  return cmap == ColorMap::PRGn || cmap == ColorMap::PRGn_POS || cmap == ColorMap::PRGn_NEG ||
         cmap == ColorMap::RdBu;
}

std::array<float, 3 * 256> get_colormapdata(ColorMap cmap);