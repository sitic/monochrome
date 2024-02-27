#pragma once

#include <CLI/CLI.hpp>

// get the default path of our config file
std::string config_file_path();

void cli_add_global_options(CLI::App& app);

std::string save_current_settings();