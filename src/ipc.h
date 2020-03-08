#pragma once

#include "globals.h"

namespace ipc {
  bool is_another_instance_running();

  void load_files(const std::vector<std::string> &files);

  void start_server();

  void stop_server();
}