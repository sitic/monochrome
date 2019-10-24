#pragma once

#include <optional>
#include <regex>
#include <string>

#include <Eigen/Dense>

#include "bmp.h"

enum class BitRange {
  FLOAT = 1,
  U8 = (1 << 8) - 1,
  U12 = (1 << 12) - 1,
  U16 = (1 << 16) - 1
};

class BaseFileRecording {
public:
  BaseFileRecording() = default;

  virtual bool good() const = 0;
  virtual int Nx() const = 0;
  virtual int Ny() const = 0;
  virtual int length() const = 0;
  virtual std::string error_msg() = 0;
  virtual std::string date() const = 0;
  virtual std::string comment() const = 0;
  virtual std::chrono::duration<float> duration() const = 0;
  virtual float fps() const = 0;
  virtual Eigen::MatrixXf read_frame(long t) = 0;
  virtual std::optional<BitRange> bitrange() const = 0;
};

class BmpFileRecording : public BaseFileRecording {
protected:
  BMPheader file;
  Eigen::Matrix<uint16, Eigen::Dynamic, Eigen::Dynamic> frame_uint16;

public:
  BmpFileRecording(const filesystem::path &path) : file(path) {
    frame_uint16.setZero(file.Nx(), file.Ny());
  }

  bool good() const override { return file.good(); };
  int Nx() const override { return file.Nx(); };
  int Ny() const override { return file.Ny(); };
  int length() const override { return file.length(); };
  std::string error_msg() override { return file.error_msg(); };
  std::string date() const override { return file.date(); };
  std::string comment() const override { return file.comment(); };
  std::chrono::duration<float> duration() const override {
    return file.duration();
  };
  float fps() const override { return file.fps(); };

  Eigen::MatrixXf read_frame(long t) override {
    file.read_frame(t, frame_uint16.data());
    return frame_uint16.cast<float>();
  };

  std::optional<BitRange> bitrange() const override {
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
  int _nx = 0;
  int _ny = 0;
  int _nt = 0;
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
      : _in(path.string(), std::ios::in | std::ios::binary) {

    const std::regex rgx(R"(^.*?_(\d+)x(\d+)x(\d+)f.*?\.dat$)");
    if (std::smatch matches;
        std::regex_match(path.filename().string(), matches, rgx)) {
      _nx = std::stoi(matches[1]);
      _ny = std::stoi(matches[2]);
      _nt = std::stoi(matches[3]);

      auto l = get_filesize();
      if (l % (_nx * _ny * sizeof(float)) != 0 ||
          l / (_nx * _ny * sizeof(float)) < _nt) {
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

  bool good() const override { return _good; };
  int Nx() const override { return _nx; };
  int Ny() const override { return _ny; };
  int length() const override { return _nt; };
  std::string error_msg() override { return _error_msg; };
  std::string date() const override { return ""; };
  std::string comment() const override { return ""; };
  std::chrono::duration<float> duration() const override { return 0s; };
  float fps() const override { return 0; };

  [[nodiscard]] Eigen::MatrixXf read_frame(long t) override {
    auto frame_size = _nx * _nx * sizeof(float);
    _in.seekg(t * frame_size, std::ios::beg);
    if (!_in.good()) {
      throw std::runtime_error("Reading failed!");
    }
    _in.read(reinterpret_cast<char *>(_frame.data()), frame_size);
    return _frame;
  };

  std::optional<BitRange> bitrange() const override { return BitRange::FLOAT; }
};