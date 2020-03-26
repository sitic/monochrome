#pragma once

#include <regex>

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
      return BitRange::U12;
    }
  }

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

 public:
  RawFileRecording(const fs::path &path) : AbstractRecording(path) {
    std::error_code error;
    _mmap.map(path.string(), error);
    if (error) {
      _good      = false;
      _error_msg = error.message();
      return;
    }

    const std::regex rgx(R"(^.*?_(\d+)x(\d+)x(\d+)f.*?\.dat$)");
    std::string filename = path.filename().string();
    if (std::smatch matches; std::regex_match(filename, matches, rgx)) {
      _nx = std::stoi(matches[1]);
      _ny = std::stoi(matches[2]);
      _nt = std::stoi(matches[3]);

      _frame_size = _nx * _ny;

      auto l = _mmap.length();
      if (l % (_nx * _ny * sizeof(float)) != 0 || l / (_nx * _ny * sizeof(float)) < _nt) {
        _error_msg = "File size does not match expected dimensions";
        return;
      }
    } else {
      _error_msg = "Unable to determine dimensions from file name";
      return;
    }

    _good = true;

    _frame.setZero(_nx, _ny);
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
  std::optional<BitRange> bitrange() const final { return BitRange::FLOAT; }

  Eigen::MatrixXf read_frame(long t) final {
    auto data = reinterpret_cast<const float *>(_mmap.data());
    std::copy(data, data + _nx * _ny, _frame.data());
    return _frame;
  };

  float get_pixel(long t, long x, long y) final {
    auto data = reinterpret_cast<const float *>(_mmap.data());
    return data[_frame_size * t + y * Nx() + x];
  }
};
