#include <thread>
#include <utility>

#include <GLFW/glfw3.h>
#include <fmt/format.h>

#include <asio/read.hpp>
#include <asio/write.hpp>
#include <asio/ts/internet.hpp>
#include <asio/local/stream_protocol.hpp>

// only use std filesystem on msvc for now, as gcc / clang sometimes require link options
#if defined(__cplusplus) && _MSC_VER >= 1920
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#endif

#ifndef WIN32
#include <unistd.h>
#endif

#include "ipc.h"

#include "message_generated.h"

namespace {
  std::string get_userid() {
#ifdef WIN32
    //    char user_name[UNLEN + 1];
    //    DWORD user_name_size = sizeof(user_name);
    //    if (GetUserName(user_name, &user_name_size))
    //      return std::string(user_name, user_name_size);
    //    else
    return "";
#else
    return fmt::format("{}", getuid());
#endif
  }

#if defined(ASIO_HAS_LOCAL_SOCKETS) && !defined(MC_FORCE_TCP_IPC)
#define MC_USE_LOCAL_SOCKETS
  using IpcProtocol = asio::local::stream_protocol;
#else
  using IpcProtocol = asio::ip::tcp;
#endif

  IpcProtocol::endpoint ipc_client_endpoint() {
#if defined(MC_USE_LOCAL_SOCKETS)
#ifdef __APPLE__  // OSX doesn't support abstract UNIX domain sockets
    std::string ep = fmt::format("/tmp/Monochrome{}.s", get_userid());
#else
    std::string ep = std::string({'\0'}) + std::string("Monochrome") + get_userid();
#endif
    return IpcProtocol::endpoint(ep);
#else
    return IpcProtocol::endpoint(asio::ip::make_address(global::tcp_host), global::tcp_port);
#endif
  }

  class IpcHostEndpoint {
    bool connected_ = false;

   public:
    IpcHostEndpoint() = default;

    ~IpcHostEndpoint() { disconnected(); }

    void connected() { connected_ = true; }

    void disconnected() {
#if defined(MC_USE_LOCAL_SOCKETS) && defined(__APPLE__)
      if (connected_) {
        fs::path path = ipc_client_endpoint().path();
        fs::remove(path);
        connected_ = false;
      }
#endif
    }

    IpcProtocol::endpoint get() {
#if defined(MC_USE_LOCAL_SOCKETS)
      return ipc_client_endpoint();
#else
      // accept connections from all hosts
      //return IpcProtocol::endpoint(IpcProtocol::v4(), global::tcp_port);

      // accept connections from localhost
      return IpcProtocol::endpoint(asio::ip::make_address(global::tcp_host), global::tcp_port);
#endif
    }
  };

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

  class IpcMessage {
   public:
    static constexpr std::size_t HeaderSize = sizeof(flatbuffers::uoffset_t) / sizeof(uint8_t);

    IpcMessage() = default;

    ~IpcMessage() {
      if (!array_msg_.empty()) {
        fmt::print("ERROR: Client disconnected before full array was received!\n");
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

    void verify_and_deliver() {
      // I think our buffers used to be aligned, but not always anymore.
      // Just skip alignment check for now, TODO: figure out what's going on
      flatbuffers::Verifier::Options opts;
      opts.check_alignment = false;

      auto verifier = flatbuffers::Verifier(body(), body_size(), opts);
      if (fbs::VerifyRootBuffer(verifier)) {
        auto root = fbs::GetRoot(body());
        switch (root->data_type()) {
          case fbs::Data_Filepaths:
            handle_message(root->data_as_Filepaths());
            break;
          case fbs::Data_Array3Meta:
            handle_message(root->data_as_Array3Meta());
            break;
          case fbs::Data_Array3MetaFlow:
            handle_message(root->data_as_Array3MetaFlow());
            break;
          case fbs::Data_Array3DataChunkf:
            handle_datachunk_message(root->data_as_Array3DataChunkf());
            break;
          case fbs::Data_Array3DataChunku8:
            handle_datachunk_message(root->data_as_Array3DataChunku8());
            break;
          case fbs::Data_Array3DataChunku16:
            handle_datachunk_message(root->data_as_Array3DataChunku16());
            break;
          case fbs::Data_Request:
            handle_datachunk_message(root->data_as_Request());
            break;
          case fbs::Data_PointsVideo:
            handle_datachunk_message(root->data_as_PointsVideo());
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

    void handle_datachunk_message(const fbs::PointsVideo* raw) {
      if (!raw) {
        fmt::print("Error parsing flatbuffer\n");
        return;
      }

      auto pv = std::make_shared<global::PointsVideo>();
      if (flatbuffers::IsFieldPresent(raw, fbs::PointsVideo::VT_NAME)) {
        pv->name = raw->name()->str();
      }
      if (flatbuffers::IsFieldPresent(raw, fbs::PointsVideo::VT_PARENT_NAME)) {
        pv->parent_name = raw->parent_name()->str();
      }
      if (flatbuffers::IsFieldPresent(raw, fbs::PointsVideo::VT_COLOR)) {
        for (int i = 0; i < 4; i++) pv->color[i] = raw->color()->values()->Get(i);
      }
      if (flatbuffers::IsFieldPresent(raw, fbs::PointsVideo::VT_POINT_SIZE)) {
        pv->point_size = raw->point_size();
      }
      if (!raw->points_data() || !raw->time_idxs()) {
        fmt::print("EROOR: parsing PointsVideo IPC packet failed!\n");
      };

      std::vector<std::size_t> idxs(raw->time_idxs()->begin(), raw->time_idxs()->end());
      auto data_fb         = raw->points_data()->data();
      std::size_t last_idx = 0;
      for (auto t : idxs) {
        std::vector<float> frame(data_fb + last_idx, data_fb + t);
        pv->data.push_back(frame);

        last_idx = t;
      }
      global::add_PointsVideo_to_load(pv);
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
      int opacity_fct =
          flatbuffers::IsFieldPresent(raw, fbs::Array3Meta::VT_OPACITY) ? raw->opacity() - 1 : -1;

      std::vector<std::pair<std::string, std::string>> metaData = {};
      if (flatbuffers::IsFieldPresent(raw, fbs::Array3Meta::VT_METADATA) && raw->metadata()) {
        for (const auto& m : *(raw->metadata())) {
          auto key = m->key();
          auto val = m->val();
          if (key && val) {
            metaData.emplace_back(key->str(), val->str());
          }
        }
      }

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
          cmap < 0 ? std::nullopt : std::optional<ColorMap>(static_cast<ColorMap>(cmap)),
          flatbuffers::IsFieldPresent(raw, fbs::Array3Meta::VT_VMIN)
              ? std::optional<float>(raw->vmin())
              : std::nullopt,
          flatbuffers::IsFieldPresent(raw, fbs::Array3Meta::VT_VMAX)
              ? std::optional<float>(raw->vmax())
              : std::nullopt,
          flatbuffers::IsFieldPresent(raw, fbs::Array3Meta::VT_PARENT_NAME)
              ? std::optional<std::string>(raw->parent_name()->str())
              : std::nullopt,
          opacity_fct < 0
              ? std::nullopt
              : std::optional<OpacityFunction>(static_cast<OpacityFunction>(opacity_fct)),
          metaData};

      std::size_t size = static_cast<std::size_t>(meta.nx) * meta.ny * meta.nt;
      if (raw->type() == fbs::ArrayDataType::ArrayDataType_FLOAT) {
        array_msg_.array = global::RawArray3::create<float>(meta, size);
      } else if (raw->type() == fbs::ArrayDataType::ArrayDataType_UINT8) {
        array_msg_.array = global::RawArray3::create<uint8_t>(meta, size);
      } else if (raw->type() == fbs::ArrayDataType::ArrayDataType_UINT16) {
        array_msg_.array = global::RawArray3::create<uint16_t>(meta, size);
      } else {
        throw std::runtime_error("IPC: unkown ArrayDataType recieved");
      }
    }

    void handle_message(const fbs::Array3MetaFlow* raw) {
      if (!raw) {
        fmt::print("Error parsing flatbuffer\n");
        return;
      }

      if (!array_msg_.empty()) {
        throw std::runtime_error(
            "Array3Meta message arrived before previous array was completely loaded");
      }

      global::RawArray3MetaData meta{raw->nx(), raw->ny(), raw->nt(), raw->name()->str()};
      if (flatbuffers::IsFieldPresent(raw, fbs::Array3MetaFlow::VT_PARENT_NAME))
        meta.parentName = raw->parent_name()->str();
      meta.is_flowfield = true;
      if (flatbuffers::IsFieldPresent(raw, fbs::Array3MetaFlow::VT_COLOR)) {
        Vec4f color;
        for (int i = 0; i < 4; i++) color[i] = raw->color()->values()->Get(i);
        meta.color = color;
      }

      std::size_t size = static_cast<std::size_t>(meta.nx) * meta.ny * meta.nt;
      array_msg_.array = global::RawArray3::create<float>(meta, size);
    }

    template <typename ChunkPtr>
    void handle_datachunk_message(const ChunkPtr* raw) {
      if (!raw) {
        fmt::print("Error parsing flatbuffer\n");
        return;
      }
      if (array_msg_.empty()) {
        fmt::print("Error: Array3DataChunk message arrived before Array3Meta\n");
        return;
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
        global::add_RawArray3_to_load(array_msg_.array);
        array_msg_.clear();
      }
    }

    void handle_datachunk_message(const fbs::Request* raw) {
      // TODO: implement
      if (!raw) {
        fmt::print("Error parsing flatbuffer\n");
        return;
      }
      auto type = raw->type();
      if (type == fbs::RequestType::RequestType_CLOSE) {
        auto recording_name = raw->arg()->str();
      } else if (type == fbs::RequestType::RequestType_CLOSE_ALL) {

      } else if (type == fbs::RequestType_TRACE_POS) {
        auto recording_name = raw->arg()->str();
      }
    }

   private:
    std::array<uint8_t, HeaderSize> header_data_ = {};
    std::vector<uint8_t> data_;
    Array3Msg array_msg_;
  };

  class IpcSession : public std::enable_shared_from_this<IpcSession> {
   public:
    IpcSession(IpcProtocol::socket socket) : socket_(std::move(socket)) {}

    void start() { do_read_header(); }

   private:
    void do_read_header() {
      auto self(this->shared_from_this());
      asio::async_read(socket_, asio::buffer(msg_.header_data(), IpcMessage::HeaderSize),
                       [this, self](std::error_code ec, std::size_t length) {
                         if (!ec && msg_.decode_header()) {
                           do_read_body();
                         }
                       });
    }

    void do_read_body() {
      auto self(this->shared_from_this());
      asio::async_read(socket_, asio::buffer(msg_.body(), msg_.body_size()),
                       [this, self](std::error_code ec, std::size_t /*length*/) {
                         if (!ec) {
                           msg_.verify_and_deliver();
                           do_read_header();
                         }
                       });
    }

    IpcMessage msg_;
    IpcProtocol::socket socket_;
  };

  class IpcServer {
   public:
    IpcServer() : ep_(), acceptor_(io_context_, ep_.get()), socket_(io_context_) {
      ep_.connected();
      do_accept();
    }

    void run() { io_context_.run(); }

    void stop() {
      io_context_.stop();
      ep_.disconnected();
    }

   private:
    void do_accept() {
      acceptor_.async_accept(socket_, [this](std::error_code ec) {
        if (!ec) {
          std::make_shared<IpcSession>(std::move(socket_))->start();
        } else {
          fmt::print("Error {}: {}\n", ec.value(), ec.message());
        }

        do_accept();
      });
    }

    IpcHostEndpoint ep_;
    asio::io_context io_context_;
    IpcProtocol::acceptor acceptor_;
    IpcProtocol::socket socket_;
  };


  std::unique_ptr<IpcServer> ipc_server;
}  // namespace

void ipc::start_server() {
  assert(!ipc_server);

  try {
    ipc_server = std::make_unique<IpcServer>();
    std::thread([]() { ipc_server->run(); }).detach();
  } catch (const asio::system_error& error) {
    fmt::print("Error starting IPC server {}: {}\n", error.code().value(), error.code().message());
  }
}

void ipc::stop_server() {
  if (ipc_server) {
    ipc_server->stop();
  }
}

bool ipc::is_another_instance_running() {
  asio::io_context io_context;
  IpcProtocol::socket socket(io_context);
  auto endpoint = ipc_client_endpoint();
  try {
    socket.connect(endpoint);
    return true;
  } catch (const asio::system_error&) {
    return false;
  }
}

void ipc::send_filepaths(const std::vector<std::string>& files) {
  asio::io_context io_context;
  IpcProtocol::socket socket(io_context);
  auto endpoint = ipc_client_endpoint();
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
  IpcProtocol::socket socket(io_context);
  auto endpoint = ipc_client_endpoint();
  socket.connect(endpoint);

  // flatbuffers can only be 2GB - 1B large, so often not large enough to contain the complete array.
  // Testing by me (only local connection) has shown that 64KB max message size seems to be the
  // fastest end-to-end transfer speed
  flatbuffers::FlatBufferBuilder builder(65536);
  const std::size_t MAX_FLOAT_ELMS = (65536 - 128) / sizeof(float);

  /* Metadata Message */
  auto fbs_start  = fbs::CreateArray3MetaDirect(builder, fbs::ArrayDataType_FLOAT, nx, ny, nt,
                                                fbs::BitRange_AUTODETECT, fbs::ColorMap_DEFAULT, NAN, NAN,
                                                fbs::OpacityFunction_NONE, name.c_str());
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
