#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <utility>

#include <Eigen/Dense>
#include <flag_set.hpp>

#include "utils/definitions.h"
#include "utils/iterators.h"
// only use std filesystem on msvc for now, as gcc / clang sometimes require link options
#if defined(__cplusplus) && _MSC_VER >= 1920
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#endif

using namespace std::chrono_literals;

enum FileCapabilities : uint8_t { AS_FLOW, SET_COMMENT, _ };

class AbstractFile {
 private:
  fs::path _path;

 public:
  AbstractFile(fs::path path) : _path(std::move(path)){};
  virtual ~AbstractFile() = default;
  fs::path path() { return _path; };

  // Is the file loaded correctly and no errors so far?
  virtual bool good() const = 0;
  // If good() == false, an error message may be requested here
  virtual std::string error_msg() = 0;

  // Metadata
  virtual int Nx() const                                                    = 0;
  virtual int Ny() const                                                    = 0;
  virtual int length() const                                                = 0;
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

  [[nodiscard]] virtual Eigen::MatrixXf read_frame(long t)      = 0;
  [[nodiscard]] virtual float get_pixel(long t, long x, long y) = 0;

  // List capabilities
  virtual flag_set<FileCapabilities> capabilities() const = 0;
  // Array is an optical flow
  virtual bool is_flow() const = 0;
  // Mark array as optical flow (only if AS_FLOW capability supported)
  virtual bool set_flow(bool _is_flow) = 0;
  // Set comment (only if SET_COMMENT capability supported)
  virtual void set_comment(const std::string& new_comment) = 0;
};