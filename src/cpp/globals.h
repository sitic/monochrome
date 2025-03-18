#pragma once

#include <string>
#include <optional>
#include <utility>
#include <memory>
#include <variant>
#include <vector>
#include <thread>
#include <mutex>

#include <asio.hpp>
#include <asio/thread_pool.hpp>
#include <fmt/format.h>
#include <subprocess.hpp>

#include "utils/vectors.h"
#include "utils/definitions.h"

namespace global {
  class Message;
  class Subprocess;

  // List of messages to be displayed in the UI
  extern std::vector<Message> messages;
  // List of subprocesses to be managed
  extern std::vector<std::shared_ptr<Subprocess>> subprocesses;
  // Thread pool for executing tasks in the background
  extern asio::thread_pool thread_pool;
  // IPC server details
  extern std::string tcp_host;
  extern short tcp_port;

  class Message {
   public:
    bool show = true;
    std::string msg;
    int id = 0;

    Message(std::string msg);
  };

  struct RawArray3MetaData {
    int nx = -1;
    int ny = -1;
    int nt = -1;
    int nc = 1;

    std::string name;
    float duration = 0;  // in seconds
    float fps      = 0;  // in Hz
    std::string date;
    std::string comment;
    std::optional<BitRange> bitrange                          = std::nullopt;
    std::optional<ColorMap> cmap                              = std::nullopt;
    std::optional<float> vmin                                 = std::nullopt;
    std::optional<float> vmax                                 = std::nullopt;
    std::optional<std::string> parentName                     = std::nullopt;
    std::optional<OpacityFunction> opacity                    = std::nullopt;
    std::vector<std::pair<std::string, std::string>> metaData = {};
    std::optional<Vec4f> color                                = std::nullopt;
  };

  // Command to be executed by the main thread
  struct RemoteCommand {
    virtual ~RemoteCommand() = default;
  };

  struct LoadFileCommand : RemoteCommand {
    LoadFileCommand(const std::string &filename_) : filename(filename_) {}
    std::string filename;
  };
  class RawArray3 : public RemoteCommand {
   private:
    RawArray3() = default;

   public:
    RawArray3MetaData meta;
    std::variant<std::vector<float>, std::vector<uint8_t>, std::vector<uint16_t>> data;

    template <typename T>
    static std::shared_ptr<RawArray3> create(RawArray3MetaData metadata_, std::size_t data_size) {
      auto a  = std::shared_ptr<RawArray3>(new RawArray3);
      a->meta = std::move(metadata_);
      a->data = std::vector<T>(data_size);
      return a;
    }

    std::size_t size() const {
      return std::visit([](auto &v) { return v.size(); }, data);
    }
  };

  struct PointsVideo : RemoteCommand {
    std::string name;
    std::string parent_name;
    std::vector<std::vector<float>> data;
    Vec4f color      = {0, 0, 0, 0};
    float point_size = -1;
    bool show        = true;

    void assign_next_color(unsigned color_count) {
      // List of colors to cycle through
      const std::array<Vec4f, 6> cycle_list = {{
          {0, 0, 0, 1},
          {1, 1, 1, 1},
          {228 / 255.f, 26 / 255.f, 28 / 255.f, 1},
          {55 / 255.f, 126 / 255.f, 184 / 255.f, 1},
          {77 / 255.f, 175 / 255.f, 74 / 255.f, 1},
          {152 / 255.f, 78 / 255.f, 163 / 255.f, 1},
      }};

      if (color_count >= cycle_list.size()) {
        color_count %= cycle_list.size();
      }
      color = cycle_list.at(color_count);
    }
  };

  struct ExportVideoCommand : RemoteCommand {
    std::string recording;
    std::string filename;
    std::string description;
    int t_start = 0;
    int t_end   = -1;
    int fps     = 30;
    bool close_after_completion = false;
  };

  struct CloseVideoCommand : RemoteCommand {
    std::string recording;
    CloseVideoCommand(std::string recording_) : recording(recording_) {}
  };

  class Subprocess {
    private:
      std::unique_ptr<subprocess::Popen> popen = nullptr;
      std::thread reader_thread;
      bool running = true;
      std::mutex _mutex;
      std::string _cout;
      std::function<void()> callback;

    public:
     subprocess::RunBuilder builder;

     int id = 0;
     bool show = true;
     std::string title;
     std::string msg;
     std::string cmd;
     std::string cout;
 
     Subprocess(subprocess::RunBuilder process, std::string title="", std::string msg="", std::function<void()> callback = std::function<void()>());
     ~Subprocess();
     // Run the subprocess if it is not already running
     bool open();
     // Update cout if new data is available
     bool tick();
     bool is_running();
   };

   template <typename... Args>
   inline void new_ui_message(const char *fmt, Args &&...args) {
     const std::string msg = fmt::format(fmt, std::forward<Args>(args)...);
     messages.emplace_back(msg);
     std::string prefix("ERROR");
     if (msg.compare(0, prefix.size(), prefix)) {
       fmt::print(stderr, msg + "\n");
     } else {
       fmt::print(msg + "\n");
     }
   }
 
   template <typename... Args>
   inline void new_ui_message(const std::string &fmt, Args &&...args) {
     return new_ui_message(fmt.c_str(), std::forward<Args>(args)...);
   }
 
   void add_file_to_load(const std::string &file);
 
   std::optional<std::string> get_file_to_load();
 
   // Add command to the queue of commands to be executed by the main thread
   void add_remote_command(std::shared_ptr<RemoteCommand> cmd);
   std::optional<std::shared_ptr<RemoteCommand>> get_remote_command();
   
   // special case for file loading to allow for double-clicking files in the file browser
   void add_file_to_load(const std::string &filename);

   void add_subprocess(subprocess::RunBuilder process, std::string title="", std::string msg="", std::function<void()> callback = std::function<void()>());
 
   // Initiate shutdown of the application
   void quit(int code = 0);
}  // namespace global