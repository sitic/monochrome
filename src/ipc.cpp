#include <thread>
#include <utility>

#include <fmt/format.h>
#include <asio/write.hpp>
#include <asio/ts/internet.hpp>

#include "ipc.h"

#include "schema/array3_generated.h"
#include "schema/filepath_generated.h"

using asio::ip::tcp;

class session : public std::enable_shared_from_this<session> {
 public:
  session(tcp::socket socket) : socket_(std::move(socket)) {}

  void start() { do_read(); }

 private:
  int i = 0;

  void do_read() {
    auto self(shared_from_this());
    auto handler = [this, self](std::error_code ec, std::size_t length) {
      if (!ec) {
        auto filepaths = ipc::GetFilepaths(data_);
        auto fp        = filepaths->file();
        for (unsigned int i = 0; i < fp->size(); i++) {
          auto file = fp->Get(i)->str();
          global::add_file_to_load(file);
        }
        //i += 1;
        //do_write(length);

        //if (data_[0] == 'a') {
        //  auto file = std::string(data_ + 1, length - 1);
        //  global::add_file_to_load(file);
        //  fmt::print("{}\n", file);
        //} else if (data_[0] == 'b') {
        //  std::size_t array_size;
        //  std::copy(data_ + 1, data_ + 1 + sizeof(array_size), array_size);
        //}
      }
    };
    socket_.async_read_some(asio::buffer(data_, max_length), handler);
  }

  void do_write(std::size_t length) {
    auto self(shared_from_this());
    std::string number = std::to_string(i);
    //std::copy(number.begin(), number.end(), data_);
    asio::async_write(socket_, asio::buffer(number.data(), number.size()),
                      [this, self](std::error_code ec, std::size_t /*length*/) {
                        if (!ec) {
                          do_read();
                        } else {
                          fmt::print("Error {}: {}\n", ec.value(), ec.message());
                        }
                      });
  }

  tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
};

class TcpServer {
 public:
  TcpServer(short port)
      : acceptor_(io_context_, tcp::endpoint(tcp::v4(), port)), socket_(io_context_) {
    do_accept();
  }

  void run() { io_context_.run(); }

  void stop() { io_context_.stop(); }

 private:
  void do_accept() {
    acceptor_.async_accept(socket_, [this](std::error_code ec) {
      if (!ec) {
        std::make_shared<session>(std::move(socket_))->start();
      } else {
        fmt::print("Error {}: {}\n", ec.value(), ec.message());
      }

      do_accept();
    });
  }

  asio::io_context io_context_;
  tcp::acceptor acceptor_;
  tcp::socket socket_;
};

void test_flat() {
  using namespace ipc;

  flatbuffers::FlatBufferBuilder builder(1024);
  std::array<float, 10 * 10 * 10> test_arr = {};

  auto arr  = builder.CreateVector(test_arr.data(), test_arr.size());
  auto name = builder.CreateString("test");
  CreateArray3(builder, arr, 10, 10, 10, name);
  //const uint8_t *flatbuf;
  //GetArray3(flatbuf);
}

bool ipc::is_another_instance_running() {
  asio::io_context io_context;
  tcp::socket socket(io_context);
  tcp::endpoint endpoint(asio::ip::make_address("127.0.0.1"), global::tcp_port);
  try {
    socket.connect(endpoint);
    return true;
  } catch (const asio::system_error& error) {
    return false;
  }
}

void ipc::load_files(const std::vector<std::string>& files) {
  asio::io_context io_context;
  tcp::socket socket(io_context);
  tcp::endpoint endpoint(asio::ip::make_address("127.0.0.1"), global::tcp_port);
  socket.connect(endpoint);

  flatbuffers::FlatBufferBuilder builder(1024);
  std::vector<flatbuffers::Offset<flatbuffers::String>> files_fb;
  for (auto file : files) {
    files_fb.push_back(builder.CreateString(file));
  }
  auto files_fb2 = builder.CreateVector(files_fb.data(), files_fb.size());
  auto fp        = CreateFilepaths(builder, files_fb2);
  builder.Finish(fp);

  asio::write(socket, asio::buffer(builder.GetBufferPointer(), builder.GetSize()));
  socket.close();
}

namespace {
  std::unique_ptr<TcpServer> tcp_server;
}

void ipc::start_server() {
  assert(!tcp_server);

  tcp_server = std::make_unique<TcpServer>(global::tcp_port);
  std::thread([]() { tcp_server->run(); }).detach();
}

void ipc::stop_server() {
  if (tcp_server) {
    tcp_server->stop();
  }
}
