#pragma once

#include <string>
#include <optional>

//#include <GLFW/glfw3.h>
//
namespace global {
  //  GLFWwindow *main_window       = nullptr;
  //  std::vector<Message> messages = {};

  extern std::string tcp_host;
  extern short tcp_port;

  void add_file_to_load(const std::string& file);

  std::optional<std::string> get_file_to_load();

  struct RawArray3 {
    int nx;
    int ny;
    int nt;
    std::string name;
    std::shared_ptr<float[]> data;
    std::ptrdiff_t data_ptr_offset = 0;
  };

  void add_RawArray3_to_load(const RawArray3& arr);

  std::optional<RawArray3> get_rawarray3_to_load();
}  // namespace global