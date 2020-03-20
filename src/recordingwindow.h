#pragma once

#include <GLFW/glfw3.h>
#include <utility>
#include <variant>

#include "recording.h"
#include "transformations.h"
#include "videorecorder.h"

namespace prm {
  static struct {
    float val = 1;

    void toggle_play_pause() {
      std::swap(old_val, val);
      if (old_val > 0) val = 0;
    }

   private:
    float old_val = 0;
  } playbackCtrl;
}  // namespace prm

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
      if (filename.size() < 64) {
        filename.resize(64);
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
      if (filename.size() < 64) {
        filename.resize(64);
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
  GLFWwindow *window = nullptr;
  static float scale_fct;

  TransformationList transformationArena;

  float &get_max(Transformations type) { return transformationArena.create_if_needed(type)->max; }

  float &get_min(Transformations type) { return transformationArena.create_if_needed(type)->min; }

  Histogram<float, 256> histogram;
  ExportCtrl export_ctrl;
  std::vector<Trace> traces;

  RecordingWindow(const fs::path &path) : RecordingWindow(autoguess_filerecording(path)){};
  RecordingWindow(std::shared_ptr<AbstractRecording> file)
      : Recording(std::move(file)), transformationArena(*this) {
    if (!good()) {
      return;
    }

    if (Trace::width() == 0) {
      // if unset, set trace edge length to something reasonable
      auto val = std::min(Nx(), Ny()) / 64;
      Trace::width(std::max(val, 2));
    }
  }

  virtual ~RecordingWindow() {
    if (window != nullptr) {
      glfwDestroyWindow(window);
    }
  }

  virtual void display(float speed,
                       Filters prefilter,
                       Transformations transformation,
                       Filters postfilter,
                       BitRange bitrange);

  void reset_traces() {
    for (auto &t : traces) {
      t.clear();
    }
  }

  void add_trace_pos(const Vec2i &npos) {
    for (auto &t : traces) {
      if (t.is_near_point(npos)) {
        t.set_pos(npos);
        return;
      }
    }
    traces.push_back({{}, npos, Trace::next_color()});
  }

  void remove_trace_pos(const Vec2i &pos) {
    const auto pred = [pos](const auto &trace) { return trace.is_near_point(pos); };

    traces.erase(std::remove_if(traces.begin(), traces.end(), pred), traces.end());
  }

  void start_recording(const std::string &filename, int fps = 30) {
    restart();
    export_ctrl.video.videoRecorder.start_recording(filename, window, fps);
    export_ctrl.video.recording = true;
    export_ctrl.video.progress  = 0;
  }

  void stop_recording() {
    export_ctrl.video.videoRecorder.stop_recording();
    export_ctrl.video.recording     = false;
    export_ctrl.video.progress      = 0;
    export_ctrl.video.export_window = false;
  }

  static void fliplr() { rotations.flipud(); }
  static void flipud() { rotations.fliplr(); }
  void add_rotation(short d_rotation) {
    rotations.add_rotation(d_rotation);
    load_frame(t_frame);
    transformationArena.reallocate();
    resize_window();
  }

  void open_window();
  void resize_window();
  fs::path save_snapshot(std::string output_png_path_template = "");
  static void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
  static void cursor_position_callback(GLFWwindow *window, double xpos, double ypos);
  static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
  static void reshape_callback(GLFWwindow *window, int w, int h);
  static void close_callback(GLFWwindow *window);
  static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

 protected:
  Vec2d mousepos;
  struct {
    bool left  = false;
    bool right = false;
  } mousebutton;
};
