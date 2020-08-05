#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <utility>
#include <variant>

#include "recordingwindow_helpers.h"
#include "colormap.h"

class RecordingWindow : public Recording {
 public:
  static float scale_fct;

  GLFWwindow *window    = nullptr;
  GLFWwindow *glcontext = nullptr;
  bool active           = true;
  std::vector<std::shared_ptr<RecordingWindow>> children;

  RecordingPlaybackCtrl playback;
  Histogram<float, 256> histogram;
  BitRange bitrange = BitRange::U16;
  ExportCtrl export_ctrl;
  std::vector<Trace> traces;
  TransformationList transformationArena;
  std::vector<FlowData> flows;  // use add_flow() to add members
  bool use_transfer_fct    = false;
  int transfer_fct_version = 0;

  RecordingWindow(const fs::path &path) : RecordingWindow(autoguess_filetype(path)){};
  RecordingWindow(std::shared_ptr<AbstractRecording> file_);

  virtual ~RecordingWindow() {
    if (window != nullptr) {
      clear_gl_memory();
      glfwDestroyWindow(window);
    }
  }

  void open_window();
  void set_context(GLFWwindow *new_context);

  virtual void display(Filters prefilter, Transformations transformation, Filters postfilter);
  virtual void render();

  virtual void load_next_frame() { load_frame(playback.step()); }

  float &get_min(Transformations type) { return transformationArena.create_if_needed(type)->min; }
  float &get_max(Transformations type) { return transformationArena.create_if_needed(type)->max; }

  void reset_traces();
  void add_trace(const Vec2i &pos);
  void remove_trace(const Vec2i &pos);

  static void fliplr() { rotations.flipud(); }
  static void flipud() { rotations.fliplr(); }
  static void set_rotation(short rotation);
  static void add_rotation(short d_rotation);

  void colormap(ColorMap cmap);
  ColorMap colormap() const { return cmap_; }

  void resize_window();
  fs::path save_snapshot(std::string output_png_path_template = "");
  void start_recording(const std::string &filename, int fps = 30, std::string description = "");
  void stop_recording();

  void add_flow(std::shared_ptr<Recording> flow);

  static void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
  static void cursor_position_callback(GLFWwindow *window, double xpos, double ypos);
  static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
  static void reshape_callback(GLFWwindow *window, int w, int h);
  static void close_callback(GLFWwindow *window);
  static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

 protected:
  void rotation_was_changed();
  void update_gl_texture();
  void clear_gl_memory();

  Vec2d mousepos;
  struct {
    bool left  = false;
    bool right = false;
  } mousebutton;

  ColorMap cmap_      = ColorMap::GRAY;
  GLuint texture      = GL_FALSE;
  GLuint ctexture     = GL_FALSE;
  GLuint ctexturediff = GL_FALSE;
  Shader frame_shader;
  GLuint frame_vao, frame_vbo, frame_ebo;
  Shader trace_shader;
  GLuint trace_vao, trace_vbo;
  std::vector<float> trace_vert;
  Shader flow_shader;
  GLuint flow_vao, flow_vbo;
  std::vector<float> flow_vert;
};

using SharedRecordingPtr = std::shared_ptr<RecordingWindow>;