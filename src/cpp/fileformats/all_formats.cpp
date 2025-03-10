#include <string>
#include <sstream>
#include <unordered_set>

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
  
  std::string random_alphanumeric_string(size_t length ) {
      auto randchar = []() -> char {
          const char charset[] =
          "0123456789"
          "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
          "abcdefghijklmnopqrstuvwxyz";
          const size_t max_index = (sizeof(charset) - 1);
          return charset[ rand() % max_index ];
      };
      std::string str(length,0);
      std::generate_n( str.begin(), length, randchar );
      return str;
  }
}

std::shared_ptr<AbstractFile> file_factory(const fs::path& path) {
  std::shared_ptr<AbstractFile> file;

  // Directory loaders
  if (fs::is_directory(path)) {
    // Check if path contains .png, .tif, or .tiff files
    const std::unordered_set<std::string> image_extensions {".png", ".tif", ".tiff", ".PNG", ".TIF", ".TIFF"};
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
    static const std::unordered_map<std::string, std::string> python_handlers = {
      {".tif", "load_tiff.py"},
      {".tiff", "load_tiff.py"},
      {".lsm", "load_tiff.py"},  // TODO: test
      {".stk", "load_tiff.py"},  // TODO: test
      {".mat", "load_mat.py"},
      {".png", "load_image.py"},
      {".jpg", "load_image.py"},
      {".jpeg", "load_image.py"},
      {".webp", "load_image.py"},
      {".bmp", "load_image.py"},
      {".dcm", "load_image.py"}, // ITK?
      {".dicom", "load_image_itk.py"},
      // {".img", "load_image_itk.py"},
      // {".img.nz", "load_image_itk.py"},
      {".nhdr", "load_image_itk.py"},
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
    };
    auto it = python_handlers.find(extension);
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

std::string get_uv_path() {
  // TODO: download uv if necessary
  return "uv";
}

void python_plugin_load(const fs::path& path, std::string script_name) {
  // Load the script template and save it to a temporary file
  std::string script = get_rc_text_file("src/python/embedded_plugins/" + script_name);
  if (!substring_replace(script, "\"monochrome\",",
                         fmt::format("\"monochrome=={}\",", MONOCHROME_VERSION)) ||
      !substring_replace(script, "sys.argv[1]",
                         fmt::format("\"{}\"", fs::absolute(path).string()))) {
    global::new_ui_message("ERROR: Unable format plugin script {}", script_name);
    return;
  }
  auto script_fn = fs::temp_directory_path() /
                   fmt::format("__monochrome_{}_{}.py", script_name.substr(0, script_name.find('.')),
                               random_alphanumeric_string(5));
  if (!write_text_file(script_fn, script)) {
    return;
  }

  auto builder = subprocess::RunBuilder({get_uv_path(), "run", script_fn.string()});
  builder.cout(subprocess::PipeOption::pipe);
  builder.cerr(subprocess::PipeOption::cout);
  subprocess::cenv["PYTHONUNBUFFERED"] = "1";
  std::string title = fmt::format("Loading file {}", path.string());
  std::string msg   = fmt::format("Loading file/folder '{}' ...\n\nMonochrome will download the required dependencies to import this file automatically. This might take a while when you do it for the first time, please wait.", path.filename().string());
  global::add_subprocess(builder, title, msg, [script_fn]() {
    if (fs::exists(script_fn)) {
      fs::remove(script_fn);
    }
  });
}
}

