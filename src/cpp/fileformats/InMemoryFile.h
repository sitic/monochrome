#pragma once

#include "AbstractFile.h"
#include "globals.h"

class InMemoryFile : public AbstractFile {
  bool _good = false;

  std::string _error_msg = "";

  std::shared_ptr<global::RawArray3> _data;
  Eigen::MatrixXf _frame;
  std::size_t _frame_size;

 public:
  InMemoryFile(std::shared_ptr<global::RawArray3> data)
      : AbstractFile(data ? data->meta.name : ""), _data(data) {
    _good = static_cast<bool>(_data);
    if (!_good) {
      _error_msg = "Empty array loaded";
      return;
    }

    _frame_size = Nx() * Ny();
    _frame.setZero(Nx(), Ny());

    if (!bitrange() && !is_flow()) {
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
  int length() const final { return _data->meta.nt; };
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

  Eigen::MatrixXf read_frame(long t) final {
    std::visit(
        [this, t](const auto& data) {
          auto data_ptr = data.data() + _frame_size * t;
          std::copy(data_ptr, data_ptr + _frame_size, _frame.data());
          if (!is_flow()) {
            auto begin = data.data() + _frame_size * t;
            std::copy(begin, begin + _frame_size, _frame.data());
          } else {
            bool isodd = t % 2;
            auto begin = data.data() + _frame_size * (t - isodd) + isodd;
            auto end   = begin + 2 * _frame_size;
            std::copy(StrideIterator(begin, 2), StrideIterator(end, 2), _frame.data());
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

  bool is_flow() const final { return _data->meta.is_flowfield; };
  bool set_flow(bool _is_flow) final {
    _data->meta.is_flowfield = _is_flow;
    return true;
  }
  void set_comment(const std::string& new_comment) final {}
  flag_set<FileCapabilities> capabilities() const final {
    return flag_set<FileCapabilities>(FileCapabilities::AS_FLOW);
  }

  // color for points if it is a flow array
  std::optional<Vec4f> color() { return _data->meta.color; }
};