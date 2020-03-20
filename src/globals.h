#pragma once

#include <string>
#include <optional>
#include <utility>

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
    std::vector<float> data;

    RawArray3(int nx_, int ny_, int nt_, std::string  name_, std::size_t data_size)
        : nx(nx_), ny(ny_), nt(nt_), name(std::move(name_)) {
      data.resize(data_size);
    }
  };

  void add_RawArray3_to_load(std::shared_ptr<RawArray3> arr);

  std::optional<std::shared_ptr<RawArray3>> get_rawarray3_to_load();
}  // namespace global