#pragma once

#include <array>
#include <Eigen/Dense>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"

#include "vectors.h"

void gl_save_snapshot(std::string out_png_path, GLFWwindow *window = nullptr);

void add_window_icon(GLFWwindow *window);

void glfw_error_callback(int error, const char *description);

void checkGlError(std::string desc = "");

std::string get_rc_text_file(const std::string &filename);

template <typename T, size_t bin_count>
class Histogram {
 public:
  std::array<T, bin_count> data = {};

  T min          = {};
  T max          = {};
  bool symmetric = false;

  Histogram() = default;
  Histogram(T _min, T _max) : min(_min), max(_max){};

  template <typename Container>
  void compute(const Container &container) {
    data.fill(0);

    for (auto val : container) {
      if (std::isnan(val)) {
        continue;
      }

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

class Shader {
 public:
  unsigned int ID                   = GL_FALSE;
  Shader()                          = default;
  Shader(const Shader &)            = delete;
  Shader &operator=(Shader const &) = delete;
  Shader(Shader &&other) noexcept { ID = other.ID; }
  Shader &operator=(Shader &&other) noexcept {
    ID = other.ID;
    return *this;
  }

  static Shader create(const std::string &vertexCode,
                       const std::string &fragmentCode,
                       const std::string &geometryCode = "") {
    Shader shader;
    shader.init(vertexCode, fragmentCode, geometryCode);
    return shader;
  }
  void init(const std::string &vertexCode,
            const std::string &fragmentCode,
            const std::string &geometryCode = "");

  void use() const { glUseProgram(ID); }

  void remove() {
    glDeleteProgram(ID);
    ID = GL_FALSE;
  }

  explicit operator bool() const { return ID != GL_FALSE; }

  void setBool(const std::string &name, bool value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
  }
  void setInt(const std::string &name, int value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
  }
  void setFloat(const std::string &name, float value) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
  }
  void setVec2(const std::string &name, const Vec2f &value) const {
    glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, value.data());
  }
  void setVec2(const std::string &name, float x, float y) const {
    glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
  }
  void setVec3(const std::string &name, const Vec3f &value) const {
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, value.data());
  }
  void setVec3(const std::string &name, float x, float y, float z) const {
    glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
  }
  void setVec4(const std::string &name, const Vec4f &value) const {
    glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, value.data());
  }
  void setVec4(const std::string &name, float x, float y, float z, float w) const {
    glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
  }

 private:
  static void checkCompileErrors(GLuint shader, const std::string &type);
};