#ifdef _WIN32 // Windows 32 and 64 bit
#include <windows.h>
#endif

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"

#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#define EIGEN_MAX_CPP_VER 14
#define EIGEN_HAS_CXX11_CONTAINERS 1
#include <Eigen/Dense>
#include <fmt/format.h>

using pix_val_t = uint16_t;

using Vec2f = Eigen::Vector2f;
using Vec3f = Eigen::Vector3f;
using PixArray = Eigen::Matrix<pix_val_t, Eigen::Dynamic, Eigen::Dynamic>;

static GLFWwindow *main_window = nullptr;

class BMPheader {
private:
  std::ifstream _in;

  char mVersion;
  uint32_t mByteOrderMark = 0x1A2B3C4D;

  uint32_t mNumFrames = 0;
  uint32_t mNumBytes = 0;
  uint32_t mFrameSize = 0xFFFFFF;
  uint32_t mFormat;

  uint32_t mFrequency = 0;
  uint32_t mFrameWidth = 0;
  uint32_t mFrameHeight = 0;

  std::string mDate;
  std::string mComment;

  uint64_t mFirstFrameTime = 0;
  uint64_t mLastFrameTime = 0;
  uint64_t mRecordingPeriod = 0;

public:
  const char Version = 'f';
  const uint32_t ByteOrderMark = 0x1A2B3C4D;
  const uint32_t HeaderLength = 1024;
  const uint32_t TrailerLength = 0;
  const std::string DateFormat = "yyyy-MM-dd hh:mm:ss:zzz";

  BMPheader(std::string path) : _in(path, std::ios::in | std::ios::binary) {
    _in.exceptions(std::ifstream::eofbit | std::ifstream::failbit |
                   std::ifstream::badbit);

    if (!_in) {
      fmt::print("ERROR: {} does not seem to be a file!\n", path);
    }

    _in.seekg(0);

    if (!_in.read(&mVersion, sizeof(char)) || mVersion != Version) {
      fmt::print("Read failed, not a bmp recording?\n");
    }

    if (!_in.read((char *)&mByteOrderMark, sizeof(uint32_t)) ||
        mByteOrderMark != ByteOrderMark) {
      fmt::print("Read failed, not a bmp recording?\n");
      return;
    }

    if ((!_in.read((char *)&mNumFrames, sizeof(uint32_t)))) {
      fmt::print("Read failed!\n");
      return;
    }

    if ((!_in.read((char *)&mFrameWidth, sizeof(uint32_t)))) {
      fmt::print("Read failed!\n");
      return;
    }

    if (!_in.read((char *)&mFrameHeight, sizeof(uint32_t))) {
      fmt::print("Read failed!\n");
      return;
    }

    if (!_in.read((char *)&mFormat, sizeof(uint32_t))) {
      fmt::print("Read failed!\n");
      return;
    }
    if (mFormat == 0) {
      mFormat = 1;
    }
    mFrameSize = (mFrameWidth * mFrameHeight);
    if (mFormat == 3) {
      mFrameSize *= 2;
    }

    uint32_t bin = 0;
    if (!_in.read((char *)&bin, sizeof(uint32_t))) {
      fmt::print("Read failed!\n");
    }

    if (!_in.read((char *)&mFrequency, sizeof(uint32_t))) {
      fmt::print("Read failed!\n");
    }

    char data;
    do {
      _in.read(&data, sizeof(char));
      if (data != '\0') {
        mDate.append(std::to_string(data));
      }
      // we dont want to return false on error since an error here should not
      // break the whole file
    } while (data != '\0');

    mComment = "";
    do {
      _in.read(&data, sizeof(char));
      if (data != '\0') {
        mComment.append(std::to_string(data));
      }
      // we dont want to return false on error since an error here should not
      // break the whole file
    } while (data != '\0');

    //// calculate the recording period, firstFrametime and lastFramtime;
    _in.seekg(HeaderLength + mFrameSize, std::ios::beg);
    if (_in.read((char *)&mFirstFrameTime, sizeof(uint64_t))) {
      fmt::print("Failed to read time of first frame.\n");
    }

    _in.seekg(0, std::ios::end);
    long length = _in.tellg();
    _in.seekg(length - sizeof(uint64_t), std::ios::beg);
    if (!_in.read((char *)&mLastFrameTime, sizeof(uint64_t))) {
      fmt::print("Failed to read field Frequency.");
    }

    mRecordingPeriod = mFirstFrameTime - mLastFrameTime;
    // mFrequency = mRecordingPeriod / 1000 / mNumFrames;
  };

  void read_frame(long t, pix_val_t *data) {
    if (!data) {
      throw std::runtime_error("WTF");
    }

    _in.seekg(HeaderLength + t * (mFrameSize + sizeof(uint64_t)),
              std::ios::beg);
    _in.read((char *)data, mFrameSize * sizeof(int8_t));
  }

  uint32_t Nx() { return mFrameWidth; }
  uint32_t Ny() { return mFrameHeight; }
  long length() { return mNumFrames; }
};

class Recording {
  std::string _path;

  BMPheader fileheader;
  long _t = 0;

public:
  PixArray frame;
  GLFWwindow *window = nullptr;

  Recording(std::string path)
      : _path(path), fileheader(path), frame(fileheader.Nx(), fileheader.Ny()) {
    load_frame(0);
  }

  void load_frame(long t) { fileheader.read_frame(t, frame.data()); }

  void load_next_frame(int frameskip = 0) {
    _t += frameskip + 1;
    while (_t > length() - 1) {
      _t -= length();
    }

    if (_t < 0) {
      _t = 0;
    }

    load_frame(_t);
  }

  long Nx() { return frame.cols(); }
  long Ny() { return frame.rows(); }
  long length() { return fileheader.length(); }
  std::string filepath() { return _path; }
};

static std::vector<std::shared_ptr<Recording>> recordings = {};

void recordings_window_close_callback(GLFWwindow *window) {
  recordings.erase(
      std::remove_if(recordings.begin(), recordings.end(),
                     [window](auto r) { return r->window == window; }),
      recordings.end());
  glfwDestroyWindow(window);
}

void load_new_file(const std::string &path) {
  fmt::print("Loading {} ...\n", path);

  recordings.emplace_back(std::make_shared<Recording>(path));
  auto rec = recordings.back();

  auto title = fmt::format("Window {}", recordings.size() + 1);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  GLFWwindow *window =
      glfwCreateWindow(rec->Nx(), rec->Ny(), title.c_str(), NULL, NULL);
  if (!window) {
    fmt::print("ERROR: window created failed for {}\n", path);
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // wait until the current frame has been drawn before
  rec->window = window;
  glfwSetWindowCloseCallback(window, recordings_window_close_callback);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity(); // Reset The Projection Matrix
  glOrtho(0, rec->Nx(), rec->Ny(), 0, -1, 1);
  //https://docs.microsoft.com/en-us/previous-versions//ms537249(v=vs.85)?redirectedfrom=MSDN
  //http://www.songho.ca/opengl/gl_projectionmatrix.html#ortho
  glMatrixMode(GL_MODELVIEW); // Select The Modelview Matrix
  glLoadIdentity();           // Reset The Modelview Matrix

  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);

  glfwMakeContextCurrent(main_window);
}

void drop_callback(GLFWwindow *window, int count, const char **paths) {
  for (int i = 0; i < count; i++) {
    load_new_file(paths[i]);
  }
}

static void glfw_error_callback(int error, const char *description) {
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

Vec3f val_to_color(const pix_val_t &val, const pix_val_t &min,
                   const pix_val_t &max) {
  float x = static_cast<float>(val - min) / static_cast<float>(max - min);

  if (x < 0) {
    x = 0;
  } else if (x > 1) {
    x = 1;
  }

  return {x, x, x};
}

void draw2dArray(const PixArray &arr, const Vec2f &position, float scale,
                 pix_val_t min, pix_val_t max) {
  auto image_width = arr.rows();
  auto image_height = arr.cols();

  Vec2f pos1, pos2, pos3, pos4;
  for (long x = 0; x < image_width; x++) {
    for (long y = 0; y < image_height; y++) {
      pos1 = Vec2f(y, image_width - x) * scale + position;
      pos2 = Vec2f(y + 1, image_width - x) * scale + position;
      pos3 = Vec2f(y + 1, image_width - (x + 1)) * scale + position;
      pos4 = Vec2f(y, image_width - (x + 1)) * scale + position;

      auto val = arr(x, y);
      auto c = val_to_color(val, min, max);

      if (std::isnan(val)) {
        c = {1, 0, 0};
      }

      glBegin(GL_QUADS);
      glColor3fv(c.data());
      glVertex2fv(pos1.data());
      glVertex2fv(pos2.data());
      glVertex2fv(pos3.data());
      glVertex2fv(pos4.data());
      glEnd();
    }
  }
}

namespace prm {
static float scale_fct = 1;
static int min = 0;
static int max = 4096;
static int frameskip = 0;
} // namespace prm

template <typename T, size_t bin_count> class Histogram {
  std::array<T, bin_count> tmp = {};

public:
  std::array<T, bin_count> data = {};

  pix_val_t max;

  Histogram(pix_val_t _max) : max(_max){};

  template <typename Container> void compute(const Container &container) {
    tmp.fill(0);

    pix_val_t bin_size = (max + 1) / bin_count;

    for (auto val : container) {
      if (val > max) {val = max;}

      const uint index = val / bin_size;
      tmp[index] += 1;
    }

    // normalize such that the maximal bin equals 1.0
    auto max_element = std::max_element(tmp.begin(), tmp.end());
    for (size_t i = 0; i < bin_count; i++) {
      data[i] = tmp[i] / static_cast<double>(*max_element);
    }
  }
};

void display() {
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  // Our state
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  Histogram<float, 256> histogram(4096);

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
      ImGui::Begin("Drag & Drop Files here");

      ImGui::SliderInt("frameskip", &prm::frameskip, 0, 50);

      ImGui::SliderInt("min", &prm::min, 0, 4096);
      ImGui::SliderInt("max", &prm::max, 0, 4096);

      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                  1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      ImGui::End();
    }

    //// 3. Show another simple window.
    // if (show_another_window) {
    //  ImGui::Begin(
    //      "Another Window",
    //      &show_another_window); // Pass a pointer to our bool variable (the
    //                             // window will have a closing button that
    //                             // will clear the bool when clicked)
    //  ImGui::Text("Hello from another window!");
    //  if (ImGui::Button("Close Me"))
    //    show_another_window = false;
    //  ImGui::End();
    //}

    for (auto &recording : recordings) {
      glfwMakeContextCurrent(recording->window);

      recording->load_next_frame(prm::frameskip);

      Vec2f offset = {0, 0};
      draw2dArray(recording->frame, offset, prm::scale_fct, prm::min, prm::max);
      glfwSwapBuffers(recording->window);

      glfwMakeContextCurrent(main_window);
      {
        ImGui::Begin(fmt::format("Recording {}", recordings.size()).c_str());

        ImGui::Text("Filepath %s", recording->filepath().c_str());
        ImGui::Text("Nx %d", recording->Nx());
        ImGui::Text("Ny %d", recording->Ny());

        histogram.compute(recordings[0]->frame.reshaped());
        ImGui::PlotHistogram("Hist1", histogram.data.data(),
                             histogram.data.size(), 0, NULL, 0, 1,
                             ImVec2(0, 80));

        ImGui::SliderInt("min", &prm::min, 0, 4096);
        ImGui::SliderInt("max", &prm::max, 0, 4096);
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

int main(int, char **) {
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit())
    exit(EXIT_FAILURE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  main_window = glfwCreateWindow(640, 640, "Simple example", NULL, NULL);
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
  // ImGui::StyleColorsClassic();

  // When viewports are enabled we tweak WindowRounding/WindowBg so platform
  // windows can look identical to regular ones.
  ImGuiStyle &style = ImGui::GetStyle();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }

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
  for (auto recording : recordings) {
    glfwDestroyWindow(recording->window);
  }
  glfwTerminate();
  exit(EXIT_SUCCESS);
}