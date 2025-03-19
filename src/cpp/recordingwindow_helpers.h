#pragma once

#include <atomic>
#include <utility>
#include <future>

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

  void decrease_speed() { val /= 2; }

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

class Trace {
 private:
  int _data_width = 0; // Tracks the width of the data when it was last updated
  Vec2i _data_imgpos; // Tracks the position of the data when it was last updated

  struct FutureTraceData {
    std::atomic<bool> ready = false;
    std::atomic<bool> cancelled = false;
    std::vector<float> data;
    std::atomic<long> progress_t = 0;
  };

  static void compute_trace(std::shared_ptr<AbstractFile> file,
                            const Vec2i &start,
                            const Vec2i &size,
                            std::shared_ptr<FutureTraceData> future_data) {
    if (!file || !file->good() || !future_data) {
      return;
    }
    long CHUNK_SIZE = 50;
    future_data->data.resize(file->length());
    for (int t = 0; t < file->length(); t++) {
      if (future_data->cancelled) {  // We are no longer interested in the result
        return;
      }
      future_data->data[t] = file->get_block(t, start, size);
      if (t % CHUNK_SIZE == 0 && t > 0) {
        future_data->progress_t = t;
      }
    }
    future_data->progress_t = future_data->data.size();
    future_data->ready = true;
  }

 public:
  int id = -1;
  int t = 0;
  Vec2i pos; // Position in the transformed image
  Vec2i original_position;  // Original position before rotations and flips
  Vec4f color;
  std::vector<float> data;
  std::shared_ptr<FutureTraceData> future_data_ptr = nullptr;
  
  Trace(const Vec2i &_pos, Recording& rec) : id(std::rand()), color(next_color()) {
    set_pos(_pos, rec);
  }
  Trace() = default;
  ~Trace() {
      if (future_data_ptr) {
          future_data_ptr->cancelled = true;
      }
  }
  void set_pos(const Vec2i &npos, Recording &rec);
  void clear() { data.clear(); }
  bool is_near_point(const Vec2i &npos) const;
  void tick(Recording& rec) {
    if (rec.apply_transformation(original_position) != pos) {
      // Rotation or flip has changed the position
      set_pos(rec.apply_transformation(original_position), rec);
    }
    if (_data_imgpos != original_position || _data_width != Trace::width() ||
        (data.empty() && !future_data_ptr)) {
      // Data is not up to date, cancel existing future if it exists
      if (future_data_ptr) {
        future_data_ptr->cancelled = true;
      }
      // Add compute trace job to thread pool
      future_data_ptr = std::make_shared<FutureTraceData>();
      future_data_ptr->cancelled = false;
      _data_width = Trace::width();
      _data_imgpos = original_position;
      Vec2i dims = {rec.file()->Nx(), rec.file()->Ny()};
      Vec2i start, size;
      std::tie(start, size) = clamp(original_position, dims);
      auto file = rec.file();
      auto future_data = future_data_ptr;
      asio::post(global::thread_pool, [file, start, size, future_data]() {
        Trace::compute_trace(file, start, size, future_data);
      });
    }
  }
  static Vec4f next_color();
  static int width(int new_width = 0);
  // get valid starting position and size based on the position of center and max size
  static std::pair<Vec2i, Vec2i> clamp(const Vec2i &pos, const Vec2i &max_size);
  void save(fs::path path);
};

class FlowData {
 public:
  std::shared_ptr<Recording> data;
  Vec4f color;
  bool show                     = true;
  static inline int skip        = 0;
  static inline float pointsize = 1.5;

  FlowData(std::shared_ptr<Recording> data_, unsigned color_count)
      : data(std::move(data_)), color(next_color(color_count)) {}
  FlowData(std::shared_ptr<Recording> data_, Vec4f color_) : data(std::move(data_)), color(color_) {}
  Vec4f next_color(unsigned color_count);
};

struct ExportCtrl {
  struct {
    bool export_window = false;
    Vec2i start;
    Vec2i size;
    Vec2i frames;
    std::string filename;
    ExportFileType type = ExportFileType::Npy;

    void assign_auto_filename(std::shared_ptr<AbstractFile> file) {
      auto fn = file->path().stem().string();

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
    bool close_after_completion = false;

    void assign_auto_filename(std::shared_ptr<AbstractFile> file){
      auto path                = file->path();
      videoRecorder.videotitle = path.filename().string();
      filename                 = path.filename().stem().string() + ".mp4";
    }
  } video;

  struct {
    bool export_window = false;
    bool save_pngs     = false;
    std::string filename;

    void assign_auto_filename(std::shared_ptr<AbstractFile> file) {
      auto path                = file->path();
      if (file->length() > 1)
        filename = path.filename().stem().string() + "_{t}.png";
      else
        filename = path.filename().stem().string() + ".png";
    }
  } png;

  struct {
    bool export_window = false;
    std::string filename;
    Trace trace;

    void assign_auto_filename(std::shared_ptr<AbstractFile> file, const Trace &_trace, int width) {
      auto path = file->path();
      trace     = _trace;
      filename  = path.filename().stem().string();
      auto pos  = trace.original_position;
      filename += fmt::format("_{}_{}_size{}.txt", pos[0], pos[1], width);
    }

  } trace;
};

std::pair<float, float> oportunistic_minmax(std::shared_ptr<AbstractFile> file,
                                            int sampling_frames = 10);