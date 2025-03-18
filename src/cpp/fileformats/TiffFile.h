#include <stdexcept>
#include <memory>
#include <utility>
#include <mutex>

#include <tiffio.h>
#include <tiffio.hxx>

#include "AbstractFile.h"
#include "utils/iterators.h"

struct TiffErrorData {
  bool* good;
  std::string* error_msg;
};

static int tiffErrorHandler(TIFF* tiff, void* user_data, const char* module, const char* fmt, va_list ap) {
  TiffErrorData* error_data = static_cast<TiffErrorData*>(user_data);
  
  char buffer[256];
  vsnprintf(buffer, sizeof(buffer), fmt, ap);
  
  *(error_data->error_msg) = module ? 
                            std::string(module) + ": " + std::string(buffer) : 
                            std::string(buffer);
  *(error_data->good) = false;
  
  return 1;
}

class TiffFile : public AbstractFile {
 private:
  bool _good = true;
  std::string _error_msg;
  std::mutex _mutex;

  TIFF* tif;

  std::vector<bool> frames_in_cache;
  std::vector<uint8_t> cache;
  uint64_t frame_size = 0;

  int _nx               = 0;
  int _ny               = 0;
  int _nc               = 0;
  int _nt               = 0;
  uint16_t _bits        = 0;
  uint16_t sformat      = 0;
  uint16_t photometric  = PHOTOMETRIC_MINISWHITE;
  uint16_t compress     = COMPRESSION_NONE;
  uint16_t planarconfig = PLANARCONFIG_CONTIG;
  std::string _comment  = "";
  std::string _date     = "";

  Eigen::MatrixXf _frame;

  bool parse_tiff(bool quick_init = false) {
    if (!tif) {
      _error_msg = "Failed to open TIFF file.";
      return false;
    }

    if (!TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &_nx) ||
        !TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &_ny)) {
      _error_msg = "Could not read image dimensions";
      return false;
    }
    _nc = TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLESPERPIXEL, &_nc);
    _nt = TIFFNumberOfDirectories(tif);
    if (_nx <= 0 || _ny <= 0 || _nc <= 0 || _nt <= 0) {
      _error_msg = "Invalid dimensions";
      return false;
    }
    if (_nc != 1 && _nc != 3 && _nc != 4) {
      _error_msg = "Unsupported number of channels";
      return false;
    }

    TIFFGetFieldDefaulted(tif, TIFFTAG_BITSPERSAMPLE, &_bits);
    if (_bits != 1 && _bits != 8 && _bits != 16 && _bits != 32) {
      _error_msg = "Unsupported bits per sample";
      return false;
    }
    frame_size = _nx * _ny * _nc * (_bits / 8);
    fmt::print("Tiff file shape: {} x {} x {} x {}, {}bits\n", _nt, _nx, _ny, _nc, _bits);

    TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLEFORMAT, &sformat);
    if (sformat != SAMPLEFORMAT_UINT && sformat != SAMPLEFORMAT_INT &&
        sformat != SAMPLEFORMAT_IEEEFP) {
      _error_msg = fmt::format("Unsupported sample format: {}", sformat);
      return false;
    }

    TIFFGetFieldDefaulted(tif, TIFFTAG_PHOTOMETRIC, &photometric);
    if (photometric != PHOTOMETRIC_MINISBLACK && photometric != PHOTOMETRIC_MINISWHITE &&
        photometric != PHOTOMETRIC_RGB) {
      _error_msg = fmt::format("Unsupported photometric interpretation: {}", photometric);
      return false;
    }
    TIFFGetFieldDefaulted(tif, TIFFTAG_COMPRESSION, &compress);
    if (TIFFIsCODECConfigured(compress) != 1) {
      _error_msg = "Unsupported compression type";
      return false;
    }
    TIFFGetFieldDefaulted(tif, TIFFTAG_PLANARCONFIG, &planarconfig);
    if (planarconfig != PLANARCONFIG_CONTIG && planarconfig != PLANARCONFIG_SEPARATE) {
      _error_msg = "Unsupported planar configuration";
      return false;
    }
    if (TIFFIsTiled(tif)) {
      _error_msg = "Tiled TIFF files are not supported";
      return false;
    }
    // if (TIFFIsByteSwapped(tif)) {
    //     _error_msg = "Byte-swapped TIFF files are not supported";
    //     return false;
    // }
    if (!TIFFIsMSB2LSB(tif)) {
      _error_msg = "!MSB2LSB TIFF files are not supported";
      return false;
    }

    if (char* description = nullptr; TIFFGetField(tif, TIFFTAG_IMAGEDESCRIPTION, &description)) {
      _comment = std::string(description);
    }
    if (char* datetime = nullptr; TIFFGetField(tif, TIFFTAG_DATETIME, &datetime))
      _date = std::string(datetime);

    _frame.resize(_nx, _ny);
    frames_in_cache.resize(_nt, false);
    cache.resize(_nt * _nx * _ny * _nc * (_bits / 8));
    return add_frame_to_cache(0, quick_init);
  }

  void close() {
    if (tif) {
      try {
        TIFFClose(tif);
      } catch (std::exception& e) {
        _error_msg = fmt::format("Failed to close TIFF file: {}", e.what());
        _good      = false;
        fmt::print("Failed to close TIFF file: {}\n", e.what());
      }
      tif = nullptr;
    }
  }

 public:
  TiffFile(fs::path path, bool fast_init_check = false) : AbstractFile(path) {
    // Setup error handler
    TiffErrorData error_data;
    error_data.good = &_good;
    error_data.error_msg = &_error_msg;
    TIFFOpenOptions* opts = TIFFOpenOptionsAlloc();
    TIFFOpenOptionsSetErrorHandlerExtR(opts, tiffErrorHandler, &error_data);
    
    // Open TIFF file
    tif = TIFFOpenExt(path.string().c_str(), "r", opts);
    TIFFOpenOptionsFree(opts);
    
    if (!parse_tiff(fast_init_check)) {
      _good = false;
      close();
    };
  }
  ~TiffFile() { close(); }


  bool good() const final { return _good; }

  std::string error_msg() final { return _error_msg; }

  int Nx() const final { return _nx; }
  int Ny() const final { return _ny; }
  int Nc() const final { return _nc; }
  int length() const final { return _nt; }

  std::string date() const final { return _date; }
  std::string comment() const final { return _comment; }
  std::chrono::duration<float> duration() const final { return 0s; }
  float fps() const final { return 0; }
  std::vector<std::pair<std::string, std::string>> metadata() const final {
    return {};
  }  // TODO add more metadata
  std::optional<BitRange> bitrange() const final {
    if (_bits == 8) {
      return BitRange::U8;
    } else if (_bits == 16) {
      return BitRange::U16;
    } else if (_bits == 32) {
      return BitRange::FLOAT;
    }
    return std::nullopt;
  }
  std::optional<ColorMap> cmap() const final {
    if (photometric == PHOTOMETRIC_MINISBLACK) {
      return ColorMap::GRAY;
    } else if (photometric == PHOTOMETRIC_MINISWHITE) {
      return ColorMap::GRAY; // TODO: GRAY_REVERSED
    }
    return std::nullopt;
  }

  bool set_directory(long t) {
    if (TIFFCurrentDirectory(tif) == t) {
      return true;
    }
    if (!TIFFSetDirectory(tif, t)) {
      _error_msg = "Failed to set directory in TIFF file";
      return false;
    }
    int l_nx, l_ny, l_nc = _nc, l_bits = _bits;
    if (!TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &l_nx) || _nx != l_nx ||
        !TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &l_ny) || _ny != l_ny) {
      _error_msg = "TIFF file images have different dimensions";
      return false;
    }
    TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLESPERPIXEL, &l_nc);
    TIFFGetFieldDefaulted(tif, TIFFTAG_BITSPERSAMPLE, &l_bits);
    if (l_nc != _nc || l_bits != _bits) {
      _error_msg = "TIFF file images have different number of channels or bits per sample.";
      return false;
    }
    return true;
  }

  bool add_frame_to_cache(long t, bool init_check = false) {
    if (t < 0 || t >= length()) {
      _error_msg = "Time index out of range";
      _good      = false;
      return false;
    }
    if (length() != 1) {
      set_directory(t);
    }
    try {
      if (planarconfig == PLANARCONFIG_CONTIG) {
        uint64_t row_size = _nx * _nc * (_bits / 8);
        if (row_size != TIFFScanlineSize(tif)) {
          _error_msg = "Buffer size mismatch";
          _good      = false;
          return false;
        }
        auto cache_ptr = cache.data() + t * frame_size;
        for (uint32_t row = 0; row < _ny; row++) {
          if (!TIFFReadScanline(tif, cache_ptr, row, 0)) {
            _error_msg = "Failed to read scanline";
            _good      = false;
            return false;
          }
          cache_ptr += row_size;
          if (init_check) return true;
        }
      } else if (planarconfig == PLANARCONFIG_SEPARATE) {
        uint64_t row_size = _nx * (_bits / 8);
        if (row_size != TIFFScanlineSize(tif)) {
          _error_msg = "Buffer size mismatch";
          _good      = false;
          return false;
        }
        auto cache_ptr = cache.data() + t * frame_size;
        for (uint32_t c = 0; c < _nc; c++) {
          for (uint32_t row = 0; row < _ny; row++) {
            if (!TIFFReadScanline(tif, cache_ptr, row, c)) {
              _error_msg = "Failed to read scanline";
              _good      = false;
              return false;
            }
            cache_ptr += row_size;
          }
          if (init_check) return true;
        }
      }
    } catch (const std::exception& e) {
      _error_msg = fmt::format("Error reading TIFF file: {}", e.what());
      _good      = false;
      return false;
    }
    frames_in_cache[t] = true;
    return true;
  }

  template <typename Scalar>
  void _copy_frame(uint32_t t, uint32_t c) {
    auto cache_ptr = cache.data() + t * frame_size;
    auto in_ptr    = reinterpret_cast<const Scalar*>(cache_ptr);
    if (Nc() == 1 || planarconfig == PLANARCONFIG_SEPARATE) {
      auto begin = in_ptr + c * _nx * _ny;
      auto end   = begin + _nx * _ny;
      std::copy(begin, end, _frame.data());
    } else {
      auto begin = in_ptr + c;
      auto end   = begin + _nx * _nc;
      std::copy(StrideIterator(begin, _nc), StrideIterator(end, _nc), _frame.data());
    }
  }

  Eigen::MatrixXf read_frame(long t, long c) final {
    std::lock_guard<std::mutex> lock(_mutex);

    if (t < 0 || t >= length()) {
      _error_msg = "Time index out of range";
      _good      = false;
      return _frame;
    }
    if (c < 0 || c >= Nc()) {
      _error_msg = "Channel index out of range";
      _good      = false;
      return _frame;
    }

    if (!frames_in_cache[t]) {
      if (!add_frame_to_cache(t)) {
        return _frame;
      }
    }
    switch (_bits) {
      case 1:
        _copy_frame<bool>(t, c);
        break;
      case 8:
        _copy_frame<uint8_t>(t, c);
        break;
      case 16:
        _copy_frame<uint16_t>(t, c);
        break;
      case 32:
        _copy_frame<float>(t, c);
        break;
      default:
        _error_msg = "Unsupported bits per sample";
        _good      = false;
    }

    if (t == length() - 1 &&
        std::all_of(frames_in_cache.begin(), frames_in_cache.end(), [](bool b) { return b; })) {
      // all frames are in cache, so we can close the file
      close();
    }
    return _frame;
  }

  template <typename Scalar>
  float _get_pixel(long t, long x, long y, long c) {
    auto cache_ptr = cache.data() + t * frame_size;
    auto ptr       = reinterpret_cast<const Scalar*>(cache_ptr);
    if (planarconfig == PLANARCONFIG_SEPARATE) {
      ptr += c * _nx * _ny;
      return ptr[y * _nx + x];
    } else {
      return ptr[y * _nx * _nc + x * _nc + c];
    }
  }

  float get_pixel(long t, long x, long y) final {
    std::lock_guard<std::mutex> lock(_mutex);
    
    if (!frames_in_cache[t]) {
      add_frame_to_cache(t);
    }
    const long c = 0;
    switch (_bits) {
      case 1:
        return _get_pixel<bool>(t, x, y, c);
      case 8:
        return _get_pixel<uint8_t>(t, x, y, c);
      case 16:
        return _get_pixel<uint16_t>(t, x, y, c);
      case 32:
        return _get_pixel<float>(t, x, y, c);
      default:
        _error_msg = "Unsupported bits per sample";
        _good      = false;
    }
    return 0;
  }

  //No support for these
  flag_set<FileCapabilities> capabilities() const final { return {}; }
  void set_comment(const std::string& new_comment) final {}
};