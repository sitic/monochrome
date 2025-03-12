#include <cmrc/cmrc.hpp>

#include "globals.h"
#include "files.h"

CMRC_DECLARE(rc);

namespace {
    std::string UV_PATH = "";
    std::mutex  UV_PATH_MUTEX;
    bool        UV_INSTALL_IN_PROGRESS = false;
}

namespace utils {

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

void install_uv() {
    std::lock_guard<std::mutex> lock(UV_PATH_MUTEX);
    
    if (UV_INSTALL_IN_PROGRESS) return;
    UV_INSTALL_IN_PROGRESS = true;
#ifdef _WIN32
    auto builder = subprocess::RunBuilder({"powershell", "-ExecutionPolicy", "ByPass", "-c",
                                           "irm https://astral.sh/uv/install.ps1 | iex"});
    std::function<void()> callback = []() {
        UV_INSTALL_IN_PROGRESS = false;
        get_uv_executable();
    };
#else
    auto script = get_rc_text_file("assets/install_uv_unix.sh");
    auto script_fn = fs::temp_directory_path() / "install_uv_unix.sh";
    if (!write_text_file(script_fn, script)) {
        return;
    }
    auto builder = subprocess::RunBuilder({"sh", script_fn.string()});
    std::function<void()> callback = [script_fn]() {
        if (fs::exists(script_fn)) {
            fs::remove(script_fn);
        }
        UV_INSTALL_IN_PROGRESS = false;
        get_uv_executable();
    };
#endif
    std::string title = fmt::format("Installing uv ...");
    std::string msg   = fmt::format("Please wait while the uv is being installed.");
    auto env = subprocess::current_env_copy();
    // env["UV_INSTALL_DIR"] = "";
    builder.env(env);
    global::add_subprocess(builder, title, msg, callback);
}

bool uv_install_in_progress() {
    return UV_INSTALL_IN_PROGRESS;
}
std::string get_uv_executable() {
    if (uv_install_in_progress()) {
        return "uv-placeholder";
    }

    std::lock_guard<std::mutex> lock(UV_PATH_MUTEX);
    if (!UV_PATH.empty() && fs::exists(UV_PATH)) {
        return UV_PATH;
    }
    try {
        UV_PATH = subprocess::find_program("uv");
        if (UV_PATH.empty()) {
            return "uv-placeholder";
        } else {
            return UV_PATH;
        }
    } catch (std::exception& e) {
        global::new_ui_message("ERROR: {}", e.what());
        return "uv-placeholder";
    }
}
} // namespace utils
