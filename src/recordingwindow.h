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
  RecordingPlaybackCtrl &operator=(const RecordingPlaybackCtrl &other) {
    if (length_ < other.length_) {
      throw std::runtime_error("Error: videos of unequal length");
    }
    t_  = other.t_;
    tf_ = other.tf_;
    return *this;
  };

  int step();
  [[nodiscard]] int current_t() const { return t_; }
  [[nodiscard]] int next_t() const;
  [[nodiscard]] int next_t(int iterations) const;
  [[nodiscard]] float progress() const;

  void set(int t);
  void restart();
};

struct ExportCtrl {
  struct {
    bool export_window = false;
    Vec2i start;
    Vec2i size;
    Vec2i frames;
    std::vector<char> filename = {};

    void assign_auto_filename(const fs::path &bmp_path) {
      auto fn    = bmp_path.filename().string();
      auto parts = split_string(fn, "_"s.c_str());
      if (parts.size() > 4) {
        fn = fmt::format("{}_{}", parts[1], parts[2]);
      }

      if (frames[0] != 0) {
        fn += "_o"s + std::to_string(frames[0]);
      }

      fn = fmt::format("{}_{}x{}x{}f.dat", fn, size[0], size[1], frames[1] - frames[0]);

      filename.assign(fn.begin(), fn.end());

      // Make sure there is enough space for the user input
      if (filename.size() < 256) {
        filename.resize(256);
      }
    }
  } raw;

  struct {
    bool export_window         = false;
    bool recording             = false;
    float progress             = 0;
    std::vector<char> filename = {};
    VideoRecorder videoRecorder;

    void assign_auto_filename(const fs::path &bmp_path) {
      videoRecorder.videotitle = bmp_path.filename().string();

      std::string fn = bmp_path.filename().stem().string() + ".mp4";
      filename.assign(fn.begin(), fn.end());

      // Make sure there is enough space for the user input
      if (filename.size() < 256) {
        filename.resize(256);
      }
    }
  } video;

  struct {
    bool export_window         = false;
    bool save_pngs             = false;
    std::vector<char> filename = {};

    void assign_auto_filename(const fs::path &bmp_path) {
      std::string fn = bmp_path.filename().stem().string() + "_{t}.png";
      filename.assign(fn.begin(), fn.end());

      // Make sure there is enough space for the user input
      if (filename.size() < 256) {
        filename.resize(256);
      }
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

  RecordingWindow(const fs::path &path) : RecordingWindow(autoguess_filerecording(path)){};
  RecordingWindow(std::shared_ptr<AbstractRecording> file_)
      : Recording(std::move(file_)), playback(good() ? length() : 0), transformationArena(*this) {
    if (!good()) {
      return;
    }

    if (Trace::width() == 0) {
      // if unset, set trace edge length to something reasonable
      auto val = std::min(Nx(), Ny()) / 64;
      Trace::width(std::max(val, 2));
    }

    if (file->bitrange()) {
      bitrange = file->bitrange().value();
    }
    float &min = get_min(Transformations::None);
    float &max = get_max(Transformations::None);
    if ((min == max) || std::isnan(min) || std::isnan(max)) {
      std::tie(min, max) = bitrange_to_float(bitrange);
    }

    if (file->cmap()) {
      cmap_ = file->cmap().value();
    } else if (bitrange == BitRange::PHASE || bitrange == BitRange::PHASE_DIFF) {
      // assume that's its a phase map and the user prefers HSV in this circumstances
      cmap_ = ColorMap::HSV;
    }
  }

  virtual ~RecordingWindow() {
    if (window != nullptr) {
      clear_gl_memory();
      glfwDestroyWindow(window);
    }
  }

  void open_window();
  void set_context(GLFWwindow *new_context);

  virtual void display(Filters prefilter, Transformations transformation, Filters postfilter);
  virtual void render() {
    if (glcontext == window) glfwSwapBuffers(window);
  }

  virtual void load_next_frame() { load_frame(playback.step()); }

  float &get_min(Transformations type) { return transformationArena.create_if_needed(type)->min; }
  float &get_max(Transformations type) { return transformationArena.create_if_needed(type)->max; }

  void reset_traces();
  void add_trace_pos(const Vec2i &npos);
  void remove_trace_pos(const Vec2i &pos);

  static void fliplr() { rotations.flipud(); }
  static void flipud() { rotations.fliplr(); }
  static void set_rotation(short rotation);
  static void add_rotation(short d_rotation);

  void colormap(ColorMap cmap);
  ColorMap colormap() const { return cmap_; }

  void resize_window();
  fs::path save_snapshot(std::string output_png_path_template = "");
  void start_recording(const std::string &filename, int fps = 30);
  void stop_recording();

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

  ColorMap cmap_  = ColorMap::GRAY;
  GLuint texture  = GL_FALSE;
  GLuint ctexture = GL_FALSE;
  Shader frame_shader;
  GLuint frame_vao, frame_vbo, frame_ebo;
  Shader trace_shader;
  GLuint trace_vao, trace_vbo;
  std::vector<float> trace_vert;
};

using SharedRecordingPtr = std::shared_ptr<RecordingWindow>;