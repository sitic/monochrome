#pragma once

#include <regex>

#include "bmp.h"
#include "npy.hpp"

#include "vectors.h"
#include "filereader.h"
#include "globals.h"
#include "iterators.h"

class BmpFileRecording : public AbstractRecording {
 protected:
  BMPheader file;
  Eigen::MatrixXf _frame;

 public:
  BmpFileRecording(const fs::path &path) : AbstractRecording(path), file(path) {
    _frame.setZero(file.Nx(), file.Ny());
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
  std::vector<std::pair<std::string, std::string>> metadata() const final {
    return file.metadata();
  };
  std::optional<BitRange> bitrange() const final {
    if (file.dataFormat() == PixelDataFormat::UINT8) {
      return BitRange::U8;
    } else {
      if (Nx() == 128 && Ny() == 128) {
        // Probably a PVCam recording
        return BitRange::U16;
      } else {
        // Probably a IDS camera recording
        return BitRange::U12;
      }
    }
  }
  std::optional<ColorMap> cmap() const final { return ColorMap::GRAY; }
  bool is_flow() const final { return false; };
  bool set_flow(bool _is_flow) final {
    global::new_ui_message("Unable to set BMP recording as flow");
    return false;
  }
  void set_comment(const std::string &new_comment) final { file.set_comment(new_comment); }
  flag_set<RecordingCapabilities> capabilities() const final {
    return flag_set<RecordingCapabilities>(RecordingCapabilities::SET_COMMENT);
  }

  Eigen::MatrixXf read_frame(long t) final {
    file.read_frame(t, _frame.data());
    return _frame;
  };

  float get_pixel(long t, long x, long y) final { return file.get_pixel(t, x, y); }
};

class RawFileRecording : public AbstractRecording {
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
  flag_set<RecordingCapabilities> capabilities() const final {
    return flag_set<RecordingCapabilities>(RecordingCapabilities::SET_FLOW);
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

class NpyFileRecording : public AbstractRecording {
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
  NpyFileRecording(const fs::path &path) : AbstractRecording(path) {
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

      if (get_dtype<uint8>().str() == header.dtype.str()) {
        dataType = PixelDataFormat::UINT8;
      } else if (get_dtype<uint16>().str() == header.dtype.str()) {
        dataType = PixelDataFormat::UINT16;
      } else if (get_dtype<float>().str() == header.dtype.str()) {
        dataType = PixelDataFormat::FLOAT;
      } else if (get_dtype<double>().str() == header.dtype.str()) {
        dataType = PixelDataFormat::DOUBLE;
      } else {
        _error_msg = fmt::format(
            "numpy dtype {} is unsupported, only uint8, uint16 and float32 are supported",
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
          _error_msg = "Flow fields have to be of shape [T, H, W, 2]";
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
      auto bytes_per_frame = _frame_size * static_cast<int>(dataType);
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
            _bitrange = utils::detect_bitrange(get_data_ptr<uint8>(0), get_data_ptr<uint8>(1));
            break;
          case PixelDataFormat::UINT16:
            _bitrange = utils::detect_bitrange(get_data_ptr<uint16>(0), get_data_ptr<uint16>(1));
            break;
          case PixelDataFormat::FLOAT:
            _bitrange = utils::detect_bitrange(get_data_ptr<float>(0), get_data_ptr<float>(1));
            break;
          case PixelDataFormat::DOUBLE:
            _bitrange = utils::detect_bitrange(get_data_ptr<double>(0), get_data_ptr<double>(1));
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
  flag_set<RecordingCapabilities> capabilities() const final {
    return flag_set<RecordingCapabilities>(RecordingCapabilities::SET_FLOW);
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
        copy_frame<uint16>(t);
        break;
      case PixelDataFormat::UINT8:
        copy_frame<uint8>(t);
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
        return get_data_ptr<uint16>(t)[y * Nx() + x];
      case PixelDataFormat::UINT8:
        return get_data_ptr<uint8>(t)[y * Nx() + x];
      default:
        throw std::logic_error("This line should never be reached");
    }
  }
};
