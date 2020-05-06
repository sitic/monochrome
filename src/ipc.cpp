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

  struct Array3Msg {
    std::size_t counter = 0;
    std::shared_ptr<global::RawArray3> array;

    void clear() {
      counter = 0;
      array   = nullptr;
    }
    bool empty() { return !array && !counter; }
    bool complete() { return array ? counter == array->size() : false; }
  };

  class TcpMessage {
   public:
    static constexpr std::size_t HeaderSize = sizeof(flatbuffers::uoffset_t);

    TcpMessage() = default;

    ~TcpMessage() {
      if (!array_msg_.empty()) {
        fmt::print("ERROR: Client disconnected before full array was recieved!\n");
      }
    }

    uint8_t* header_data() { return header_data_.data(); }

    uint8_t* body() { return data_.data(); }

    [[nodiscard]] std::size_t header_size() const { return data_.size(); }

    [[nodiscard]] std::size_t body_size() const { return data_.size(); }

    bool decode_header() {
      auto msg_size = flatbuffers::GetPrefixedSize(header_data());
      // 2GB - 1, max flatbuffer size, see FLATBUFFERS_MAX_BUFFER_SIZE
      constexpr std::size_t max_size = ((1ULL << (sizeof(flatbuffers::soffset_t) * 8 - 1)) - 1);
      if (msg_size > max_size) {
        return false;
      } else {
        data_.resize(msg_size);
        return true;
      }
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
          case fbs::Data_Array3Meta:
            handle_message(root->data_as_Array3Meta());
            break;
          case fbs::Data_Array3DataChunkf:
            handle_datachunk_message(root->data_as_Array3DataChunkf());
            break;
          case fbs::Data_Array3DataChunku16:
            handle_datachunk_message(root->data_as_Array3DataChunku16());
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

    void handle_message(const fbs::Array3Meta* raw) {
      if (!raw) {
        fmt::print("Error parsing flatbuffer\n");
        return;
      }

      if (!array_msg_.empty()) {
        throw std::runtime_error(
            "Array3Meta message arrived before previous array was completely loaded");
      }

      int cmap     = raw->cmap() - 1;
      int bitrange = raw->bitrange() - 1;
      global::RawArray3MetaData meta{
          raw->nx(),
          raw->ny(),
          raw->nt(),
          raw->name()->str(),
          raw->duration(),
          raw->fps(),
          raw->date()->str(),
          raw->comment()->str(),
          bitrange < 0 ? std::nullopt : std::optional<BitRange>(static_cast<BitRange>(bitrange)),
          cmap < 0 ? std::nullopt : std::optional<ColorMap>(static_cast<ColorMap>(cmap))};

      std::size_t size = static_cast<std::size_t>(meta.nx) * meta.ny * meta.nt;
      if (raw->type() == fbs::ArrayDataType::ArrayDataType_FLOAT) {
        array_msg_.array = global::RawArray3::create_float(meta, size);
      } else if (raw->type() == fbs::ArrayDataType::ArrayDataType_UINT16) {
        array_msg_.array = global::RawArray3::create_u16(meta, size);
      } else {
        throw std::runtime_error("TODO");
      }
    }

    template <typename ChunkPtr>
    void handle_datachunk_message(const ChunkPtr* raw) {
      if (!raw) {
        fmt::print("Error parsing flatbuffer\n");
        return;
      }
      if (array_msg_.empty()) {
        throw std::runtime_error("Array3DataChunk message arrived before Array3Meta");
      }

      auto idx  = raw->startidx();
      auto data = raw->data();

      if (idx + data->size() > array_msg_.array->size()) {
        throw std::runtime_error(
            "Recieved Array3DataChunk does not fit into the dimensions specified in Array3Meta");
      }
      std::visit([&data, idx](auto& v) { std::copy(data->begin(), data->end(), v.begin() + idx); },
                 array_msg_.array->data);
      array_msg_.counter += data->size();
      if (array_msg_.complete()) {
        fmt::print("Loading of {} complete!\n", array_msg_.array->meta.name);
        global::add_RawArray3_to_load(array_msg_.array);
        array_msg_.clear();
      }
    }

   private:
    std::array<uint8_t, HeaderSize> header_data_ = {};
    std::vector<uint8_t> data_;
    Array3Msg array_msg_;
  };

  class TcpSession : public std::enable_shared_from_this<TcpSession> {
   public:
    TcpSession(tcp::socket socket) : socket_(std::move(socket)) {}

    void start() { do_read_header(); }

   private:
    void do_read_header() {
      auto self(shared_from_this());
      asio::async_read(socket_, asio::buffer(msg_.header_data(), TcpMessage::HeaderSize),
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

    TcpMessage msg_;
    tcp::socket socket_;
  };

  class TcpServer {
   public:
    TcpServer(const tcp::endpoint& endpoint)
        : acceptor_(io_context_, endpoint), socket_(io_context_) {
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
    // auto endpoint = tcp::endpoint(tcp::v4(), global::tcp_port); // accept connections from all hosts
    auto endpoint = tcp::endpoint(asio::ip::make_address(global::tcp_host), global::tcp_port);
    tcp_server    = std::make_unique<TcpServer>(endpoint);
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
  auto start_time = std::chrono::high_resolution_clock::now();

  asio::io_context io_context;
  tcp::socket socket(io_context);
  tcp::endpoint endpoint(asio::ip::make_address(global::tcp_host), global::tcp_port);
  socket.connect(endpoint);

  // flatbuffers can only be 2GB - 1B large, so often not large enough to contain the complete array.
  // Testing by me (only local connection) has shown that 64KB max message size seems to be the
  // fastest end-to-end transfer speed
  flatbuffers::FlatBufferBuilder builder(65536);
  const std::size_t MAX_FLOAT_ELMS = (65536 - 128) / sizeof(float);

  /* Metadata Message */
  auto fbs_start =
      fbs::CreateArray3MetaDirect(builder, fbs::ArrayDataType_FLOAT, nx, ny, nt, name.c_str());
  auto root_start = CreateRoot(builder, fbs::Data_Array3Meta, fbs_start.Union());
  builder.FinishSizePrefixed(root_start);
  asio::write(socket, asio::buffer(builder.GetBufferPointer(), builder.GetSize()));
  builder.Clear();

  const std::size_t data_size = nx * ny * static_cast<std::size_t>(nt);
  for (std::size_t idx = 0; idx < data_size; idx += MAX_FLOAT_ELMS) {
    auto end      = std::min(idx + MAX_FLOAT_ELMS, data_size);
    auto fbs_data = builder.CreateVector<float>(data + idx, end - idx);
    auto fbs      = fbs::CreateArray3DataChunkf(builder, idx, fbs_data);
    auto root     = CreateRoot(builder, fbs::Data_Array3DataChunkf, fbs.Union());
    builder.FinishSizePrefixed(root);
    asio::write(socket, asio::buffer(builder.GetBufferPointer(), builder.GetSize()));
    builder.Clear();
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto to_ms    = [](const auto& d) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
  };
  fmt::print("File {} was uploaded to remote process in {}ms\n", name, to_ms(end_time - start_time));
}
