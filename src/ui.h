#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace global {
  extern GLFWwindow *main_window;
  extern std::vector<std::shared_ptr<RecordingWindow>> recordings;
}  // namespace global

namespace prm {
  static int main_window_width  = 0;
  static int main_window_height = 0;
  static int max_trace_length   = 200;
  static int max_display_fps    = 60;

  static Filters prefilter              = Filters::None;
  static Transformations transformation = Transformations::None;
  static Filters postfilter             = Filters::None;
  static BitRange bitrange              = BitRange::U12;
}  // namespace prm

void show_main_ui() {
  ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos);
  ImGui::SetNextWindowSizeConstraints(ImVec2(prm::main_window_width, 0),
                                      ImVec2(prm::main_window_width, FLT_MAX));
  auto flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize |
               ImGuiWindowFlags_NoSavedSettings;
  ImGui::Begin("Drag & drop .dat files into this window", nullptr, flags);

  {
    ImGui::Columns(2);
    {
      if (ImGui::Button(ICON_FA_REDO_ALT)) {
        for (const auto &r : global::recordings) {
          r->playback.restart();
        }
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
      int item = static_cast<int>(prm::bitrange);
      ImGui::Combo("Data Format", &item, BitRangeNames, IM_ARRAYSIZE(BitRangeNames));
      prm::bitrange = static_cast<BitRange>(item);
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
    {
      int trace_width = Trace::width();
      if (ImGui::InputInt("Trace Width", &trace_width, 2, 5)) {
        Trace::width(trace_width);
      }
    }
    ImGui::NextColumn();
    { ImGui::SliderInt("Trace Length", &prm::max_trace_length, 10, 1000); }
    ImGui::Columns(1);
  }

  {
    int max_display_fps = prm::max_display_fps;
    auto label =
        fmt::format("Max FPS (current avg. {:.1f} fps)###dfps", ImGui::GetIO().Framerate);
    if (ImGui::InputInt(label.c_str(), &max_display_fps)) {
      if (ImGui::IsItemDeactivated() && max_display_fps > 0) prm::max_display_fps = max_display_fps;
    }
  }

  ImGui::Separator();
  auto selectable_factory = [](auto &p, auto default_val) {
    return [&p, default_val](const char *label, auto e) {
      bool is_active = p == e;
      if (ImGui::Selectable(label, is_active)) {

        p = is_active ? default_val : e;
        for (const auto &r : global::recordings) {
          r->reset_traces();
        }
      }

      return is_active;
    };
  };

  auto kernel_size_select = [](unsigned int &val, auto reset_fn) {
    ImGui::Indent(10);
    const int step = 2;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
    if (ImGui::InputScalar("Kernel size", ImGuiDataType_U32, &val, &step, nullptr, "%d")) {
      for (const auto &r : global::recordings) {
        reset_fn(r.get());
      }
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
      if (ImGui::DragFloat("##sigma", &sigma, 0.01, 0, 5, "sigma = %.2f")) {
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
        for (const auto &r : global::recordings) {
          auto transform =
              r->transformationArena.create_if_needed(Transformations::ContrastEnhancement, 0);
          auto c = dynamic_cast<Transformation::ContrastEnhancement *>(transform);
          assert(c);
          c->reset();
        }
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
      if (ImGui::DragFloat("##sigma", &sigma, 0.01, 0, 5, "sigma = %.2f")) {
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

void show_recording_ui(const std::shared_ptr<RecordingWindow> &recording) {
  ImGui::SetNextWindowSizeConstraints(ImVec2(prm::main_window_width, 0), ImVec2(FLT_MAX, FLT_MAX));
  ImGui::Begin(recording->path().filename().string().c_str(), nullptr,
               ImGuiWindowFlags_AlwaysAutoResize);
  int t = recording->current_frame();
  ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogram));
  ImGui::SetNextItemWidth(-1);
  if (ImGui::SliderInt("##progress", &t, 0, recording->length() - 1, "Frame %d")) {
    recording->playback.set(t - static_cast<int>(prm::playbackCtrl.val));
  }
  ImGui::PopStyleColor(1);

  if (!recording->date().empty()) {
    ImGui::Text("Date: %s", recording->date().c_str());
  }

  if (!recording->comment().empty()) {
    ImGui::Text("Comment: %s", recording->comment().c_str());
  }

  ImGui::Columns(3);
  if (recording->duration().count() > 0) {
    ImGui::Text("Duration  %.3fs", recording->duration().count());
    ImGui::NextColumn();
  }
  if (recording->fps() != 0) {
    ImGui::Text("FPS  %.3f", recording->fps());
    ImGui::NextColumn();
  }
  ImGui::Text("Frames %d", recording->length());
  ImGui::NextColumn();
  ImGui::Text("Width  %d", recording->Nx());
  ImGui::NextColumn();
  ImGui::Text("Height %d", recording->Ny());
  ImGui::NextColumn();
  if (ImGui::Button(ICON_FA_FILE_EXPORT u8" raw")) {
    auto &ctrl         = recording->export_ctrl.raw;
    ctrl.export_window = true;
    ctrl.start         = {0, 0};
    ctrl.size          = {recording->Nx(), recording->Ny()};
    ctrl.frames        = {0, recording->length()};
    ctrl.assign_auto_filename(recording->path());
  }
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_FILE_EXPORT u8" " ICON_FA_VIDEO)) {
    auto &ctrl         = recording->export_ctrl.video;
    ctrl.export_window = true;
    ctrl.assign_auto_filename(recording->path());
  }
  ImGui::SameLine();
  if (ImGui::Button(ICON_FA_FILE_EXPORT u8" " ICON_FA_FILE_IMAGE)) {
    auto &ctrl         = recording->export_ctrl.png;
    ctrl.export_window = true;
    ctrl.assign_auto_filename(recording->path());
  }
  ImGui::Columns(1);

  ImGui::PushItemWidth(prm::main_window_width * 0.75f);
  ImGui::PlotHistogram("Histogram", recording->histogram.data.data(),
                       recording->histogram.data.size(), 0, nullptr, 0,
                       recording->histogram.max_value(), ImVec2(0, 100));

  ImGui::SliderFloat("min", &recording->get_min(prm::transformation), recording->histogram.min,
                     recording->histogram.max);
  ImGui::SliderFloat("max", &recording->get_max(prm::transformation), recording->histogram.min,
                     recording->histogram.max);

  for (auto &[trace, pos, color] : recording->traces) {
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
      auto &ctrl         = recording->export_ctrl.raw;
      ctrl.export_window = true;
      ctrl.assign_auto_filename(recording->path());
      auto w      = Trace::width();
      ctrl.start  = {pos[0] - w / 2, pos[1] - w / 2};
      ctrl.size   = {w, w};
      ctrl.frames = {0, recording->length()};
    }
    ImGui::EndGroup();
    ImGui::PopID();
  }
  ImGui::End();
}

void show_export_recording_ui(const std::shared_ptr<RecordingWindow> &recording) {
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
