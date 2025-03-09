#include <string>
#include <ghc/fs_std_fwd.hpp>

#pragma once

std::string get_rc_text_file(const std::string &filename);

bool write_text_file(const fs::path &filepath, const std::string &content);