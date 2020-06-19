#pragma once

#include <array>

enum class ColorMap : int { GRAY, DIFF, HSV, BLACKBODY, DIFF_POS, DIFF_NEG};
inline const char *ColorMapsNames[6] = {"Gray", "Diff", "HSV", "Black Body", "Diff Positive", "Diff Negative"};


std::array<float, 3 * 256> get_colormapdata(ColorMap cmap);