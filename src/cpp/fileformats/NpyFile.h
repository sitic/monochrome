#pragma once

#include "mio/mio.hpp"
#include "npy.hpp"
#include "AbstractFile.h"

class NpyFile : public AbstractFile {
  // WARNING: bool isn't well defined as numpy stores it as a byte, but C++ doesn't guarantee that sizeof(bool) == 1
  enum class PixelDataFormat : int { UINT8 = 1, UINT16 = 2, FLOAT = 4, DOUBLE = 8, BOOL = -1 };

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

  PixelDataFormat dataType;

  template <typename Scalar>
  const Scalar *get_data_ptr(long t) const {
    auto ptr = _mmap.data() + t * _frame_size * sizeof(Scalar);
    return reinterpret_cast<const Scalar *>(ptr);
  }

  template <typename T>
  void copy_frame(long t) {
    if (!is_flow()) {
      auto begin = get_data_ptr<T>(t);
      std::copy(begin, begin + _frame_size, _frame.data());
    } else {
      bool isodd = t % 2;
      auto begin = get_data_ptr<T>(t - isodd) + isodd;
      auto end   = begin + 2 * _frame_size;
      std::copy(StrideIterator(begin, 2), StrideIterator(end, 2), _frame.data());
    }
  }

  template <typename Scalar>
  npy::dtype_t get_dtype() const {
    static_assert(npy::has_typestring<Scalar>::value, "scalar type not understood");
    return npy::has_typestring<Scalar>::dtype;
  }

 public:
  NpyFile(const fs::path &path) : AbstractFile(path) {
    try {
      std::ifstream in(path.string(), std::ios::in | std::ios::binary);
      std::string header_s = npy::read_header(in);
      auto header          = npy::parse_header(header_s);

      // the rest of the file is the raw data, memory-map it
      std::error_code error;
      _mmap.map(path.string(), in.tellg(), mio::map_entire_file, error);
      if (error) {
        _error_msg = error.message();
        return;
      }

      if (get_dtype<uint8_t>().str() == header.dtype.str()) {
        dataType = PixelDataFormat::UINT8;
      } else if (get_dtype<uint16_t>().str() == header.dtype.str()) {
        dataType = PixelDataFormat::UINT16;
      } else if (get_dtype<float>().str() == header.dtype.str()) {
        dataType = PixelDataFormat::FLOAT;
      } else if (get_dtype<double>().str() == header.dtype.str()) {
        dataType = PixelDataFormat::DOUBLE;
      } else if (get_dtype<bool>().str() == header.dtype.str()) {
        dataType = PixelDataFormat::BOOL;
      } else {
        _error_msg = fmt::format(
            "numpy dtype {} is unsupported, only bool, uint8, uint16 and float32 are supported",
            header.dtype.str());
        return;
      }

      if (header.shape.size() == 2) {
        _nt      = 1;
        _ny      = header.shape[0];
        _nx      = header.shape[1];
        _is_flow = false;
      } else if (header.shape.size() == 3) {
        _nt      = header.shape[0];
        _ny      = header.shape[1];
        _nx      = header.shape[2];
        _is_flow = false;
      } else if (header.shape.size() == 4) {
        if (header.shape[3] != 2 || dataType != PixelDataFormat::FLOAT) {
          _error_msg =
              fmt::format("Unsupported array dimenensions ({}, {}, {}, {})", header.shape[0],
                          header.shape[1], header.shape[2], header.shape[3]);
          return;
        }
        _nt      = 2 * header.shape[1];
        _ny      = header.shape[2];
        _nx      = header.shape[3];
        _is_flow = true;
      } else {
        _error_msg = fmt::format("found {}D array, which are unsupported", header.shape.size());
        return;
      }
      _frame_size = _nx * _ny;
      if (_nx < 1 || _ny < 1 || _nt < 1) {
        _error_msg = fmt::format("Invalid array dimensions ({}, {}, {})", _nt, _ny, _nx);
        return;
      }

      auto l               = _mmap.length();
      auto bytes_per_frame = _frame_size * std::abs(static_cast<int>(dataType));
      if (l % bytes_per_frame != 0 || l / bytes_per_frame < _nt) {
        _error_msg = "File size does not match expected dimensions";
        return;
      }

      if (l / bytes_per_frame > _nt) {
        if (_nt != 1) {
          fmt::print(
              "detected incorrect dimensions (file corrupted?), nt={} was given but based on the "
              "filesize nt has to be {}\n",
              _nt, l / bytes_per_frame);
        }
        _nt = l / bytes_per_frame;
      }

      _good = true;

      _frame.setZero(_nx, _ny);
      if (!is_flow()) {
        switch (dataType) {
          case PixelDataFormat::UINT8:
            _bitrange = utils::detect_bitrange(get_data_ptr<uint8_t>(0), get_data_ptr<uint8_t>(1));
            break;
          case PixelDataFormat::UINT16:
            _bitrange = utils::detect_bitrange(get_data_ptr<uint16_t>(0), get_data_ptr<uint16_t>(1));
            break;
          case PixelDataFormat::FLOAT:
            _bitrange = utils::detect_bitrange(get_data_ptr<float>(0), get_data_ptr<float>(1));
            break;
          case PixelDataFormat::DOUBLE:
            _bitrange = utils::detect_bitrange(get_data_ptr<double>(0), get_data_ptr<double>(1));
            break;
          case PixelDataFormat::BOOL:
            _bitrange = BitRange::FLOAT;
            break;
        }
      }
    } catch (const std::runtime_error &e) {
      _error_msg = e.what();
      return;
    }
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
    switch (dataType) {
      case PixelDataFormat::FLOAT:
        copy_frame<float>(t);
        break;
      case PixelDataFormat::DOUBLE:
        copy_frame<double>(t);
        break;
      case PixelDataFormat::UINT16:
        copy_frame<uint16_t>(t);
        break;
      case PixelDataFormat::UINT8:
        copy_frame<uint8_t>(t);
        break;
      case PixelDataFormat::BOOL:
        copy_frame<bool>(t);
        break;
      default:
        throw std::logic_error("This line should never be reached");
    }
    return _frame;
  };

  float get_pixel(long t, long x, long y) final {
    switch (dataType) {
      case PixelDataFormat::FLOAT:
        return get_data_ptr<float>(t)[y * Nx() + x];
      case PixelDataFormat::DOUBLE:
        return get_data_ptr<double>(t)[y * Nx() + x];
      case PixelDataFormat::UINT16:
        return get_data_ptr<uint16_t>(t)[y * Nx() + x];
      case PixelDataFormat::UINT8:
        return get_data_ptr<uint8_t>(t)[y * Nx() + x];
      case PixelDataFormat::BOOL:
        return get_data_ptr<bool>(t)[y * Nx() + x];
      default:
        throw std::logic_error("This line should never be reached");
    }
  }
};