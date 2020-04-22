#include <lodepng/lodepng.h>

#include "fonts/MultiRecorderVideo.h"

#include "utils.h"
#include "globals.h"

#if defined(__unix__) || defined(__unix) || defined(__APPLE__)
#include <unistd.h>
#include <pwd.h>
std::string get_user_homedir() {
  const char* homedir;

  if ((homedir = getenv("HOME")) == nullptr) {
    homedir = getpwuid(getuid())->pw_dir;
  }
  return homedir;
}
#endif

std::string config_file_path() {
#ifdef _WIN32
  return "%APPDATA%\\quickVidViewer\\quickVidViewer.ini";
#elif defined(__unix__) || defined(__unix) || defined(__APPLE__)
  return fmt::format("{}/.config/quickVidViewer.ini", get_user_homedir());
#else
  return "";
#endif
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

std::vector<GLint> generate_quad_vert(int Nx, int Ny) {
  std::vector<GLint> vert(Nx * Ny * 8);
  for (int x = 0; x < Nx; x++) {
    for (int y = 0; y < Ny; y++) {
      vert[x * Ny * 8 + y * 8 + 0] = x;
      vert[x * Ny * 8 + y * 8 + 1] = Ny - y;
      vert[x * Ny * 8 + y * 8 + 2] = x + 1;
      vert[x * Ny * 8 + y * 8 + 3] = Ny - y;
      vert[x * Ny * 8 + y * 8 + 4] = x + 1;
      vert[x * Ny * 8 + y * 8 + 5] = Ny - (y + 1);
      vert[x * Ny * 8 + y * 8 + 6] = x;
      vert[x * Ny * 8 + y * 8 + 7] = Ny - (y + 1);
    }
  }
  return vert;
}


void draw2dArray(const Eigen::MatrixXf& arr,
                 const std::vector<GLint>& vert,
                 std::vector<GLfloat>& buffer,
                 float min,
                 float max) {
  const int Nx = arr.rows();
  const int Ny = arr.cols();

  const std::size_t buffer_size = Nx * Ny * 4 * 3;
  if (buffer.size() != buffer_size) {
    buffer.resize(buffer_size);
  }

  for (int x = 0; x < Nx; x++) {
    for (int y = 0; y < Ny; y++) {
      const auto val = arr(x, y);
      const auto c   = val_to_color<float>(val, min, max);
      for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 3; j++) {
          buffer[x * Ny * 4 * 3 + y * 4 * 3 + i * 3 + j] = c[j];
        }
      }
    }
  }

  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(2, GL_INT, 0, vert.data());
  glEnableClientState(GL_COLOR_ARRAY);
  glColorPointer(3, GL_FLOAT, 0, buffer.data());
  glDrawArrays(GL_QUADS, 0, Nx * Ny * 4);
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);
}

void drawPixel(int x, int y, int Ny, int dx, const Vec4f& color) {
  y -= dx / 2;
  x -= dx / 2;

  /* clang-format off */
  std::array<GLint, 4*2> vert = {{
    x, Ny - y,
    x + dx, Ny - y,
    x + dx, Ny - (y + dx),
    x, Ny - (y + dx)
  }};
  /* clang-format on */

  glLineWidth(2);
  glColor4fv(color.data());
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(2, GL_INT, 0, vert.data());
  glDrawArrays(GL_LINE_LOOP, 0, 4);
  glDisableClientState(GL_VERTEX_ARRAY);
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
    global::new_ui_message("snapshot png encoder error {}: {}", error, lodepng_error_text(error));
  }

  glfwMakeContextCurrent(prev_context);
}

void add_window_icon(GLFWwindow* window) {
  unsigned icon_width, icon_height;
  std::vector<unsigned char> icon_image;
  auto error = lodepng::decode(icon_image, icon_width, icon_height,
                               reinterpret_cast<const unsigned char*>(icons::multirecvideopng_data),
                               icons::multirecvideopng_size);
  if (error) fmt::print("lodepng error {}: {}\n", error, lodepng_error_text(error));
  GLFWimage glfwicon = {static_cast<int>(icon_width), static_cast<int>(icon_height),
                        icon_image.data()};
  glfwSetWindowIcon(window, 1, &glfwicon);
}

void glfw_error_callback(int error, const char* description) {
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}
