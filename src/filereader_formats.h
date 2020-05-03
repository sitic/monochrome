#pragma once

#include <regex>

#include "vectors.h"
#include "filereader.h"
#include "bmp.h"

class BmpFileRecording : public AbstractRecording {
 protected:
  BMPheader file;
  Eigen::Matrix<uint16_t, Eigen::Dynamic, Eigen::Dynamic> frame_uint16;

 public:
  BmpFileRecording(const fs::path &path) : AbstractRecording(path), file(path) {
    frame_uint16.setZero(file.Nx(), file.Ny());
  }

  bool good() const final { return file.good(); };
  int Nx() const final { return file.Nx(); };
  int Ny() const final { return file.Ny(); };
  int length() const final { return file.length(); };
  std::string error_msg() final { return file.error_msg(); };
  std::string date() const final { return file.date(); };
  std::string comment() const final { return file.comment(); };
  std::chrono::duration<float> duration() const final { return file.duration(); };
  float fps() const final { return file.fps(); };
  std::optional<BitRange> bitrange() const final {
    if (Nx() == 128 && Ny() == 128) {
      // Probably a PVCam recording
      return BitRange::U16;
    } else {
      // Probably a IDS camera recording
      return BitRange::U12;
    }
  }
  std::optional<ColorMap> cmap() const final { return ColorMap::GRAY; }

  Eigen::MatrixXf read_frame(long t) final {
    file.read_frame(t, frame_uint16.data());
    return frame_uint16.cast<float>();
  };

  float get_pixel(long t, long x, long y) final { return file.get_pixel(t, x, y); }
};

class RawFileRecording : public AbstractRecording {
  mio::mmap_source _mmap;
  int _nx    = 0;
  int _ny    = 0;
  int _nt    = 0;
  bool _good = false;

  std::size_t _frame_size = 0;
  std::string _error_msg  = "";

  Eigen::MatrixXf _frame;
  std::optional<BitRange> _bitrange;

  const float *get_data_ptr(long t) const {
    auto ptr = _mmap.data() + t * _frame_size * sizeof(float);
    return reinterpret_cast<const float *>(ptr);
  }

  static Vec3i calc_dims_from_filename(const fs::path &path) {
    std::string filename = path.filename().string();
    int nx, ny, nt;
    // The filename has to something like prefix_{width}x{height}x{#frames}_suffix.dat
    // Note that we don't the same order as in numpy ({width}x{height} instead of {height}x{width})
    const std::regex rgx(R"(^.*?_(\d+)x(\d+)(x(\d+))?f?.*?\.dat$)");
    if (std::smatch matches; std::regex_match(filename, matches, rgx)) {
      nx = std::stoi(matches[1]);
      ny = std::stoi(matches[2]);
      if (matches[4].matched) {
        nt = std::stoi(matches[4]);
      } else {
        nt = 1;
      }
      return {nx, ny, nt};
    } else {
      return {-1, -1, -1};
    }
  }

 public:
  RawFileRecording(const fs::path &path) : RawFileRecording(path, calc_dims_from_filename(path)) {}
  RawFileRecording(const fs::path &path, Vec3i dims)
      : RawFileRecording(path, dims[0], dims[1], dims[2]) {}
  RawFileRecording(const fs::path &path, int nx, int ny, int nt)
      : AbstractRecording(path), _nx(nx), _ny(ny), _nt(nt), _frame_size(nx * ny) {
    if (_nx <= 0 || _ny <= 0 || _nt <= 0) {
      _error_msg = "Unable to determine dimensions from file name";
      return;
    }

    std::error_code error;
    _mmap.map(path.string(), error);
    if (error) {
      _good      = false;
      _error_msg = error.message();
      return;
    }
    auto l               = _mmap.length();
    auto bytes_per_frame = _frame_size * sizeof(float);
    if (l % bytes_per_frame != 0 || l / bytes_per_frame < _nt) {
      _error_msg = "File size does not match expected dimensions";
      return;
    }

    if (l / bytes_per_frame > _nt) {
      if (_nt != 1) {
        fmt::print(
            "detected incorrect dimensions, nt={} was given but based on the"
            "filesize nt has to be {}\n",
            _nt, l / bytes_per_frame);
      }
      _nt = l / bytes_per_frame;
    }

    _good = true;

    _frame.setZero(_nx, _ny);
    _bitrange = detect_bitrange(get_data_ptr(0), get_data_ptr(1));
  }

  bool good() const final { return _good; };
  int Nx() const final { return _nx; };
  int Ny() const final { return _ny; };
  int length() const final { return _nt; };
  std::string error_msg() final { return _error_msg; };
  std::string date() const final { return ""; };
  std::string comment() const final { return ""; };
  std::chrono::duration<float> duration() const final { return 0s; };
  float fps() const final { return 0; };
  std::optional<BitRange> bitrange() const final { return _bitrange; }
  std::optional<ColorMap> cmap() const final { return std::nullopt; }

  Eigen::MatrixXf read_frame(long t) final {
    auto data = get_data_ptr(t);
    std::copy(data, data + _frame_size, _frame.data());
    return _frame;
  };

  float get_pixel(long t, long x, long y) final { return get_data_ptr(t)[y * Nx() + x]; }
};
