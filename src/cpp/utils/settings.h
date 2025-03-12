#pragma once

#include <ghc/fs_std_fwd.hpp>
#include <CLI/CLI.hpp>

namespace settings {

// get the default path of our config file
std::string config_file_path();
std::string save_current_settings();
void cli_add_global_options(CLI::App& app);

// Recent files
std::vector<fs::path> get_recent_files();
void add_recent_file(const fs::path& file);
void clear_recent_files();

} // namespace settings