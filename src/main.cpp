#ifdef _WIN32 // Windows 32 and 64 bit
#include <windows.h>
#endif

#include <cstdlib>
#include <vector>

#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"

#include <fmt/format.h>

#include "recording.h"
#include "utils.h"

GLFWwindow *main_window = nullptr;

std::vector<std::shared_ptr<RecordingWindow>> recordings = {};

enum class HistRange { FLOAT, U8, U12, U16 };

constexpr static int bitrange(HistRange x) noexcept {
  switch (x) {
  case HistRange::FLOAT:
    return 1;
  case HistRange::U8:
    return (1 << 8) - 1;
  case HistRange::U12:
    return (1 << 12) - 1;
  case HistRange::U16:
    return (1 << 16) - 1;
  }

  return 0; // silence compile warning
}

namespace prm {
constexpr static int calc_bitrange(bool is_12bit) noexcept {
  return is_12bit ? bitrange(HistRange::U12) : bitrange(HistRange::U16);
}

static int main_window_width = 500;
static int main_window_height = 500;

static bool auto_scale = true;
static bool is_idscam = true;
static bool diff_frames = false;

static int bitrange = calc_bitrange(is_idscam);
static float min = 0;
static float max = bitrange;

static float speed = 1;
static float scale_fct = 1;
} // namespace prm

void load_new_file(filesystem::path path) {
  fmt::print("Loading {} ...\n", path.string());

  if (!filesystem::is_regular_file(path)) {
    fmt::print("ERROR: {} does not appear to be a file, skipping\n",
               path.string());
    return;
  }

  if (path.extension().string() != ".dat") {
    fmt::print("ERROR: {} does not have extension '.dat', skipping\n",
               path.string());
    return;
  }

  recordings.emplace_back(std::make_shared<RecordingWindow>(path));
  auto rec = recordings.back();
  if (!rec->good()) {
    recordings.pop_back();
    fmt::print("ERROR: loading file failed, skipping\n");
    return;
  }

  if ((rec->Nx() == 128) && (rec->Ny() == 128)) {
    if (prm::is_idscam) {
      fmt::print("Looks like we are loading a PVCam file, setting bitrange to "
                 "16bit\n");
      prm::is_idscam = false;
      prm::bitrange = prm::calc_bitrange(prm::is_idscam);
      prm::max = prm::bitrange;
    }
  }

  rec->open_window(prm::scale_fct);
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

    {
      ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos);
      ImGui::SetNextWindowSizeConstraints(ImVec2(prm::main_window_width, 0),
                                          ImVec2(FLT_MAX, FLT_MAX));
      auto flags = ImGuiWindowFlags_NoCollapse |
                   ImGuiWindowFlags_AlwaysAutoResize |
                   ImGuiWindowFlags_NoSavedSettings;
      ImGui::Begin("Drag & drop MultiRecorder .dat files into this window",
                   nullptr, flags);

      {
        ImGui::Columns(2);
        ImGui::SliderFloat("speed", &prm::speed, 0, 5);
        ImGui::NextColumn();
        if (ImGui::SliderFloat("scaling", &prm::scale_fct, 0.5, 5)) {
          for (const auto &r : recordings) {
            r->resize_window(prm::scale_fct);
          }
        }
        ImGui::NextColumn();
        ImGui::Checkbox("Auto min max", &prm::auto_scale);
        ImGui::NextColumn();
        if (ImGui::Checkbox("12bit file", &prm::is_idscam)) {
          prm::bitrange = prm::calc_bitrange(prm::is_idscam);
          prm::max = prm::bitrange;
        }
        ImGui::NextColumn();
        if (ImGui::Checkbox("Show frame difference", &prm::diff_frames)) {
          for (const auto &r : recordings) {
            r->reset_traces();
          }
        }
        ImGui::NextColumn();
        int trace_width = RecordingWindow::Trace::width();
        if (ImGui::InputInt("Trace width", &trace_width, 2, 5)) {
          RecordingWindow::Trace::width(trace_width);
        }
        ImGui::Columns(1);
      }

      ImGui::Text("Application average %.1f FPS", ImGui::GetIO().Framerate);
      ImGui::End();
    }

    // Check if recording window should close
    recordings.erase(std::remove_if(recordings.begin(), recordings.end(),
                                    [](const auto &r) -> bool {
                                      return glfwWindowShouldClose(r->window);
                                    }),
                     recordings.end());

    for (const auto &recording : recordings) {
      float &min = prm::diff_frames
                       ? recording->auto_diff_min
                       : prm::auto_scale ? recording->auto_min : prm::min;
      float &max = prm::diff_frames
                       ? recording->auto_diff_max
                       : prm::auto_scale ? recording->auto_max : prm::max;

      recording->display(min, max, prm::bitrange, prm::diff_frames, prm::speed);

      glfwMakeContextCurrent(main_window);
      {
        ImGui::SetNextWindowSizeConstraints(ImVec2(prm::main_window_width, 0),
                                            ImVec2(FLT_MAX, FLT_MAX));
        ImGui::Begin(recording->path().filename().c_str(), nullptr,
                     ImGuiWindowFlags_AlwaysAutoResize);
        {
          ImGui::Text("Date: %s", recording->date().c_str());

          if (!recording->comment().empty()) {
            ImGui::Text("Comment: %s", recording->comment().c_str());
          }

          ImGui::Columns(3);
          ImGui::Text("Duration  %.3fs", recording->duration().count());
          ImGui::NextColumn();
          ImGui::Text("FPS  %.3f", recording->fps());
          ImGui::NextColumn();
          ImGui::Text("Frames %d", recording->length());
          ImGui::NextColumn();
          ImGui::Text("Width  %d", recording->Nx());
          ImGui::NextColumn();
          ImGui::Text("Height %d", recording->Ny());
          ImGui::Columns(1);
        }

        ImGui::PushItemWidth(prm::main_window_width * 0.75f);
        ImGui::PlotHistogram("Histogram", recording->histogram.data.data(),
                             recording->histogram.data.size(), 0, nullptr, 0,
                             recording->histogram.max_value(), ImVec2(0, 100));

        ImGui::SliderFloat("min", &min, recording->histogram.min,
                           recording->histogram.max);
        ImGui::SliderFloat("max", &max, recording->histogram.min,
                           recording->histogram.max);

        auto progress_label = fmt::format(
            "Frame {}/{}", recording->current_frame() + 1, recording->length());
        ImGui::ProgressBar(recording->progress(), ImVec2(-1, 0),
                           progress_label.c_str());

        ImGui::Separator();
        for (auto &[trace, pos, color] : recording->traces) {
          auto label = pos.to_string();
          ImGui::PushID(label.c_str());

          ImGui::PushStyleColor(ImGuiCol_PlotLines,
                                ImVec4(color[0], color[1], color[2], 1));
          ImGui::PlotLines("", trace.data(), trace.size(), 0, NULL, FLT_MAX,
                           FLT_MAX, ImVec2(0, 100));
          ImGui::PopStyleColor(1);
          ImGui::SameLine();
          ImGui::BeginGroup();
          ImGui::ColorEdit3(label.c_str(), color.data(),
                            ImGuiColorEditFlags_NoInputs |
                                ImGuiColorEditFlags_NoLabel);
          // ImGui::SameLine();
          if (ImGui::Button("Reset")) {
            trace.clear();
          }
          // ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.y);
          if (ImGui::Button("Remove")) {
            recording->remove_trace_pos(pos[0], pos[1]);
          }
          ImGui::EndGroup();
          ImGui::PopID();
        }
        ImGui::End();
      }
    }
    glfwMakeContextCurrent(main_window);

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

int main(int, char **) {
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit())
    exit(EXIT_FAILURE);

  auto primary_monitor = glfwGetPrimaryMonitor();
  auto mode = glfwGetVideoMode(primary_monitor);

  prm::main_window_width = mode->width / 4;
  prm::main_window_height = 1.5 * prm::main_window_width;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  main_window =
      glfwCreateWindow(prm::main_window_width, prm::main_window_height,
                       "Quick MultiRecorder Viewer", NULL, NULL);
  if (!main_window) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }
  glfwMakeContextCurrent(main_window);
  glfwSwapInterval(1); // wait until the current frame has been drawn before
                       // drawing the next one

  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  // io.ConfigFlags |=
  //    ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable
  // Gamepad Controls
  // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
  // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable
  // Multi-Viewport /
  //                                                    // Platform Windows
  // io.ConfigViewportsNoAutoMerge = true;
  // io.ConfigViewportsNoTaskBarIcon = true;

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

  // When viewports are enabled we tweak WindowRounding/WindowBg so platform
  // windows can look identical to regular ones.
  ImGuiStyle &style = ImGui::GetStyle();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }

  // TODO: Better HIDIP handling
  float xscale, yscale;
  glfwGetMonitorContentScale(primary_monitor, &xscale, &yscale);
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
  // io.Fonts->AddFontDefault();
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
  // ImFont* font =
  // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f,
  // NULL, io.Fonts->GetGlyphRangesJapanese()); IM_ASSERT(font != NULL);

  display();

  glfwDestroyWindow(main_window);
  recordings.clear();
  glfwTerminate();
  exit(EXIT_SUCCESS);
}