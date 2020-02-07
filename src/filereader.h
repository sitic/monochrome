#pragma once

#include <optional>
#include <regex>
#include <string>

#include <Eigen/Dense>

#include "bmp.h"

enum class BitRange : int { FLOAT, U8, U12, U16 };
const char *BitRangeNames[] = {"float", "uint8", "uint12", "uint16"};

float bitrange_to_float(BitRange br) {
  switch (br) {
    case BitRange::FLOAT:
      return 1;
    case BitRange::U8:
      return (1 << 8) - 1;
    case BitRange::U12:
      return (1 << 12) - 1;
    case BitRange::U16:
      return (1 << 16) - 1;
  }
  throw std::logic_error("This line should not be reached");
}

class BaseFileRecording {
 private:
  filesystem::path _path;

 public:
  BaseFileRecording(const filesystem::path &path) : _path(path){};
  virtual ~BaseFileRecording() = default;
  filesystem::path path() { return _path; };

  virtual bool good() const                             = 0;
  virtual int Nx() const                                = 0;
  virtual int Ny() const                                = 0;
  virtual int length() const                            = 0;
  virtual std::string error_msg()                       = 0;
  virtual std::string date() const                      = 0;
  virtual std::string comment() const                   = 0;
  virtual std::chrono::duration<float> duration() const = 0;
  virtual float fps() const                             = 0;
  virtual std::optional<BitRange> bitrange() const      = 0;

  [[nodiscard]] virtual Eigen::MatrixXf read_frame(long t) = 0;
};

class BmpFileRecording : public BaseFileRecording {
 protected:
  BMPheader file;
  Eigen::Matrix<uint16, Eigen::Dynamic, Eigen::Dynamic> frame_uint16;

 public:
  BmpFileRecording(const filesystem::path &path) : BaseFileRecording(path), file(path) {
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

  Eigen::MatrixXf read_frame(long t) final {
    file.read_frame(t, frame_uint16.data());
    return frame_uint16.cast<float>();
  };

  std::optional<BitRange> bitrange() const final {
    if (Nx() == 128 && Ny() == 128) {
      // Probably a PVCam recording
      return BitRange::U16;
    } else {
      return BitRange::U12;
    }
  }
};

class RawFileRecording : public BaseFileRecording {
  std::ifstream _in;
  int _nx    = 0;
  int _ny    = 0;
  int _nt    = 0;
  bool _good = false;

  std::string _error_msg = "";

  Eigen::MatrixXf _frame;

  std::size_t get_filesize() {
    auto old_pos = _in.tellg();

    _in.seekg(0, std::ios::end);
    auto file_size = _in.tellg();

    _in.seekg(old_pos, std::ios::beg);
    return file_size;
  }

 public:
  RawFileRecording(const filesystem::path &path)
      : _in(path.string(), std::ios::in | std::ios::binary), BaseFileRecording(path) {

    const std::regex rgx(R"(^.*?_(\d+)x(\d+)x(\d+)f.*?\.dat$)");
    std::string filename = path.filename().string();
    if (std::smatch matches; std::regex_match(filename, matches, rgx)) {
      _nx = std::stoi(matches[1]);
      _ny = std::stoi(matches[2]);
      _nt = std::stoi(matches[3]);

      auto l = get_filesize();
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

  Eigen::MatrixXf read_frame(long t) final {
    auto frame_size = _nx * _nx * sizeof(float);
    _in.seekg(t * frame_size, std::ios::beg);
    if (!_in.good()) {
      throw std::runtime_error("Reading failed!");
    }
    _in.read(reinterpret_cast<char *>(_frame.data()), frame_size);
    return _frame;
  };

  std::optional<BitRange> bitrange() const final { return BitRange::FLOAT; }
};