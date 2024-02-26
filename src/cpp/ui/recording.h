#pragma once
#include "recording_traces.h"
#include "recording_histogram.h"
#include "recording_controls.h"
#include "recording_points.h"

int show_recording_ui(const SharedRecordingPtr &rec, int rec_nr, RecordingWindow *parent = nullptr) {
  auto name = fmt::format("{}###{}", rec->name(), static_cast<void *>(rec.get()));
  if (ImGui::CollapsingHeader(name.c_str(),
                              ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth)) {
    if (!rec->active) {
      // layer was just activated
      rec->active   = true;
      if (!parent) {
        glfwShowWindow(rec->window);
      } else {
        rec->playback = parent->playback;
      }
    }
  } else {
    rec->active = false;
    if (!parent) {
      glfwHideWindow(rec->window);
    }
    return rec_nr;
  }
  ImGui::PushID(rec.get());

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
      auto rec_name = rec->name();
      if (ImGui::InputText("Name", &rec_name)) {
        rec->set_name(rec_name);
      }
      ImGui::Checkbox("Show", &rec->active);

      ImGui::Separator();

      display_recording_metadata(rec);

      ImGui::Separator();

      if (display_recording_buttons(rec, parent)) {
        ImGui::Columns(1);
        ImGui::PopID();
        ImGui::EndTabItem();
        ImGui::EndTabBar();
        return rec_nr;
      }

      ImGui::Separator();
      show_transformations_ui();

      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }

  ImGui::PopID();
  for (const auto &crec : rec->children) {
    rec_nr = show_recording_ui(crec, rec_nr, rec.get());
  }
  // Actually delete children which have been selected for deletionq
  rec->children.erase(std::remove_if(rec->children.begin(), rec->children.end(),
                                     [](const auto &r) { return r->glcontext == r->window; }),
                      rec->children.end());
  return rec_nr + 1;
}
