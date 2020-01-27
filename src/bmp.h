#pragma once

#include <chrono>
#include <fstream>

#include <fmt/format.h>

#include "definitions.h"
#include "filesystem/filesystem.hpp"

using namespace std::chrono_literals;

class BMPheader {
 private:
  std::ifstream _in;

  uint32 mNumFrames = 0;
  uint32 mFormat;

  uint32 mFrequency   = 0;
  uint32 mFrameWidth  = 0;
  uint32 mFrameHeight = 0;

  std::string mDate;
  std::string mComment;

  uint64 mFirstFrameTime = 0;
  uint64 mLastFrameTime  = 0;
  std::chrono::duration<float> mRecordingLength;
  float mFPS = 0;

  size_t mFrameBytes = 0;

  template <typename T>
  bool read(T &x) {
    if (!_in.good()) {
      return false;
    }

    _in.read(reinterpret_cast<char *>(&x), sizeof(T));
    return _in.good();
  }

  std::string read_string() {
    if (!_in.good()) {
      return "";
    }

    char data;
    std::vector<char> vec;
    do {
      _in.read(&data, sizeof(char));
      if (data != '\0') {
        vec.push_back(data);
      }
    } while (data != '\0');
    return std::string(vec.begin(), vec.end());
  }

  std::size_t get_filesize() {
    auto pos = _in.tellg();
    _in.seekg(0, std::ios::end);
    auto file_size = _in.tellg();
    _in.seekg(pos, std::ios::beg);
    return file_size;
  }

  bool _good             = false;
  std::string _error_msg = "";

 public:
  const char Version           = 'f';
  const uint32 ByteOrderMark   = 0x1A2B3C4D;
  const size_t HeaderLength    = 1024;
  const size_t FrameTailLength = sizeof(uint64);

  BMPheader(filesystem::path path) : _in(path.string(), std::ios::in | std::ios::binary) {

    if (!_in.good() || get_filesize() <= HeaderLength) {
      _error_msg = fmt::format("ERROR: {} does not seem to be a file!", path.string());
      return;
    }

    char mVersion;
    if (!read(mVersion) || mVersion != Version) {
      _error_msg = fmt::format("Parsing '{}' failed, not a bmp recording?", path.string());
      return;
    }

    uint32 mByteOrderMark;
    if (!read(mByteOrderMark) || mByteOrderMark != ByteOrderMark) {
      _error_msg = fmt::format("Parsing '{}' failed, not a bmp recording?", path.string());
      return;
    }

    read(mNumFrames);
    read(mFrameWidth);
    read(mFrameHeight);
    read(mFormat);
    if (mFormat != 3) {
      _error_msg = fmt::format(
          "ERROR: Only uint16 data supported currently, file "
          "header says pixel format is '{}', expected '3'.",
          mFormat);
      return;
    }
    mFrameBytes = (mFrameWidth * mFrameHeight) * sizeof(uint16);

    uint32 bin = 0;
    read(bin);
    read(mFrequency);

    mDate    = read_string();
    mComment = read_string();

    // calculate the recording period, firstFrametime and lastFramtime;
    _in.seekg(HeaderLength + mFrameBytes, std::ios::beg);
    read(mFirstFrameTime);

    _in.seekg(0, std::ios::end);
    const std::streamoff file_size = _in.tellg();
    _in.seekg(file_size - sizeof(uint64), std::ios::beg);
    read(mLastFrameTime);

    auto num_frames = (file_size - HeaderLength) / (mFrameBytes + FrameTailLength);
    if (num_frames != mNumFrames) {
      _error_msg = fmt::format(
          "WARNING: Header says there should be {} frames, but "
          "only {} found in file! The file might be corrupted.",
          mNumFrames, num_frames);
      mNumFrames = num_frames;
    }

    mRecordingLength = std::chrono::milliseconds(mLastFrameTime - mFirstFrameTime);
    mFPS             = mNumFrames / mRecordingLength.count();

    // if we got to this point, this is a valid MultiRecoder header
    _good = _in.good();
  }

  // Does it appear to be a valid MultiRecorder file?
  bool good() const { return _good; }

  // If good() returns false, there might be an error msg here
  // If good() is true, there might still be a warning msg here
  std::string error_msg() const { return _error_msg; }

  uint32 Nx() const { return mFrameWidth; }
  uint32 Ny() const { return mFrameHeight; }
  long length() const { return mNumFrames; }

  std::string date() const { return mDate; }
  std::string comment() const { return mComment; }
  std::chrono::duration<float> duration() const { return mRecordingLength; }
  float fps() const { return mFPS; }

  void read_frame(long t, uint16 *data) {
    if (!data) {
      throw std::runtime_error("read_frame() called with nullptr as argument");
    }

    _in.seekg(HeaderLength + t * (mFrameBytes + FrameTailLength), std::ios::beg);

    if (!_in.good()) {
      throw std::runtime_error("Reading failed!");
    }
    _in.read(reinterpret_cast<char *>(data), mFrameBytes);
  }
};
