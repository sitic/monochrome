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
        prm::playbackCtrl.deacrease_speed();
      }
      ImGui::SameLine();
      ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.75f);
      ImGui::DragFloat("##speed", &prm::playbackCtrl.val, 0.05, 0, 20, "playback speed = %.2f");
      ImGui::SameLine();
      if (ImGui::Button(ICON_FA_FORWARD)) {
        prm::playbackCtrl.increase_speed();
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
  ImGui::SetNextItemOpen(false, ImGuiCond_Once);
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

  ImGui::SetNextItemOpen(false, ImGuiCond_Once);
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
//      ImGui::SliderInt("Mask", &Transformation::ContrastEnhancement::maskVersion, 0, 2);
      ImGui::Unindent(10);
    }
    ImGui::TreePop();
  }

  ImGui::NextColumn();

  ImGui::SetNextItemOpen(false, ImGuiCond_Once);
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