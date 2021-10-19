#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_stdlib.h"
#include "implot.h"
#include "utils/plot_utils.h"
#include "globals.h"

namespace global {
  extern GLFWwindow *main_window;
  extern std::vector<SharedRecordingPtr> recordings;
  extern std::queue<std::tuple<SharedRecordingPtr, SharedRecordingPtr, bool>> merge_queue;

  template <typename Func>
  void do_forall_recordings(Func &&f) {
    for (const auto &rec : recordings) {
      f(rec);
      for (const auto &crec : rec->children) {
        f(crec);
      }
    }
  }
}  // namespace global

namespace prm {
  int main_window_width     = 0;
  int main_window_multipier = 1;
  int main_window_height    = 0;
  int trace_length          = 200;
  int max_trace_length      = 2000;
  int display_fps           = 60;
  double lastframetime      = 0;

  Filters prefilter              = Filters::None;
  Transformations transformation = Transformations::None;
  Filters postfilter             = Filters::None;

  const std::array<ColorMap, 7> cmaps = {ColorMap::GRAY,      ColorMap::DIFF,     ColorMap::HSV,
                                         ColorMap::BLACKBODY, ColorMap::DIFF_POS, ColorMap::DIFF_NEG,
                                         ColorMap::VIRIDIS};
  std::map<ColorMap, GLuint> cmap_texs;
}  // namespace prm

void show_main_ui() {
  ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
  ImGui::SetNextWindowSizeConstraints(ImVec2(prm::main_window_width, 0),
                                      ImVec2(prm::main_window_width, FLT_MAX));
  auto flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize |
               ImGuiWindowFlags_NoSavedSettings;
  ImGui::Begin("Drag & drop .dat or .npy files into this window", nullptr, flags);

  {
    ImGui::Columns(2);
    {
      if (ImGui::Button(ICON_FA_REDO_ALT)) {
        global::do_forall_recordings([](auto &r) { r->playback.restart(); });
      }
      ImGui::SameLine();
      if (prm::playbackCtrl.val == 0) {
        if (ImGui::Button(ICON_FA_PLAY)) {
          prm::playbackCtrl.toggle_play_pause();
        }
      } else {
        if (ImGui::Button(ICON_FA_PAUSE)) {
          prm::playbackCtrl.toggle_play_pause();
        }
      }
      ImGui::SameLine();
      if (ImGui::Button(ICON_FA_BACKWARD)) {
        prm::playbackCtrl.val /= 2;
      }
      ImGui::SameLine();
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.75f);
      ImGui::DragFloat("##speed", &prm::playbackCtrl.val, 0.05, 0, 20, "playback speed = %.2f");
      ImGui::SameLine();
      if (ImGui::Button(ICON_FA_FORWARD)) {
        prm::playbackCtrl.val *= 2;
      }
      ImGui::SameLine();
      if (ImGui::Button(ICON_FA_FAST_FORWARD)) {
        if (prm::playbackCtrl.val == 1) {
          prm::playbackCtrl.val += 9;
        } else {
          prm::playbackCtrl.val += 10;
        }
      }
    }

    ImGui::NextColumn();
    {
      bool resize_windows = false;
      if (ImGui::Button(ICON_FA_SEARCH_MINUS)) {
        RecordingWindow::scale_fct /= 2;
        resize_windows = true;
      }
      ImGui::SameLine();
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.75f);
      if (ImGui::DragFloat("##scaling", &RecordingWindow::scale_fct, 0.05, 0.5, 10,
                           "window scaling = %.1f")) {
        resize_windows = true;
      }
      ImGui::SameLine();
      if (ImGui::Button(ICON_FA_SEARCH_PLUS)) {
        RecordingWindow::scale_fct *= 2;
        resize_windows = true;
      }
      if (resize_windows && RecordingWindow::scale_fct != 0.f) {
        for (const auto &r : global::recordings) {
          r->resize_window();
        }
      }
    }

    ImGui::NextColumn();
    {
      int trace_width = Trace::width();
      if (ImGui::InputInt("ROI Width", &trace_width, 2, 5)) {
        Trace::width(trace_width);
      }
    }

    ImGui::NextColumn();
    {
      ImGui::Text("Image Flip");
      ImGui::SameLine();
      if (ImGui::Button(ICON_MDI_FLIP_VERTICAL)) RecordingWindow::flipud();
      ImGui::SameLine();
      if (ImGui::Button(ICON_MDI_FLIP_HORIZONTAL)) RecordingWindow::fliplr();
      ImGui::SameLine();
      ImGui::Text("Rotate");
      ImGui::SameLine();
      if (ImGui::Button(ICON_MDI_ROTATE_LEFT)) RecordingWindow::add_rotation(-90);
      ImGui::SameLine();
      if (ImGui::Button(ICON_MDI_ROTATE_RIGHT)) RecordingWindow::add_rotation(90);
      ImGui::SameLine();
      if (ImGui::Button(ICON_FA_REMOVE_FORMAT)) RecordingWindow::set_rotation(0);
    }

    ImGui::NextColumn();
    { ImGui::SliderInt("Trace Length", &prm::trace_length, 10, prm::max_trace_length); }

    ImGui::NextColumn();
    {
      int max_display_fps = prm::display_fps;
      auto label = fmt::format("Max FPS (current avg. {:.0f}fps)###dfps", ImGui::GetIO().Framerate);
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.25f);
      if (ImGui::InputInt(label.c_str(), &max_display_fps)) {
        if (ImGui::IsItemDeactivated() && max_display_fps > 0) {
          prm::display_fps   = max_display_fps;
          prm::lastframetime = glfwGetTime();
        }
      }
    }

    ImGui::Columns(1);
  }

  ImGui::Separator();
  auto selectable_factory = [](auto &p, auto default_val) {
    return [&p, default_val](const char *label, auto e) {
      bool is_active = p == e;
      if (ImGui::Selectable(label, is_active)) {

        p = is_active ? default_val : e;

        global::do_forall_recordings([](auto &r) { r->reset_traces(); });
      }

      return is_active;
    };
  };

  auto kernel_size_select = [](unsigned int &val, auto reset_fn) {
    ImGui::Indent(10);
    const int step = 2;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
    if (ImGui::InputScalar("Kernel size", ImGuiDataType_U32, &val, &step, nullptr, "%d")) {
      global::do_forall_recordings([&reset_fn](auto &r) { reset_fn(r.get()); });
    }
    ImGui::Unindent(10);
  };

  ImGui::Columns(3);
  ImGui::SetNextItemOpen(true, ImGuiCond_Once);
  if (ImGui::TreeNode("Pre Filters")) {
    auto selectable = selectable_factory(prm::prefilter, Filters::None);
    if (selectable("Gauss", Filters::Gauss)) {
      ImGui::Indent(10);
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
      float sigma = Transformation::GaussFilter::get_sigma();
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 1.f);
      if (ImGui::DragFloat("##sigma", &sigma, 0.01, 0, 5, u8"σ = %.2f")) {
        Transformation::GaussFilter::set_sigma(sigma);
      }
      ImGui::Unindent(10);
    }
    if (selectable("Mean", Filters::Mean)) {
      kernel_size_select(Transformation::MeanFilter::kernel_size, [](RecordingWindow *r) {
        auto transform = r->transformationArena.create_if_needed(Filters::Mean, 0);
        auto c         = dynamic_cast<Transformation::MeanFilter *>(transform);
        assert(c);
        c->reset();
      });
    }
    if (selectable("Median", Filters::Median)) {
      kernel_size_select(Transformation::MedianFilter::kernel_size, [](RecordingWindow *r) {
        auto transform = r->transformationArena.create_if_needed(Filters::Median, 0);
        auto c         = dynamic_cast<Transformation::MedianFilter *>(transform);
        assert(c);
        c->reset();
      });
    }
    ImGui::TreePop();
  }

  ImGui::NextColumn();

  ImGui::SetNextItemOpen(true, ImGuiCond_Once);
  if (ImGui::TreeNode("Transformations")) {
    auto selectable = selectable_factory(prm::transformation, Transformations::None);
    if (selectable("Frame Difference", Transformations::FrameDiff)) {
      ImGui::Indent(10);
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
      ImGui::SliderInt("Frames", &Transformation::FrameDiff::n_frame_diff, 1, 100);
      if (ImGui::Button("Add As Overlays")) {
        std::vector<std::pair<SharedRecordingPtr, SharedRecordingPtr>> new_recordings;
        for (auto rec : global::recordings) {
          auto r = std::make_shared<FixedTransformRecordingWindow>(
              rec, prm::prefilter, prm::transformation, prm::postfilter, "Frame Difference");
          new_recordings.push_back({r, rec});
        }
        for (auto [r, rec] : new_recordings) {
          global::recordings.push_back(r);
          r->open_window();
          global::merge_queue.push({r, rec, false});
        }
        prm::transformation = Transformations::None;
        prm::prefilter      = Filters::None;
        prm::postfilter     = Filters::None;
      }
      ImGui::Unindent(10);
    }
    if (selectable("Contrast Enhancement", Transformations::ContrastEnhancement)) {
      ImGui::Indent(10);
      const int step = 2;
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
      if (ImGui::InputScalar("Kernel size", ImGuiDataType_U32,
                             &Transformation::ContrastEnhancement::kernel_size, &step, nullptr,
                             "%d")) {
        global::do_forall_recordings([](auto &r) {
          auto transform =
              r->transformationArena.create_if_needed(Transformations::ContrastEnhancement, 0);
          auto c = dynamic_cast<Transformation::ContrastEnhancement *>(transform);
          assert(c);
          c->reset();
        });
      }
      ImGui::SliderInt("Mask", &Transformation::ContrastEnhancement::maskVersion, 0, 2);
      ImGui::Unindent(10);
    }
    ImGui::TreePop();
  }

  ImGui::NextColumn();

  ImGui::SetNextItemOpen(true, ImGuiCond_Once);
  if (ImGui::TreeNode("Post Filters")) {
    auto selectable = selectable_factory(prm::postfilter, Filters::None);
    if (selectable("Gauss", Filters::Gauss)) {
      ImGui::Indent(10);
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
      float sigma = Transformation::GaussFilter::get_sigma();
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 1.f);
      if (ImGui::DragFloat("##sigma", &sigma, 0.01, 0, 5, u8"σ = %.2f")) {
        Transformation::GaussFilter::set_sigma(sigma);
      }
      ImGui::Unindent(10);
    }
    if (selectable("Mean", Filters::Mean)) {
      kernel_size_select(Transformation::MeanFilter::kernel_size, [](RecordingWindow *r) {
        auto transform = r->transformationArena.create_if_needed(Filters::Mean, 1);
        auto c         = dynamic_cast<Transformation::MeanFilter *>(transform);
        assert(c);
        c->reset();
      });
    }
    if (selectable("Median", Filters::Median)) {
      kernel_size_select(Transformation::MedianFilter::kernel_size, [](RecordingWindow *r) {
        auto transform = r->transformationArena.create_if_needed(Filters::Median, 1);
        auto c         = dynamic_cast<Transformation::MedianFilter *>(transform);
        assert(c);
        c->reset();
      });
    }
    ImGui::TreePop();
  }
  ImGui::Columns(1);
  ImGui::End();
}

int show_recording_ui(const SharedRecordingPtr &rec, int rec_nr, RecordingWindow *parent = nullptr) {
  auto name = rec->name();
  if (name.empty()) name = fmt::format("##{}", static_cast<void *>(rec.get()));
  if (!parent) {
    int x           = std::clamp(rec_nr / 3, 0, prm::main_window_multipier - 1);
    float y         = (rec_nr % 3) * 0.3f + 0.2f * (x == 0);
    auto window_pos = ImVec2(x * prm::main_window_width, y * prm::main_window_height);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(prm::main_window_width, 0), ImVec2(FLT_MAX, FLT_MAX));
    ImGui::SetNextWindowCollapsed(!rec->active, ImGuiCond_Always);
    rec->active = ImGui::Begin(name.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  } else {
    if (ImGui::CollapsingHeader(name.c_str(),
                                ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth)) {
      if (!rec->active) {
        // layer was just activated
        rec->active   = true;
        rec->playback = parent->playback;
      }
    } else {
      rec->active = false;
      return rec_nr;
    }
  }
  ImGui::PushID(rec.get());
  int t = rec->current_frame();
  ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogram));
  ImGui::SetNextItemWidth(-1);
  if (ImGui::SliderInt("##progress", &t, 0, rec->length() - 1, "Frame %d")) {
    if (t < rec->length()) {
      rec->playback.set_next(t);
    }
  }
  ImGui::PopStyleColor(1);

  // Show metadata
  if (!rec->date().empty()) ImGui::TextWrapped("Date: %s", rec->date().c_str());
  if (!rec->comment().empty()) {
    ImGui::TextWrapped("Comment: %s", rec->comment().c_str());
    if (rec->file()->capabilities()[FileCapabilities::SET_COMMENT]) {
      ImGui::SameLine();
      if (ImGui::SmallButton(ICON_FA_EDIT)) {
        rec->comment_edit_ui.comment = rec->comment();
        rec->comment_edit_ui.show    = true;
      }

      if (rec->comment_edit_ui.show) {
        auto window_name = fmt::format("Edit Comment: {}", rec->name());
        ImGui::Begin(window_name.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::InputText("##comment", &rec->comment_edit_ui.comment);
        if (ImGui::Button("Save")) {
          rec->file()->set_comment(rec->comment_edit_ui.comment);
          rec->comment_edit_ui.show = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
          rec->comment_edit_ui.show    = false;
          rec->comment_edit_ui.comment = rec->comment();
        }
        ImGui::End();
      }
    }
  }
  ImGui::Columns(3);
  ImGui::Text("Frames %d", rec->length());
  ImGui::NextColumn();
  ImGui::Text("Width  %d", rec->Nx());
  ImGui::NextColumn();
  ImGui::Text("Height %d", rec->Ny());
  ImGui::NextColumn();
  if (rec->duration().count() > 0) {
    ImGui::TextWrapped("Duration  %.3fs", rec->duration().count());
    ImGui::NextColumn();
  }
  if (rec->fps() != 0) {
    ImGui::TextWrapped("FPS  %.3f", rec->fps());
    ImGui::NextColumn();
  }
  for (const auto &v : rec->file()->metadata()) {
    ImGui::TextWrapped("%s %s", v.first.c_str(), v.second.c_str());
    ImGui::NextColumn();
  }
  if (!parent) {
    if (ImGui::Button(ICON_FA_FILE_EXPORT u8" raw")) {
      auto &ctrl         = rec->export_ctrl.raw;
      ctrl.export_window = true;
      ctrl.start         = {0, 0};
      ctrl.size          = {rec->Nx(), rec->Ny()};
      ctrl.frames        = {0, rec->length()};
      ctrl.assign_auto_filename(rec->path());
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FILE_EXPORT u8" " ICON_FA_VIDEO)) {
      auto &ctrl         = rec->export_ctrl.video;
      ctrl.export_window = true;
      ctrl.assign_auto_filename(rec->path());
      ctrl.tend        = rec->length();
      ctrl.description = rec->comment();
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FILE_EXPORT u8" " ICON_FA_FILE_IMAGE)) {
      auto &ctrl         = rec->export_ctrl.png;
      ctrl.export_window = true;
      ctrl.assign_auto_filename(rec->path());
    }
    if (global::recordings.size() > 1) {
      ImGui::SameLine();
      if (ImGui::Button(ICON_FA_LAYER_GROUP)) ImGui::OpenPopup("merge_popup");
      if (ImGui::BeginPopup("merge_popup")) {
        for (auto &r : global::recordings) {
          if (r.get() == rec.get()) continue;
          auto l = fmt::format("Merge into '{}'", r->name());
          if (ImGui::Selectable(l.c_str())) {
            global::merge_queue.push({rec, r, false});
          }
          if (rec->file()->capabilities()[FileCapabilities::AS_FLOW]) {
            auto l2 = l + " as flow"s;
            if (ImGui::Selectable(l2.c_str())) {
              global::merge_queue.push({rec, r, true});
            }
          }
        }
        ImGui::EndPopup();
      }
    }
  } else {
    if (ImGui::Button(ICON_FA_TRASH_ALT)) {
      rec->set_context(nullptr);
      // Child will be deleted later, after we have left the loop over all children.
      ImGui::Columns(1);
      ImGui::PopID();
      return rec_nr;
    }
  }
  ImGui::Columns(1);


  // Histogram and other controls
  ImGui::PushItemWidth(prm::main_window_width * 0.7f);
  ImGui::PlotHistogram("##histogram", rec->histogram.data.data(), rec->histogram.data.size(), 0,
                       nullptr, 0, rec->histogram.max_value(), ImVec2(0, 100));
  ImGui::SameLine();
  {
    ImGui::BeginGroup();
    ImGui::Text("Histogram");
    ImGui::NewLine();
    // Bitrange switcher
    {
      int item = static_cast<int>(rec->bitrange);
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
      ImGui::Combo("Format", &item, BitRangeNames, IM_ARRAYSIZE(BitRangeNames));
      if (auto br = static_cast<BitRange>(item); rec->bitrange != br) {
        rec->bitrange      = br;
        float &min         = rec->get_min(Transformations::None);
        float &max         = rec->get_max(Transformations::None);
        std::tie(min, max) = utils::bitrange_to_float(br);
      }
    }
    // Colormap switcher
    {
      ColorMap current_cmap = rec->colormap();
      auto itemwidth        = ImGui::GetContentRegionAvail().x * 0.5f;
      ImGui::SetNextItemWidth(itemwidth);
      ImVec2 combo_pos = ImGui::GetCursorScreenPos();
      if (ImGui::BeginCombo("Colormap", "")) {
        for (auto [cmap, tex_id] : prm::cmap_texs) {
          auto l = ColorMapsNames[static_cast<int>(cmap)];
          ImGui::PushID(l);
          bool is_selected = (cmap == current_cmap);
          if (ImGui::Selectable("", is_selected) && (cmap != current_cmap)) {
            rec->colormap(cmap);
          }
          ImGui::SameLine();
          ImGui::Image((void *)(intptr_t)tex_id, ImVec2(80, ImGui::GetTextLineHeight()));
          ImGui::SameLine();
          ImGui::Text("%s", l);
          ImGui::SameLine();
          ImGui::Spacing();
          ImGui::PopID();
        }
        ImGui::EndCombo();
      }
      const auto style = ImGui::GetStyle();
      ImGui::SetCursorScreenPos(
          ImVec2(combo_pos.x + style.FramePadding.x, combo_pos.y + style.FramePadding.y));
      const ImVec2 i_size = {itemwidth - ImGui::GetFrameHeight() - 2.5f * style.FramePadding.x,
                             ImGui::GetTextLineHeight()};
      ImGui::Image((void *)(intptr_t)prm::cmap_texs[current_cmap], i_size);
    }
    if (parent) {
      ImGui::Checkbox("Overlay", &rec->as_overlay);
      if (rec->as_overlay) {
        auto itemwidth = ImGui::GetContentRegionAvail().x * 0.5f;
        ImGui::SetNextItemWidth(itemwidth);
        ImGui::Combo("Overlay Method", &rec->overlay_method, OverlayMethodNames,
                     IM_ARRAYSIZE(OverlayMethodNames));
      }
    }
    ImGui::EndGroup();
  }

  // Controls for min and max values
  bool symmetrize_minmax = false;
  if (ImGui::SliderFloat("min", &rec->get_min(prm::transformation), rec->histogram.min,
                         rec->histogram.max)) {
    symmetrize_minmax =
        (rec->histogram.symmetric || prm::transformation == Transformations::FrameDiff);
  }
  if (rec->get_min(prm::transformation) < 0) {
    ImGui::SameLine();
    if (ImGui::Checkbox("Symmetric", &rec->histogram.symmetric)) {
      symmetrize_minmax = rec->histogram.symmetric;
    }
  }
  if (ImGui::SliderFloat("max", &rec->get_max(prm::transformation), rec->histogram.min,
                         rec->histogram.max)) {
    if (prm::transformation == Transformations::FrameDiff) {
      rec->get_min(prm::transformation) = -rec->get_max(prm::transformation);
    }
  }
  if (symmetrize_minmax) {
    rec->get_max(prm::transformation) = -rec->get_min(prm::transformation);
  }

  // Controls for flows
  if (!rec->flows.empty()) {
    for (auto &flow : rec->flows) {
      ImGui::PushID(flow.data.get());
      ImGui::Text("Flow %s", flow.data->name().c_str());
      ImGui::SameLine();
      ImGui::ColorEdit4("", flow.color.data(),
                        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
      ImGui::SameLine();
      if (flow.show)
        flow.show = !ImGui::Button(ICON_FA_EYE_SLASH);
      else
        flow.show = ImGui::Button(ICON_FA_EYE);
      ImGui::SameLine();
      if (ImGui::Button(ICON_FA_TRASH_ALT)) {
        flow.data = nullptr;
      }
      ImGui::PopID();
    }
    // Actually remove deleted flows
    rec->flows.erase(
        std::remove_if(rec->flows.begin(), rec->flows.end(), [](const auto &f) { return !f.data; }),
        rec->flows.end());
    ImGui::Columns(2);
    ImGui::SliderInt("flow skip", &FlowData::skip, 1, 25);
    ImGui::NextColumn();
    ImGui::SliderFloat("flow point size", &FlowData::pointsize, 0, 10);
    ImGui::Columns(1);
  }

  if (!rec->points_videos.empty()) {
    ImGui::Separator();
    for (const auto &vid : rec->points_videos) {
      ImGui::PushID(vid.get());
      ImGui::ColorEdit4("", vid->color.data(), ImGuiColorEditFlags_NoLabel);
      ImGui::SliderFloat("point size", &vid->point_size, 0, 10);
      ImGui::Separator();
      ImGui::PopID();
    }
    ImGui::Separator();
  }

  // Plot traces and show their controls
  for (auto &[trace, pos, color, scale] : rec->traces) {
    int size = trace.size();
    if (size == 0) continue;

    auto label = pos.to_string();
    ImGui::PushID(label.c_str());
    auto data = trace.data();
    if (size > prm::trace_length) {
      data += (size - prm::trace_length);
      size = prm::trace_length;
    }

    scale.left  = data - trace.data();
    scale.right = trace.size();
    scale.scale(data, data + size);
    ImPlot::LinkNextPlotLimits(
        scale.scaleX ? &scale.left : nullptr, scale.scaleX ? &scale.right : nullptr,
        scale.scaleY ? &scale.lower : nullptr, scale.scaleY ? &scale.upper : nullptr);
    auto ptitle = "###trace" + label;
    if (ImPlot::BeginPlot(ptitle.c_str(), nullptr, nullptr,
                          ImVec2(ImGui::GetContentRegionAvail().x * 0.85f, 180),
                          ImPlotFlags_AntiAliased)) {
      ImPlot::SetNextLineStyle({color[0], color[1], color[2], color[3]});
      auto title = "###ttrace" + label;
      ImPlot::PlotLine(title.c_str(), trace.data(), trace.size());
      ImPlotUtils::draw_liney(scale.restarts);
      ImPlot::EndPlot();
    }

    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text("%s", label.c_str());
    ImGui::SameLine();
    ImGui::ColorEdit3(label.c_str(), color.data(),
                      ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    ImGui::Checkbox("Scale X", &scale.scaleX);
    ImGui::Checkbox("Scale Y", &scale.scaleY);
    if (ImGui::Button("Reset")) {
      trace.clear();
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_TRASH_ALT)) {
      rec->remove_trace(pos);
    }
    if (ImGui::Button("Export Trace")) {
      auto &ctrl         = rec->export_ctrl.trace;
      ctrl.export_window = true;
      ctrl.assign_auto_filename(rec->path(), pos, Trace::width());
      ctrl.tend = rec->length();
    }
    if (ImGui::Button("Export ROI")) {
      auto &ctrl                      = rec->export_ctrl.raw;
      ctrl.export_window              = true;
      std::tie(ctrl.start, ctrl.size) = Trace::clamp(pos, {rec->Nx(), rec->Ny()});
      ctrl.frames                     = {0, rec->length()};
      ctrl.assign_auto_filename(rec->path());
    }
    ImGui::EndGroup();
    ImGui::PopID();
  }
  ImGui::PopID();
  for (const auto &crec : rec->children) {
    rec_nr = show_recording_ui(crec, rec_nr, rec.get());
  }
  // Actually delete children which have been selected for deletionq
  rec->children.erase(std::remove_if(rec->children.begin(), rec->children.end(),
                                     [](const auto &r) { return r->glcontext == r->window; }),
                      rec->children.end());
  if (!parent) {
    ImGui::End();
  }
  return rec_nr + 1;
}

/* Create directory, return false on error */
bool create_directory(std::string path) {
  std::error_code error;
  fs::create_directory(path, error);
  if (error) {
    global::new_ui_message("Failed to create directory: {}", error.message());
    return false;
  }
  return true;
}

void show_export_recording_ui(const SharedRecordingPtr &recording) {
  // Use the directory path of the recording as best guest for the
  // export directory, make it static so that it only has to be changed
  // by the user once
  static auto export_dir = recording->path().parent_path().string();

  /*
   * Export array to .npy or .dat file
   */
  if (auto &ctrl = recording->export_ctrl.raw; ctrl.export_window) {
    ImGui::SetNextWindowSizeConstraints(ImVec2(0.75f * prm::main_window_width, 0),
                                        ImVec2(prm::main_window_width, FLT_MAX));
    ImGui::Begin("Export Raw ROI", &(ctrl.export_window), ImGuiWindowFlags_AlwaysAutoResize);

    bool refresh = ImGui::InputInt2("Top Left Position", ctrl.start.data());
    refresh |= ImGui::InputInt2("Array Size", ctrl.size.data());
    refresh |= ImGui::InputInt2("Start & End Frames", ctrl.frames.data());
    ImGui::Spacing();

    int item                           = static_cast<int>(ctrl.type);
    const char *ExportFileTypenames[2] = {"Binary .dat", ".npy"};
    ImGui::Combo("Format", &item, ExportFileTypenames, IM_ARRAYSIZE(ExportFileTypenames));
    if (auto type = static_cast<ExportFileType>(item); ctrl.type != type) {
      ctrl.type = type;
      refresh   = true;
    }

    if (refresh) ctrl.assign_auto_filename(recording->path());
    ImGui::InputText("Directory", &export_dir);
    ImGui::InputText("Filename", &ctrl.filename);

    static bool norm  = false;
    auto checkbox_lbl = fmt::format("Normalize values to [0, 1] using min = {} and max = {}",
                                    recording->get_min(Transformations::None),
                                    recording->get_max(Transformations::None));
    ImGui::Checkbox(checkbox_lbl.c_str(), &norm);

    ImGui::Spacing();
    if (ImGui::Button("Start Export (freezes everything)", ImVec2(-1.0f, 0.0f)) &&
        create_directory(export_dir)) {
      fs::path path(export_dir);
      path /= ctrl.filename;
      fmt::print("Exporting ROI to {}\n", path.string());

      Vec2f minmax = norm ? Vec2f(recording->get_min(Transformations::None),
                                  recording->get_max(Transformations::None))
                          : Vec2f(0, 0);

      bool success =
          recording->export_ROI(path, ctrl.start, ctrl.size, ctrl.frames, ctrl.type, minmax);

      if (success) {
        global::new_ui_message("Export to {} completed successfully", path.string());
        ctrl.export_window = false;
      }
    }
    ImGui::End();
  }

  /*
   * Export frames as video
   */
  if (auto &ctrl = recording->export_ctrl.video; ctrl.export_window) {
    ImGui::SetNextWindowSizeConstraints(ImVec2(0.75f * prm::main_window_width, 0),
                                        ImVec2(prm::main_window_width, FLT_MAX));
    ImGui::Begin("Export Video", &(ctrl.export_window));
    ImGui::TextWrapped(
        "Export the recording window as an .mp4 file. "
        "Exports exactly what is shown in the recording "
        "window, so don't change anything during the export. "
        "Restarts to the beginning of the file and ends automatically.");

    if (!ctrl.recording) {
      ImGui::InputText("Directory", &export_dir);
      ImGui::InputText("Filename", &ctrl.filename);
      ImGui::InputTextMultiline("Description", &ctrl.description);
      static int fps = 30;
      ImGui::InputInt("FPS", &fps);
      ImGui::InputInt("t start", &ctrl.tstart);
      ImGui::InputInt("t end", &ctrl.tend);

      if (ImGui::Button("Start Export") && create_directory(export_dir)) {
        recording->start_recording(fs::path(export_dir) / ctrl.filename, fps, ctrl.description);
      }
    } else {
      int cur_frame    = recording->current_frame() / prm::playbackCtrl.val + 1;
      int total_frames = recording->length() / prm::playbackCtrl.val;
      auto label       = fmt::format("Exporting frame {:d}/{:d}", cur_frame, total_frames);
      ImGui::ProgressBar(ctrl.progress, ImVec2(-1, 0), label.c_str());

      if (ImGui::Button(ICON_FA_STOP " Stop Export")) {
        recording->stop_recording();
        global::new_ui_message("Exporting video stopped");
      }
    }
    ImGui::End();
  }

  /*
   * Export frame(s) as png(s)
   */
  if (auto &ctrl = recording->export_ctrl.png; ctrl.export_window) {
    ImGui::SetNextWindowSizeConstraints(ImVec2(0.75f * prm::main_window_width, 0),
                                        ImVec2(prm::main_window_width, FLT_MAX));
    ImGui::Begin("Export .png", &ctrl.export_window);
    ImGui::InputText("Directory", &export_dir);
    ImGui::InputText("Filename", &ctrl.filename);

    const auto make_snapshot = [&recording, &ctrl]() {
      fs::path path(export_dir);
      path /= ctrl.filename;
      return recording->save_snapshot(path.string());
    };

    if (!ctrl.save_pngs) {
      if (ImGui::Button("Single .png") && create_directory(export_dir)) {
        auto fn = make_snapshot();
        global::new_ui_message("Saved screenshot to {}", fn.string());
      }

      ctrl.save_pngs = ImGui::Button("Start exporting .png series") && create_directory(export_dir);
    } else {
      auto fn = make_snapshot();
      if (ImGui::Button("Stop exporting .png series")) {
        ctrl.save_pngs     = false;
        ctrl.export_window = false;
        global::new_ui_message("Stopped exporting .png series, last screenshot {}", fn.string());
      }
    }
    ImGui::End();
  }

  /*
   * Export trace as txt file
   */
  if (auto &ctrl = recording->export_ctrl.trace; ctrl.export_window) {
    ImGui::SetNextWindowSizeConstraints(ImVec2(0.75f * prm::main_window_width, 0),
                                        ImVec2(prm::main_window_width, FLT_MAX));
    ImGui::Begin("Save Trace", &ctrl.export_window);
    ImGui::InputText("Directory", &export_dir);
    ImGui::InputText("Filename", &ctrl.filename);
    ImGui::InputInt("t start", &ctrl.tstart);
    ImGui::InputInt("t end", &ctrl.tend);
    if (ImGui::Button("Export") && create_directory(export_dir)) {
      recording->save_trace(ctrl.pos, fs::path(export_dir) / ctrl.filename,
                            {ctrl.tstart, ctrl.tend});
    }
    ImGui::End();
  }
}
