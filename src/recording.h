#pragma once

#include <string>

#include <Eigen/Dense>
#include <GLFW/glfw3.h>

#include "bmp.h"
#include "utils.h"

extern GLFWwindow *main_window;

class RecordingWindow;
extern std::vector<std::shared_ptr<RecordingWindow>> recordings;

class Recording {
protected:
  filesystem::path _path;

  BMPheader fileheader;
  int _t = 0;
  float _tf = 0;

  Eigen::Matrix<uint16, Eigen::Dynamic, Eigen::Dynamic> raw_frame;

  long t_frame = 0;
  long t_prev_frame = 0;

public:
  Eigen::MatrixXf frame;
  Eigen::MatrixXf prev_frame;
  Eigen::MatrixXf frame_diff;

  float auto_max = 0;
  float auto_min = 0;

  float auto_diff_max = 0;
  float auto_diff_min = 0;

  Recording(filesystem::path path)
      : _path(path), fileheader(path),
        raw_frame(fileheader.Nx(), fileheader.Ny()),
        frame(fileheader.Nx(), fileheader.Ny()),
        prev_frame(fileheader.Nx(), fileheader.Ny()),
        frame_diff(fileheader.Nx(), fileheader.Ny()) {

    if (!fileheader.good()) {
      return;
    }

    // load a frame from the middle to calculate auto min/max
    load_frame(length() / 2);
    auto_max = frame.maxCoeff();
    auto_min = frame.minCoeff();

    prev_frame = frame;
    load_frame(length() / 2 + 1);
    compute_frame_diff();

    auto_diff_max = std::max(std::abs(frame_diff.minCoeff()),
                             std::abs(frame_diff.maxCoeff()));
    auto_diff_min = -auto_diff_max;
  }

  bool good() { return fileheader.good(); }
  int Nx() { return frame.cols(); }
  int Ny() { return frame.rows(); }
  int length() { return fileheader.length(); }
  filesystem::path path() { return _path; }
  std::string date() { return fileheader.date(); };
  std::string comment() { return fileheader.comment(); };
  std::chrono::duration<float> duration() { return fileheader.duration(); }
  float fps() { return fileheader.fps(); }

  void load_frame(long t) {
    fileheader.read_frame(t, raw_frame.data());
    frame = raw_frame.cast<float>();
    t_frame = t;
  }

  void load_next_frame(float speed = 1) {
    _tf += speed;

    while (_tf > length() - 1) {
      _tf -= length() - 1;
      _t = 0;
    }

    if (std::floor(_tf) > _t) {
      _t = std::floor(_tf);
    }

    load_frame(_t);
  }

  void compute_frame_diff() {
    if (t_prev_frame != t_frame) {
      frame_diff = frame - prev_frame;
      prev_frame.swap(frame);
      std::swap(t_prev_frame, t_frame);
    }
  }

  int current_frame() { return _t; }
  float progress() { return _t / static_cast<float>(length() - 1); }
};

class RecordingWindow : public Recording {
public:
  GLFWwindow *window = nullptr;

  struct Trace {
    std::array<int, 2> pos;
    std::vector<float> data;
    std::array<float, 4> color;

    static std::array<float, 4> next_color() {
      static std::array<std::array<float, 4>, 4> cycle_list = {{
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
  };
  std::vector<Trace> traces;

  std::array<double, 2> mousepos;

  Histogram<float, 256> histogram;

  RecordingWindow(filesystem::path path) : Recording(path){};
  ~RecordingWindow() {
    if (window != nullptr) {
      glfwDestroyWindow(window);
    }
  }

  void reset_traces() {
    for (auto &t : traces) {
      t.data.clear();
    }
  }

  void add_trace_pos(int x, int y) {
    traces.push_back({.pos = {x, y}, .data = {}, .color = Trace::next_color()});
  }

  void remove_trace_pos(int x, int y) {
    traces.erase(
        std::remove_if(traces.begin(), traces.end(), [x, y](const auto &trace) {
          return trace.pos[0] == x && trace.pos[1] == y;
        }));
  }

  void display(float min, float max, float bitrange, bool diff_frames = false,
               float speed = 1) {
    if (!window)
      throw std::runtime_error(
          "No window set, but RecordingWindow::display() called");

    glfwMakeContextCurrent(window);

    load_next_frame(speed);

    if (diff_frames) {
      compute_frame_diff();
      draw2dArray(frame_diff, min, max);

      histogram.min = -bitrange / 10.f;
      histogram.max = bitrange / 10.f;
      histogram.compute(frame_diff.reshaped());
    } else {
      draw2dArray(frame, min, max);

      histogram.min = 0;
      histogram.max = bitrange;
      histogram.compute(frame.reshaped());
    }

    for (auto &[pos, trace, color] : traces) {
      if (diff_frames) {
        trace.push_back(frame_diff(pos[0], pos[1]));
      } else {
        trace.push_back(frame(pos[0], pos[1]));
      }

      drawPixel(pos[0], pos[1], Nx(), 4, color);
    }

    glfwSwapBuffers(window);

    glfwMakeContextCurrent(main_window);
  }

  void open_window(float scale_fct) {
    if (window) {
      throw std::runtime_error("ERROR: window was already initialized");
    }

    auto title = _path.filename().string();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    window = glfwCreateWindow(Nx(), Ny(), title.c_str(), NULL, NULL);
    if (!window) {
      fmt::print("ERROR: window created failed for {}\n", title);
    }

    auto prev_window = glfwGetCurrentContext();
    glfwMakeContextCurrent(window);

    glfwSwapInterval(1); // wait until the current frame has been drawn before
    glfwSetWindowCloseCallback(window, RecordingWindow::close_callback);
    glfwSetKeyCallback(window, RecordingWindow::key_callback);
    glfwSetWindowSizeCallback(window, RecordingWindow::reshape_callback);
    glfwSetCursorPosCallback(window, RecordingWindow::cursor_position_callback);
    glfwSetMouseButtonCallback(window, RecordingWindow::mouse_button_callback);
    glfwSetWindowAspectRatio(window, Nx(), Ny());

    resize_window(scale_fct);

    glfwMakeContextCurrent(prev_window);
  }

  void resize_window(float scale = 1) {
    int width = std::ceil(scale * Nx());
    int height = std::ceil(scale * Ny());

    glfwSetWindowSize(window, width, height);
    reshape_callback(window, width, height);
  }

  static void cursor_position_callback(GLFWwindow *window, double xpos,
                                       double ypos) {
    std::shared_ptr<RecordingWindow> rec = from_window_ptr(window);
    rec->mousepos = {xpos, ypos};
  }

  static void mouse_button_callback(GLFWwindow *window, int button, int action,
                                    int mods) {
    std::shared_ptr<RecordingWindow> rec = from_window_ptr(window);

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
      int w, h;
      glfwGetWindowSize(window, &w, &h);
      int x = rec->mousepos[0] * rec->Nx() / w;
      int y = rec->mousepos[1] * rec->Ny() / h;

      fmt::print("Click on pos ({}, {}), calc index ({}, {})\n",
                 rec->mousepos[0], rec->mousepos[1], x, y);

      // TODO: FIXME
      std::swap(x, y);
      rec->add_trace_pos(x, y);
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
    }
  }

protected:
  static std::shared_ptr<RecordingWindow> from_window_ptr(GLFWwindow *_window) {
    return *std::find_if(
        recordings.begin(), recordings.end(),
        [_window](const auto &r) { return r->window == _window; });
  }
};