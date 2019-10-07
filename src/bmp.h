#pragma once

#include <chrono>
#include <fstream>
using namespace std::chrono_literals;

#include "definitions.h"
#include "filesystem/filesystem.hpp"

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
  std::chrono::duration<float> mRecordingLength;
  float mFPS = 0;

  size_t mFrameBytes = 0;

  template <typename T> bool read(T &x) {
    _in.read(reinterpret_cast<char *>(&x), sizeof(T));
    return _in.good();
  }

  std::string read_string() {
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

  bool _good = false;

public:
  const char Version = 'f';
  const uint32 ByteOrderMark = 0x1A2B3C4D;
  const size_t HeaderLength = 1024;
  const size_t FrameTailLength = sizeof(uint64);

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
      fmt::print("Parsing '{}' failed, not a bmp recording?\n", path.string());
      return;
    }

    uint32 mByteOrderMark;
    if (!read(mByteOrderMark) || mByteOrderMark != ByteOrderMark) {
      fmt::print("Parsing '{}' failed, not a bmp recording?\n", path.string());
      return;
    }

    read(mNumFrames);
    read(mFrameWidth);
    read(mFrameHeight);
    read(mFormat);
    if (mFormat != 3) {
      auto msg =
          fmt::format("ERROR: Only uint16 data supported currently, file "
                      "header says pixel format is '{}', expected '3'.",
                      mFormat);
      throw std::runtime_error(msg);
    }
    mFrameBytes = (mFrameWidth * mFrameHeight) * sizeof(uint16);

    uint32 bin = 0;
    read(bin);
    read(mFrequency);

    mDate = read_string();
    mComment = read_string();

    // calculate the recording period, firstFrametime and lastFramtime;
    _in.seekg(HeaderLength + mFrameBytes, std::ios::beg);
    read(mFirstFrameTime);

    _in.seekg(0, std::ios::end);
    const long file_size = _in.tellg();
    _in.seekg(file_size - sizeof(uint64), std::ios::beg);
    read(mLastFrameTime);

    auto num_frames =
        (file_size - HeaderLength) / (mFrameBytes + FrameTailLength);
    if (num_frames != mNumFrames) {
      fmt::print("WARNING: Expected {} frames, but only {} present in file!\n",
                 mNumFrames, num_frames);
      mNumFrames = num_frames;
    }

    mRecordingLength =
        std::chrono::milliseconds(mLastFrameTime - mFirstFrameTime);
    mFPS = mNumFrames / mRecordingLength.count();

    // if we got to this point, this is a valid MultiRecoder header
    _good = _in.good();
  };

  // Does it appear to be a valid MultiRecorder file?
  bool good() { return _good; }

  uint32 Nx() { return mFrameWidth; }
  uint32 Ny() { return mFrameHeight; }
  long length() { return mNumFrames; }

  std::string date() { return mDate; };
  std::string comment() { return mComment; };
  std::chrono::duration<float> duration() { return mRecordingLength; };
  float fps() { return mFPS; }

  void read_frame(long t, uint16 *data) {
    if (!data) {
      throw std::runtime_error("read_frame() called with nullptr as argument");
    }

    _in.seekg(HeaderLength + t * (mFrameBytes + FrameTailLength),
              std::ios::beg);
    _in.read(reinterpret_cast<char *>(data), mFrameBytes);
  }
};
