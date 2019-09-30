#pragma once

#include <array>
#include <GLFW/glfw3.h>

#include "definitions.h"

template <typename T, size_t bin_count> class Histogram {
public:
  std::array<T, bin_count> data = {};

  pixel _max;

  Histogram(pixel max) : _max(max){};

  template <typename Container> void compute(const Container &container) {
    data.fill(0);

    pixel bin_size = (_max + 1) / bin_count;

    for (auto val : container) {
      if (val > _max) {
        val = _max;
      }

      const unsigned int index = val / bin_size;
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

void pixel_normalize(PixArray&arr, PixArray& minmax) {

}

void draw2dArray(const PixArray &arr, const Vec2f &position, float scale,
                 pixel min, pixel max) {
  auto image_width = arr.rows();
  auto image_height = arr.cols();

  Vec2f pos1, pos2, pos3, pos4;
  for (long x = 0; x < image_width; x++) {
    for (long y = 0; y < image_height; y++) {
      pos1 = Vec2f(y, image_width - x) * scale + position;
      pos2 = Vec2f(y + 1, image_width - x) * scale + position;
      pos3 = Vec2f(y + 1, image_width - (x + 1)) * scale + position;
      pos4 = Vec2f(y, image_width - (x + 1)) * scale + position;

      auto val = arr(x, y);
      auto c = val_to_color(val, min, max);

      //if (std::isnan(val)) {
      //  c = {1, 0, 0};
      //}

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