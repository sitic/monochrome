#pragma once

#include <GLFW/glfw3.h>

#include "recording.h"
#include "transformations.h"
#include "videorecorder.h"

extern GLFWwindow *main_window;

class RecordingWindow;
extern std::vector<std::shared_ptr<RecordingWindow>> recordings;

struct ExportCtrl {
  struct {
    bool export_window = false;
    Vec2i start;
    Vec2i size;
    Vec2i frames;
    std::vector<char> filename = {};

    void assign_auto_filename(const filesystem::path &bmp_path) {
      auto fn = bmp_path.filename().string();
      auto parts = split_string(fn, "_"s.c_str());
      if (parts.size() > 4) {
        fn = fmt::format("{}_{}", parts[1], parts[2]);
      }

      if (frames[0] != 0) {
        fn += "_o"s + std::to_string(frames[0]);
      }

      fn = fmt::format("{}_{}x{}x{}f.dat", fn, size[0], size[1],
                       frames[1] - frames[0]);

      filename.assign(fn.begin(), fn.end());

      // Make sure there is enough space for the user input
      if (filename.size() < 64) {
        filename.resize(64);
      }
    }
  } raw;

  struct {
    bool export_window = false;
    bool recording = false;
    float progress = 0;
    std::vector<char> filename = {};
    VideoRecorder videoRecorder;

    void assign_auto_filename(const filesystem::path &bmp_path) {
      videoRecorder.videotitle = bmp_path.filename().string();

      std::string fn = bmp_path.filename().with_extension(".mp4").string();
      filename.assign(fn.begin(), fn.end());

      // Make sure there is enough space for the user input
      if (filename.size() < 64) {
        filename.resize(64);
      }
    }
  } video;

  struct {
    bool export_window = false;
    bool save_pngs = false;
    std::vector<char> filename = {};

    void assign_auto_filename(const filesystem::path &bmp_path) {
      std::string fn = bmp_path.filename().stem().string() + "_{t}.png";
      filename.assign(fn.begin(), fn.end());

      // Make sure there is enough space for the user input
      if (filename.size() < 64) {
        filename.resize(64);
      }
    }
  } png;
};

struct Trace {
  std::vector<float> data;
  Vec2i pos;
  Vec4f color;

  static Vec4f next_color() {
    // List of colors to cycle through
    static std::array<Vec4f, 4> cycle_list = {{
        {228 / 255.f, 26 / 255.f, 28 / 255.f, 1},
        {55 / 255.f, 126 / 255.f, 184 / 255.f, 1},
        {77 / 255.f, 175 / 255.f, 74 / 255.f, 1},
        {152 / 255.f, 78 / 255.f, 163 / 255.f, 1},
    }};

    static int count = -1;
    count++;
    if (count >= cycle_list.size()) {
      count = 0;
    }
    return cycle_list.at(count);
  }

  static int width(int new_width = 0) {
    static int w = 0;

    if (new_width > 0) {
      w = new_width;
    }

    return w;
  }
};

class RecordingWindow : public Recording {
public:
  GLFWwindow *window = nullptr;
  static float scale_fct;

  Transformation::None no_transformation;
  Transformation::FrameDiff frameDiff;
  Transformation::ContrastEnhancement contrastEnhancement;
  Transformation::MeanFilter meanFilter;
  Transformation::MedianFilter medianFilter;

  float &get_max(Transformations type) {
    switch (type) {
    case Transformations::None:
      return no_transformation.max;
    case Transformations::FrameDiff:
      return frameDiff.max;
    case Transformations::ContrastEnhancement:
      return contrastEnhancement.max;
    default:
      throw std::logic_error("This line should not be reachable");
    }
  }

  float &get_min(Transformations type) {
    switch (type) {
    case Transformations::None:
      return no_transformation.min;
    case Transformations::FrameDiff:
      return frameDiff.min;
    case Transformations::ContrastEnhancement:
      return contrastEnhancement.min;
    default:
      throw std::logic_error("This line should not be reachable");
    }
  }

  Histogram<float, 256> histogram;
  ExportCtrl export_ctrl;
  std::vector<Trace> traces;

  RecordingWindow(filesystem::path path) : Recording(path) {
    if (!good()) {
      return;
    }

    if (Trace::width() == 0) {
      // if unset, set trace edge length to something reasonable
      auto min = std::min(Nx(), Ny());
      Trace::width(min / 64);
    }

    no_transformation.assign(*this);
    frameDiff.assign(*this);
    contrastEnhancement.assign(*this);
    meanFilter.assign(*this);
    medianFilter.assign(*this);
  };

  ~RecordingWindow() {
    if (window != nullptr) {
      glfwDestroyWindow(window);
    }
  }

  void display(float speed, Transformations transformation, BitRange bitrange) {
    if (!window)
      throw std::runtime_error(
          "No window set, but RecordingWindow::display() called");

    glfwMakeContextCurrent(window);

    load_next_frame(speed);

    Eigen::MatrixXf *arr = &frame;
    if (transformation == Transformations::None) {
      histogram.min = 0;
      histogram.max = bitrange_to_float(bitrange);
    } else if (transformation == Transformations::FrameDiff) {
      frameDiff.compute(frame, t_frame);
      arr = &frameDiff.frame;

      histogram.min = frameDiff.min_init() * 1.5;
      histogram.max = frameDiff.max_init() * 1.5;
    } else if (transformation == Transformations::ContrastEnhancement) {
      contrastEnhancement.compute(frame, t_frame);
      arr = &contrastEnhancement.frame;

      histogram.min = 0;
      histogram.max = 1;
    }

    if (meanFilter.enabled) {
      meanFilter.compute(*arr, t_frame);
      arr = &meanFilter.frame;
    }

    if (medianFilter.enabled) {
      medianFilter.compute(*arr, t_frame);
      arr = &medianFilter.frame;
    }

    draw2dArray(*arr, get_min(transformation), get_max(transformation));
    histogram.compute(arr->reshaped());

    for (auto &[trace, pos, color] : traces) {
      // We need to make sure, the block is inside the frame
      const auto test_fct = [Nx = Nx(), Ny = Ny(),
                             w = Trace::width()](const Vec2i &v) {
        auto lim = [](auto x, auto N) { return ((x > 0) && (x < N)); };
        return (lim(v[0], Nx) && lim(v[1], Ny));
      };

      auto w = Trace::width();
      Vec2i start = {pos[0] - w / 2, pos[1] - w / 2};
      // shrink the trace block width if it is too large
      while (!test_fct(start) || !test_fct(start + Vec2i(w, w))) {
        w -= 1;
        Trace::width(w);
        start = {pos[0] - w / 2, pos[1] - w / 2};
      }

      auto block = arr->block(start[0], start[1], w, w);
      trace.push_back(block.mean());
      drawPixel(pos[0], pos[1], Ny(), w, color);
    }

    glfwSwapBuffers(window);

    if (export_ctrl.video.recording) {
      auto cur = progress();
      if (cur < export_ctrl.video.progress) {
        stop_recording();
        new_ui_message("Exporting video finished!");
      } else {
        export_ctrl.video.videoRecorder.add_frame();
        export_ctrl.video.progress = cur;
      }
    }

    glfwMakeContextCurrent(main_window);
  }

  void reset_traces() {
    for (auto &t : traces) {
      t.data.clear();
    }
  }

  void add_trace_pos(const Vec2i &npos) {
    for (auto &[trace, pos, color] : traces) {
      auto d = pos - npos;
      if (std::abs(d[0]) < Trace::width() / 2 &&
          std::abs(d[1]) < Trace::width() / 2) {
        trace.clear();
        pos = npos;
        return;
      }
    }
    traces.push_back({{}, npos, Trace::next_color()});
  }

  void remove_trace_pos(const Vec2i &pos) {
    const auto pred = [pos](const auto &trace) {
      auto d = pos - trace.pos;
      auto max_dist = Trace::width() / 2;
      return (std::abs(d[0]) < max_dist && std::abs(d[1]) < max_dist);
    };

    traces.erase(std::remove_if(traces.begin(), traces.end(), pred),
                 traces.end());
  }

  void start_recording(const std::string &filename, int fps = 30) {
    restart();
    export_ctrl.video.videoRecorder.start_recording(filename, window, fps);
    export_ctrl.video.recording = true;
    export_ctrl.video.progress = 0;
  }

  void stop_recording() {
    export_ctrl.video.videoRecorder.stop_recording();
    export_ctrl.video.recording = false;
    export_ctrl.video.progress = 0;
    export_ctrl.video.export_window = false;
  }

  void open_window() {
    if (window) {
      throw std::runtime_error("ERROR: window was already initialized");
    }

    auto title = _path.filename().string();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    window = glfwCreateWindow(Nx(), Ny(), title.c_str(), nullptr, nullptr);
    if (!window) {
      new_ui_message("ERROR: window created failed for {}", title);
      return;
    }

    auto prev_window = glfwGetCurrentContext();
    glfwMakeContextCurrent(window);

    // we don't need to wait for the next frame here, because the main window
    // limits us to 60fps already
    glfwSwapInterval(0);

    glfwSetWindowCloseCallback(window, RecordingWindow::close_callback);
    glfwSetKeyCallback(window, RecordingWindow::key_callback);
    glfwSetWindowSizeCallback(window, RecordingWindow::reshape_callback);
    glfwSetCursorPosCallback(window, RecordingWindow::cursor_position_callback);
    glfwSetMouseButtonCallback(window, RecordingWindow::mouse_button_callback);
    glfwSetScrollCallback(window, RecordingWindow::scroll_callback);
    glfwSetWindowAspectRatio(window, Nx(), Ny());

    resize_window();

    glfwMakeContextCurrent(prev_window);
  }

  void resize_window() {
    int width = std::ceil(RecordingWindow::scale_fct * Nx());
    int height = std::ceil(RecordingWindow::scale_fct * Ny());

    glfwSetWindowSize(window, width, height);
    reshape_callback(window, width, height);
  }

  static void scroll_callback(GLFWwindow *window, double xoffset,
                              double yoffset) {
    // if traces shown, change trace width, else window width
    std::shared_ptr<RecordingWindow> rec = from_window_ptr(window);
    if (!rec->traces.empty()) {
      auto w = Trace::width();
      int new_w = (yoffset < 0) ? 0.95f * w : 1.05f * w;
      if (new_w == w) {
        new_w = (yoffset < 0) ? w - 1 : w + 1;
      }
      Trace::width(new_w);
    } else {
      scale_fct = (yoffset < 0) ? 0.95f * scale_fct : 1.05f * scale_fct;
      for (const auto &r : recordings) {
        r->resize_window();
      }
    }
  }

  static void cursor_position_callback(GLFWwindow *window, double xpos,
                                       double ypos) {
    std::shared_ptr<RecordingWindow> rec = from_window_ptr(window);
    rec->mousepos = {xpos, ypos};
    if (rec->mousebutton.left) {
      int w, h;
      glfwGetWindowSize(window, &w, &h);
      int x = rec->mousepos[0] * rec->Nx() / w;
      int y = rec->mousepos[1] * rec->Ny() / h;

      rec->add_trace_pos({x, y});
    }
  }

  static void mouse_button_callback(GLFWwindow *window, int button, int action,
                                    int mods) {
    std::shared_ptr<RecordingWindow> rec = from_window_ptr(window);
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
      if (action == GLFW_PRESS) {
        rec->mousebutton.left = true;
        rec->cursor_position_callback(window, rec->mousepos[0],
                                      rec->mousepos[1]);
      } else {
        rec->mousebutton.left = false;
      }
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
      if (action == GLFW_PRESS) {
        int w, h;
        glfwGetWindowSize(window, &w, &h);
        int x = rec->mousepos[0] * rec->Nx() / w;
        int y = rec->mousepos[1] * rec->Ny() / h;

        rec->remove_trace_pos({x, y});
      }
    }
  }

  static void reshape_callback(GLFWwindow *window, int w, int h) {
    std::shared_ptr<RecordingWindow> rec = from_window_ptr(window);

    if (!rec) {
      throw std::runtime_error("Error in RecordingWindow::reshape_callback, "
                               "could not find associated recording");
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

  static void close_callback(GLFWwindow *window) {
    recordings.erase(
        std::remove_if(recordings.begin(), recordings.end(),
                       [window](auto r) { return r->window == window; }),
        recordings.end());
  }

  static void key_callback(GLFWwindow *window, int key, int scancode,
                           int action, int mods) {
    if ((key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) && action == GLFW_PRESS) {
      // Don't call RecordingWindow::close_callback() directly here,
      // causes a segfault in glfw
      glfwSetWindowShouldClose(window, GLFW_TRUE);
    } else if (key == GLFW_KEY_P && action == GLFW_PRESS) {
      std::shared_ptr<RecordingWindow> rec = from_window_ptr(window);
      auto fn = rec->save_snapshot();
      new_ui_message("Saved screenshot to {}", fn.string());
    }
  }

  filesystem::path save_snapshot(std::string output_png_path_template = "") {
    if (output_png_path_template.empty()) {
      output_png_path_template = path().stem().string() + "_{t}.png";
    }
    auto out_path =
        fmt::format(output_png_path_template, fmt::arg("t", t_frame));

    gl_save_snapshot(out_path, window);
    return out_path;
  }

protected:
  Vec2d mousepos;
  struct {
    bool left = false;
    bool right = false;
  } mousebutton;

  static std::shared_ptr<RecordingWindow> from_window_ptr(GLFWwindow *_window) {
    return *std::find_if(
        recordings.begin(), recordings.end(),
        [_window](const auto &r) { return r->window == _window; });
  }
};

// this should be in a cpp file, but does not matter in this case
float RecordingWindow::scale_fct = 1;
unsigned Transformation::ContrastEnhancement::kernel_size = 3;
unsigned Transformation::MeanFilter::kernel_size = 3;
unsigned Transformation::MedianFilter::kernel_size = 3;
bool Transformation::MeanFilter::enabled = false;
bool Transformation::MedianFilter::enabled = false;