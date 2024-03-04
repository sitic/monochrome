#pragma once
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"

#include "recording_traces.h"
#include "recording_histogram.h"
#include "recording_controls.h"
#include "recording_points.h"

void show_progess_slider(const SharedRecordingPtr &rec) {
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
}

void show_recording_ui(const SharedRecordingPtr &rec, RecordingWindow *parent = nullptr) {
  ImGui::PushID(rec.get());

  auto name = fmt::format("{}###{}", rec->name(), static_cast<void *>(rec.get()));
  ImGui::SetNextItemOpen(true, ImGuiCond_Once);
  auto active                = rec->active;
  bool dont_delete_recording = true;
  if (ImGui::CollapsingHeader(name.c_str(), &dont_delete_recording)) {
    // if (ImGui::TreeNode(name.c_str())) { // ImGuiTreeNodeFlags_DefaultOpen
    if (!active) {
      ImGui::BeginDisabled();
    }

    ImGui::Spacing();
    ImGui::Indent();
    ImGuiChildFlags child_flags = ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY |
                                  ImGuiChildFlags_AlwaysUseWindowPadding;
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
    ImGui::BeginChild(name.c_str(), ImVec2(-FLT_MIN, 0), child_flags);

    show_progess_slider(rec);

    if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_AutoSelectNewTabs)) {
      // ImGui::Indent();
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
        show_controls_ui(rec, parent);
        ImGui::EndTabItem();
      }
      // ImGui::Unindent();
      ImGui::EndTabBar();
    }

    if (!rec->children.empty()) {
      ImGui::SeparatorText("Overlays");

      for (const auto &crec : rec->children) {
        show_recording_ui(crec, rec.get());
      }
    }

    // Actually delete children which have been selected for deletionq
    rec->children.erase(std::remove_if(rec->children.begin(), rec->children.end(),
                                       [](const auto &r) { return r->glcontext == r->window; }),
                        rec->children.end());
    // ImGui::TreePop();
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::Unindent();

    if (!active) {
      ImGui::EndDisabled();
    }
  }

  if (!dont_delete_recording) {
    if (parent) {
      // Child will be deleted later
      rec->set_context(nullptr);
    } else {
      glfwSetWindowShouldClose(rec->window, GLFW_TRUE);
    }
  }
  ImGui::PopID();
}
