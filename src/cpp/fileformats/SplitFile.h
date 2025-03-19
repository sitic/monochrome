#pragma once

#include <algorithm>

#include "AbstractFile.h"
#include "globals.h"

class SplitFile : public AbstractFile {
    std::shared_ptr<AbstractFile> _file;
    int _t = 0;
    bool _good = true;


 public:
 SplitFile(std::shared_ptr<AbstractFile> file, int t)
      : AbstractFile(file? file->path(): ""), _file(file) {
    _file = file;
    _t = t;
    if (!_file || _t >= _file->length()) {
      _good = false;
    }
  }

  bool good() const final { return _good && _file->good(); };
  std::string error_msg() final { return _file->error_msg(); };

  int Nx() const final { return _file->Nx(); };
  int Ny() const final { return _file->Ny(); };
  int Nc() const final { return _file->Nc(); };
  int length() const final { return 1; };
  std::string date() const final { return _file->date(); };
  std::string comment() const final { return _file->comment(); };
  std::chrono::duration<float> duration() const final {
    return _file->duration();
  };
  float fps() const final { return _file->fps(); };
  std::vector<std::pair<std::string, std::string>> metadata() const final {
    return _file->metadata();
  };
  std::optional<BitRange> bitrange() const final { return _file->bitrange(); }
  std::optional<ColorMap> cmap() const final { return _file->cmap(); }
  std::optional<float> vmin() const final { return _file->vmin(); };
  std::optional<float> vmax() const final { return _file->vmax(); };
  std::optional<OpacityFunction> opacity() const final { return _file->opacity(); };

  Eigen::MatrixXf read_frame(long t, long c) final {
    return _file->read_frame(_t, c);
  };

  float get_pixel(long t, long x, long y) final {
        return _file->get_pixel(_t, x, y);
  }

  float get_block(long t, const Vec2i& start, const Vec2i& size) final {
      return _file->get_block(_t, start, size);
  }

  void set_comment(const std::string& new_comment) final {}
  flag_set<FileCapabilities> capabilities() const final { return {}; }
};