#pragma once

#include <fstream>

#include <nlohmann/json.hpp>

#include "BmpFile.h"
#include "CollageFile.h"

// Loader for OmniRecorder `.omni` recordings: a JSON sidecar which ties the per-camera
// .dat files of a multi-camera recording together. Opens all cameras as a collage
// arranged like the OmniRecorder live view (one row for up to 4 cameras, two rows
// otherwise, each camera rotated by its display rotation).
class OmniFile : public CollageFile {
 public:
  explicit OmniFile(const fs::path &path) : CollageFile(path) {
    nlohmann::json j;
    try {
      std::ifstream in(path.string(), std::ios::in | std::ios::binary);
      if (!in.good()) {
        set_error("Unable to open .omni file");
        return;
      }
      j = nlohmann::json::parse(in);
    } catch (const nlohmann::json::exception &e) {
      set_error(fmt::format("Invalid JSON in .omni file: {}", e.what()));
      return;
    }

    std::vector<TileSpec> tiles;
    Options opts;
    try {
      const int version = j.value("omni_version", 1);
      if (version > 1) {
        append_warning(fmt::format(
            "WARNING: .omni version {} is newer than supported (1), attempting to load anyway.",
            version));
      }

      if (!j.contains("cameras") || !j["cameras"].is_array() || j["cameras"].empty()) {
        set_error(".omni file contains no cameras");
        return;
      }
      const auto &cameras = j["cameras"];

      // Grid layout like the OmniRecorder live view: one row for up to 4 cameras,
      // otherwise two rows with the top row getting the extra camera
      const int N   = static_cast<int>(cameras.size());
      const int top = (N <= 4) ? N : (N + 1) / 2;

      for (int i = 0; i < N; i++) {
        const auto &cam    = cameras[i];
        const auto rel     = cam.at("file").get<std::string>();
        const fs::path dat = path.parent_path() / fs::u8path(rel);
        if (!fs::is_regular_file(dat)) {
          set_error(fmt::format("Camera file '{}' referenced in .omni file not found", dat.string()));
          return;
        }
        if (cam.contains("file_size_bytes")) {
          const auto expected = cam["file_size_bytes"].get<uint64_t>();
          const auto actual   = static_cast<uint64_t>(fs::file_size(dat));
          if (actual != expected) {
            append_warning(
                fmt::format("WARNING: '{}' is {} bytes, expected {}. The file may be truncated.",
                            rel, actual, expected));
          }
        }

        TileSpec tile;
        tile.file         = std::make_shared<BmpFile>(dat);
        tile.rotation_deg = cam.value("rotation_deg", 0);
        tile.col          = (i < top) ? i : i - top;
        tile.row          = (i < top) ? 0 : 1;
        tiles.push_back(std::move(tile));
      }

      // Center a shorter bottom row (odd number of cameras) like the OmniRecorder live view
      if (N > 4 && N % 2 == 1 && tiles.front().file->good()) {
        int nx = tiles.front().file->Nx(), ny = tiles.front().file->Ny();
        if (const int rot = ((tiles.front().rotation_deg % 360) + 360) % 360;
            rot == 90 || rot == 270) {
          std::swap(nx, ny);
        }
        const int bottom = N - top;
        const int x_off  = ((top - bottom) * (nx + opts.gap_px)) / 2;
        std::vector<Vec2i> offsets;
        for (const auto &tile : tiles) {
          offsets.push_back({tile.col * (nx + opts.gap_px) + (tile.row == 1 ? x_off : 0),
                             tile.row * (ny + opts.gap_px)});
        }
        opts.pixel_offsets = std::move(offsets);
      }

      opts.date    = j.value("date", "");
      opts.comment = j.contains("meta") ? j["meta"].value("comment", "") : "";

      if (j.contains("exp_nr")) {
        opts.extra_metadata.emplace_back("Experiment", fmt::format("{}", j["exp_nr"].get<int>()));
      }
      if (j.contains("rec_nr")) {
        opts.extra_metadata.emplace_back("Recording", fmt::format("{}", j["rec_nr"].get<int>()));
      }
      opts.extra_metadata.emplace_back("Cameras", fmt::format("{}", N));

      if (j.contains("settings")) {
        const auto &settings = j["settings"];
        opts.fps             = settings.value("fps", 0.0f);
        if (settings.contains("gain")) {
          opts.extra_metadata.emplace_back("Gain",
                                           fmt::format("{}", settings["gain"].get<float>()));
        }
        if (settings.contains("exposure_us")) {
          opts.extra_metadata.emplace_back(
              "Exposure", fmt::format("{} µs", settings["exposure_us"].get<float>()));
        }
      }

      if (j.contains("frames")) {
        const auto &frames = j["frames"];
        opts.duration      = std::chrono::duration<float>(frames.value("duration_s", 0.0f));
        if (frames.contains("count")) {
          opts.length_override = frames["count"].get<long>();
        }
        if (frames.contains("error")) {
          const auto &error = frames["error"];
          opts.extra_metadata.emplace_back(
              "Dropped frames", fmt::format("{}", error.value("count", 0)));
          if (error.contains("by_camera")) {
            for (const auto &[cam_idx, frame_ids] : error["by_camera"].items()) {
              opts.extra_metadata.emplace_back(fmt::format("Dropped (camera {})", cam_idx),
                                               fmt::format("{} frames", frame_ids.size()));
            }
          }
        }
      }

      if (j.contains("geometry")) {
        const auto pixel_format = j["geometry"].value("pixel_format", "");
        if (pixel_format == "uint12") {
          opts.bitrange = BitRange::U12;
        } else if (pixel_format == "uint16") {
          opts.bitrange = BitRange::U16;
        } else if (pixel_format == "uint8") {
          opts.bitrange = BitRange::U8;
        }
      }

      for (int i = 0; i < N; i++) {
        const auto &cam = cameras[i];
        opts.extra_metadata.emplace_back(
            fmt::format("Camera {}", cam.value("index", i)),
            fmt::format("{} ({}, {})", cam.value("name", "?"), cam.value("serial", "?"),
                        cam.value("model", "?")));
      }
    } catch (const nlohmann::json::exception &e) {
      set_error(fmt::format("Failed to parse .omni file: {}", e.what()));
      return;
    }

    // Fall back to the first camera's own values if the .omni lacks them
    if (opts.fps <= 0 && tiles.front().file->good()) {
      opts.fps = tiles.front().file->fps();
    }
    if (opts.duration.count() <= 0 && tiles.front().file->good()) {
      opts.duration = tiles.front().file->duration();
    }

    init(std::move(tiles), std::move(opts));
  }
};
