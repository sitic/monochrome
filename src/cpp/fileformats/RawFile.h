#pragma once

#include <regex>

#include "mio/mio.hpp"
#include "AbstractFile.h"
#include "utils/vectors.h"

class RawFile : public AbstractFile {
  mio::mmap_source _mmap;
  int _nx       = 0;
  int _ny       = 0;
  int _nt       = 0;
  bool _good    = false;
  bool _is_flow = false;

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
    // Note that we don't use the same order as in numpy ({width}x{height} instead of {height}x{width})
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
  RawFile(const fs::path &path) : RawFile(path, calc_dims_from_filename(path)) {}
  RawFile(const fs::path &path, Vec3i dims) : RawFile(path, dims[0], dims[1], dims[2]) {}
  RawFile(const fs::path &path, int nx, int ny, int nt)
      : AbstractFile(path), _nx(nx), _ny(ny), _nt(nt), _frame_size(nx * ny) {
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
    _bitrange = utils::detect_bitrange(get_data_ptr(0), get_data_ptr(1));
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
  std::vector<std::pair<std::string, std::string>> metadata() const final { return {}; };
  std::optional<BitRange> bitrange() const final { return _bitrange; }
  std::optional<ColorMap> cmap() const final { return std::nullopt; }
  bool is_flow() const final { return _is_flow; };
  bool set_flow(bool _flow) final {
    _is_flow = _flow;
    return true;
  }
  void set_comment(const std::string &new_comment) final {}
  flag_set<FileCapabilities> capabilities() const final {
    return flag_set<FileCapabilities>(FileCapabilities::AS_FLOW);
  }

  Eigen::MatrixXf read_frame(long t) final {
    if (!is_flow()) {
      auto begin = get_data_ptr(t);
      std::copy(begin, begin + _frame_size, _frame.data());
    } else {
      bool isodd = t % 2;
      auto begin = get_data_ptr(t - isodd) + isodd;
      auto end   = begin + 2 * _frame_size;
      std::copy(StrideIterator(begin, 2), StrideIterator(end, 2), _frame.data());
    }
    return _frame;
  };

  float get_pixel(long t, long x, long y) final { return get_data_ptr(t)[y * Nx() + x]; }
};