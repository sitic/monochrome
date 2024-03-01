#pragma once

#include <utility>

#include "recording.h"
#include "transformations.h"
#include "utils/videorecorder.h"

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

  void increase_speed() { val *= 2; }

  void deacrease_speed() { val /= 2; }

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
  void synchronize_with(const RecordingPlaybackCtrl &other, bool warn = true);

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

    void assign_auto_filename(const fs::path &path) {
      videoRecorder.videotitle = path.filename().string();
      filename                 = path.filename().stem().string() + ".mp4";
    }
  } video;

  struct {
    bool export_window = false;
    bool save_pngs     = false;
    std::string filename;

    void assign_auto_filename(const fs::path &path) {
      filename = path.filename().stem().string() + "_{t}.png";
    }
  } png;

  struct {
    bool export_window = false;
    Vec2i pos;
    std::string filename;
    int tstart = 0;
    int tend   = -1;

    void assign_auto_filename(const fs::path &path, const Vec2i &pos_, int width) {
      pos      = pos_;
      filename = path.filename().stem().string();
      filename += fmt::format("_{}_{}_size{}.txt", pos_[0], pos_[1], width);
    }

  } trace;
};

class SmoothScaler {
  inline static const float autoScaleFrequency = 20;

  void smooth_scale(const float &min_value, const float &max_value) {
    // prepare a smooth transition, do not smooth if diff is small.
    double smooth       = (upper - lower) / 20;
    double smooth_lower = lower + smooth / 4;
    double smooth_upper = upper - smooth / 4;

    if (double diff = min_value - smooth_lower; diff > smooth || diff < 0) {
      lower += diff / autoScaleFrequency;
    }
    if (double diff = smooth_upper - max_value; diff > smooth || diff < 0) {
      upper -= diff / autoScaleFrequency;
    }
  }

 public:
  std::vector<int> restarts = {};
  bool scaleX               = true;
  bool scaleY               = true;
  double lower              = 0;
  double upper              = 0;
  double left               = 0;
  double right              = 0;

  SmoothScaler() = default;

  void scale(Vec2f *first, Vec2f *last) {
    float min = first->operator[](1);
    float max = min;

    for (auto it = first + 1; it < last; it++) {
      const float val = it->operator[](1);
      if (val > max) {
        max = val;
      } else if (val < min) {
        min = val;
      }
    }
    //auto first_y = RawIterator(first->data() + 1);
    //auto last_y  = RawIterator(last->data() + 1);
    //auto [minItr, maxItr] =
    //    std::minmax_element(StrideIterator(first_y, 2), StrideIterator(last_y, 2));
    //float minValue = *minItr;
    //float maxValue = *maxItr;

    smooth_scale(min, max);
  }

  template <typename It>
  void scale(It begin, It end) {
    auto [minItr, maxItr] = std::minmax_element(begin, end);
    float minValue        = *minItr;
    float maxValue        = *maxItr;

    smooth_scale(minValue, maxValue);
  }
};

struct Trace {
  int id;
  Vec2i pos;
  Vec4f color;
  SmoothScaler scale;
  std::vector<float> data;

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
  bool show                     = true;
  static inline int skip        = 2;
  static inline float pointsize = 1.5;

  FlowData(std::shared_ptr<Recording> data_, unsigned color_count)
      : data(std::move(data_)), color(next_color(color_count)) {}
  FlowData(std::shared_ptr<Recording> data_, Vec4f color_) : data(std::move(data_)), color(color_) {}
  Vec4f next_color(unsigned color_count);
};

std::pair<float, float> oportunistic_minmax(std::shared_ptr<AbstractFile> file,
                                            int sampling_frames = 10);