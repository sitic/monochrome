#pragma once

#include <GLFW/glfw3.h>
#include <utility>
#include <variant>

#include "recordingwindow_helpers.h"
#include "utils/colormap.h"

class RecordingWindow;
using SharedRecordingPtr = std::shared_ptr<RecordingWindow>;

enum class OverlayMethod : int { Linear, Diff, Positive, Negative };
inline const char *OverlayMethodNames[4] = {"Linear", "Diff", "Positive", "Negative"};

class RecordingWindow : public Recording {
 public:
  static inline float scale_fct = 1;

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
  std::vector<FlowData> flows;                                      // use add_flow() to add members
  std::vector<std::shared_ptr<global::PointsVideo>> points_videos;  // use add_points_video() to add
  bool as_overlay    = false;
  int overlay_method = 3;
  struct {
    bool show = false;
    std::string comment;
  } comment_edit_ui;

  RecordingWindow(const fs::path &path) : RecordingWindow(file_factory(path)){};
  RecordingWindow(std::shared_ptr<AbstractFile> file_);

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

  virtual float &get_min(Transformations type) {
    return transformationArena.create_if_needed(type)->min;
  }
  virtual float &get_max(Transformations type) {
    return transformationArena.create_if_needed(type)->max;
  }

  void reset_traces();
  void add_trace(const Vec2i &pos);
  void remove_trace(const Vec2i &pos);
  void save_trace(const Vec2i &pos, fs::path ouput_txt_path, Vec2i t0tmax = {0, 0});

  static void flip_lr() { rotations.flip_ud(); }
  static void flip_ud() { rotations.flip_lr(); }
  static void flip_reset() { rotations.flip_reset(); }
  static void set_rotation(short rotation);
  static void add_rotation(short d_rotation);

  void colormap(ColorMap cmap);
  ColorMap colormap() const { return cmap_; }

  void set_name(const std::string &new_name) override;
  void resize_window();
  fs::path save_snapshot(std::string output_png_path_template = "");
  void start_recording(const fs::path &filename, int fps = 30, std::string description = "");
  void stop_recording();

  void add_flow(std::shared_ptr<Recording> flow);
  void add_points_video(std::shared_ptr<global::PointsVideo> pv);

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
    bool holding_left  = false;
    bool pressing_left = false;
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
  Shader points_shader;
  GLuint points_vao, points_vbo;
  std::vector<float> points_vert;
};

class FixedTransformRecordingWindow : public RecordingWindow {
  Filters fixed_prefilter_;
  Transformations fixed_transformation_;
  Filters fixed_postfilter_;

 public:
  FixedTransformRecordingWindow(SharedRecordingPtr parent,
                                Filters prefilter,
                                Transformations transformation,
                                Filters postfilter,
                                std::string name);
  void display(Filters prefilter, Transformations transformation, Filters postfilter) override {
    RecordingWindow::display(fixed_prefilter_, fixed_transformation_, fixed_postfilter_);
  };
  float &get_min(Transformations type) override {
    return RecordingWindow::get_min(fixed_transformation_);
  }
  float &get_max(Transformations type) override {
    return RecordingWindow::get_max(fixed_transformation_);
  }
};