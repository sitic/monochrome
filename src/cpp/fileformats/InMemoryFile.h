#pragma once

#include <algorithm>

#include "AbstractFile.h"
#include "globals.h"

class InMemoryFile : public AbstractFile {
  bool _good = false;

  std::string _error_msg = "";

  std::shared_ptr<global::RawArray3> _data;
  Eigen::MatrixXf _frame;
  std::size_t _frame_size;

  static fs::path get_filepath(std::shared_ptr<global::RawArray3> data) {
    if (!data) return fs::path();
    fs::path path = data->meta.name;
    // Check if metaData contains a file path, pop it if it does
    auto pred = [](const auto& entry) { return entry.first == "filepath" && !entry.second.empty(); };
    auto it = std::find_if(data->meta.metaData.begin(), data->meta.metaData.end(), pred);
    if (it != data->meta.metaData.end()) {
      path = it->second;
      data->meta.metaData.erase(it);
    }
    return path;
  }  

 public:
  InMemoryFile(std::shared_ptr<global::RawArray3> data)
      : AbstractFile(InMemoryFile::get_filepath(data)), _data(data) {
    _good = static_cast<bool>(_data);
    if (!_good) {
      _error_msg = "Empty array loaded";
      return;
    }

    _frame_size = Nx() * Ny();
    _frame.setZero(Nx(), Ny());

    if (!bitrange() && Nc() != 2) {
      std::visit(
          [this](const auto& data) {
            _data->meta.bitrange = utils::detect_bitrange(data.begin(), data.begin() + _frame_size);
          },
          _data->data);
    }
  }

  bool good() const final { return _good; };
  std::string error_msg() final { return _error_msg; };

  int Nx() const final { return _data->meta.nx; };
  int Ny() const final { return _data->meta.ny; };
  int Nc() const final { return _data->meta.nc; };
  int length() const final { return _data->meta.nt / _data->meta.nc; };
  std::string date() const final { return _data->meta.date; };
  std::string comment() const final { return _data->meta.comment; };
  std::chrono::duration<float> duration() const final {
    return std::chrono::duration<float>(_data->meta.duration);
  };
  float fps() const final { return _data->meta.fps; };
  std::vector<std::pair<std::string, std::string>> metadata() const final {
    return _data->meta.metaData;
  };
  std::optional<BitRange> bitrange() const final { return _data->meta.bitrange; }
  std::optional<ColorMap> cmap() const final { return _data->meta.cmap; }
  std::optional<float> vmin() const final { return _data->meta.vmin; };
  std::optional<float> vmax() const final { return _data->meta.vmax; };
  std::optional<OpacityFunction> opacity() const final { return _data->meta.opacity; };

  Eigen::MatrixXf read_frame(long t, long c) final {
    std::visit(
        [this, t, c](const auto& data) {
          if (Nc() == 1) { // ignore c, since we have only one channel
            auto begin = data.data() + _frame_size * t;
            std::copy(begin, begin + _frame_size, _frame.data());
          } else {
            auto begin   = data.data() + _frame_size * (t * Nc()) + c;
            auto end     = begin + _frame_size * Nc();
            std::copy(StrideIterator(begin, Nc()), StrideIterator(end, Nc()), _frame.data());
          }
        },
        _data->data);
    return _frame;
  };

  float get_pixel(long t, long x, long y) final {
    return std::visit(
        [this, t, x, y](const auto& data) {
          return static_cast<float>(data[_frame_size * t + y * Nx() + x]);
        },
        _data->data);
  }

  // MSVC compiler faults when implementing this function in the lambda with std::remove_reference for some reason
  template <typename T>
  Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>> get_frame_map(std::vector<T>& data, long t) {
    return Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>>(
      data.data() + _frame_size * t, Nx(), Ny()
    );
  }
  float get_block(long t, const Vec2i& start, const Vec2i& size) final {
      return std::visit(
        [this, t, &start, &size](auto& data) {
            auto map = get_frame_map(data, t);
            return map.block(start[0], start[1], size[0], size[1]).template cast<float>().mean();
        },
        _data->data);
  }

  void set_comment(const std::string& new_comment) final {}
  flag_set<FileCapabilities> capabilities() const final { return {}; }

  // color for points if it is a flow array
  std::optional<Vec4f> color() { return _data->meta.color; }
};