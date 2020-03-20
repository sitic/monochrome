#include <lodepng/lodepng.h>

#include "utils.h"
#include "fonts/MultiRecorderVideo.h"

#if defined(unix) || defined(__unix__) || defined(__unix)
#include <unistd.h>
#include <pwd.h>
std::string get_user_homedir() {
  const char *homedir;

  if ((homedir = getenv("HOME")) == nullptr) {
    homedir = getpwuid(getuid())->pw_dir;
  }
  return homedir;
}
#endif

void drawPixel(int x, int y, int Ny, int dx, const Vec4f& color) {
  y -= dx / 2;
  x -= dx / 2;

  const Vec2i pos1 = {x, Ny - y};
  const Vec2i pos2 = {x + dx, Ny - y};
  const Vec2i pos3 = {x + dx, Ny - (y + dx)};
  const Vec2i pos4 = {x, Ny - (y + dx)};

  glLineWidth(2);
  glBegin(GL_LINE_LOOP);
  glColor4fv(color.data());
  glVertex2iv(pos1.data());
  glVertex2iv(pos2.data());
  glVertex2iv(pos3.data());
  glVertex2iv(pos4.data());
  glEnd();
}

std::vector<std::string_view> split_string(std::string_view input, std::string_view delims) {
  std::vector<std::string_view> output;
  size_t first = 0;

  while (first < input.size()) {
    const auto second = input.find_first_of(delims, first);

    if (first != second) output.emplace_back(input.substr(first, second - first));

    if (second == std::string_view::npos) break;

    first = second + 1;
  }

  return output;
}

void gl_save_snapshot(const std::string& out_png_path, GLFWwindow* window, bool alpha_channel) {
  auto prev_context = glfwGetCurrentContext();
  if (window) glfwMakeContextCurrent(window);

  int width, height;
  glfwGetWindowSize(window, &width, &height);

  GLenum gl_px_format           = alpha_channel ? GL_RGBA : GL_RGB;
  const unsigned pix_byte_count = alpha_channel ? 4 : 3;
  LodePNGColorType png_px_format =
      alpha_channel ? LodePNGColorType::LCT_RGBA : LodePNGColorType::LCT_RGB;

  std::vector<GLubyte> image(width * height * pix_byte_count);
  glReadPixels(0, 0, width, height, gl_px_format, GL_UNSIGNED_BYTE, image.data());

  // flip y
  for (int i = 0; i < height / 2; i++) {
    std::swap_ranges(image.begin() + i * width * pix_byte_count,
                     image.begin() + (i + 1) * width * pix_byte_count,
                     image.begin() + (height - i - 1) * width * pix_byte_count);
  }

  // Save as png
  unsigned error = lodepng::encode(out_png_path, image, width, height, png_px_format);

  if (error) {
    new_ui_message("snapshot png encoder error {}: {}", error, lodepng_error_text(error));
  }

  glfwMakeContextCurrent(prev_context);
}

void add_window_icon(GLFWwindow* window) {
  unsigned icon_width, icon_height;
  std::vector<unsigned char> icon_image;
  unsigned error =
      lodepng::decode(icon_image, icon_width, icon_height,
                      reinterpret_cast<const unsigned char *>(icons::multirecvideopng_data),
                      icons::multirecvideopng_size);
  if (error) fmt::print("lodepng error {}: {}\n", error, lodepng_error_text(error));
  GLFWimage glfwicon = {static_cast<int>(icon_width), static_cast<int>(icon_height),
                        icon_image.data()};
  glfwSetWindowIcon(window, 1, &glfwicon);
}
