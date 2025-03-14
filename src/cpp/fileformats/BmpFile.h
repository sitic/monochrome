#pragma once

#include "AbstractFile.h"
#include "BmpFileParser.h"
#include "globals.h"

class BmpFile : public AbstractFile {
 protected:
  BmpFileParser file;
  Eigen::MatrixXf _frame;

 public:
  BmpFile(const fs::path &path) : AbstractFile(path), file(path) {
    _frame.setZero(file.Nx(), file.Ny());
  }

  bool good() const final { return file.good(); };
  int Nx() const final { return file.Nx(); };
  int Ny() const final { return file.Ny(); };
  int Nc() const final { return 1; };
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
    if (file.dataFormat() == BmpFileParser::PixelDataFormat::UINT8) {
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
  void set_comment(const std::string &new_comment) final { file.set_comment(new_comment); }
  flag_set<FileCapabilities> capabilities() const final {
    return flag_set<FileCapabilities>(FileCapabilities::SET_COMMENT);
  }

  Eigen::MatrixXf read_frame(long t, long c) final {
    (void)c; // only one channel supported
    file.read_frame(t, _frame.data());
    return _frame;
  };

  float get_pixel(long t, long x, long y) final { return file.get_pixel(t, x, y); }

  float get_block(long t, const Vec2i& start, const Vec2i& size) final {
    // std::visit pattern throws C1001 error in MSVC, so we use std::get_if instead
    auto data_variant = file.get_frame_ptr(t);
    if (auto ptr = std::get_if<const uint16_t*>(&data_variant)) {
      Eigen::Map<const Eigen::Matrix<uint16_t, Eigen::Dynamic, Eigen::Dynamic>> frame(*ptr, Nx(), Ny());
      return frame.block(start[0], start[1], size[0], size[1]).template cast<float>().mean();
    } else if (auto ptr = std::get_if<const uint8_t*>(&data_variant)) {
      Eigen::Map<const Eigen::Matrix<uint8_t, Eigen::Dynamic, Eigen::Dynamic>> frame(*ptr, Nx(), Ny());
      return frame.block(start[0], start[1], size[0], size[1]).template cast<float>().mean();
    } else {
      throw std::runtime_error("Unsupported pixel type");
    }
  }
};
