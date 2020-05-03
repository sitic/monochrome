#pragma once

#include <chrono>
#include <optional>
#include <string>

#include <Eigen/Dense>
#include <utility>

#include "globals.h"
// only use std filesystem on msvc for now, as gcc / clang sometimes require link options
#if defined(__cplusplus) && _MSC_VER >= 1920
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#endif

using namespace std::chrono_literals;

class AbstractRecording {
 private:
  fs::path _path;

 public:
  AbstractRecording(fs::path path) : _path(std::move(path)){};
  virtual ~AbstractRecording() = default;
  fs::path path() { return _path; };

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
  virtual std::optional<ColorMap> cmap() const          = 0;

  [[nodiscard]] virtual Eigen::MatrixXf read_frame(long t)      = 0;
  [[nodiscard]] virtual float get_pixel(long t, long x, long y) = 0;
};

class InMemoryRecording : public AbstractRecording {
  bool _good = false;

  std::string _error_msg = "";

  std::shared_ptr<global::RawArray3> _data;
  Eigen::MatrixXf _frame;
  std::size_t _frame_size;

 public:
  InMemoryRecording(std::shared_ptr<global::RawArray3> data)
      : AbstractRecording(data ? data->meta.name : ""), _data(data) {
    _good = static_cast<bool>(_data);
    if (!_good) {
      _error_msg = "Empty array loaded";
      return;
    }

    _frame_size = Nx() * Ny();
    _frame.setZero(Nx(), Ny());

    if (!_data->meta.bitrange) {
      _data->meta.bitrange = detect_bitrange(_data->data.begin(), _data->data.begin() + _frame_size);
    }
  }

  bool good() const final { return _good; };
  int Nx() const final { return _data->meta.nx; };
  int Ny() const final { return _data->meta.ny; };
  int length() const final { return _data->meta.nt; };
  std::string error_msg() final { return _error_msg; };
  std::string date() const final { return ""; };
  std::string comment() const final { return ""; };
  std::chrono::duration<float> duration() const final {
    return std::chrono::duration<float>(_data->meta.duration);
  };
  float fps() const final { return _data->meta.fps; };
  std::optional<BitRange> bitrange() const final { return _data->meta.bitrange; }
  std::optional<ColorMap> cmap() const final { return _data->meta.cmap; }

  Eigen::MatrixXf read_frame(long t) final {
    auto data_ptr = _data->data.data() + _frame_size * t;
    std::copy(data_ptr, data_ptr + _frame_size, _frame.data());
    return _frame;
  };

  float get_pixel(long t, long x, long y) final {
    return _data->data[_frame_size * t + y * Nx() + x];
  }
};