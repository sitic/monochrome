#pragma once
#include "imgui.h"

#include "utils/plot_utils.h"

void show_traces_ui(const SharedRecordingPtr &rec) {
  ImGui::BeginTabBar("##traces");
  // ImGui::Indent();

  if (ImGui::BeginTabItem("View")) {
    for (auto &trace : rec->traces) {
      ImGui::PushID(trace.id);

      int size = trace.data.size();
      if (size == 0) {
        ImGui::PopID();
        ImGui::EndTabItem();
        continue;
      }

      auto label = trace.pos.to_string();
      auto data  = trace.data.data();
      if (size > prm::trace_length) {
        data += (size - prm::trace_length);
        size = prm::trace_length;
      }

      trace.scale.left  = data - trace.data.data();
      trace.scale.right = trace.data.size();
      trace.scale.scale(data, data + size);
      ImPlot::SetNextAxisLinks(ImAxis_X1, trace.scale.scaleX ? &trace.scale.left : nullptr,
                               trace.scale.scaleX ? &trace.scale.right : nullptr);
      ImPlot::SetNextAxisLinks(ImAxis_Y1, trace.scale.scaleY ? &trace.scale.lower : nullptr,
                               trace.scale.scaleY ? &trace.scale.upper : nullptr);
      auto ptitle    = fmt::format("Pos {}, width {}###trace", label, Trace::width());
      auto plot_size = ImVec2(ImGui::GetContentRegionAvail().x, 180);
      if (ImPlot::BeginPlot(ptitle.c_str(), plot_size)) {
        ImPlot::SetupAxes(nullptr, nullptr);
        ImPlot::SetNextLineStyle({trace.color[0], trace.color[1], trace.color[2], trace.color[3]});
        auto title = "###ttrace" + label;
        ImPlot::PlotLine(title.c_str(), trace.data.data(), trace.data.size());
        ImPlotUtils::draw_liney(trace.scale.restarts);
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
    {
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
      ImGui::SliderInt("Trace length [frames]", &prm::trace_length, 10, prm::max_trace_length);
    }
    ImGui::Spacing();
    ImGui::SeparatorText("Individual Trace Settings");
    for (auto &trace : rec->traces) {
      ImGui::PushID(trace.id);
      auto title = "Pos " + trace.pos.to_string();
      auto color = ImVec4(trace.color[0], trace.color[1], trace.color[2], trace.color[3]);
      ImGui::PushStyleColor(ImGuiCol_Header, color);
      ImGui::PushStyleColor(ImGuiCol_HeaderHovered, color);
      bool dont_delete_trace = true;
      if (ImGui::CollapsingHeader(title.c_str(), &dont_delete_trace,
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        if (ImGui::Button(u8"Delete " ICON_FA_TRASH_ALT)) {
          rec->remove_trace(trace.pos);
        }

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.3f);
        if (ImGui::InputInt2("Center position", trace.pos.data())) {
          trace.pos[0] = std::clamp(trace.pos[0], 0, rec->Nx());
          trace.pos[1] = std::clamp(trace.pos[1], 0, rec->Ny());
        }

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.3f);
        ImGui::ColorEdit3("Color", trace.color.data());

        ImGui::Spacing();
        if (ImGui::Button("Export video trace as .txt file")) {
          auto &ctrl         = rec->export_ctrl.trace;
          ctrl.export_window = true;
          ctrl.assign_auto_filename(rec->path(), trace.pos, Trace::width());
          ctrl.tend = rec->length();
        }
        if (ImGui::Button("Export ROI as .npy file")) {
          auto &ctrl                      = rec->export_ctrl.raw;
          ctrl.export_window              = true;
          std::tie(ctrl.start, ctrl.size) = Trace::clamp(trace.pos, {rec->Nx(), rec->Ny()});
          ctrl.frames                     = {0, rec->length()};
          ctrl.assign_auto_filename(rec->path());
        }
        ImGui::Spacing();
        if (ImGui::Button("Reset data")) {
          trace.data.clear();
        }
        ImGui::Spacing();
        ImGui::Checkbox("Auto scale x-axis", &trace.scale.scaleX);
        ImGui::Checkbox("Auto scale y-axis", &trace.scale.scaleY);
        ImGui::Unindent();
      }
      if (!dont_delete_trace) rec->remove_trace(trace.pos);
      ImGui::PopStyleColor(2);
      ImGui::PopID();
    }
    ImGui::EndTabItem();
  }

  // ImGui::Unindent();
  ImGui::EndTabBar();
}