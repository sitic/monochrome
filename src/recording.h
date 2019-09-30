#pragma once

#include <GLFW/glfw3.h>
#include <string>

#include "bmp.h"

extern GLFWwindow *main_window;

class Recording {
  filesystem::path _path;

  BMPheader fileheader;
  int _t = 0;
  float _tf = 0;

public:
  PixArray frame;
  Pix2Array minmax;
  GLFWwindow *window = nullptr;

  Recording(filesystem::path path)
      : _path(path), fileheader(path), frame(fileheader.Nx(), fileheader.Ny()),
        minmax(fileheader.Nx(), fileheader.Ny()) {
    load_frame(0);
  }

  bool good() { return fileheader.good(); }
  int Nx() { return frame.cols(); }
  int Ny() { return frame.rows(); }
  int length() { return fileheader.length(); }
  filesystem::path path() { return _path; }
  std::string date() { return fileheader.date(); };
  std::string comment() { return fileheader.comment(); };

  void load_frame(long t) { fileheader.read_frame(t, frame.data()); }

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
    recordings_window_close_callback(window);
  }
}