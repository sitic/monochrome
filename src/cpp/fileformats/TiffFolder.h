#pragma once

#include <strnatcmp.h>

#include "TiffFolder.h"

class TiffFolder : public AbstractFile {
 private:
  bool _good = true;
  std::string _error_msg;
  std::vector<std::shared_ptr<TiffFile>> _files;

  int _nx = 0, _ny = 0, _nc = 0, _nt = 0;
  std::string _date    = "";
  std::string _comment = "";


 public:
  TiffFolder(fs::path path) : AbstractFile(path) {
    std::vector<fs::path> tiff_files;
    const std::unordered_set<std::string> extensions = {".tif", ".tiff", ".TIF", ".TIFF"};
    for (const auto& entry : fs::directory_iterator(path)) {
      if (entry.is_regular_file() && extensions.count(entry.path().extension().string()) > 0) {
        tiff_files.push_back(entry.path());
      }
    }
    std::sort(tiff_files.begin(), tiff_files.end(), [](const fs::path& a, const fs::path& b) {
      std::string a_stem = a.stem().string();
      std::string b_stem = b.stem().string();
      return strnatcmp(a_stem.c_str(), b_stem.c_str()) < 0;
    });
    for (const auto& entry : tiff_files) {
      auto file = std::make_shared<TiffFile>(entry, true);
      _files.push_back(file);
      if (!file->good()) {
        _error_msg = "Failed to load TIFF file: " + entry.string();
        _good      = false;
        break;
      }
      if (file->length() != 1) {
        _error_msg = "Image folder contains multiple frames per file";
        _good      = false;
        break;
      }
    }

    if (_good) {
      _nx      = _files[0]->Nx();
      _ny      = _files[0]->Ny();
      _nc      = _files[0]->Nc();
      _nt      = _files.size();
      _date    = _files[0]->date();
      _comment = _files[0]->comment();
    }
    for (const auto& file : _files) {
      if (file->length() != 1) {
        _error_msg = "Image folder contains multiple frames per file";
        _good      = false;
        break;
      }
      if (file->Nx() != _nx || file->Ny() != _ny || file->Nc() != _nc) {
        _error_msg = "Inconsistent dimensions in TIFF files";
        _good      = false;
        break;
      }
    }
  }


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
    if (!_good) {
      return {};
    } else {
      return _files[0]->metadata();
    }
  }
  std::optional<BitRange> bitrange() const final {
    if (!_good) {
      return std::nullopt;
    } else {
      return _files[0]->bitrange();
    }
  }
  std::optional<ColorMap> cmap() const final {
    if (!_good) {
      return std::nullopt;
    } else {
      return _files[0]->cmap();
    }
  }

  Eigen::MatrixXf read_frame(long t, long c) final {
    if (t < 0 || t >= length()) {
      _error_msg = "Time index out of range";
      _good      = false;
      return Eigen::MatrixXf();
    }
    return _files[t]->read_frame(0, c);
  }

  float get_pixel(long t, long x, long y) final {
    if (t < 0 || t >= length()) {
      _error_msg = "Time index out of range";
      _good      = false;
      return 0;
    }
    return _files[t]->get_pixel(0, x, y);
  }

  //No support for these
  flag_set<FileCapabilities> capabilities() const final { return {}; }
  void set_comment(const std::string& new_comment) final {}
};