#pragma once
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"

#include "recording_traces.h"
#include "recording_histogram.h"
#include "recording_controls.h"
#include "recording_points.h"

void show_recording_ui(const SharedRecordingPtr &rec, RecordingWindow *parent = nullptr) {
  auto name = fmt::format("{}###{}", rec->name(), static_cast<void *>(rec.get()));
  ImGui::SetNextItemOpen(true, ImGuiCond_Once);
  auto active = rec->active;
  if (ImGui::TreeNode(name.c_str())) { // ImGuiTreeNodeFlags_DefaultOpen
    if (!active) {
      ImGui::BeginDisabled();
    }
    
    ImGui::Spacing();
    // ImGui::Indent();
    
    int t = rec->current_frame();
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogram));
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderInt("##progress", &t, 0, rec->length() - 1, "Frame %d")) {
      rec->playback.set_next(t);
      for (auto &c : rec->children) {
        c->playback.set_next(t);
      }
    }
    ImGui::PopStyleColor(1);

    if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_AutoSelectNewTabs)) {
      ImGui::Indent();
      if (ImGui::BeginTabItem("Histogram")) {
        show_histogram_ui(rec, parent);
        ImGui::EndTabItem();
      }

      // Controls for flows
      if (!rec->flows.empty() && ImGui::BeginTabItem("Flows")) {
        show_flow_ui(rec);
        ImGui::EndTabItem();
      }

      if (!rec->points_videos.empty() && ImGui::BeginTabItem("Points")) {
        show_points_ui(rec);
        ImGui::EndTabItem();
      }

      // Plot traces and show their controls
      if (!rec->traces.empty() && ImGui::BeginTabItem("Traces")) {
        show_traces_ui(rec);
        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Metadata & Controls")) {
        if (show_controls_ui(rec, parent)) {
          // ImGui::EndTabItem();
          // ImGui::EndTabBar();
          // ImGui::Unindent();
          // ImGui::TreePop();
          // return rec_nr;
        }
        ImGui::EndTabItem();
      }
      ImGui::Unindent();
      ImGui::EndTabBar();
    }

    for (const auto &crec : rec->children) {
      show_recording_ui(crec, rec.get());
    }
    // ImGui::Unindent();

    // Actually delete children which have been selected for deletionq
    rec->children.erase(std::remove_if(rec->children.begin(), rec->children.end(),
                                      [](const auto &r) { return r->glcontext == r->window; }),
                        rec->children.end());

    if (!active) {
      ImGui::EndDisabled();
    }
    ImGui::TreePop();
  }
}
