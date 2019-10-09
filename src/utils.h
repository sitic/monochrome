#pragma once

#include <GLFW/glfw3.h>
#include <array>

#include "definitions.h"

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
std::array<T, 3> val_to_color(const T &val, const T &min, const T &max) {
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
  int image_width = arr.rows();
  int image_height = arr.cols();

  using Vec2 = std::array<int, 2>;
  Vec2 pos1, pos2, pos3, pos4;

  for (int x = 0; x < image_width; x++) {
    for (int y = 0; y < image_height; y++) {
      pos1 = {y, image_width - x};
      pos2 = {y + 1, image_width - x};
      pos3 = {y + 1, image_width - (x + 1)};
      pos4 = {y, image_width - (x + 1)};

      auto val = arr(x, y);
      auto c = val_to_color<T>(val, min, max);

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

void drawPixel(int x, int y, int w, int dx, const std::array<float, 4> &color) {
  y -= dx/2;
  x -= dx/2;

  using Vec2 = std::array<int, 2>;
  Vec2 pos1, pos2, pos3, pos4;
  pos1 = {y, w - x};
  pos2 = {y + dx, w - x};
  pos3 = {y + dx, w - (x + dx)};
  pos4 = {y, w - (x + dx)};

  glBegin(GL_QUADS);
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