#pragma once

#include <string>

#include <Eigen/Dense>
#include <GLFW/glfw3.h>

#include "bmp.h"
#include "utils.h"
#include "vectors.h"

using namespace std::string_literals;

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
  int Nx() { return fileheader.Nx(); }
  int Ny() { return fileheader.Ny(); }
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

  void export_ROI(filesystem::path path, Vec2i start, Vec2i size, Vec2i t0tmax) {
    if (t0tmax[0] < 0 || t0tmax[1] > length() || t0tmax[0] > t0tmax[1]) {
      fmt::print("ERROR: start or end frame invalid");
      return;
    }

    filesystem::remove(path);
    std::ofstream out(path.string(), std::ios::out | std::ios::binary);
    auto cur_frame = t_frame;

    std::size_t framesize = size[0] * Ny() * sizeof(float);
    for (int t = t0tmax[0]; t < t0tmax[1]; t++) {
      load_frame(t);
      auto block = frame.block(start[0], start[1], size[0], size[1]);
      out.write(reinterpret_cast<const char *>(block.data()),
                block.size() * sizeof(float));
    }
    load_frame(cur_frame);
  }
};

class RecordingWindow : public Recording {
public:
  GLFWwindow *window = nullptr;

  struct {
    bool export_window = false;
    Vec2i start;
    Vec2i size;
    int length = 0;
    std::vector<char> filename = {};

    void assign_auto_filename(const filesystem::path &bmp_path) {
      auto fn = bmp_path.filename().string();
      auto parts = split_string(fn, "_"s.c_str());
      if (parts.size() > 4) {
        fn = fmt::format("{}_{}", parts[1], parts[2]);
      }

      fn = fmt::format("{}_{}x{}x{}f.dat", fn, size[0], size[1], length);

      filename.assign(fn.begin(), fn.end());
    }
  } export_ctrl;

  Histogram<float, 256> histogram;

  struct Trace {
    std::vector<float> data;
    Vec2i pos;
    Vec4f color;

    static Vec4f next_color() {
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
  std::vector<Trace> traces;

  RecordingWindow(filesystem::path path) : Recording(path) {
    if (good() && Trace::width() == 0) {
      // if unset, set trace edge length to something reasonable
      auto min = std::min(Nx(), Ny());
      Trace::width(min / 64);
    }
  };

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
    Vec2i npos = {x, y};

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

  void remove_trace_pos(int x, int y) {
    Vec2i pos = {x, y};
    traces.erase(
        std::remove_if(traces.begin(), traces.end(), [pos](const auto &trace) {
          auto d = pos - trace.pos;
          return (std::abs(d[0]) < Trace::width() / 2 &&
                  std::abs(d[1]) < Trace::width() / 2);
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
      histogram.min = -bitrange / 10.f;
      histogram.max = bitrange / 10.f;
    } else {
      histogram.min = 0;
      histogram.max = bitrange;
    }

    auto &arr = diff_frames ? frame_diff : frame;
    draw2dArray(arr, min, max);
    histogram.compute(arr.reshaped());

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

      auto block = arr.block(start[0], start[1], w, w);
      trace.push_back(block.mean());
      drawPixel(pos[0], pos[1], Ny(), w, color);
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
    glfwSetScrollCallback(window, RecordingWindow::scroll_callback);
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

  static void scroll_callback(GLFWwindow *window, double xoffset,
                              double yoffset) {
    auto w = Trace::width();
    int new_w = (yoffset < 0) ? 0.95f * w : 1.05f * w;
    if (new_w == w) {
      new_w = (yoffset < 0) ? w - 1 : w + 1;
    }
    Trace::width(new_w);
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

      fmt::print("Click on pos ({}, {}), calc index ({}, {})\n",
                 rec->mousepos[0], rec->mousepos[1], x, y);

      rec->add_trace_pos(x, y);
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
        rec->remove_trace_pos(rec->mousepos[0], rec->mousepos[1]);
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
    }
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