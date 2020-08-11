#pragma once

#include <utility>

#include "filereader.h"
#include "utils.h"
#include "vectors.h"

using namespace std::string_literals;

enum class ExportFileType { Binary, Npy };

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

  // get flow signs based on current rotation and flip
  std::pair<float, float> flow_signs();
};

class Recording {
 protected:
  std::shared_ptr<AbstractRecording> _file;
  long t_frame = 0;

  static RotationCtrl rotations;

  void apply_rotation() { rotations.apply(frame); }

 public:
  Eigen::MatrixXf frame;

  Recording(const fs::path &path) : Recording(autoguess_filetype(path)){};
  Recording(std::shared_ptr<AbstractRecording> _file) : _file(std::move(_file)) {
    if (!_file || !_file->good()) {
      return;
    }

    frame.setZero(_file->Nx(), _file->Ny());
    apply_rotation();
  }
  std::shared_ptr<Recording> copy() const { return std::make_shared<Recording>(_file); }

  static std::shared_ptr<AbstractRecording> autoguess_filetype(const fs::path &path);
  std::shared_ptr<AbstractRecording> file() const { return _file; }
  void file(std::shared_ptr<AbstractRecording> new_file) { _file = new_file; }

  [[nodiscard]] bool good() const { return _file && _file->good(); }
  int Nx() const { return frame.rows(); }
  int Ny() const { return frame.cols(); }
  int length() const { return _file->length(); }
  fs::path path() const { return _file->path(); }
  std::string name() const { return _file->path().filename().string(); }
  std::string date() const { return _file->date(); };
  std::string comment() const { return _file->comment(); };
  std::chrono::duration<float> duration() const { return _file->duration(); }
  float fps() const { return _file->fps(); }
  std::optional<BitRange> bitrange() const { return _file->bitrange(); }
  std::optional<ColorMap> cmap() const { return _file->cmap(); }

  void load_frame(long t);

  int current_frame() const { return t_frame; }

  bool export_ROI(fs::path path,
                  Vec2i start,
                  Vec2i size,
                  Vec2i t0tmax,
                  ExportFileType exportType,
                  Vec2f minmax = {0, 0});
};