#pragma once

#include <string>
#include <optional>

//#include <GLFW/glfw3.h>
//
namespace global {
  //  GLFWwindow *main_window       = nullptr;
  //  std::vector<Message> messages = {};

  extern short tcp_port;

  void add_file_to_load(const std::string& file);

  std::optional<std::string> get_file_to_load();

  struct RawArray3 {
    std::shared_ptr<float> data;
    std::size_t nx;
    std::size_t ny;
    std::size_t nt;
    std::string name;
  };

  void add_RawArray3_to_load(const RawArray3& arr);

  std::optional<RawArray3> get_rawarray3_to_load();
}  // namespace global