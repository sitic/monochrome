#include <cmrc/cmrc.hpp>

#include "files.h"
#include "globals.h"

CMRC_DECLARE(rc);

std::string get_rc_text_file(const std::string& filename) {
    auto fs   = cmrc::rc::get_filesystem();
    auto file = fs.open(filename);
    return std::string(file.begin(), file.end());
  }

bool write_text_file(const fs::path& path, const std::string& content) {
    std::ofstream file(path, std::ios::out);
    if (!file.is_open()) {
        global::new_ui_message("ERROR: Unable to open file for writing: {}", path.string());
        return false;
    }
    file << content;
    if (!file.good()) {
        global::new_ui_message("ERROR: Unable to write to file: {}", path.string());
        return false;
    }
    file.close();
    return true;
}
