#pragma once

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

  ImGui::PushID(recording.get());

  /*
   * Export array to .npy or .dat file
   */
  if (auto &ctrl = recording->export_ctrl.raw; ctrl.export_window) {
    ImGui::SetNextWindowSizeConstraints(ImVec2(0.7f * ImGui::GetMainViewport()->Size[0], 0),
                                        ImVec2(ImGui::GetMainViewport()->Size[0], FLT_MAX));
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
                                    recording->get_min(true),
                                    recording->get_max(true));
    ImGui::Checkbox(checkbox_lbl.c_str(), &norm);

    ImGui::Spacing();
    if (ImGui::Button("Start Export (freezes everything)", ImVec2(-1.0f, 0.0f)) &&
        create_directory(export_dir)) {
      fs::path path(export_dir);
      path /= ctrl.filename;
      fmt::print("Exporting ROI to {}\n", path.string());

      Vec2f minmax = norm ? Vec2f(recording->get_min(true),
                                  recording->get_max(true))
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
    ImGui::SetNextWindowSizeConstraints(ImVec2(0.7f * ImGui::GetMainViewport()->Size[0], 0),
                                        ImVec2(ImGui::GetMainViewport()->Size[0], FLT_MAX));
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
    ImGui::SetNextWindowSizeConstraints(ImVec2(0.7f * ImGui::GetMainViewport()->Size[0], 0),
                                        ImVec2(ImGui::GetMainViewport()->Size[0], FLT_MAX));
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
    ImGui::SetNextWindowSizeConstraints(ImVec2(0.7f * ImGui::GetMainViewport()->Size[0], 0),
                                        ImVec2(ImGui::GetMainViewport()->Size[0], FLT_MAX));
    ImGui::Begin("Save Trace", &ctrl.export_window);
    ImGui::InputText("Directory", &export_dir);
    ImGui::InputText("Filename", &ctrl.filename);
    ImGui::InputInt("t start", &ctrl.tstart);
    ImGui::InputInt("t end", &ctrl.tend);
    if (ImGui::Button("Export") && create_directory(export_dir)) {
      recording->save_trace(ctrl.pos, fs::path(export_dir) / ctrl.filename,
                            {ctrl.tstart, ctrl.tend});
      ctrl.export_window = false;
      global::new_ui_message("Saved trace to {}", (fs::path(export_dir) / ctrl.filename).string());
    }
    ImGui::End();
  }

  ImGui::PopID();
}
