#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <utility>

#include <Eigen/Dense>
#include <flag_set.hpp>
#include <ghc/fs_std_fwd.hpp>

#include "utils/definitions.h"
#include "utils/iterators.h"
#include "utils/vectors.h"

using namespace std::chrono_literals;

enum FileCapabilities : uint8_t { SET_COMMENT, _ };

class AbstractFile {
 private:
  fs::path _path;

 protected:
  // Error/warning state shared by all file loaders. Semantics (relied on by
  // file_factory()):
  //  - !good(): loading failed, error_msg() is shown as an error in the UI
  //  - good() with non-empty error_msg(): loaded with non-fatal warnings,
  //    which are still shown in the UI
  // Files start out !good(); constructors call set_good() on success.
  bool _good             = false;
  std::string _error_msg = "";

  void set_good() { _good = true; }
  void set_error(std::string msg) {
    _good      = false;
    _error_msg = std::move(msg);
  }
  void append_warning(const std::string &msg) {
    if (!_error_msg.empty()) _error_msg += "\n";
    _error_msg += msg;
  }

 public:
  AbstractFile(fs::path path) : _path(std::move(path)) {};
  virtual ~AbstractFile() = default;
  fs::path path() const { return _path; };

  // Is the file loaded correctly and no errors so far?
  virtual bool good() const { return _good; }
  // Error or warning messages, see above
  virtual std::string error_msg() { return _error_msg; }

  // Metadata
  virtual int Nx() const     = 0;  // width
  virtual int Ny() const     = 0;  // height
  virtual int Nc() const     = 0;  // number of channels
  virtual int length() const = 0;  // number of frames

  virtual std::string date() const                                          = 0;
  virtual std::string comment() const                                       = 0;
  virtual std::chrono::duration<float> duration() const                     = 0;
  virtual float fps() const                                                 = 0;
  virtual std::vector<std::pair<std::string, std::string>> metadata() const = 0;
  virtual std::optional<BitRange> bitrange() const                          = 0;
  virtual std::optional<ColorMap> cmap() const                              = 0;
  virtual std::optional<float> vmin() const { return std::nullopt; };
  virtual std::optional<float> vmax() const { return std::nullopt; };
  virtual std::optional<OpacityFunction> opacity() const { return std::nullopt; };

  [[nodiscard]] virtual Eigen::MatrixXf read_frame(long t, long c) = 0;
  // Get pixel value at position (x, y) in frame t
  [[nodiscard]] virtual float get_pixel(long t, long x, long y) = 0;
  // Get the average value in a 2D image block
  [[nodiscard]] virtual float get_block(long t, const Vec2i& start, const Vec2i& size) {
    float sum = 0;
    for (int y = start[1]; y < start[1] + size[1]; ++y) {
      for (int x = start[0]; x < start[0] + size[0]; ++x) {
        sum += get_pixel(t, x, y);
      }
    }
    return sum / (size[0] * size[1]);
  }
  // Get a trace of the data, averaged over a region of interest
  [[nodiscard]] virtual std::vector<float> get_trace(Vec2i start, Vec2i size) {
    std::vector<float> trace(length());
    for (int t = 0; t < length(); t++) {
      trace[t] = get_block(t, start, size);
    }
    return trace;
  }

  // List capabilities
  virtual flag_set<FileCapabilities> capabilities() const = 0;
  // Set comment (only if SET_COMMENT capability supported)
  virtual void set_comment(const std::string& new_comment) = 0;
};