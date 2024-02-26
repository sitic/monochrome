#pragma once

#include <chrono>
#include <fstream>
#include <numeric>

#include <fmt/format.h>

// only use std filesystem on msvc for now, as gcc / clang sometimes require link options
#if defined(__cplusplus) && _MSC_VER >= 1920
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#endif

#include "mio/mio.hpp"
#include "pugixml.hpp"

using namespace std::chrono_literals;

using uint8  = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

class BmpFileParser {
 public:
  enum class PixelDataFormat : int { UINT8 = 1, UINT16 = 2, FLOAT = 4, DOUBLE = 8 };
  const char Version           = 'f';
  const uint32 ByteOrderMark   = 0x1A2B3C4D;
  const size_t HeaderLength    = 1024;
  const size_t FrameTailLength = sizeof(uint64);

  BmpFileParser(fs::path path) : _in(path.string(), std::ios::in | std::ios::binary) {

    if (!_in.good() || get_filesize() <= HeaderLength) {
      _error_msg = fmt::format("ERROR: Path does not seem to be a file!");
      return;
    }

    char mVersion;
    if (!read(mVersion) || mVersion != Version) {
      _error_msg = fmt::format("Unsupported file version '{}'.", mVersion);
      return;
    }

    uint32 mByteOrderMark;
    if (!read(mByteOrderMark) || mByteOrderMark != ByteOrderMark) {
      _error_msg = fmt::format("Unsupported byte order '{}'", mByteOrderMark);
      return;
    }

    read(mNumFrames);
    read(mFrameWidth);
    read(mFrameHeight);
    read(mFormat);
    if (mFormat == 1) {
      dataType = PixelDataFormat::UINT8;

      // Unfortunately, some old MultiRecoder versions use '1' here for uint16.
      // Try to identify and fix those cases:
      size_t frame_bytes = (mFrameWidth * mFrameHeight) * static_cast<int>(PixelDataFormat::UINT16);
      auto num_frames    = (get_filesize() - HeaderLength) / (frame_bytes + FrameTailLength);
      if (num_frames == mNumFrames) {
        dataType = PixelDataFormat::UINT16;
      }
    } else if (mFormat == 3) {
      dataType = PixelDataFormat::UINT16;
    } else {
      _error_msg = fmt::format(
          "ERROR: Unkown pixel format '{}', expected '3' (for uint16) or '1' (for uint8).", mFormat);
      return;
    }
    mFrameBytes = (mFrameWidth * mFrameHeight) * static_cast<int>(dataType);

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
          "WARNING: Header says there should be {} frames, but found {} in file! "
          "The file might be corrupted.",
          mNumFrames, num_frames);
      mNumFrames = num_frames;
    }

    mRecordingLength = std::chrono::milliseconds(mLastFrameTime - mFirstFrameTime);
    mFPS             = mNumFrames / mRecordingLength.count();

    parse_xml(path);

    std::error_code error;
    _mmap.map(path.string(), HeaderLength, mio::map_entire_file, error);
    if (error) {
      _good      = false;
      _error_msg = error.message();
      return;
    }
    // if we got to this point, this is a valid MultiRecoder header
    _good = _in.good();
    _in.close();
  }

  // Does it appear to be a valid MultiRecorder file?
  bool good() const { return _good; }

  // If good() returns false, there might be an error msg here
  // If good() is true, there might still be a warning msg here
  std::string error_msg() const { return _error_msg; }

  uint32 Nx() const { return mFrameWidth; }
  uint32 Ny() const { return mFrameHeight; }
  long length() const { return mNumFrames; }

  PixelDataFormat dataFormat() const { return dataType; }

  std::string date() const { return mDate; }
  std::string comment() const { return mComment; }
  void set_comment(const std::string &new_comment) {
    mComment = new_comment;
    if (mXML.good) {
      auto backup_path = mXML.path.parent_path() / (mXML.path.filename().string() + ".original");
      if (!fs::is_regular_file(backup_path)) {
        fs::rename(mXML.path, backup_path);
      }

      mXML.doc.child("recordingMetaData")
          .child("general")
          .child("comment")
          .first_child()
          .set_value(new_comment.c_str());
      mXML.doc.save_file(mXML.path.c_str(), " ", pugi::format_indent | pugi::format_no_declaration);
    }
  }
  std::chrono::duration<float> duration() const { return mRecordingLength; }
  float fps() const { return mFPS; }
  std::vector<std::pair<std::string, std::string>> metadata() const { return mMetadata; }

  template <typename T>
  void read_frame(long t, T data) const {
    if (!data) {
      throw std::runtime_error("read_frame() called with nullptr as argument");
    }

    switch (dataType) {
      case PixelDataFormat::UINT16:
        copy_frame<uint16>(t, data);
        break;
      case PixelDataFormat::UINT8:
        copy_frame<uint8>(t, data);
        break;
      default:
        throw std::logic_error("This line should never be reached");
    }
  }

  uint16 get_pixel(long t, long x, long y) const {
    switch (dataType) {
      case PixelDataFormat::UINT16:
        return get_data_ptr<uint16>(t)[y * Nx() + x];
      case PixelDataFormat::UINT8:
        return get_data_ptr<uint8>(t)[y * Nx() + x];
      default:
        throw std::logic_error("This line should never be reached");
    }
  }

 private:
  std::ifstream _in;
  mio::mmap_source _mmap;

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

  BmpFileParser::PixelDataFormat dataType;

  struct {
    pugi::xml_document doc;
    bool good = false;
    fs::path path;
  } mXML;

  bool _good             = false;
  std::string _error_msg = "";

  std::vector<std::pair<std::string, std::string>> mMetadata;

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

  template <typename Scalar>
  const Scalar *get_data_ptr(long t) const {
    auto ptr = _mmap.data() + t * (mFrameBytes + FrameTailLength);
    return reinterpret_cast<const Scalar *>(ptr);
  }

  template <typename T, typename D>
  void copy_frame(long t, D data) const {
    auto begin = get_data_ptr<T>(t);
    std::copy(begin, begin + (mFrameWidth * mFrameHeight), data);
  }

  std::vector<std::string> split_string(std::string_view input, std::string_view delims) {
    std::vector<std::string> output;
    size_t first = 0;

    while (first < input.size()) {
      const auto second = input.find_first_of(delims, first);

      if (first != second) output.emplace_back(input.substr(first, second - first));

      if (second == std::string_view::npos) break;

      first = second + 1;
    }

    return output;
  }

  void parse_xml(const fs::path &dat_path) {
    if (auto parts = split_string(dat_path.stem().string(), "_"); parts.size() > 3) {
      mXML.path = dat_path.parent_path() / fmt::format("{}_{}_{}.xml", parts[0], parts[1], parts[2]);
      if (fs::is_regular_file(mXML.path)) {
        mXML.good = mXML.doc.load_file(mXML.path.c_str(), pugi::parse_full);
        if (mXML.good) {
          auto recordingMetaData = mXML.doc.child("recordingMetaData");
          auto general_section   = recordingMetaData.child("general");
          // sometimes the comment is not saved in the .dat file, only the .xml file
          if (mComment.empty()) {
            mComment = general_section.child("comment").child_value();
          }

          // recording duration is often more accurate in the xml file
          auto recordingTime             = general_section.child("recordingTime");
          std::string recordingTimeUnit  = recordingTime.attribute("unit").value();
          std::string recordingTimeValue = recordingTime.child_value();
          if (recordingTimeUnit == "ms" && !recordingTimeValue.empty()) {
            mRecordingLength = std::chrono::milliseconds(std::stoul(recordingTimeValue));
            mFPS             = mNumFrames / mRecordingLength.count();
          }

          // Concatenate `parts[3:]` with spaces to get camera model name
          std::string camera_model =
              std::accumulate(parts.begin() + 4, parts.end(), parts[3],
                              [](std::string s0, std::string const &s1) { return s0 += " " + s1; });
          auto camera = recordingMetaData.child("Cameras").find_child_by_attribute(
              "Camera", "model", camera_model.c_str());
          auto profile = camera.child("Profile");
          auto get_val = [&](const char *name) -> std::string {
            return profile.find_child_by_attribute("value", "name", name).attribute("value").value();
          };
          auto get_float = [](std::string str) -> std::optional<float> {
            if (str.empty()) return std::nullopt;
            try {
              return std::stof(str);
            } catch (const std::exception) {
              return std::nullopt;
            }
          };

          auto framerate = get_float(get_val("Framerate"));
          if (framerate) mFPS = framerate.value();

          auto gain = get_val("Gain");
          if (!gain.empty()) mMetadata.emplace_back("Gain", gain);

          auto exposure = get_float(get_val("Exposure"));
          if (exposure) {
            mMetadata.emplace_back("Exposure", fmt::format("{:.2f} ms", exposure.value()));
          }
        }
      } else {
        fmt::print("XML file {} not found!\n", mXML.path.string());
      }
    }
  }
};
