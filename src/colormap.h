#pragma once

#include <array>

enum class ColorMap : int { GRAY, DIFF, HSV, BLACKBODY};
inline const char *ColorMapsNames[4] = {"Gray", "Diff", "HSV", "Black Body"};


std::array<float, 3 * 256> get_colormapdata(ColorMap cmap);