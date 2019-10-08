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

  GLFWwindow *window = nullptr;

  int auto_max = 0;
  int auto_min = 0;

  int auto_diff_max = 0;
  int auto_diff_min = 0;

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
      frame.swap(prev_frame);
      t_prev_frame = t_frame;
    }
  }

  int current_frame() { return _t; }
  float progress() { return _t / static_cast<float>(length() - 1); }
};

class RecordingWindow : public Recording {
public:
  using trace_t = std::pair<Vec2i, std::vector<float>>;
  std::vector<trace_t> traces;

  Histogram<float, 256> histogram;

  RecordingWindow(filesystem::path path) : Recording(path){
    //traces.push_back({Vec2i(5, 5), {}});
  };

  ~RecordingWindow() {
    if (window) {
      glfwDestroyWindow(window);
      window = nullptr;
    }
  }

  void reset_traces() {
    //for (auto &[pos, trace] : traces) {
    //  trace.clear();
    //}
  }

  void display(float min, float max, float bitrange, bool diff_frames = false,
               float speed = 1) {
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
    glfwSwapBuffers(window);

    //for (auto &[pos, trace] : traces) {
    //  if (diff_frames) {
    //    trace.push_back(frame_diff(pos.x(), pos.y()));
    //  } else {
    //    trace.push_back(frame(pos.x(), pos.y()));
    //  }
    //}

    glfwMakeContextCurrent(main_window);
  }

  void resize_window(float scale = 1) {
    int width = std::ceil(scale * Nx());
    int height = std::ceil(scale * Ny());

    glfwSetWindowSize(window, width, height);
    reshape_callback(window, width, height);
  }

  static void reshape_callback(GLFWwindow *window, int w, int h) {
    std::shared_ptr<RecordingWindow> rec;
    for (auto r : recordings) {
      if (r->window == window) {
        rec = r;
      }
    }

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
    glfwDestroyWindow(window);
  }

  static void key_callback(GLFWwindow *window, int key, int scancode,
                           int action, int mods) {
    if ((key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) && action == GLFW_PRESS) {
      // Don't call RecordingWindow::close_callback() directly here,
      // causes a segfault in glfw
      glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
  }
};