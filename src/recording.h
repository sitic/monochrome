#pragma once

#include <utility>

#include "utils/utils.h"
#include "utils/vectors.h"
#include "fileformats/all_formats.h"

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
  std::shared_ptr<AbstractFile> _file;
  long t_frame = 0;

  static inline RotationCtrl rotations = {};

  void apply_rotation() { rotations.apply(frame); }

 public:
  Eigen::MatrixXf frame;

  Recording(const fs::path &path) : Recording(file_factory(path)){};
  Recording(std::shared_ptr<AbstractFile> _file) : _file(std::move(_file)) {
    if (!_file || !_file->good()) {
      return;
    }

    frame.setZero(_file->Nx(), _file->Ny());
    apply_rotation();
  }
  std::shared_ptr<Recording> copy() const { return std::make_shared<Recording>(_file); }

  std::shared_ptr<AbstractFile> file() const { return _file; }
  void file(std::shared_ptr<AbstractFile> new_file) { _file = new_file; }

  [[nodiscard]] bool good() const { return _file && _file->good(); }
  int Nx() const { return frame.rows(); }
  int Ny() const { return frame.cols(); }
  int length() const { return _file->length(); }
  fs::path path() const { return _file->path(); }
  virtual std::string name() const { return _file->path().filename().string(); }
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