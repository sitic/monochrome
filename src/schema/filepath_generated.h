// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_FILEPATH_IPC_H_
#define FLATBUFFERS_GENERATED_FILEPATH_IPC_H_

#include "flatbuffers/flatbuffers.h"

namespace ipc {

struct Filepaths;

struct Filepaths FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_FILE = 4
  };
  const flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>> *file() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>> *>(VT_FILE);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_FILE) &&
           verifier.VerifyVector(file()) &&
           verifier.VerifyVectorOfStrings(file()) &&
           verifier.EndTable();
  }
};

struct FilepathsBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_file(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>> file) {
    fbb_.AddOffset(Filepaths::VT_FILE, file);
  }
  explicit FilepathsBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  FilepathsBuilder &operator=(const FilepathsBuilder &);
  flatbuffers::Offset<Filepaths> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<Filepaths>(end);
    return o;
  }
};

inline flatbuffers::Offset<Filepaths> CreateFilepaths(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>> file = 0) {
  FilepathsBuilder builder_(_fbb);
  builder_.add_file(file);
  return builder_.Finish();
}

inline flatbuffers::Offset<Filepaths> CreateFilepathsDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const std::vector<flatbuffers::Offset<flatbuffers::String>> *file = nullptr) {
  auto file__ = file ? _fbb.CreateVector<flatbuffers::Offset<flatbuffers::String>>(*file) : 0;
  return ipc::CreateFilepaths(
      _fbb,
      file__);
}

inline const ipc::Filepaths *GetFilepaths(const void *buf) {
  return flatbuffers::GetRoot<ipc::Filepaths>(buf);
}

inline const ipc::Filepaths *GetSizePrefixedFilepaths(const void *buf) {
  return flatbuffers::GetSizePrefixedRoot<ipc::Filepaths>(buf);
}

inline bool VerifyFilepathsBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<ipc::Filepaths>(nullptr);
}

inline bool VerifySizePrefixedFilepathsBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<ipc::Filepaths>(nullptr);
}

inline void FinishFilepathsBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<ipc::Filepaths> root) {
  fbb.Finish(root);
}

inline void FinishSizePrefixedFilepathsBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<ipc::Filepaths> root) {
  fbb.FinishSizePrefixed(root);
}

}  // namespace ipc

#endif  // FLATBUFFERS_GENERATED_FILEPATH_IPC_H_
