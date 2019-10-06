#pragma once

#include <GLFW/glfw3.h>
#include <array>

#include "definitions.h"

template <typename T, size_t bin_count> class Histogram {
public:
  std::array<T, bin_count> data = {};

  Histogram() = default;

  template <typename Container>
  void compute(const Container &container, T min, T max) {
    data.fill(0);

    const unsigned int bin_size = (max - min + 1) / bin_count;

    for (auto val : container) {

      if (val > max) {
        val = max;
      } else if (val < min) {
        val = min;
      }

      const unsigned int index = (val - min) / bin_size;
      data[index] += 1;
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
Vec3f val_to_color(const T &val, const T &min, const T &max) {
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
  auto image_width = arr.rows();
  auto image_height = arr.cols();

  for (long x = 0; x < image_width; x++) {
    for (long y = 0; y < image_height; y++) {
      const Vec2f pos1 = Vec2f(y, image_width - x);
      const Vec2f pos2 = Vec2f(y + 1, image_width - x);
      const Vec2f pos3 = Vec2f(y + 1, image_width - (x + 1));
      const Vec2f pos4 = Vec2f(y, image_width - (x + 1));

      auto val = arr(x, y);
      auto c = val_to_color<T>(val, min, max);

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

static void glfw_error_callback(int error, const char *description) {
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}