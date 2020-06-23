#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <utility>
#include <variant>

#include "colormap.h"
#include "recording.h"
#include "transformations.h"
#include "videorecorder.h"

struct PlaybackCtrl;
namespace prm {
  extern PlaybackCtrl playbackCtrl;
}  // namespace prm

struct PlaybackCtrl {
  float val = 1;

  void play() {
    if (val == 0) toggle_play_pause();
  }

  void pause() {
    if (val != 0) toggle_play_pause();
  }

  void toggle_play_pause() {
    std::swap(old_val, val);
    if (old_val > 0) val = 0;
  }

 private:
  float old_val = 0;
};

class RecordingPlaybackCtrl {
 private:
  int t_    = 0;
  float tf_ = 0;
  const int length_;

  std::pair<int, float> next_timestep(float speed_) const;

 public:
  RecordingPlaybackCtrl(int length) : length_(length) {}
  // Copies the current playback position
  RecordingPlaybackCtrl &operator=(const RecordingPlaybackCtrl &other);

  int step();
  [[nodiscard]] int current_t() const { return t_; }
  [[nodiscard]] int next_t() const;
  [[nodiscard]] int next_t(int iterations) const;
  [[nodiscard]] float progress() const;
  [[nodiscard]] bool is_last() const;

  void set(int t);
  void set_next(int t);
  void restart();
};

struct ExportCtrl {
  struct {
    bool export_window = false;
    Vec2i start;
    Vec2i size;
    Vec2i frames;
    std::string filename;
    ExportFileType type = ExportFileType::Npy;

    void assign_auto_filename(const fs::path &file_path) {
      auto fn = file_path.stem().string();

      // TODO: This removes some common, unnecessary parts. But this is propably to BMP filerecording specific and should be removed?
      auto parts = split_string(fn, "_"s.c_str());
      if (parts.size() > 4) {
        fn = fmt::format("{}_{}", parts[1], parts[2]);
      }

      if (frames[0] != 0) {
        fn += "_o"s + std::to_string(frames[0]);
      }

      std::string extension;
      if (type == ExportFileType::Binary)
        filename = fmt::format("{}_{}x{}x{}f.dat", fn, size[0], size[1], frames[1] - frames[0]);
      else if (type == ExportFileType::Npy)
        filename = fn + ".npy";
      else
        throw std::logic_error("Unkown ExportFileType type");
    }
  } raw;

  struct {
    bool export_window = false;
    bool recording     = false;
    float progress     = 0;
    std::string filename;
    std::string description;
    VideoRecorder videoRecorder;
    int tstart = 0;
    int tend   = -1;

    void assign_auto_filename(const fs::path &bmp_path) {
      videoRecorder.videotitle = bmp_path.filename().string();
      filename                 = bmp_path.filename().stem().string() + ".mp4";
    }
  } video;

  struct {
    bool export_window = false;
    bool save_pngs     = false;
    std::string filename;

    void assign_auto_filename(const fs::path &bmp_path) {
      filename = bmp_path.filename().stem().string() + "_{t}.png";
    }
  } png;
};

struct Trace {
  std::vector<float> data;
  Vec2i pos;
  Vec4f color;

  void set_pos(const Vec2i &npos);
  void clear() { data.clear(); }
  bool is_near_point(const Vec2i &npos) const;
  static Vec4f next_color();
  static int width(int new_width = 0);
  // get valid starting position and size based on the position of center and max size
  static std::pair<Vec2i, Vec2i> clamp(const Vec2i &pos, const Vec2i &max_size);
};

class FlowData {
 public:
  std::shared_ptr<Recording> data;
  Vec4f color;
  bool show = true;
  static int skip;
  static float pointsize;

  FlowData(std::shared_ptr<Recording> data_, unsigned color_count)
      : data(std::move(data_)), color(next_color(color_count)) {}
  Vec4f next_color(unsigned color_count);
};

class TransformationList {
 private:
  Recording &m_parent;

 public:
  class TransformationCtrl {
    int m_gen;
    std::variant<Transformations, Filters> m_type;
    std::unique_ptr<Transformation::Base> m_transform;

   public:
    TransformationCtrl(std::variant<Transformations, Filters> type, Recording &rec, int _gen = 0);

    int gen() const { return m_gen; }
    std::variant<Transformations, Filters> type() const { return m_type; }
    Transformation::Base *transformation() const { return m_transform.get(); }
  };

  std::vector<TransformationCtrl> transformations;

  TransformationList(Recording &rec);
  void reallocate();
  Transformation::Base *create_if_needed(std::variant<Transformations, Filters> type, int gen = 0);
};

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