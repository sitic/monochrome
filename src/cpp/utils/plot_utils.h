#pragma once

#include <cmath>
#include <vector>
#include "implot.h"
#include "implot_internal.h"

namespace ImPlotUtils {
  void draw_liney(const std::vector<int> &values, ImVec4 color = {1, 0, 0, 1}, float thickness = 1) {
    ImPlotContext &gp = *ImPlot::GetCurrentContext();
    assert(gp.CurrentPlot != NULL);
    float yt             = gp.CurrentPlot->PlotRect.Min.y;
    float yb             = gp.CurrentPlot->PlotRect.Max.y;
    ImU32 col32          = ImGui::ColorConvertFloat4ToU32(color);
    ImDrawList &DrawList = *ImPlot::GetPlotDrawList();
    ImPlot::PushPlotClipRect();
    for (const auto &value : values) {
      float x = std::round(ImPlot::PlotToPixels(value, 0).x);
      if (x < gp.CurrentPlot->PlotRect.Min.x || x > gp.CurrentPlot->PlotRect.Max.x)
        continue;  // point is outside
      DrawList.AddLine(ImVec2(x, yt), ImVec2(x, yb), col32, thickness);
    }
    ImPlot::PopPlotClipRect();
  };
}  // namespace ImPlotUtils