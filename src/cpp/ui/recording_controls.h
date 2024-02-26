#pragma once

void display_recording_metadata(const SharedRecordingPtr &rec) {
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
  ImGui::Columns(1);
}

bool display_recording_buttons(const SharedRecordingPtr &rec, RecordingWindow *parent) {
  if (!parent) {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Export to video file: ");
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FILE_EXPORT u8" " ICON_FA_VIDEO)) {
      auto &ctrl         = rec->export_ctrl.video;
      ctrl.export_window = true;
      ctrl.assign_auto_filename(rec->path());
      ctrl.tend        = rec->length();
      ctrl.description = rec->comment();
    }
    ImGui::Text("Export as image sequence: ");
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FILE_EXPORT u8" " ICON_FA_FILE_IMAGE)) {
      auto &ctrl         = rec->export_ctrl.png;
      ctrl.export_window = true;
      ctrl.assign_auto_filename(rec->path());
    }
    ImGui::Text("Export as raw video: ");
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_FILE_EXPORT u8" raw")) {
      auto &ctrl         = rec->export_ctrl.raw;
      ctrl.export_window = true;
      ctrl.start         = {0, 0};
      ctrl.size          = {rec->Nx(), rec->Ny()};
      ctrl.frames        = {0, rec->length()};
      ctrl.assign_auto_filename(rec->path());
    }
    return false;
  } else {
    if (ImGui::Button(ICON_FA_TRASH_ALT)) {
      rec->set_context(nullptr);
      // Child will be deleted later, after we have left the loop over all children.
      return true;
    }
  }
  return false;
}

void show_transformations_ui() {
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
    if (selectable("Optical Mapping Contrast Enhancement", Transformations::ContrastEnhancement)) {
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
//      ImGui::SliderInt("Mask", &Transformation::ContrastEnhancement::maskVersion, 0, 2);
      ImGui::Unindent(10);
    }
    ImGui::TreePop();
  }

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
}

bool show_controls_ui(const SharedRecordingPtr &rec, RecordingWindow *parent) {
    ImGui::SeparatorText("General");
    auto rec_name = rec->name();
    if (ImGui::InputText("Name", &rec_name)) {
    rec->set_name(rec_name);
    }
    ImGui::Checkbox("Show", &rec->active);

    if (global::recordings.size() > 1) {
    if (ImGui::Button(u8"Add as layer onto other recording " ICON_FA_LAYER_GROUP)) ImGui::OpenPopup("merge_popup");
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

    ImGui::SeparatorText("Metadata");
    display_recording_metadata(rec);

    ImGui::SeparatorText("Export");
    if (display_recording_buttons(rec, parent)) {
        return true;
    }

    ImGui::SeparatorText("Transformations");
    show_transformations_ui();

    return false;
}