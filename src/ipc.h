#pragma once

#include "globals.h"

namespace ipc {
  void start_server();

  void stop_server();

  bool is_another_instance_running();

  void send_filepaths(const std::vector<std::string>& files);

  void send_array3(const float* data, int nx, int ny, int nt, const std::string& name);
}  // namespace ipc