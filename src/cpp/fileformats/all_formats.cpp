#include <string>
#include <sstream>
#include <unordered_set>
#include <random>

// Asio needs to be imported before windows.h
#include <asio.hpp>
#include <subprocess.hpp>

#include "BmpFile.h"
#include "RawFile.h"
#include "NpyFile.h"
#include "TiffFile.h"
#include "TiffFolder.h"

#include "all_formats.h"
#include "utils/files.h"

namespace {
  void python_plugin_load(const fs::path& path, std::string script_name);

  bool substring_replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos) return false;
    str.replace(start_pos, from.length(), to);
    return true;
  }

  std::string random_alphanumeric_string(size_t length) {
    thread_local std::mt19937 rng{std::random_device{}()};
    auto randchar = []() -> char {
      const char charset[] =
          "0123456789"
          "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
          "abcdefghijklmnopqrstuvwxyz";
      const size_t max_index = (sizeof(charset) - 1);
      std::uniform_int_distribution<size_t> dist(0, max_index - 1);
      return charset[dist(rng)];
    };
    std::string str(length, 0);
    std::generate_n(str.begin(), length, randchar);
    return str;
  }

  bool directory_has_files_with_extension(const fs::path& path,
                                          const std::unordered_set<std::string>& extensions) {
    for (const auto& entry : fs::directory_iterator(path)) {
      if (entry.is_regular_file() && extensions.count(entry.path().extension().string()) > 0) {
        return true;
      }
    }
    return false;
  }
}  // namespace

std::shared_ptr<AbstractFile> file_factory(const fs::path& path) {
  std::shared_ptr<AbstractFile> file;

  // Directory loaders
  if (fs::is_directory(path)) {
    // Native directory handlers
    if (directory_has_files_with_extension(path, {".tif", ".tiff", ".TIF", ".TIFF"})) {
      file = std::make_shared<TiffFolder>(path);
      if (!file->good()) {
        // Load failed, let the "load_folder.py" plugin handle it
        file = nullptr;
      }
    }
    // Python-based directory handlers
    using file_extension_vt = std::unordered_set<std::string>;
    const std::unordered_map<std::string, file_extension_vt> folder_handlers = {
        {"load_folder.py", {".png", ".tif", ".tiff", ".PNG", ".TIF", ".TIFF"}},
        {"load_folder_dcm.py", {".dcm", ".DCM"}},
    };
    if (!file) {
      for (const auto& [script_name, extensions] : folder_handlers) {
        if (directory_has_files_with_extension(path, extensions)) {
          python_plugin_load(path, script_name);
          return nullptr;
        }
      }
      global::new_ui_message(
          "ERROR: Unable to load directory, no TIFF or PNG files found.\nPath: {}", path.string());
      return nullptr;
    }
  }

  // File loaders
  std::string extension = path.extension().string();
  std::transform(extension.begin(), extension.end(), extension.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  std::string subext = path.stem().extension().string();
  std::transform(subext.begin(), subext.end(), subext.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  // Native file handlers
  if (extension == ".npy") {
    file = std::make_shared<NpyFile>(path);
  } else if (extension == ".dat") {
    file = std::make_shared<RawFile>(path);
    if (!file->good()) {
      file = std::make_shared<BmpFile>(path);
    }
  } else if (extension == ".tif" || extension == ".tiff") {
    file = std::make_shared<TiffFile>(path);
    if (!file->good()) {
      fmt::print("TiffFile load failed, will use python plugin instead. Error: {}\n",
                 file->error_msg());
      // Load failed, let the "load_tiff.py" plugin handle it
      file = nullptr;
    }
  }
  // Python plugin based file handlers
  if (!file) {
    const std::unordered_map<std::string, std::string> python_handlers = {
        {".tif", "load_tiff.py"},
        {".tiff", "load_tiff.py"},
        {".tf8", "load_tiff.py"},
        {".ptif", "load_tiff.py"},
        {".ptiff", "load_tiff.py"},
        {".lsm", "load_tiff.py"},  // ZEISS LSM
        {".btf", "load_tiff.py"},
        {".bif", "load_tiff.py"},  // Roche Digital Pathology
        {".gel", "load_tiff.py"},
        {".ndpi", "load_tiff.py"}, // Hamamatsu Slide Scanner
        {".stk", "load_tiff.py"},
        {".qptiff", "load_tiff.py"},  // Perkin Elmer Vectra
        {".mat", "load_mat.py"},
        {".png", "load_image.py"},
        {".jpg", "load_image.py"},
        {".jpeg", "load_image.py"},
        {".webp", "load_image.py"},
        {".bmp", "load_image.py"},
        {".pgm", "load_image.py"},
        {".dcm", "load_image_itk.py"},
        {".dicom", "load_image_itk.py"},
        {".gdcm", "load_image_itk.py"},
        {".gipl", "load_image_itk.py"},
        {".hdf5", "load_image_itk.py"},
        {".hdr", "load_image_itk.py"},
        {".ipl", "load_image_itk.py"},
        {".img", "load_image_itk.py"},
        {".img.nz", "load_image_itk.py"},
        {".mgh", "load_image_itk.py"},
        {".mha", "load_image_itk.py"},
        {".mhd", "load_image_itk.py"},
        {".mnc", "load_image_itk.py"},
        {".mnc2", "load_image_itk.py"},
        {".nhdr", "load_image_itk.py"},
        {".nia", "load_image_itk.py"},
        {".nii", "load_image_itk.py"},
        {".nii.gz", "load_image_itk.py"},
        {".nrrd", "load_image_itk.py"},
        {".vtk", "load_image_itk.py"},
        {".mp4", "load_encoded_video.py"},
        {".avi", "load_encoded_video.py"},
        {".webm", "load_encoded_video.py"},
        {".mov", "load_encoded_video.py"},
        {".mkv", "load_encoded_video.py"},
        {".gif", "load_encoded_video.py"},
        {".wmv", "load_encoded_video.py"},
        {".m4v", "load_encoded_video.py"},
        {".gsd", "load_micam.py"},
        {".gsh", "load_micam.py"},
        {".rsh", "load_micam.py"},
        {".rsm", "load_micam.py"},
        {".rsd", "load_micam.py"},
        {".zzz_test", "load_plugin_test.py"},  // For testing purposes
    };
    auto it = python_handlers.find(extension);
    if (it == python_handlers.end()) {
      it = python_handlers.find(subext + extension);
    }
    if (it != python_handlers.end()) {
      python_plugin_load(path, it->second);
      return nullptr;
    } else {
      global::new_ui_message("ERROR: Unable to load file, it has an unknown extension.\nFile: {}",
                             path.string());
      return nullptr;
    }
  }

  if (file && !file->good()) {
    if (!file->error_msg().empty()) {
      global::new_ui_message("ERROR: file loading failed: \"{}\"\nFile: {}", file->error_msg(),
                             path.string());
    } else {
      global::new_ui_message("ERROR: file loading has failed with unknown error.\nFile: {}",
                             path.string());
    }
    return nullptr;
  }

  if (!file->error_msg().empty()) {
    global::new_ui_message("{}", file->error_msg());
  }

  return file;
}

std::string create_npy_fileheader_float(std::vector<unsigned long> shape) {
  std::ostringstream stream;
  bool fortran_order = npy::big_endian;  // no targeted platform is little endian
  npy::dtype_t dtype = npy::has_typestring<float>::dtype;
  npy::header_t header{dtype, fortran_order, shape};
  npy::write_header(stream, header);
  return stream.str();
}

namespace {
  void python_plugin_load(const fs::path& path, std::string script_name) {
    // Load the script template and save it to a temporary file
    std::string script = utils::get_rc_text_file("src/python/embedded_plugins/" + script_name);
#ifdef MONOCHROME_PLUGIN_WHL
    auto req_str = fmt::format("\"monochrome @ {}\",", MONOCHROME_PLUGIN_WHL);
#else
    auto req_str = fmt::format("\"monochrome=={}\",", MONOCHROME_VERSION);
#endif
    if (!substring_replace(script, "\"monochrome\",", req_str) ||
        !substring_replace(script, "sys.argv[1]",
                           fmt::format("r\"{}\"", fs::absolute(path).string()))) {
      global::new_ui_message("ERROR: Unable format plugin script {}", script_name);
      return;
    }
#ifdef _WIN32
    while (substring_replace(script, "\r\n", "\n")) {
    };
#endif
    auto script_fn = fs::temp_directory_path() /
                     fmt::format("monochrome_{}_{}.py", script_name.substr(0, script_name.find('.')),
                                 random_alphanumeric_string(5));
    if (!utils::write_text_file(script_fn, script)) {
      return;
    }

    auto builder = subprocess::RunBuilder({utils::get_uv_executable(), "run", script_fn.string()});
    auto env     = subprocess::current_env_copy();
    env["PYTHONUNBUFFERED"] = "1";
    builder.env(env);
    std::string title = fmt::format("Loading file {}", path.string());
    std::string msg   = fmt::format(
        "Loading '{}'...\n\n"
          "Required plugin dependencies will be downloaded automatically."
          " This may take some time on first import.",
        path.filename().string());
    global::add_subprocess(builder, title, msg, [script_fn]() {
      if (fs::exists(script_fn)) {
        fs::remove(script_fn);
      }
    });
  }
}  // namespace
