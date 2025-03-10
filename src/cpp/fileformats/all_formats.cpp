#include <string>
#include <sstream>

// Asio needs to be imported before windows.h
#include <asio.hpp>
#include <subprocess.hpp>

#include "BmpFile.h"
#include "RawFile.h"
#include "NpyFile.h"

#include "all_formats.h"
#include "utils/files.h"

namespace {
  void ipc_python_load(const fs::path& path, std::string script_name);

  bool match_file_extension(std::vector<fs::path> extensions, const fs::path& path) {
    auto extension = path.extension();
    for (const auto& ext : extensions) {
      if (ext == extension) {
        return true;
      }
    }
    return false;
  }

  bool match_file_extension(std::vector<fs::path> extensions, const std::string& extension) {
    return match_file_extension(extensions, fs::path("file" + extension));
  }
}

std::shared_ptr<AbstractFile> file_factory(const fs::path& path) {
  std::shared_ptr<AbstractFile> file;

  // Directory loaders
  if (fs::is_directory(path)) {
    // Check if path contains .png, .tif, or .tiff files
    bool has_matching_files = false;
    for (const auto& entry : fs::directory_iterator(path)) {
      if (entry.is_regular_file()) {
        if (match_file_extension({".png", ".tif", ".tiff", ".PNG", ".TIF", ".TIFF"}, entry.path())) {
          has_matching_files = true;
          break;
        }
      }
    }

    if (has_matching_files) {
      ipc_python_load(path, "load_folder.py");
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
  if (extension == ".npy") {
    file = std::make_shared<NpyFile>(path);
  } else if (extension == ".dat") {
    file = std::make_shared<RawFile>(path);
    if (!file->good()) {
      file = std::make_shared<BmpFile>(path);
    }
  } else if (match_file_extension({".tif", ".tiff"}, extension)) {
    ipc_python_load(path, "load_tiff.py");
    return nullptr;
  } else if (match_file_extension({".mat"}, extension)) {
    ipc_python_load(path, "load_mat.py");
    return nullptr;
  } else if (match_file_extension({".png", ".jpg", ".jpeg", ".webp", ".bmp"}, extension)) {
    ipc_python_load(path, "load_image.py");
    return nullptr;
  } else if (match_file_extension({".mp4", ".avi", ".webm", ".mov", ".mkv", ".gif", ".wmv", ".m4v"}, extension)) {
    ipc_python_load(path, "load_encoded_video.py");
    return nullptr;
  } else if (match_file_extension({".gsd", ".gsh", ".rsh", ".rsm", ".rsd"}, extension)) {
    ipc_python_load(path, "load_micam.py");
    return nullptr;
  } else {
    global::new_ui_message(
        "ERROR: Unable to load file, it has an unknown extension. Only .dat and .npy are "
        "supported.\nFile: {}",
        path.string());
    return nullptr;
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

void ipc_python_load(const fs::path& path, std::string script_name) {
  // Load the script template and save it to a temporary file
  std::string script = get_rc_text_file("src/python/embedded_file_loaders/" + script_name);
  substring_replace(script, "MONOCHROME_VERSION", MONOCHROME_VERSION);
  substring_replace(script, "MC_FILEPATH", fmt::format("\"{}\"", fs::absolute(path).string()));
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
  std::string msg   = fmt::format("Loading file/folder, please wait ...", path.string());
  global::add_subprocess(builder, title, msg, [script_fn]() {
    if (fs::exists(script_fn)) {
      fs::remove(script_fn);
    }
  });
}
}
