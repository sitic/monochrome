#pragma once

#include <string>
#include <ghc/fs_std_fwd.hpp>

namespace utils {
  std::string get_rc_text_file(const std::string &filename);

  bool write_text_file(const fs::path &filepath, const std::string &content);

  void load_file_filepicker();
  void load_folder_filepicker();

  void install_uv();
  bool uv_install_in_progress();
  std::string get_uv_executable();

}  // namespace utils