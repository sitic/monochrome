#pragma once
#include "imgui.h"

#include "utils/plot_utils.h"

void show_traces_ui(const SharedRecordingPtr &rec) {
  ImGui::BeginTabBar("##traces");
  // ImGui::Indent();

  if (ImGui::BeginTabItem("View")) {
    for (auto &trace : rec->traces) {
      ImGui::PushID(trace.id);

      // Update trace if needed
      trace.tick(*rec);
      if (trace.future_data_ptr && trace.future_data_ptr->future.valid()) {
        auto fdata = trace.future_data_ptr;
        using namespace std::chrono_literals;
        if (fdata->future.wait_for(0ms) == std::future_status::ready) {
          if (fdata->cancelled) {
            trace.future_data_ptr = nullptr;
            trace.data.clear();
          } else{
            trace.data = fdata->future.get();
            trace.has_new_data = true;
            trace.future_data_ptr = nullptr;
          }
        }
      }

      if (trace.has_new_data) {
        ImPlot::SetNextAxisToFit(ImAxis_Y1);
        trace.has_new_data = false;
      }
      ImPlot::SetNextAxisLimits(ImAxis_X1, 0, rec->length() - 1, ImGuiCond_Once);
      auto label = trace.original_position.to_string();
      auto ptitle    = fmt::format("Pos {}, width {}###trace", label, Trace::width());
      auto plot_size = ImVec2(ImGui::GetContentRegionAvail().x, 180);
      if (ImPlot::BeginPlot(ptitle.c_str(), plot_size)) {
        ImPlot::SetupAxes(nullptr, nullptr);
        ImPlot::SetNextLineStyle({trace.color[0], trace.color[1], trace.color[2], trace.color[3]});
        auto title = "###ttrace" + label;
        ImPlot::PlotLine(title.c_str(), trace.data.data(), trace.data.size());
        ImPlotUtils::draw_liney({rec->current_frame()});

        if (trace.future_data_ptr) {
          auto limits = ImPlot::GetPlotLimits();
          ImVec2 text_size = ImGui::CalcTextSize("LOADING...");
          ImVec2 center_screen = ImPlot::PlotToPixels(ImPlotPoint((limits.X.Max + limits.X.Min) / 2, (limits.Y.Max + limits.Y.Min) / 2));
          ImVec2 text_pos = {center_screen.x - text_size.x * 0.5f, center_screen.y - text_size.y * 0.5f};

          float padding = 5.0f; // Padding around the text
          ImU32 rect_color = ImGui::ColorConvertFloat4ToU32(ImVec4(0.2f, 0.2f, 0.2f, 0.8f));
          ImDrawList* draw_list = ImPlot::GetPlotDrawList();
          ImPlot::GetPlotDrawList()->AddRectFilled(
            ImVec2(text_pos.x - padding, text_pos.y - padding),
            ImVec2(text_pos.x + text_size.x + padding, text_pos.y + text_size.y + padding),
            rect_color
          );
          ImPlot::GetPlotDrawList()->AddText(text_pos, IM_COL32_WHITE, "LOADING...");
        }
        ImPlot::EndPlot();
      }
      ImGui::PopID();
    }
    ImGui::EndTabItem();
  }

  if (ImGui::BeginTabItem("Settings")) {
    ImGui::SeparatorText("Global Settings");
    {
      int trace_width = Trace::width();
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
      if (ImGui::InputInt("ROI width [px]", &trace_width, 2, 5)) {
        Trace::width(trace_width);
      }
    }
    ImGui::Spacing();
    ImGui::SeparatorText("Individual Trace Settings");
    for (auto &trace : rec->traces) {
      ImGui::PushID(trace.id);
      auto title = "Position " + trace.original_position.to_string();
      auto color = ImVec4(trace.color[0], trace.color[1], trace.color[2], trace.color[3]);
      ImGui::PushStyleColor(ImGuiCol_Header, color);
      ImGui::PushStyleColor(ImGuiCol_HeaderHovered, color);
      bool dont_delete_trace = true;
      if (ImGui::CollapsingHeader(title.c_str(), &dont_delete_trace,
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        if (ImGui::Button(u8"Delete " ICON_FA_TRASH_ALT, ImVec2(ImGui::GetContentRegionAvail().x * 0.3f, 0))) {
          rec->remove_trace(trace.pos);
        }

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.3f);
        if (ImGui::InputInt2("Center position", trace.original_position.data())) {
          trace.original_position[0] = std::clamp(trace.original_position[0], 0, rec->file()->Nx() - 1);
          trace.original_position[1] = std::clamp(trace.original_position[1], 0, rec->file()->Ny() - 1);
          trace.pos = rec->inverse_transformation(trace.original_position);
        }

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.3f);
        {
          auto [start, size] = Trace::clamp(trace.original_position, {rec->file()->Nx(), rec->file()->Ny()});
          static std::string str;
          str = fmt::format("[{0}:{1}, {2}:{3}]", start[0], start[0] + size[0], start[1],
                               start[1] + size[1]);
          ImGui::InputText("ROI coordinates###trace_pos", &str, ImGuiInputTextFlags_ReadOnly);
        }

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.3f);
        ImGui::ColorEdit3("Color", trace.color.data());
        ImGui::Unindent();
      }
      if (!dont_delete_trace) rec->remove_trace(trace.pos);
      ImGui::PopStyleColor(2);
      ImGui::PopID();
    }
    ImGui::EndTabItem();
  }

  if (ImGui::BeginTabItem("Export")) {
    for (auto &trace : rec->traces) {
      ImGui::PushID(trace.id);
      auto title = "Pos " + trace.pos.to_string();
      auto color = ImVec4(trace.color[0], trace.color[1], trace.color[2], trace.color[3]);
      ImGui::PushStyleColor(ImGuiCol_Header, color);
      ImGui::PushStyleColor(ImGuiCol_HeaderHovered, color);
      if (ImGui::CollapsingHeader(title.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        if (ImGui::Button("Export video trace as .txt file")) {
          auto &ctrl         = rec->export_ctrl.trace;
          ctrl.export_window = true;
          ctrl.assign_auto_filename(rec->path(), trace, Trace::width());
        }
        if (ImGui::Button("Export ROI as .npy file")) {
          auto &ctrl                      = rec->export_ctrl.raw;
          ctrl.export_window              = true;
          std::tie(ctrl.start, ctrl.size) = Trace::clamp(trace.pos, {rec->Nx(), rec->Ny()});
          ctrl.frames                     = {0, rec->length()};
          ctrl.assign_auto_filename(rec->path());
        }
        ImGui::Unindent();
      }
      ImGui::PopStyleColor(2);
      ImGui::PopID();
    }
    ImGui::EndTabItem();
  }

  // ImGui::Unindent();
  ImGui::EndTabBar();
}