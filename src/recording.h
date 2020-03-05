#pragma once

#include <utility>

#include "filereader.h"
#include "utils.h"
#include "vectors.h"

using namespace std::string_literals;

struct RotationCtrl {
 private:
  short _rotation = 0;
  bool _fliplr    = false;
  bool _flipud    = false;

 public:
  short get_rotation() { return _rotation; }

  void set_rotation(short rotation) {
    const std::array<short, 4> valid_rotations = {{0, 90, 180, 270}};
    if (std::none_of(valid_rotations.begin(), valid_rotations.end(),
                     [rotation](auto r) { return r == rotation; })) {
      throw std::logic_error(
          "set_rotation() has to be called with either "
          "0, 90, 180,  or 270 as parameter");
    }

    _rotation = rotation;
  }

  void add_rotation(short delta_rotation) {
    if (std::abs(delta_rotation) != 90) {
      throw std::logic_error(
          "add_rotation() has to be called with either "
          "+90 or -90 as parameter");
    }

    if (_rotation == 0 && delta_rotation < 0) {
      _rotation = 270;
    } else if (_rotation == 270 && delta_rotation > 0) {
      _rotation = 0;
    } else {
      _rotation += delta_rotation;
    }
  }

  void fliplr() {
    if (_rotation == 0 || _rotation == 180) {
      _fliplr = !_fliplr;
    } else {
      _flipud = !_flipud;
    }
  }

  void flipud() {
    if (_rotation == 0 || _rotation == 180) {
      _flipud = !_flipud;
    } else {
      _fliplr = !_fliplr;
    }
  }

  void apply(Eigen::MatrixXf &arr) {
    if (_fliplr && _flipud) {
      arr = arr.rowwise().reverse().colwise().reverse().eval();
    } else if (_fliplr) {
      arr.rowwise().reverseInPlace();
    } else if (_flipud) {
      arr.colwise().reverseInPlace();
    }

    switch (_rotation) {
      case 0:
        // do nothing
        break;
      case 90:
        arr = arr.transpose().colwise().reverse().eval();
        break;
      case 180:
        arr = arr.transpose().colwise().reverse().transpose().colwise().reverse().eval();
        break;
      case 270:
        arr = arr.transpose().rowwise().reverse().eval();
        break;
      default:
        throw std::logic_error("Invalid rotation value! (Should be 0, 90, 180, or 270");
    }
  }
};

class Recording {
 protected:
  std::shared_ptr<AbstractRecording> file;
  int _t    = 0;
  float _tf = 0;

  long t_frame = 0;

  static RotationCtrl rotations;

  void apply_rotation() { rotations.apply(frame); }

  static std::shared_ptr<AbstractRecording> autoguess_filerecording(const filesystem::path &path) {
    std::shared_ptr<AbstractRecording> file = std::make_shared<RawFileRecording>(path);
    if (!file->good()) {
      file = std::make_shared<BmpFileRecording>(path);
    }

    if (!file->error_msg().empty()) {
      new_ui_message(file->error_msg());
    }

    return file;
  }

 public:
  Eigen::MatrixXf frame;

  Recording(const filesystem::path &path) : Recording(autoguess_filerecording(path)){};
  Recording(std::shared_ptr<AbstractRecording> _file) : file(std::move(_file)) {
    if (!file->good()) {
      return;
    }

    frame.setZero(file->Nx(), file->Ny());
    apply_rotation();
  }

  std::shared_ptr<AbstractRecording> get_file_ptr() const { return file; }
  void set_file_ptr(std::shared_ptr<AbstractRecording> new_file) { file = new_file; }

  [[nodiscard]] bool good() const { return file->good(); }
  int Nx() const { return frame.rows(); }
  int Ny() const { return frame.cols(); }
  int length() const { return file->length(); }
  filesystem::path path() const { return file->path(); }
  std::string date() const { return file->date(); };
  std::string comment() const { return file->comment(); };
  std::chrono::duration<float> duration() const { return file->duration(); }
  float fps() const { return file->fps(); }

  std::optional<BitRange> bitrange() const { return file->bitrange(); }

  void load_frame(long t) {
    frame   = file->read_frame(t);
    t_frame = t;
    apply_rotation();
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
      _t  = 0;
      _tf = 0;
    }

    load_frame(_t);
  }

  void restart() {
    _tf = std::numeric_limits<float>::lowest();
    _t  = std::numeric_limits<int>::lowest();
  }

  int current_frame() { return _t; }

  // Set the internal frame index to some value, use if want to manipulate the
  // next load_next_frame() call without calling load_frame()
  void set_frame_index(int t) {
    _t  = t;
    _tf = t;
  }

  float progress() { return _t / static_cast<float>(length() - 1); }

  bool export_ROI(
      filesystem::path path, Vec2i start, Vec2i size, Vec2i t0tmax, Vec2f minmax = {0, 0}) {
    if (start[0] < 0 || start[1] < 0 || start[0] + size[0] > Nx() || start[1] + size[1] > Ny()) {
      new_ui_message(
          "ERROR: export_ROI() called with invalid array sizes, "
          "start={}, size={}",
          start, size);
      return false;
    }

    if (t0tmax[0] < 0 || t0tmax[1] > length() || t0tmax[0] > t0tmax[1]) {
      new_ui_message("ERROR: start or end frame invalid, start frame {}, end frame {}", t0tmax[0],
                     t0tmax[1]);
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

      out.write(reinterpret_cast<const char *>(block.data()), block.size() * sizeof(float));
    }
    load_frame(cur_frame);

    if (!out.good()) {
      new_ui_message("ERROR: the writing to file {} seems to have failed", path.string());
      return false;
    }

    return true;
  }
};