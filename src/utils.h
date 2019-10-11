#pragma once

#include <GLFW/glfw3.h>
#include <array>

#include "definitions.h"
#include "vectors.h"

template <typename T, size_t bin_count> class Histogram {
public:
  std::array<T, bin_count> data = {};

  T min = {};
  T max = {};

  Histogram() = default;
  Histogram(T _min, T _max) : min(_min), max(_max){};

  template <typename Container> void compute(const Container &container) {
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

template <typename T>
void draw2dArray(const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> &arr,
                 float min, float max) {
  const int Nx = arr.rows();
  const int Ny = arr.cols();

  for (int x = 0; x < Nx; x++) {
    for (int y = 0; y < Ny; y++) {
      const Vec2i pos1 = {x, Ny - y};
      const Vec2i pos2 = {x + 1, Ny - y};
      const Vec2i pos3 = {x + 1, Ny - (y + 1)};
      const Vec2i pos4 = {x, Ny - (y + 1)};

      const auto val = arr(x, y);
      const auto c = val_to_color<T>(val, min, max);

      glBegin(GL_QUADS);
      glColor3fv(c.data());
      glVertex2iv(pos1.data());
      glVertex2iv(pos2.data());
      glVertex2iv(pos3.data());
      glVertex2iv(pos4.data());
      glEnd();
    }
  }
}

void drawPixel(int x, int y, int Ny, int dx, const Vec4f &color) {
  y -= dx / 2;
  x -= dx / 2;

  const Vec2i pos1 = {x, Ny - y};
  const Vec2i pos2 = {x + dx, Ny - y};
  const Vec2i pos3 = {x + dx, Ny - (y + dx)};
  const Vec2i pos4 = {x, Ny - (y + dx)};

  glBegin(GL_LINE_LOOP);
  glColor4fv(color.data());
  glVertex2iv(pos1.data());
  glVertex2iv(pos2.data());
  glVertex2iv(pos3.data());
  glVertex2iv(pos4.data());
  glEnd();
}

static void glfw_error_callback(int error, const char *description) {
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

std::vector<std::string_view> split_string(std::string_view input,
                                           std::string_view delims = " ") {
  std::vector<std::string_view> output;
  size_t first = 0;

  while (first < input.size()) {
    const auto second = input.find_first_of(delims, first);

    if (first != second)
      output.emplace_back(input.substr(first, second - first));

    if (second == std::string_view::npos)
      break;

    first = second + 1;
  }

  return output;
}