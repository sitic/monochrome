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

#include "all_formats.h"
#include "utils/files.h"

namespace {
  void python_plugin_load(const fs::path& path, std::string script_name);

  bool substring_replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
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
}

std::shared_ptr<AbstractFile> file_factory(const fs::path& path) {
  std::shared_ptr<AbstractFile> file;

  // Directory loaders
  if (fs::is_directory(path)) {
    // load_folder.py plugin supports a folder of .png, .tif, .tiff, or .dcm files
    const std::unordered_set<std::string> image_extensions {".png", ".tif", ".tiff", ".dcm", ".PNG", ".TIF", ".TIFF", ".DCM"};
    bool has_matching_files = false;
    for (const auto& entry : fs::directory_iterator(path)) {
      if (entry.is_regular_file() && image_extensions.count(entry.path().extension().string()) > 0) {
        has_matching_files = true;
        break;
      }
    }

    if (has_matching_files) {
      python_plugin_load(path, "load_folder.py");
      return nullptr;
    } else {
      global::new_ui_message(
        "ERROR: Unable to load directory, no TIFF or PNG files found.\nPath: {}",
        path.string());
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
  } else {
    // Python-based file handlers
    const std::unordered_map<std::string, std::string> python_handlers = {
      {".tif", "load_tiff.py"},
      {".tiff", "load_tiff.py"},
      {".lsm", "load_tiff.py"},
      {".stk", "load_tiff.py"},
      {".mat", "load_mat.py"},
      {".png", "load_image.py"},
      {".jpg", "load_image.py"},
      {".jpeg", "load_image.py"},
      {".webp", "load_image.py"},
      {".bmp", "load_image.py"},
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
      {".zzz_test", "load_plugin_test.py"}, // For testing purposes
    };
    auto it = python_handlers.find(extension);
    if (it == python_handlers.end()) {
      it = python_handlers.find(subext + extension);
    }
    if (it != python_handlers.end()) {
      python_plugin_load(path, it->second);
      return nullptr;
    } else {
      global::new_ui_message(
          "ERROR: Unable to load file, it has an unknown extension.\nFile: {}",
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
  if (!substring_replace(script, "\"monochrome\",",
                         fmt::format("\"monochrome=={}\",", MONOCHROME_VERSION)) ||
      !substring_replace(script, "sys.argv[1]",
                         fmt::format("r\"{}\"", fs::absolute(path).string()))) {
    global::new_ui_message("ERROR: Unable format plugin script {}", script_name);
    return;
  }
#ifdef _WIN32
  while (substring_replace(script, "\r\n", "\n")) {};
#endif
  auto script_fn = fs::temp_directory_path() /
                   fmt::format("monochrome_{}_{}.py", script_name.substr(0, script_name.find('.')),
                               random_alphanumeric_string(5));
  if (!write_text_file(script_fn, script)) {
    return;
  }

  auto builder = subprocess::RunBuilder({utils::get_uv_executable(), "run", script_fn.string()});
  auto env = subprocess::current_env_copy();
  env["PYTHONUNBUFFERED"] = "1";
  builder.env(env);
  std::string title = fmt::format("Loading file {}", path.string());
  std::string msg = fmt::format(
      "Loading '{}'...\n\n"
      "Required dependencies will be downloaded automatically.\n"
      "This may take some time on first import.", 
      path.filename().string());
  global::add_subprocess(builder, title, msg, [script_fn]() {
    if (fs::exists(script_fn)) {
      fs::remove(script_fn);
    }
  });
}
}
