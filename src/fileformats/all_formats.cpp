#include <string>
#include <sstream>

#include "BmpFile.h"
#include "RawFile.h"
#include "NpyFile.h"

#include "all_formats.h"

std::shared_ptr<AbstractFile> file_factory(const fs::path& path) {
  std::shared_ptr<AbstractFile> file;
  if (!fs::is_regular_file(path)) {
    global::new_ui_message("ERROR: {} does not appear to be a file", path.string());
    return file;
  }

  if (path.extension() == ".npy") {
    file = std::make_shared<NpyFile>(path);
  } else if (path.extension() == ".dat") {
    file = std::make_shared<RawFile>(path);
    if (!file->good()) {
      file = std::make_shared<BmpFile>(path);
    }
  } else {
    global::new_ui_message("ERROR: '{}' does has unknown extension. Only .dat and .npy are supported",
                           path.string());
    return nullptr;
  }

  if (file && !file->good()) {
    if (!file->error_msg().empty()) {
      global::new_ui_message("ERROR: loading file '{}' has failed with error message: {}",
                             path.string(), file->error_msg());
    } else {
      global::new_ui_message("ERROR: loading file '{}' has failed with unkown error");
    }
    return nullptr;
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
