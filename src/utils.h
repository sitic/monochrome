#pragma once

#include <array>
#include <Eigen/Dense>
#include <GLFW/glfw3.h>

#include "vectors.h"

#if defined(unix) || defined(__unix__) || defined(__unix)
std::string get_user_homedir();
#endif

template <typename T, size_t bin_count>
class Histogram {
 public:
  std::array<T, bin_count> data = {};

  T min = {};
  T max = {};

  Histogram() = default;
  Histogram(T _min, T _max) : min(_min), max(_max){};

  template <typename Container>
  void compute(const Container &container) {
    data.fill(0);

    for (auto val : container) {

      if (val > max) {
        val = max;
      } else if (val < min) {
        val = min;
      }

      const unsigned int index = (bin_count - 1) * (val - min) / (max - min);
      data.at(index) += 1;
    }
  }

  T max_value() {
    auto max_element = std::max_element(data.begin(), data.end());
    return *max_element;
  }

  void normalize() {
    // normalize such that the maximal bin equals 1.0
    auto m = max_value();
    for (size_t i = 0; i < bin_count; i++) {
      data[i] /= m;
    }
  }
};

std::vector<std::string_view> split_string(std::string_view input, std::string_view delims = " ");

template <typename T>
Vector3<T> val_to_color(const T &val, const T &min, const T &max) {
  float x = static_cast<float>(val - min) / static_cast<float>(max - min);

  if (x < 0) {
    x = 0;
  } else if (x > 1) {
    x = 1;
  }

  return {x, x, x};
}

std::vector<GLint> generate_quad_vert(int Nx, int Ny);

void draw2dArray(const Eigen::MatrixXf &arr,
                 const std::vector<GLint> &vert,
                 std::vector<GLfloat> &buffer,
                 float min,
                 float max);

void drawPixel(int x, int y, int Ny, int dx, const Vec4f &color);

void gl_save_snapshot(const std::string &out_png_path,
                      GLFWwindow *window = nullptr,
                      bool alpha_channel = false);

void add_window_icon(GLFWwindow *window);

void glfw_error_callback(int error, const char *description);