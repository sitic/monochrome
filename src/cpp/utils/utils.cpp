#include <fstream>

#include <lodepng/lodepng.h>
#include <cmrc/cmrc.hpp>

#include "utils.h"
#include "globals.h"

CMRC_DECLARE(rc);

void gl_save_snapshot(std::string out_png_path, GLFWwindow* window) {
  auto prev_context = glfwGetCurrentContext();
  if (window) glfwMakeContextCurrent(window);

  int width, height;
  glfwGetFramebufferSize(window, &width, &height);

  const unsigned channels = 4;  // RGB: 3, RGBA: 4
  std::vector<GLubyte> image(width * height * channels);

  //glPixelStorei(GL_PACK_ALIGNMENT, 1); // needed when using GL_RGB
  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, image.data());

  // flip y
  for (int i = 0; i < height / 2; i++) {
    std::swap_ranges(image.begin() + i * width * channels,
                     image.begin() + (i + 1) * width * channels,
                     image.begin() + (height - i - 1) * width * channels);
  }

  // Save as png
  unsigned error = lodepng::encode(out_png_path, image, width, height, LodePNGColorType::LCT_RGBA);

  if (error) {
    global::new_ui_message("snapshot png encoder error {}: {}", error, lodepng_error_text(error));
  }

  glfwMakeContextCurrent(prev_context);
}

void add_window_icon(GLFWwindow* window) {
  auto fs       = cmrc::rc::get_filesystem();
  auto icondata = fs.open("assets/Monochrome_256x256.png");
  unsigned icon_width, icon_height;
  std::vector<unsigned char> icon_image;
  auto error = lodepng::decode(icon_image, icon_width, icon_height,
                               std::vector<unsigned char>(icondata.begin(), icondata.end()));
  if (error) {
    fmt::print("lodepng error {}: {}\n", error, lodepng_error_text(error));
    return;
  }
  GLFWimage glfwicon = {static_cast<int>(icon_width), static_cast<int>(icon_height),
                        icon_image.data()};
  glfwSetWindowIcon(window, 1, &glfwicon);
}

void glfw_error_callback(int error, const char* description) {
  if (std::string(description) == "Cocoa: Regular windows do not have icons on macOS") {
    return;
  }
  fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

void checkGlError(std::string desc) {
  if (GLenum e = glGetError(); e) fmt::print("opengl error on {}: {}\n", desc, e);
}

void Shader::init(const std::string& vertexCode,
                  const std::string& fragmentCode,
                  const std::string& geometryCode) {
  // compile shaders
  GLuint vertex, fragment, geometry;
  // vertex shader
  vertex                  = glCreateShader(GL_VERTEX_SHADER);
  const char* vShaderCode = vertexCode.c_str();
  glShaderSource(vertex, 1, &vShaderCode, nullptr);
  glCompileShader(vertex);
  checkCompileErrors(vertex, "VERTEX");
  // fragment Shader
  fragment                = glCreateShader(GL_FRAGMENT_SHADER);
  const char* fShaderCode = fragmentCode.c_str();
  glShaderSource(fragment, 1, &fShaderCode, nullptr);
  glCompileShader(fragment);
  checkCompileErrors(fragment, "FRAGMENT");
  // if geometry shader is given, compile geometry shader
  if (!geometryCode.empty()) {
    const char* gShaderCode = geometryCode.c_str();
    geometry                = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(geometry, 1, &gShaderCode, nullptr);
    glCompileShader(geometry);
    checkCompileErrors(geometry, "GEOMETRY");
  }
  // shader Program
  ID = glCreateProgram();
  glAttachShader(ID, vertex);
  glAttachShader(ID, fragment);
  if (!geometryCode.empty()) glAttachShader(ID, geometry);
  glLinkProgram(ID);
  checkCompileErrors(ID, "PROGRAM");
  // delete the shaders as they're linked into our program now and no longer necessery
  glDeleteShader(vertex);
  glDeleteShader(fragment);
  if (!geometryCode.empty()) glDeleteShader(geometry);
}

void Shader::checkCompileErrors(GLuint shader, const std::string& type) {
  GLint success;
  GLchar infoLog[1024];
  if (type != "PROGRAM") {
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
      fmt::print("ERROR::SHADER_COMPILATION_ERROR of type: {}\n", type);
      fmt::print("{}\n\n -- --------------------------------------------------- --\n", infoLog);
    }
  } else {
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if (!success) {
      glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
      fmt::print("ERROR::PROGRAM_LINKING_ERROR of type: {}\n", type);
      fmt::print("{}\n\n -- --------------------------------------------------- --\n", infoLog);
    }
  }
}

std::string get_rc_text_file(const std::string& filename) {
  auto fs   = cmrc::rc::get_filesystem();
  auto file = fs.open(filename);
  return std::string(file.begin(), file.end());
}
