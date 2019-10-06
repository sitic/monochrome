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

#include "definitions.h"
#include "recording.h"
#include "utils.h"

GLFWwindow *main_window = nullptr;

std::vector<std::shared_ptr<Recording>> recordings = {};

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
static int min = 0;
static int max = bitrange;

static float speed = 1;
static float scale_fct = 1;
} // namespace prm

void recording_reshape_callback(GLFWwindow *window, int w, int h) {
  std::shared_ptr<Recording> rec;
  for (auto r : recordings) {
    if (r->window == window) {
      rec = r;
    }
  }

  if (!rec) {
    throw std::runtime_error("Error in recording_reshape_callback, could not "
                             "find associated recording");
  }

  glfwMakeContextCurrent(window);
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity(); // Reset The Projection Matrix
  glOrtho(0, rec->Nx(), 0, rec->Ny(), -1, 1);
  // https://docs.microsoft.com/en-us/previous-versions//ms537249(v=vs.85)?redirectedfrom=MSDN
  // http://www.songho.ca/opengl/gl_projectionmatrix.html#ortho
  glMatrixMode(GL_MODELVIEW); // Select The Modelview Matrix
  glLoadIdentity();           // Reset The Modelview Matrix

  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);

  glfwMakeContextCurrent(main_window);
}

void resize_recording_window(std::shared_ptr<Recording> rec) {
  auto window = rec->window;
  int width = std::ceil(prm::scale_fct * rec->Nx());
  int height = std::ceil(prm::scale_fct * rec->Ny());

  glfwSetWindowSize(window, width, height);
  recording_reshape_callback(window, width, height);
}

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

  recordings.emplace_back(std::make_shared<Recording>(path));
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

  auto title = path.filename().string();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  GLFWwindow *window =
      glfwCreateWindow(rec->Nx(), rec->Ny(), title.c_str(), NULL, NULL);
  if (!window) {
    fmt::print("ERROR: window created failed for {}\n",
               path.filename().string());
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // wait until the current frame has been drawn before
  rec->window = window;
  glfwSetWindowCloseCallback(window, recordings_window_close_callback);
  glfwSetKeyCallback(window, recording_window_callback);
  glfwSetWindowSizeCallback(window, recording_reshape_callback);
  glfwSetWindowAspectRatio(window, rec->Nx(), rec->Ny());

  resize_recording_window(rec);
  glfwMakeContextCurrent(main_window);
}

void display() {
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  // Our state
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  Histogram<float, 256> histogram;

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
        if (ImGui::SliderFloat("scale", &prm::scale_fct, 0.5, 5)) {
          for (const auto &r : recordings) {
            resize_recording_window(r);
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
        ImGui::Checkbox("Show frame difference", &prm::diff_frames);
        ImGui::Columns(1);
      }

      ImGui::Text("Application average %.1f FPS", ImGui::GetIO().Framerate);
      ImGui::End();
    }

    // Check if recording window should close
    for (auto &recording : recordings) {
      if (glfwWindowShouldClose(recording->window)) {
        recordings_window_close_callback(recording->window);
      }
    }

    for (auto &recording : recordings) {
      glfwMakeContextCurrent(recording->window);

      recording->load_next_frame(prm::speed);

      int &min = prm::diff_frames
                     ? recording->auto_diff_min
                     : prm::auto_scale ? recording->auto_min : prm::min;
      int &max = prm::diff_frames
                     ? recording->auto_diff_max
                     : prm::auto_scale ? recording->auto_max : prm::max;

      if (prm::diff_frames) {
        recording->tmp = recording->frame - recording->prev_frame;
        std::swap(recording->frame, recording->prev_frame);
        draw2dArray(recording->tmp, {0, 0}, 1, recording->auto_diff_min,
                    recording->auto_diff_max);
      } else {
        draw2dArray(recording->frame, {0, 0}, 1, min, max);
      }
      glfwSwapBuffers(recording->window);

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
        if (prm::diff_frames) {
          histogram.compute(recording->tmp.reshaped(), -prm::bitrange / 10,
                            prm::bitrange / 10);
        } else {
          histogram.compute(recording->frame.reshaped(), 0, prm::bitrange);
        }

        ImGui::PlotHistogram("Histogram", histogram.data.data(),
                             histogram.data.size(), 0, nullptr, 0,
                             histogram.max_value(), ImVec2(0, 100));

        if (!prm::diff_frames) {
          ImGui::SliderInt("min", &min, 0, prm::bitrange);
          ImGui::SliderInt("max", &max, 0, prm::bitrange);
        } else {
          ImGui::SliderInt("min", &min, -prm::bitrange / 10,
                           prm::bitrange / 10);
          ImGui::SliderInt("max", &max, -prm::bitrange / 10,
                           prm::bitrange / 10);
        }
        auto progress_label = fmt::format(
            "Frame {}/{}", recording->current_frame() + 1, recording->length());
        ImGui::ProgressBar(recording->progress(), ImVec2(-1, 0),
                           progress_label.c_str());
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

  // TODO: HIDIP handling
  //style.ScaleAllSizes(2);

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
  for (const auto &recording : recordings) {
    glfwDestroyWindow(recording->window);
  }
  glfwTerminate();
  exit(EXIT_SUCCESS);
}