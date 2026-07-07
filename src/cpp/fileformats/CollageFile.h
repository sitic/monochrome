#pragma once

#include <algorithm>
#include <limits>
#include <set>

#include <fmt/format.h>

#include "AbstractFile.h"

// Virtual video which arranges multiple single-channel videos in a grid ("collage"),
// e.g. all cameras of a multi-camera recording. The children stay memory-mapped; only
// the currently composed frame is held in memory.
class CollageFile : public AbstractFile {
 public:
  struct TileSpec {
    std::shared_ptr<AbstractFile> file;
    int col          = 0;      // grid cell (0-based)
    int row          = 0;
    int rotation_deg = 0;      // display rotation, multiple of 90, clockwise-positive
    bool fliplr      = false;  // horizontal mirror, applied after rotation
  };

  struct Options {
    int gap_px      = 2;     // spacing between tiles
    float gap_value = 0.0f;  // pixel value of the spacing / uncovered area
    std::string date;
    std::string comment;
    float fps                             = 0;
    std::chrono::duration<float> duration = 0s;
    std::optional<BitRange> bitrange;  // fallback: first tile's bitrange
    std::optional<ColorMap> colormap = ColorMap::GRAY;
    std::vector<std::pair<std::string, std::string>> extra_metadata;
    std::optional<long> length_override;
    // Explicit per-tile top-left corners (x, y), overrides the col/row grid layout
    // (e.g. to center a shorter bottom row)
    std::optional<std::vector<Vec2i>> pixel_offsets;
  };

  CollageFile(fs::path path, std::vector<TileSpec> tiles, Options opts)
      : AbstractFile(std::move(path)) {
    init(std::move(tiles), std::move(opts));
  }

  int Nx() const final { return _nx; }
  int Ny() const final { return _ny; }
  int Nc() const final { return 1; }
  int length() const final { return _nt; }

  std::string date() const final { return _opts.date; }
  std::string comment() const final { return _opts.comment; }
  std::chrono::duration<float> duration() const final { return _opts.duration; }
  float fps() const final { return _opts.fps; }
  std::vector<std::pair<std::string, std::string>> metadata() const final {
    return _opts.extra_metadata;
  }
  std::optional<BitRange> bitrange() const final {
    if (_opts.bitrange) return _opts.bitrange;
    return _tiles.empty() ? std::nullopt : _tiles.front().file->bitrange();
  }
  std::optional<ColorMap> cmap() const final { return _opts.colormap; }

  flag_set<FileCapabilities> capabilities() const final { return {}; }
  void set_comment(const std::string &) final {}

  Eigen::MatrixXf read_frame(long t, long c) final {
    (void)c;  // only one channel supported
    if (_nt <= 0) return _collage;
    t = std::clamp(t, 0L, _nt - 1);
    if (t == _cached_t) return _collage;

    for (const auto &tile : _tiles) {
      Eigen::MatrixXf f = tile.file->read_frame(t, 0);
      auto block        = _collage.block(tile.x0, tile.y0, tile.nx, tile.ny);
      switch (tile.rot) {
        case 90:
          block = f.transpose().colwise().reverse();
          break;
        case 180:
          block = f.reverse();
          break;
        case 270:
          block = f.transpose().rowwise().reverse();
          break;
        default:
          block = f;
          break;
      }
      if (tile.fliplr) {
        block.colwise().reverseInPlace();
      }
    }
    _cached_t = t;
    return _collage;
  }

  float get_pixel(long t, long x, long y) final {
    if (const Tile *tile = find_tile(x, y)) {
      auto raw = to_child_coords(*tile, x - tile->x0, y - tile->y0);
      return tile->file->get_pixel(t, raw[0], raw[1]);
    }
    return _opts.gap_value;
  }

  // Decompose the block into per-tile intersections so single-tile regions (the common
  // case for traces) hit the childrens' fast mmap-based implementations
  float get_block(long t, const Vec2i &start, const Vec2i &size) final {
    const long total_area = static_cast<long>(size[0]) * size[1];
    if (total_area <= 0) return _opts.gap_value;

    double sum    = 0;
    long uncovered = total_area;
    for (const auto &tile : _tiles) {
      const int u0 = std::max(start[0], tile.x0);
      const int u1 = std::min(start[0] + size[0], tile.x0 + tile.nx);
      const int v0 = std::max(start[1], tile.y0);
      const int v1 = std::min(start[1] + size[1], tile.y0 + tile.ny);
      if (u0 >= u1 || v0 >= v1) continue;

      // Map the intersection's extreme corners into the child's raw coordinates
      const Vec2i a = to_child_coords(tile, u0 - tile.x0, v0 - tile.y0);
      const Vec2i b = to_child_coords(tile, u1 - 1 - tile.x0, v1 - 1 - tile.y0);
      const Vec2i child_start = {std::min(a[0], b[0]), std::min(a[1], b[1])};
      const Vec2i child_size  = {std::abs(a[0] - b[0]) + 1, std::abs(a[1] - b[1]) + 1};

      const long area = static_cast<long>(child_size[0]) * child_size[1];
      sum += static_cast<double>(tile.file->get_block(t, child_start, child_size)) * area;
      uncovered -= area;
    }
    sum += static_cast<double>(_opts.gap_value) * uncovered;
    return static_cast<float>(sum / total_area);
  }

 protected:
  // For subclasses which need to parse a description file before calling init()
  explicit CollageFile(fs::path path) : AbstractFile(std::move(path)) {}

  void init(std::vector<TileSpec> tiles, Options opts) {
    _opts = std::move(opts);
    if (tiles.empty()) {
      set_error("Collage contains no videos");
      return;
    }
    if (_opts.pixel_offsets && _opts.pixel_offsets->size() != tiles.size()) {
      set_error("Number of pixel offsets does not match number of collage tiles");
      return;
    }

    // Validate children and compute the common (rotated) tile size
    int tile_nx = -1, tile_ny = -1;
    long min_len = std::numeric_limits<long>::max(), max_len = 0;
    int n_cols = 0, n_rows = 0;
    std::set<std::pair<int, int>> cells;
    for (const auto &spec : tiles) {
      if (!spec.file) {
        set_error("Collage tile has no video");
        return;
      }
      if (!spec.file->good()) {
        set_error(fmt::format("Failed to load '{}': {}", spec.file->path().filename().string(),
                              spec.file->error_msg()));
        return;
      }
      if (spec.file->Nc() != 1) {
        set_error(fmt::format("Only single-channel videos are supported in a collage, '{}' has {}",
                              spec.file->path().filename().string(), spec.file->Nc()));
        return;
      }
      if (spec.rotation_deg % 90 != 0) {
        set_error(fmt::format("Invalid rotation {}°, has to be a multiple of 90°",
                              spec.rotation_deg));
        return;
      }
      if (spec.col < 0 || spec.row < 0 || !cells.emplace(spec.col, spec.row).second) {
        set_error(fmt::format("Invalid or duplicate grid cell ({}, {})", spec.col, spec.row));
        return;
      }

      const short rot = static_cast<short>(((spec.rotation_deg % 360) + 360) % 360);
      int nx = spec.file->Nx(), ny = spec.file->Ny();
      if (rot == 90 || rot == 270) std::swap(nx, ny);
      if (tile_nx < 0) {
        tile_nx = nx;
        tile_ny = ny;
      } else if (nx != tile_nx || ny != tile_ny) {
        set_error(fmt::format(
            "Collage videos have inconsistent dimensions after rotation: {}x{} vs {}x{}", tile_nx,
            tile_ny, nx, ny));
        return;
      }

      min_len = std::min(min_len, static_cast<long>(spec.file->length()));
      max_len = std::max(max_len, static_cast<long>(spec.file->length()));
      n_cols  = std::max(n_cols, spec.col + 1);
      n_rows  = std::max(n_rows, spec.row + 1);
      _tiles.push_back({spec.file, rot, spec.fliplr, 0, 0, nx, ny});
    }

    // Tile positions and overall size
    if (_opts.pixel_offsets) {
      for (size_t i = 0; i < _tiles.size(); i++) {
        _tiles[i].x0 = (*_opts.pixel_offsets)[i][0];
        _tiles[i].y0 = (*_opts.pixel_offsets)[i][1];
        if (_tiles[i].x0 < 0 || _tiles[i].y0 < 0) {
          set_error("Negative collage tile offset");
          return;
        }
        _nx = std::max(_nx, _tiles[i].x0 + _tiles[i].nx);
        _ny = std::max(_ny, _tiles[i].y0 + _tiles[i].ny);
      }
    } else {
      for (size_t i = 0; i < _tiles.size(); i++) {
        _tiles[i].x0 = tiles[i].col * (tile_nx + _opts.gap_px);
        _tiles[i].y0 = tiles[i].row * (tile_ny + _opts.gap_px);
      }
      _nx = n_cols * tile_nx + (n_cols - 1) * _opts.gap_px;
      _ny = n_rows * tile_ny + (n_rows - 1) * _opts.gap_px;
    }

    // Frame count: be defensive about truncated files
    if (min_len != max_len) {
      append_warning(fmt::format(
          "WARNING: Collage videos have differing frame counts ({} to {}), some files may be "
          "truncated. Using the shortest.",
          min_len, max_len));
    }
    _nt = min_len;
    if (_opts.length_override) {
      if (*_opts.length_override > min_len) {
        append_warning(fmt::format(
            "WARNING: Recording metadata expects {} frames but the shortest video has only {}.",
            *_opts.length_override, min_len));
      }
      _nt = std::min(_nt, *_opts.length_override);
    }
    if (_nt <= 0) {
      set_error("Collage contains no frames");
      return;
    }

    // Gap pixels are only written once, tiles always overwrite the same blocks
    _collage.setConstant(_nx, _ny, _opts.gap_value);
    set_good();
  }

 private:
  struct Tile {
    std::shared_ptr<AbstractFile> file;
    short rot;    // normalized rotation: 0, 90, 180 or 270 (clockwise)
    bool fliplr;
    int x0, y0;   // top-left corner within the collage
    int nx, ny;   // tile size after rotation
  };

  const Tile *find_tile(long x, long y) const {
    for (const auto &tile : _tiles) {
      if (x >= tile.x0 && x < tile.x0 + tile.nx && y >= tile.y0 && y < tile.y0 + tile.ny) {
        return &tile;
      }
    }
    return nullptr;
  }

  // Map local display coordinates (u, v) within a tile to the child's raw coordinates,
  // inverting fliplr and rotation
  static Vec2i to_child_coords(const Tile &tile, int u, int v) {
    if (tile.fliplr) u = tile.nx - 1 - u;
    const int W = tile.file->Nx(), H = tile.file->Ny();
    switch (tile.rot) {
      case 90:
        return {v, H - 1 - u};
      case 180:
        return {W - 1 - u, H - 1 - v};
      case 270:
        return {W - 1 - v, u};
      default:
        return {u, v};
    }
  }

  std::vector<Tile> _tiles;
  int _nx = 0, _ny = 0;
  long _nt = 0;
  Options _opts;
  Eigen::MatrixXf _collage;
  long _cached_t = -1;
};
