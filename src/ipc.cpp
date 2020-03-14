#include <thread>
#include <utility>

#include <fmt/format.h>

#include <asio/read.hpp>
#include <asio/write.hpp>
#include <asio/ts/internet.hpp>

// only use std filesystem on msvc for now, as gcc / clang sometimes require link options
#if defined(__cplusplus) && _MSC_VER >= 1920
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#endif

#include "ipc.h"

#include "schema/message_generated.h"

using asio::ip::tcp;

namespace {
  class Message {
   public:
    static constexpr std::size_t HeaderSize = sizeof(flatbuffers::uoffset_t);

    Message() : body_size_(0) {}

    const uint8_t* header_data() const { return header_data_; }

    uint8_t* header_data() { return header_data_; }

    std::size_t size() const { return body_size_; }

    const uint8_t* body() const { return reinterpret_cast<const uint8_t*>(data_); }

    uint8_t* body() { return reinterpret_cast<uint8_t*>(data_); }

    std::size_t body_size() const { return body_size_; }

    bool decode_header() {
      body_size_ = flatbuffers::GetPrefixedSize(header_data());
      fmt::print("Message Length {}\n", body_size_);
      delete[] data_;
      data_ = new uint8_t[body_size_ / sizeof(uint8_t)];
      //data_ = std::make_shared<float[]>(body_size_ / sizeof(float));
      return true;
    }

    void verify_and_deliver(const tcp::endpoint& remote_ep) {
      auto verifier = flatbuffers::Verifier(body(), body_size());
      if (fbs::VerifyRootBuffer(verifier)) {
        auto root = fbs::GetRoot(body());
        fmt::print("Rx {} message from {}:{}\n", fbs::EnumNameData(root->data_type()),
                   remote_ep.address().to_string(), remote_ep.port());
        switch (root->data_type()) {
          case fbs::Data_Filepaths:
            handle_message(root->data_as_Filepaths());
            break;
          case fbs::Data_Array3:
            handle_message(root->data_as_Array3());
            break;
          default:
            throw std::runtime_error("Unknown message body type");
        }
      } else {
        fmt::print("ERROR: flatbuffers verifier failed\n");
      }
    }

    void handle_message(const fbs::Filepaths* raw) {
      if (!raw) {
        fmt::print("Error parsing flatbuffer\n");
        return;
      }

      auto filepaths = raw->file();
      for (unsigned int i = 0; i < filepaths->size(); i++) {
        auto file = filepaths->Get(i)->str();
        global::add_file_to_load(file);
      }
    }

    void handle_message(const fbs::Array3* raw) {
      //if (!raw) {
      //  fmt::print("Error parsing flatbuffer\n");
      //  return;
      //}
      //
      //auto fbs_data = raw->data();
      //auto nx       = raw->nx();
      //auto ny       = raw->ny();
      //auto nt       = raw->nt();
      //auto name     = raw->name()->str();
      //
      //if (nx * ny * nt != fbs_data->size()) {
      //  throw std::runtime_error("error parsing flatbuffer, sizes do not match!");
      //}
      //std::ptrdiff_t data_offset = fbs_data->data() - data_.get();
      //if (data_offset < 0 || data_offset > body_size_ * sizeof(float)) {
      //  throw std::runtime_error("error creating array, ptr arithmetic seem wrong");
      //}
      //global::add_RawArray3_to_load({nx, ny, nt, name, data_, data_offset});
    }

   private:
    uint8_t header_data_[HeaderSize];
    uint8_t* data_;
    std::size_t body_size_;
  };

  class TcpSession : public std::enable_shared_from_this<TcpSession> {
   public:
    TcpSession(tcp::socket socket) : socket_(std::move(socket)) {}

    void start() { do_read_header(); }

   private:
    void do_read_header() {
      auto self(shared_from_this());
      asio::async_read(socket_, asio::buffer(msg_.header_data(), Message::HeaderSize),
                       [this, self](std::error_code ec, std::size_t length) {
                         if (!ec && msg_.decode_header()) {
                           do_read_body();
                         }
                       });
    }

    void do_read_body() {
      auto self(shared_from_this());
      asio::async_read(socket_, asio::buffer(msg_.body(), msg_.body_size()),
                       [this, self](std::error_code ec, std::size_t /*length*/) {
                         if (!ec) {
                           msg_.verify_and_deliver(socket_.remote_endpoint());
                           do_read_header();
                         }
                       });
    }

    Message msg_;
    tcp::socket socket_;
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
          std::make_shared<TcpSession>(std::move(socket_))->start();
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


  std::unique_ptr<TcpServer> tcp_server;
}  // namespace

void ipc::start_server() {
  assert(!tcp_server);

  try {
    tcp_server = std::make_unique<TcpServer>(global::tcp_port);
    std::thread([]() { tcp_server->run(); }).detach();
  } catch (const asio::system_error& error) {
    fmt::print("Error starting tcp server {}: {}\n", error.code().value(), error.code().message());
  }
}

void ipc::stop_server() {
  if (tcp_server) {
    tcp_server->stop();
  }
}

bool ipc::is_another_instance_running() {
  asio::io_context io_context;
  tcp::socket socket(io_context);
  tcp::endpoint endpoint(asio::ip::make_address(global::tcp_host), global::tcp_port);
  try {
    socket.connect(endpoint);
    return true;
  } catch (const asio::system_error& error) {
    return false;
  }
}

void ipc::send_filepaths(const std::vector<std::string>& files) {
  asio::io_context io_context;
  tcp::socket socket(io_context);
  tcp::endpoint endpoint(asio::ip::make_address(global::tcp_host), global::tcp_port);
  socket.connect(endpoint);

  flatbuffers::FlatBufferBuilder builder(256);
  std::vector<flatbuffers::Offset<flatbuffers::String>> files_fb;

  for (const auto& file : files) {
    auto absolute_path = fs::absolute(file).generic_string();
    files_fb.push_back(builder.CreateString(absolute_path));
  }
  auto files_fb2 = builder.CreateVector(files_fb.data(), files_fb.size());
  auto fp        = fbs::CreateFilepaths(builder, files_fb2);
  auto root      = CreateRoot(builder, fbs::Data_Filepaths, fp.Union());
  builder.FinishSizePrefixed(root);

  asio::write(socket, asio::buffer(builder.GetBufferPointer(), builder.GetSize()));
  socket.close();
}

void ipc::send_array3(const float* data, int nx, int ny, int nt, const std::string& name) {
  asio::io_context io_context;
  tcp::socket socket(io_context);
  tcp::endpoint endpoint(asio::ip::make_address(global::tcp_host), global::tcp_port);
  socket.connect(endpoint);

  flatbuffers::FlatBufferBuilder builder(nx * ny * nt * sizeof(float) + 512);
  auto fbs  = fbs::CreateArray3(builder, builder.CreateVector<float>(data, nx * ny * nt), nx, ny, nt,
                               builder.CreateString(name));
  auto root = CreateRoot(builder, fbs::Data_Filepaths, fbs.Union());
  builder.FinishSizePrefixed(root);

  asio::write(socket, asio::buffer(builder.GetBufferPointer(), builder.GetSize()));
  socket.close();
}
