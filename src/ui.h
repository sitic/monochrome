#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace global {
  extern GLFWwindow *main_window;
  extern std::vector<SharedRecordingPtr> recordings;
  extern std::queue<std::pair<SharedRecordingPtr, SharedRecordingPtr>> merge_queue;

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
  static int main_window_width  = 0;
  static int main_window_height = 0;
  static int max_trace_length   = 200;
  static int max_display_fps    = 60;
  static double lastframetime   = 0;

  static Filters prefilter              = Filters::None;
  static Transformations transformation = Transformations::None;
  static Filters postfilter             = Filters::None;

  const std::array<ColorMap, 4> cmaps = {ColorMap::GRAY, ColorMap::DIFF, ColorMap::HSV,
                                         ColorMap::BLACKBODY};
  static std::map<ColorMap, GLuint> cmap_texs;
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
      ImGui::DragFloat("##speed", &prm::playbackCtrl.val, 0.05, 0, 20, "playback speed = %.1f");
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
      if (resize_windows) {
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
    }

    ImGui::NextColumn();
    { ImGui::SliderInt("Trace Length", &prm::max_trace_length, 10, 1000); }

    ImGui::NextColumn();
    {
      int max_display_fps = prm::max_display_fps;
      auto label = fmt::format("Max FPS (current avg. {:.0f}fps)###dfps", ImGui::GetIO().Framerate);
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.25f);
      if (ImGui::InputInt(label.c_str(), &max_display_fps)) {
        if (ImGui::IsItemDeactivated() && max_display_fps > 0) {
          prm::max_display_fps = max_display_fps;
          prm::lastframetime   = glfwGetTime();
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
      ImGui::SliderInt("Frames", &Transformation::FrameDiff::n_frame_diff, 1, 20);
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
    selectable("Flicker Segmentation", Transformations::FlickerSegmentation);
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
  if (!parent) {
    ImGui::SetNextWindowPos(ImVec2(0, (rec_nr * 0.3f + 0.2f) * prm::main_window_height),
                            ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(prm::main_window_width, 0), ImVec2(FLT_MAX, FLT_MAX));
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
    rec->playback.set(t - static_cast<int>(prm::playbackCtrl.val));
  }
  ImGui::PopStyleColor(1);

  if (!rec->date().empty()) {
    ImGui::Text("Date: %s", rec->date().c_str());
  }

  if (!rec->comment().empty()) {
    ImGui::Text("Comment: %s", rec->comment().c_str());
  }

  ImGui::Columns(3);
  if (rec->duration().count() > 0) {
    ImGui::Text("Duration  %.3fs", rec->duration().count());
    ImGui::NextColumn();
  }
  if (rec->fps() != 0) {
    ImGui::Text("FPS  %.3f", rec->fps());
    ImGui::NextColumn();
  }
  ImGui::Text("Frames %d", rec->length());
  ImGui::NextColumn();
  ImGui::Text("Width  %d", rec->Nx());
  ImGui::NextColumn();
  ImGui::Text("Height %d", rec->Ny());
  if (rec_nr != -1) {
    ImGui::NextColumn();
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
          auto l = fmt::format("Merge into '{}'", r->get_file_ptr()->path().filename().string());
          if (ImGui::Selectable(l.c_str())) {
            global::merge_queue.push({rec, r});
          }
        }
        ImGui::EndPopup();
      }
    }
  }
  ImGui::Columns(1);

  ImGui::PushItemWidth(prm::main_window_width * 0.7f);
  ImGui::PlotHistogram("##histogram", rec->histogram.data.data(), rec->histogram.data.size(), 0,
                       nullptr, 0, rec->histogram.max_value(), ImVec2(0, 100));
  ImGui::SameLine();
  {
    ImGui::BeginGroup();
    ImGui::Text("Histogram");
    ImGui::NewLine();
    {
      int item = static_cast<int>(rec->bitrange);
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
      ImGui::Combo("Format", &item, BitRangeNames, IM_ARRAYSIZE(BitRangeNames));
      if (auto br = static_cast<BitRange>(item); rec->bitrange != br) {
        rec->bitrange      = br;
        float &min         = rec->get_min(Transformations::None);
        float &max         = rec->get_max(Transformations::None);
        std::tie(min, max) = bitrange_to_float(br);
      }
    }
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
          ImGui::Text(l);
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
    ImGui::EndGroup();
  }
  ImGui::SliderFloat("min", &rec->get_min(prm::transformation), rec->histogram.min,
                     rec->histogram.max);
  ImGui::SliderFloat("max", &rec->get_max(prm::transformation), rec->histogram.min,
                     rec->histogram.max);

  for (auto &[trace, pos, color] : rec->traces) {
    auto label = pos.to_string();
    ImGui::PushID(label.c_str());
    int size  = trace.size();
    auto data = trace.data();
    if (size > prm::max_trace_length) {
      data += (size - prm::max_trace_length);
      size = prm::max_trace_length;
    }

    ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(color[0], color[1], color[2], 1));
    ImGui::PlotLines("", data, size, 0, NULL, FLT_MAX, FLT_MAX, ImVec2(0, 100));
    ImGui::PopStyleColor(1);
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::ColorEdit3(label.c_str(), color.data(),
                      ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
    if (ImGui::Button("Reset")) {
      trace.clear();
    }
    if (ImGui::Button("Export ROI")) {
      auto &ctrl         = rec->export_ctrl.raw;
      ctrl.export_window = true;
      ctrl.assign_auto_filename(rec->path());
      auto w      = Trace::width();
      ctrl.start  = {pos[0] - w / 2, pos[1] - w / 2};
      ctrl.size   = {w, w};
      ctrl.frames = {0, rec->length()};
    }
    ImGui::EndGroup();
    ImGui::PopID();
  }
  ImGui::PopID();
  for (const auto &crec : rec->children) {
    rec_nr = show_recording_ui(crec, rec_nr, rec.get());
  }
  if (!parent) {
    ImGui::End();
  }
  return rec_nr++;
}

void show_export_recording_ui(const SharedRecordingPtr &recording) {
  // Use the directory path of the recording as best guest for the
  // export directory, make it static so that it only has to be changed
  // by the user once
  const auto gen_export_dir = [](const fs::path &dir) {
    std::string s = dir.string();
    std::vector<char> v(s.begin(), s.end());
    // Maximum size for user input
    if (v.size() < 512) {
      v.resize(512);
    }
    return v;
  };
  static auto export_dir = gen_export_dir(recording->path().parent_path());

  if (auto &ctrl = recording->export_ctrl.raw; ctrl.export_window) {
    ImGui::SetNextWindowSizeConstraints(ImVec2(0.75f * prm::main_window_width, 0),
                                        ImVec2(prm::main_window_width, FLT_MAX));
    ImGui::Begin("Export Raw ROI", &(ctrl.export_window), ImGuiWindowFlags_AlwaysAutoResize);

    bool refresh = ImGui::InputInt2("Top Left Position", ctrl.start.data());
    refresh |= ImGui::InputInt2("Array Size", ctrl.size.data());
    refresh |= ImGui::InputInt2("Start & End Frames", ctrl.frames.data());
    if (refresh) ctrl.assign_auto_filename(recording->path());

    ImGui::Spacing();

    ImGui::InputText("Directory", export_dir.data(), export_dir.size());
    ImGui::InputText("Filename", ctrl.filename.data(), ctrl.filename.size());

    static bool norm  = false;
    auto checkbox_lbl = fmt::format("Normalize values to [0, 1] using min = {} and max = {}",
                                    recording->get_min(Transformations::None),
                                    recording->get_max(Transformations::None));
    ImGui::Checkbox(checkbox_lbl.c_str(), &norm);

    ImGui::Spacing();
    if (ImGui::Button("Start Export (freezes everything)", ImVec2(-1.0f, 0.0f))) {
      fs::path path(export_dir.data());
      path /= ctrl.filename.data();
      fmt::print("Exporting ROI to {}\n", path.string());

      Vec2f minmax = norm ? Vec2f(recording->get_min(Transformations::None),
                                  recording->get_max(Transformations::None))
                          : Vec2f(0, 0);

      bool success = recording->export_ROI(path, ctrl.start, ctrl.size, ctrl.frames, minmax);

      if (success) {
        global::new_ui_message("Export to {} completed successfully", path.string());
        ctrl.export_window = false;
      }
    }
    ImGui::End();
  }

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
      ImGui::InputText("Directory", export_dir.data(), export_dir.size());
      ImGui::InputText("Filename", ctrl.filename.data(), ctrl.filename.size());
      static int fps = 30;
      ImGui::InputInt("FPS", &fps);

      if (ImGui::Button("Start Export")) {
        fs::path path(export_dir.data());
        path /= ctrl.filename.data();
        recording->start_recording(path.string(), fps);
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

  if (auto &ctrl = recording->export_ctrl.png; ctrl.export_window) {
    ImGui::SetNextWindowSizeConstraints(ImVec2(0.75f * prm::main_window_width, 0),
                                        ImVec2(prm::main_window_width, FLT_MAX));
    ImGui::Begin("Export .png", &(ctrl.export_window));
    ImGui::InputText("Directory", export_dir.data(), export_dir.size());
    ImGui::InputText("Filename", ctrl.filename.data(), ctrl.filename.size());

    const auto make_snapshot = [&recording, &ctrl]() {
      fs::path path(export_dir.data());
      path /= ctrl.filename.data();
      return recording->save_snapshot(path.string());
    };

    if (!ctrl.save_pngs) {
      if (ImGui::Button("Single .png")) {
        auto fn = make_snapshot();
        global::new_ui_message("Saved screenshot to {}", fn.string());
      }

      ctrl.save_pngs = ImGui::Button("Start exporting .png series");
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
}
