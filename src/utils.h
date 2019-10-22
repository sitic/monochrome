#pragma once

#include <GLFW/glfw3.h>
#include <array>
#include <lodepng/lodepng.h>

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

class Message {
public:
  bool show = true;
  std::string msg;
  int id = 0;

  Message(const std::string &msg) : msg(msg) {
    static int _id = -1;
    _id += 1;
    id = _id;
  };
};

extern std::vector<Message> messages;
template <typename... Args>
inline void new_ui_message(const char *fmt, Args &&... args) {
  const std::string msg = fmt::format(fmt, std::forward<Args>(args)...);
  messages.emplace_back(msg);
  fmt::print(msg);
}

template <typename... Args>
inline void new_ui_message(const std::string &fmt, Args &&... args) {
  return new_ui_message(fmt.c_str(), std::forward<Args>(args)...);
}

void save_snapshot(const std::string &out_png_path,
                   GLFWwindow *window = nullptr, bool alpha_channel = false) {
  auto prev_context = glfwGetCurrentContext();
  if (window)
    glfwMakeContextCurrent(window);

  int width, height;
  glfwGetWindowSize(window, &width, &height);

  GLenum gl_px_format = alpha_channel ? GL_RGBA : GL_RGB;
  const unsigned pix_byte_count = alpha_channel ? 4 : 3;
  LodePNGColorType png_px_format =
      alpha_channel ? LodePNGColorType::LCT_RGBA : LodePNGColorType::LCT_RGB;

  std::vector<GLubyte> image(width * height * pix_byte_count);
  glReadPixels(0, 0, width, height, gl_px_format, GL_UNSIGNED_BYTE,
               image.data());

  // flip y
  for (int i = 0; i < height / 2; i++) {
    std::swap_ranges(image.begin() + i * width * pix_byte_count,
                     image.begin() + (i + 1) * width * pix_byte_count,
                     image.begin() + (height - i - 1) * width * pix_byte_count);
  }

  // Save as png
  unsigned error =
      lodepng::encode(out_png_path, image, width, height, png_px_format);

  if (error) {
    new_ui_message("snapshot png encoder error {}: {}", error,
                   lodepng_error_text(error));
  }

  glfwMakeContextCurrent(prev_context);
}