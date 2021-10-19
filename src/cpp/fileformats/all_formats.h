#pragma once

// InMemory is needed for IPC
#include "InMemoryFile.h"

// Autodetect filetype and open it, returns nullptr on error or unkown filetype
std::shared_ptr<AbstractFile> file_factory(const fs::path &path);

// Creates the header for a .npy file with array of type float
std::string create_npy_fileheader_float(std::vector<unsigned long> shape);