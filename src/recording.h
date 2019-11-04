#pragma once

#include "filereader.h"
#include "utils.h"
#include "vectors.h"

using namespace std::string_literals;

class Recording {
protected:
  filesystem::path _path;

  std::shared_ptr<BaseFileRecording> file;
  int _t = 0;
  float _tf = 0;

  long t_frame = 0;

public:
  Eigen::MatrixXf frame;

  Recording(const filesystem::path &path) : _path(path) {

    file = std::make_shared<RawFileRecording>(path);
    if (!file->good()) {
      file = std::make_shared<BmpFileRecording>(path);
    }

    if (!file->error_msg().empty()) {
      new_ui_message(file->error_msg());
    }
    if (!file->good()) {
      return;
    }

    frame.setZero(file->Nx(), file->Ny());
  }

  bool good() const { return file->good(); }
  int Nx() const { return file->Nx(); }
  int Ny() const { return file->Ny(); }
  int length() const { return file->length(); }
  filesystem::path path() const { return _path; }
  std::string date() const { return file->date(); };
  std::string comment() const { return file->comment(); };
  std::chrono::duration<float> duration() const { return file->duration(); }
  float fps() const { return file->fps(); }

  std::optional<BitRange> bitrange() const { return file->bitrange(); }

  void load_frame(long t) {
    frame = file->read_frame(t);
    t_frame = t;
  }

  void load_next_frame(float speed = 1) {
    _tf += speed;

    while (_tf > length() - 1) {
      _tf -= length() - 1;
      _t = 0;
    }

    if (std::floor(_tf) > _t) {
      _t = std::floor(_tf);
    }

    if (_t < 0) {
      // should never happen, but just in case
      _t = 0;
      _tf = 0;
    }

    load_frame(_t);
  }

  void restart() {
    _tf = std::numeric_limits<float>::lowest();
    _t = std::numeric_limits<int>::lowest();
  }

  int current_frame() { return _t; }

  // Set the internal frame index to some value, use if want to manipulate the
  // next load_next_frame() call without calling load_frame()
  void set_frame_index(int t) {
    _t = t;
    _tf = t;
  }

  float progress() { return _t / static_cast<float>(length() - 1); }

  bool export_ROI(filesystem::path path, Vec2i start, Vec2i size, Vec2i t0tmax,
                  Vec2f minmax = {0, 0}) {
    if (start[0] < 0 || start[1] < 0 || start[0] + size[0] > Nx() ||
        start[1] + size[1] > Ny()) {
      new_ui_message("ERROR: export_ROI() called with invalid array sizes, "
                     "start={}, size={}",
                     start, size);
      return false;
    }

    if (t0tmax[0] < 0 || t0tmax[1] > length() || t0tmax[0] > t0tmax[1]) {
      new_ui_message(
          "ERROR: start or end frame invalid, start frame {}, end frame {}",
          t0tmax[0], t0tmax[1]);
      return false;
    }

    filesystem::remove(path);
    std::ofstream out(path.string(), std::ios::out | std::ios::binary);
    auto cur_frame = t_frame;

    for (int t = t0tmax[0]; t < t0tmax[1]; t++) {
      load_frame(t);
      auto block = frame.block(start[0], start[1], size[0], size[1]);

      if (minmax[0] != minmax[1]) {
        auto normalize = [min = minmax[0], max = minmax[1]](const float &val) {
          return (val - min) / (max - min);
        };
        block = block.unaryExpr(normalize);
      }

      out.write(reinterpret_cast<const char *>(block.data()),
                block.size() * sizeof(float));
    }
    load_frame(cur_frame);

    if (!out.good()) {
      new_ui_message("ERROR: the writing to file {} seems to have failed",
                     path.string());
      return false;
    }

    return true;
  }
};