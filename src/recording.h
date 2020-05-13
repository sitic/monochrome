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
  void set_rotation(short rotation);
  void add_rotation(short delta_rotation);

  void fliplr();
  void flipud();

  void apply(Eigen::MatrixXf &arr);
};

class Recording {
 protected:
  std::shared_ptr<AbstractRecording> file;
  long t_frame = 0;

  static RotationCtrl rotations;

  void apply_rotation() { rotations.apply(frame); }

  static std::shared_ptr<AbstractRecording> autoguess_filerecording(const fs::path &path);

 public:
  Eigen::MatrixXf frame;

  Recording(const fs::path &path) : Recording(autoguess_filerecording(path)){};
  Recording(std::shared_ptr<AbstractRecording> _file) : file(std::move(_file)) {
    if (!file->good()) {
      return;
    }

    frame.setZero(file->Nx(), file->Ny());
    apply_rotation();
  }
  std::shared_ptr<Recording> copy() const { return std::make_shared<Recording>(file); }

  std::shared_ptr<AbstractRecording> get_file_ptr() const { return file; }
  void set_file_ptr(std::shared_ptr<AbstractRecording> new_file) { file = new_file; }

  [[nodiscard]] bool good() const { return file && file->good(); }
  int Nx() const { return frame.rows(); }
  int Ny() const { return frame.cols(); }
  int length() const { return file->length(); }
  fs::path path() const { return file->path(); }
  std::string name() const { return file->path().filename().string(); }
  std::string date() const { return file->date(); };
  std::string comment() const { return file->comment(); };
  std::chrono::duration<float> duration() const { return file->duration(); }
  float fps() const { return file->fps(); }
  std::optional<BitRange> bitrange() const { return file->bitrange(); }
  std::optional<ColorMap> cmap() const { return file->cmap(); }

  void load_frame(long t);

  int current_frame() const { return t_frame; }

  bool export_ROI(fs::path path, Vec2i start, Vec2i size, Vec2i t0tmax, Vec2f minmax = {0, 0});
};