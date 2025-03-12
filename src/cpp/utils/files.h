#pragma once

#include <string>
#include <ghc/fs_std_fwd.hpp>

namespace utils {
  std::string get_rc_text_file(const std::string &filename);

  /* Create directory, return false on error */
  bool create_directory(fs::path path);

  /* Write text to file, return false on error */
  bool write_text_file(const fs::path &filepath, const std::string &content);

  /* Load media files with filepicker */
  void load_file_filepicker();
  /* Load folder of files with filepicker */
  void load_folder_filepicker();

  void install_uv();
  bool uv_install_in_progress();
  std::string get_uv_executable();

}  // namespace utils