#pragma once

#include <string>

#include <Eigen/Dense>
#include <GLFW/glfw3.h>

#include "bmp.h"

extern GLFWwindow *main_window;

class Recording {
  filesystem::path _path;

  BMPheader fileheader;
  int _t = 0;
  float _tf = 0;

  Eigen::Matrix<uint16, Eigen::Dynamic, Eigen::Dynamic> raw_frame;

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

    // load a frame from the middle to calculate auto min/max
    load_frame(length() / 2);
    auto_max = frame.maxCoeff();
    auto_min = frame.minCoeff();

    prev_frame = frame;
    load_frame(length() / 2 + 1);
    frame_diff = frame - prev_frame;

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

  int current_frame() { return _t; }
  float progress() { return _t / static_cast<float>(length() - 1); }
};

extern std::vector<std::shared_ptr<Recording>> recordings;

void recordings_window_close_callback(GLFWwindow *window) {
  recordings.erase(
      std::remove_if(recordings.begin(), recordings.end(),
                     [window](auto r) { return r->window == window; }),
      recordings.end());
  glfwDestroyWindow(window);
}

void recording_window_callback(GLFWwindow *window, int key, int scancode,
                               int action, int mods) {
  if ((key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) && action == GLFW_PRESS) {
    // Don't call recordings_window_close_callback() directly here,
    // causes a segfault in glfw
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  }
}