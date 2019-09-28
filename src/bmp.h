#pragma once

#include <fstream>

#include "filesystem/filesystem.hpp"
#include "definitions.h"

class BMPheader {
private:
  std::ifstream _in;

  uint32 mNumFrames = 0;
  uint32 mFormat;

  uint32 mFrequency = 0;
  uint32 mFrameWidth = 0;
  uint32 mFrameHeight = 0;

  std::string mDate;
  std::string mComment;

  uint64 mFirstFrameTime = 0;
  uint64 mLastFrameTime = 0;
  uint64 mRecordingPeriod = 0;

  size_t mFrameSize = 0;
  size_t mBytesPerPixel = sizeof(pixel);

  template <typename T> bool read(T &x) {
    _in.read(reinterpret_cast<char *>(&x), sizeof(T));
    return _in.good();
  }

  bool _good = false;

public:
  const char Version = 'f';
  const uint32 ByteOrderMark = 0x1A2B3C4D;
  const size_t HeaderLength = 1024;
  const size_t TrailerLength = 0;
  const size_t FrameTailLength = sizeof(uint64);
  const std::string DateFormat = "yyyy-MM-dd hh:mm:ss:zzz";

  BMPheader(filesystem::path path)
      : _in(path.string(), std::ios::in | std::ios::binary) {
    _in.exceptions(std::ifstream::eofbit | std::ifstream::failbit |
                   std::ifstream::badbit);

    if (!_in.good()) {
      fmt::print("ERROR: {} does not seem to be a file!\n", path.string());
    }

    _in.seekg(0);

    char mVersion;
    if (!read(mVersion) || mVersion != Version) {
      fmt::print("Read failed, not a bmp recording?\n");
      return;
    }

    uint32 mByteOrderMark;
    if (!read(mByteOrderMark) || mByteOrderMark != ByteOrderMark) {
      fmt::print("Read failed, not a bmp recording?\n");
      return;
    }

    if (!read(mNumFrames)) {
      return;
    }

    if (!read(mFrameWidth)) {
      return;
    }

    if (!read(mFrameHeight)) {
      return;
    }

    if (!read(mFormat)) {
      return;
    }
    if (mFormat == 0) {
      mFormat = 1;
    }

    if (mFormat != 3) {
      throw std::runtime_error("Only uint16 data supported currently!");
    }

    mFrameSize = (mFrameWidth * mFrameHeight);

    uint32 bin = 0;
    read(bin);
    read(mFrequency);

    auto read_string = [this]() {
      char data;
      std::vector<char> vec;
      do {
        _in.read(&data, sizeof(char));
        if (data != '\0') {
          vec.push_back(data);
        }
      } while (data != '\0');
      return std::string(vec.begin(), vec.end());
    };
    mDate = read_string();
    mComment = read_string();

    //// calculate the recording period, firstFrametime and lastFramtime;
    _in.seekg(HeaderLength + mFrameSize, std::ios::beg);
    read(mFirstFrameTime);

    _in.seekg(0, std::ios::end);
    const long file_size = _in.tellg();
    _in.seekg(file_size - sizeof(uint64), std::ios::beg);
    read(mLastFrameTime);

    mRecordingPeriod = mFirstFrameTime - mLastFrameTime;
    // mFrequency = mRecordingPeriod / 1000 / mNumFrames;

    auto num_frames = (file_size - HeaderLength) /
                      (mFrameSize * mBytesPerPixel + FrameTailLength);
    if (num_frames != mNumFrames) {
      fmt::print("WARNING: Expected {} frames, but only {} present in file!\n",
                 mNumFrames, num_frames);
      mNumFrames = num_frames;
    }

    // if we got to this point, this is a valid MultiRecoder header
    _good = _in.good();
  };

  bool good() { return _good; }

  void read_frame(long t, pixel *data) {
    if (!data) {
      throw std::runtime_error("WTF");
    }

    _in.seekg(HeaderLength +
                  t * (mFrameSize * mBytesPerPixel + FrameTailLength),
              std::ios::beg);
    _in.read(reinterpret_cast<char *>(data), mFrameSize * mBytesPerPixel);
  }

  uint32 Nx() { return mFrameWidth; }
  uint32 Ny() { return mFrameHeight; }
  long length() { return mNumFrames; }

  std::string date() { return mDate; };
  std::string comment() { return mComment; };
};
