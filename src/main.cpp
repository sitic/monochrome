#ifdef _WIN32  // Windows 32 and 64 bit
#include <windows.h>
#endif

#include <cstdlib>
#include <vector>

#include <GLFW/glfw3.h>
#include <fmt/format.h>
#include <CLI/CLI.hpp>

#include "fonts/IconsFontAwesome5.h"
#include "fonts/IconsMaterialDesignIcons.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"

#include "recordingwindow.h"

GLFWwindow *main_window = nullptr;

std::vector<std::shared_ptr<RecordingWindow>> recordings = {};
std::vector<Message> messages                            = {};

namespace prm {
  static int main_window_width  = 0;
  static int main_window_height = 0;
  static int max_trace_length   = 200;

  static Filters prefilter              = Filters::None;
  static Transformations transformation = Transformations::None;
  static Filters postfilter             = Filters::None;
  static BitRange bitrange              = BitRange::U12;
}  // namespace prm

RotationCtrl Recording::rotations        = {};
float RecordingWindow::scale_fct         = 1;
float Transformation::GaussFilter::sigma = 1;
deriche_coeffs Transformation::GaussFilter::c;
unsigned Transformation::ContrastEnhancement::kernel_size = 3;
unsigned Transformation::MeanFilter::kernel_size          = 3;
unsigned Transformation::MedianFilter::kernel_size        = 3;
int Transformation::ContrastEnhancement::maskVersion      = 0;

void load_new_file(filesystem::path path) {
  fmt::print("Loading {} ...\n", path.string());

  if (!filesystem::is_regular_file(path)) {
    new_ui_message("ERROR: {} does not appear to be a file, skipping", path.string());
    return;
  }

  if (path.extension() != ".dat") {
    new_ui_message("ERROR: {} does not have extension '.dat', skipping", path.string());
    return;
  }

  recordings.emplace_back(std::make_shared<RecordingWindow>(path));
  auto rec = recordings.back();
  if (!rec->good()) {
    recordings.pop_back();
    new_ui_message("ERROR: loading file failed, skipping");
    return;
  }

  if (auto br = rec->bitrange(); br) {
    if (br.value() != prm::bitrange) {
      prm::bitrange = br.value();
    }
  }

  rec->open_window();
}

void display() {
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  // Our state
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  // keep running until main window is closed
  while (!glfwWindowShouldClose(main_window)) {
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to
    // tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to
    // your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input
    // data to your main application. Generally you may always pass all inputs
    // to dear imgui, and hide them from your application based on those two
    // flags.
    glfwPollEvents();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // ImGui::ShowDemoWindow();

    {
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
            for (const auto &r : recordings) {
              r->restart();
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
            for (const auto &r : recordings) {
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
          if (ImGui::Button(ICON_MDI_FLIP_VERTICAL)) {
            for (const auto &r : recordings) {
              r->flipud();
            }
          }
          ImGui::SameLine();
          if (ImGui::Button(ICON_MDI_FLIP_HORIZONTAL)) {
            for (const auto &r : recordings) {
              r->fliplr();
            }
          }
          ImGui::SameLine();
          ImGui::Text("Rotate");
          ImGui::SameLine();
          if (ImGui::Button(ICON_MDI_ROTATE_LEFT)) {
            for (const auto &r : recordings) {
              r->add_rotation(-90);
            }
          }
          ImGui::SameLine();
          if (ImGui::Button(ICON_MDI_ROTATE_RIGHT)) {
            for (const auto &r : recordings) {
              r->add_rotation(90);
            }
          }
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

      ImGui::Text("Application average %.1f FPS", ImGui::GetIO().Framerate);

      ImGui::Separator();
      auto selectable_factory = [](auto &p, auto default_val) {
        return [&p, default_val](const char *label, auto e) {
          bool is_active = p == e;
          if (ImGui::Selectable(label, is_active)) {

            p = is_active ? default_val : e;
            for (const auto &r : recordings) {
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
          for (const auto &r : recordings) {
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
            // TODO: clean up
            auto transform = r->transformationArena.create_if_needed(Filters::Mean, 0);
            auto c         = dynamic_cast<Transformation::MeanFilter *>(transform);
            assert(c);
            c->reset();
          });
        }
        if (selectable("Median", Filters::Median)) {
          kernel_size_select(Transformation::MedianFilter::kernel_size, [](RecordingWindow *r) {
            // TODO: clean up
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
        selectable("Frame Difference", Transformations::FrameDiff);
        if (selectable("Contrast Enhancement", Transformations::ContrastEnhancement)) {
          ImGui::Indent(10);
          const int step = 2;
          ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
          if (ImGui::InputScalar("Kernel size", ImGuiDataType_U32,
                                 &Transformation::ContrastEnhancement::kernel_size, &step, nullptr,
                                 "%d")) {
            for (const auto &r : recordings) {
              // TODO: clean up
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
            // TODO: clean up
            auto transform = r->transformationArena.create_if_needed(Filters::Mean, 0);
            auto c         = dynamic_cast<Transformation::MeanFilter *>(transform);
            assert(c);
            c->reset();
          });
        }
        if (selectable("Median", Filters::Median)) {
          kernel_size_select(Transformation::MedianFilter::kernel_size, [](RecordingWindow *r) {
            // TODO: clean up
            auto transform = r->transformationArena.create_if_needed(Filters::Median, 0);
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

    // Check if recording window should close
    recordings.erase(
        std::remove_if(recordings.begin(), recordings.end(),
                       [](const auto &r) -> bool { return glfwWindowShouldClose(r->window); }),
        recordings.end());

    for (const auto &recording : recordings) {
      recording->display(prm::playbackCtrl.val, prm::prefilter, prm::transformation, prm::postfilter,
                         prm::bitrange);

      ImGui::SetNextWindowSizeConstraints(ImVec2(prm::main_window_width, 0),
                                          ImVec2(FLT_MAX, FLT_MAX));
      ImGui::Begin(recording->path().filename().string().c_str(), nullptr,
                   ImGuiWindowFlags_AlwaysAutoResize);
      int t = recording->current_frame();
      ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImGui::GetStyleColorVec4(ImGuiCol_PlotHistogram));
      ImGui::SetNextItemWidth(-1);
      if (ImGui::SliderInt("##progress", &t, 0, recording->length() - 1, "Frame %d")) {
        recording->set_frame_index(t - static_cast<int>(prm::playbackCtrl.val));
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
        ctrl.assign_auto_filename(recording->path());
        ctrl.start  = {0, 0};
        ctrl.size   = {recording->Nx(), recording->Ny()};
        ctrl.frames = {0, recording->length()};
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

      // Use the directory path of the recording as best guest for the
      // export directory, make it static so that it only has to be changed
      // by the user once
      const auto gen_export_dir = [](const filesystem::path &dir) {
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

        static bool norm = false;
        ImGui::Checkbox("Normalize to [0, 1]", &norm);

        ImGui::Spacing();
        if (ImGui::Button("Start Export (freezes everything)", ImVec2(-1.0f, 0.0f))) {
          filesystem::path path(export_dir.data());
          path /= ctrl.filename.data();
          fmt::print("Exporting ROI to {}\n", path.string());

          Vec2f minmax = norm ? Vec2f(recording->get_min(Transformations::None),
                                      recording->get_max(Transformations::None))
                              : Vec2f(0, 0);

          bool success = recording->export_ROI(path, ctrl.start, ctrl.size, ctrl.frames, minmax);

          if (success) {
            new_ui_message("Export to {} completed successfully", path.string());
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
            "Export the recording window as an .mp4 file."
            "Exports exactly what is shown in the recording "
            "window, so don't change anything during the export."
            "Restarts to the beginning of the file and ends automatically.");

        if (!ctrl.recording) {
          ImGui::InputText("Directory", export_dir.data(), export_dir.size());
          ImGui::InputText("Filename", ctrl.filename.data(), ctrl.filename.size());
          static int fps = 30;
          ImGui::InputInt("FPS", &fps);

          if (ImGui::Button("Start Export")) {
            filesystem::path path(export_dir.data());
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
            new_ui_message("Exporting video stopped");
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
          filesystem::path path(export_dir.data());
          path /= ctrl.filename.data();
          return recording->save_snapshot(path.string());
        };

        if (!ctrl.save_pngs) {
          if (ImGui::Button("Single .png")) {
            auto fn = make_snapshot();
            new_ui_message("Saved screenshot to {}", fn.string());
          }

          ctrl.save_pngs = ImGui::Button("Start exporting .png series");
        } else {
          auto fn = make_snapshot();
          if (ImGui::Button("Stop exporting .png series")) {
            ctrl.save_pngs     = false;
            ctrl.export_window = false;
            new_ui_message("Stopped exporting .png series, last screenshot {}", fn.string());
          }
        }
        ImGui::End();
      }
    }

    // Check if message window should be cleared
    messages.erase(std::remove_if(messages.begin(), messages.end(),
                                  [](const auto &msg) -> bool { return !msg.show; }),
                   messages.end());
    for (auto &msg : messages) {
      if (msg.show) {
        auto label = fmt::format("Message {}", msg.id);
        ImGui::SetNextWindowSizeConstraints(ImVec2(0.6f * prm::main_window_width, 0),
                                            ImVec2(FLT_MAX, FLT_MAX));
        ImGui::Begin(label.c_str(), &(msg.show), ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::TextWrapped("%s", msg.msg.c_str());
        if (ImGui::Button("Ok", ImVec2(-1.0f, 0.0f))) {
          msg.show = false;
        }
        ImGui::End();
      }
    }

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(main_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    // If you are using this code with non-legacy OpenGL header/contexts
    // (which you should not, prefer using imgui_impl_opengl3.cpp!!), you may
    // need to backup/reset/restore current shader using the commented lines
    // below. GLint last_program; glGetIntegerv(GL_CURRENT_PROGRAM,
    // &last_program); glUseProgram(0);
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    // glUseProgram(last_program);

    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we
    // save/restore it to make it easier to paste this code elsewhere.
    //  For this specific demo app we could also call
    //  glfwMakeContextCurrent(window) directly)
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
      GLFWwindow *backup_current_context = glfwGetCurrentContext();
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
      glfwMakeContextCurrent(backup_current_context);
    }
    glfwSwapBuffers(main_window);
  }
}

void drop_callback(GLFWwindow *window, int count, const char **paths) {
  for (int i = 0; i < count; i++) {
    load_new_file(paths[i]);
  }
}

int main(int argc, char **argv) {
  CLI::App app{"Quick Raw Video Viewer"};
  std::vector<std::string> files;
  app.add_option("files", files, "List of files to open")->check(CLI::ExistingFile);
  app.add_option("--scale", RecordingWindow::scale_fct, "Recording window size multiplier")
      ->check(CLI::PositiveNumber);
  app.add_option("--speed", prm::playbackCtrl.val, "Recording playback speed multiplier")
      ->check(CLI::NonNegativeNumber);
  app.add_option("--window-width", prm::main_window_width, "Window width of the main window");
  app.add_option("--window-height", prm::main_window_height, "Window height of the main window");
  app.add_option("--max_trace_length", prm::max_trace_length);
  std::string config_file;
#ifdef _WIN32
  config_file = "%APPDATA%\\quickVidViewer\\quickVidViewer.ini";
#elif defined(unix) || defined(__unix__) || defined(__unix)
  config_file = fmt::format("{}/.config/quickVidViewer.ini", get_user_homedir());
#endif
  bool print_config             = false;
  CLI::Option *print_config_opt = nullptr;
  if (!config_file.empty()) {
    app.set_config("--config", config_file,
                   "Configuration file to load command line arguments from");
    print_config_opt = app.add_flag("--print-config", print_config);
  }

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError &e) {
    return app.exit(e);
  }

  if (print_config) {
    app.remove_option(print_config_opt);
    fmt::print(app.config_to_str(true, true));
    exit(EXIT_SUCCESS);
  }

  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit()) exit(EXIT_FAILURE);

  auto primary_monitor = glfwGetPrimaryMonitor();
  auto mode            = glfwGetVideoMode(primary_monitor);

  if (prm::main_window_width == 0) prm::main_window_width = std::max(600, mode->width / 4);
  if (prm::main_window_height == 0) prm::main_window_height = 1.5 * prm::main_window_width;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  main_window = glfwCreateWindow(prm::main_window_width, prm::main_window_height,
                                 "Quick Raw Video Viewer", nullptr, nullptr);
  if (!main_window) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }
  glfwMakeContextCurrent(main_window);
  // wait until the current frame has been drawn before drawing the next one
  glfwSwapInterval(2);

  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable
  // Gamepad Controls
  // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
  // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable
  // Multi-Viewport /
  //                                                    // Platform Windows
  // io.ConfigViewportsNoAutoMerge = true;
  // io.ConfigViewportsNoTaskBarIcon = true;

  // Disable .ini generation/loading for now
  io.IniFilename = NULL;

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

  // When viewports are enabled we tweak WindowRounding/WindowBg so platform
  // windows can look identical to regular ones.
  ImGuiStyle &style = ImGui::GetStyle();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    style.WindowRounding              = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }

  // TODO: Better HIDIP handling
  float xscale, yscale;
  glfwGetMonitorContentScale(primary_monitor, &xscale, &yscale);
  xscale = std::max(xscale, yscale);
  style.ScaleAllSizes(xscale);

  // Setup Platform/Renderer bindings
  ImGui_ImplGlfw_InitForOpenGL(main_window, true);
  ImGui_ImplOpenGL2_Init();

  glfwSetDropCallback(main_window, drop_callback);

  // Load Fonts
  // - If no fonts are loaded, dear imgui will use the default font. You can
  // also load multiple fonts and use ImGui::PushFont()/PopFont() to select
  // them.
  // - AddFontFromFileTTF() will return the ImFont* so you can store it if you
  // need to select the font among multiple.
  // - If the file cannot be loaded, the function will return NULL. Please
  // handle those errors in your application (e.g. use an assertion, or
  // display an error and quit).
  // - The fonts will be rasterized at a given size (w/ oversampling) and
  // stored into a texture when calling
  // ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame
  // below will call.
  // - Read 'misc/fonts/README.txt' for more instructions and details.
  // - Remember that in C/C++ if you want to include a backslash \ in a string
  // literal you need to write a double backslash \\ !
  io.Fonts->AddFontDefault();
  ImFontConfig icons_config;
  icons_config.MergeMode  = true;
  icons_config.PixelSnapH = true;

  static const ImWchar fontawesome_icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
  io.Fonts->AddFontFromMemoryCompressedTTF(fonts::fontawesome5_solid_compressed_data,
                                           fonts::fontawesome5_solid_compressed_size, 11,
                                           &icons_config, fontawesome_icons_ranges);
  static const ImWchar materialdesignicons_icons_ranges[] = {ICON_MIN_MDI, ICON_MAX_MDI, 0};
  io.Fonts->AddFontFromMemoryCompressedTTF(fonts::materialdesignicons_compressed_data,
                                           fonts::materialdesignicons_compressed_size, 11,
                                           &icons_config, materialdesignicons_icons_ranges);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
  // ImFont* font =
  // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f,
  // NULL, io.Fonts->GetGlyphRangesJapanese()); IM_ASSERT(font != NULL);

  for (const auto &file : files) {
    load_new_file(file);
  }
  display();

  // Cleanup
  ImGui_ImplOpenGL2_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(main_window);
  recordings.clear();
  glfwTerminate();
  exit(EXIT_SUCCESS);
}
