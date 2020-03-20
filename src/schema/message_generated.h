// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_MESSAGE_FBS_H_
#define FLATBUFFERS_GENERATED_MESSAGE_FBS_H_

#include "flatbuffers/flatbuffers.h"

namespace fbs {

struct Array3Meta;

struct Array3DataChunk;

struct Filepaths;

struct Root;

enum Data {
  Data_NONE = 0,
  Data_Filepaths = 1,
  Data_Array3Meta = 2,
  Data_Array3DataChunk = 3,
  Data_MIN = Data_NONE,
  Data_MAX = Data_Array3DataChunk
};

inline const Data (&EnumValuesData())[4] {
  static const Data values[] = {
    Data_NONE,
    Data_Filepaths,
    Data_Array3Meta,
    Data_Array3DataChunk
  };
  return values;
}

inline const char * const *EnumNamesData() {
  static const char * const names[] = {
    "NONE",
    "Filepaths",
    "Array3Meta",
    "Array3DataChunk",
    nullptr
  };
  return names;
}

inline const char *EnumNameData(Data e) {
  if (e < Data_NONE || e > Data_Array3DataChunk) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNamesData()[index];
}

template<typename T> struct DataTraits {
  static const Data enum_value = Data_NONE;
};

template<> struct DataTraits<Filepaths> {
  static const Data enum_value = Data_Filepaths;
};

template<> struct DataTraits<Array3Meta> {
  static const Data enum_value = Data_Array3Meta;
};

template<> struct DataTraits<Array3DataChunk> {
  static const Data enum_value = Data_Array3DataChunk;
};

bool VerifyData(flatbuffers::Verifier &verifier, const void *obj, Data type);
bool VerifyDataVector(flatbuffers::Verifier &verifier, const flatbuffers::Vector<flatbuffers::Offset<void>> *values, const flatbuffers::Vector<uint8_t> *types);

struct Array3Meta FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_NX = 4,
    VT_NY = 6,
    VT_NT = 8,
    VT_NAME = 10
  };
  int32_t nx() const {
    return GetField<int32_t>(VT_NX, 0);
  }
  int32_t ny() const {
    return GetField<int32_t>(VT_NY, 0);
  }
  int32_t nt() const {
    return GetField<int32_t>(VT_NT, 0);
  }
  const flatbuffers::String *name() const {
    return GetPointer<const flatbuffers::String *>(VT_NAME);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int32_t>(verifier, VT_NX) &&
           VerifyField<int32_t>(verifier, VT_NY) &&
           VerifyField<int32_t>(verifier, VT_NT) &&
           VerifyOffset(verifier, VT_NAME) &&
           verifier.VerifyString(name()) &&
           verifier.EndTable();
  }
};

struct Array3MetaBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_nx(int32_t nx) {
    fbb_.AddElement<int32_t>(Array3Meta::VT_NX, nx, 0);
  }
  void add_ny(int32_t ny) {
    fbb_.AddElement<int32_t>(Array3Meta::VT_NY, ny, 0);
  }
  void add_nt(int32_t nt) {
    fbb_.AddElement<int32_t>(Array3Meta::VT_NT, nt, 0);
  }
  void add_name(flatbuffers::Offset<flatbuffers::String> name) {
    fbb_.AddOffset(Array3Meta::VT_NAME, name);
  }
  explicit Array3MetaBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  Array3MetaBuilder &operator=(const Array3MetaBuilder &);
  flatbuffers::Offset<Array3Meta> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<Array3Meta>(end);
    return o;
  }
};

inline flatbuffers::Offset<Array3Meta> CreateArray3Meta(
    flatbuffers::FlatBufferBuilder &_fbb,
    int32_t nx = 0,
    int32_t ny = 0,
    int32_t nt = 0,
    flatbuffers::Offset<flatbuffers::String> name = 0) {
  Array3MetaBuilder builder_(_fbb);
  builder_.add_name(name);
  builder_.add_nt(nt);
  builder_.add_ny(ny);
  builder_.add_nx(nx);
  return builder_.Finish();
}

inline flatbuffers::Offset<Array3Meta> CreateArray3MetaDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    int32_t nx = 0,
    int32_t ny = 0,
    int32_t nt = 0,
    const char *name = nullptr) {
  auto name__ = name ? _fbb.CreateString(name) : 0;
  return fbs::CreateArray3Meta(
      _fbb,
      nx,
      ny,
      nt,
      name__);
}

struct Array3DataChunk FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_STARTIDX = 4,
    VT_DATA = 6
  };
  uint64_t startidx() const {
    return GetField<uint64_t>(VT_STARTIDX, 0);
  }
  const flatbuffers::Vector<float> *data() const {
    return GetPointer<const flatbuffers::Vector<float> *>(VT_DATA);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint64_t>(verifier, VT_STARTIDX) &&
           VerifyOffset(verifier, VT_DATA) &&
           verifier.VerifyVector(data()) &&
           verifier.EndTable();
  }
};

struct Array3DataChunkBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_startidx(uint64_t startidx) {
    fbb_.AddElement<uint64_t>(Array3DataChunk::VT_STARTIDX, startidx, 0);
  }
  void add_data(flatbuffers::Offset<flatbuffers::Vector<float>> data) {
    fbb_.AddOffset(Array3DataChunk::VT_DATA, data);
  }
  explicit Array3DataChunkBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  Array3DataChunkBuilder &operator=(const Array3DataChunkBuilder &);
  flatbuffers::Offset<Array3DataChunk> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<Array3DataChunk>(end);
    return o;
  }
};

inline flatbuffers::Offset<Array3DataChunk> CreateArray3DataChunk(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t startidx = 0,
    flatbuffers::Offset<flatbuffers::Vector<float>> data = 0) {
  Array3DataChunkBuilder builder_(_fbb);
  builder_.add_startidx(startidx);
  builder_.add_data(data);
  return builder_.Finish();
}

inline flatbuffers::Offset<Array3DataChunk> CreateArray3DataChunkDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t startidx = 0,
    const std::vector<float> *data = nullptr) {
  auto data__ = data ? _fbb.CreateVector<float>(*data) : 0;
  return fbs::CreateArray3DataChunk(
      _fbb,
      startidx,
      data__);
}

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
  return fbs::CreateFilepaths(
      _fbb,
      file__);
}

struct Root FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_DATA_TYPE = 4,
    VT_DATA = 6
  };
  Data data_type() const {
    return static_cast<Data>(GetField<uint8_t>(VT_DATA_TYPE, 0));
  }
  const void *data() const {
    return GetPointer<const void *>(VT_DATA);
  }
  template<typename T> const T *data_as() const;
  const Filepaths *data_as_Filepaths() const {
    return data_type() == Data_Filepaths ? static_cast<const Filepaths *>(data()) : nullptr;
  }
  const Array3Meta *data_as_Array3Meta() const {
    return data_type() == Data_Array3Meta ? static_cast<const Array3Meta *>(data()) : nullptr;
  }
  const Array3DataChunk *data_as_Array3DataChunk() const {
    return data_type() == Data_Array3DataChunk ? static_cast<const Array3DataChunk *>(data()) : nullptr;
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint8_t>(verifier, VT_DATA_TYPE) &&
           VerifyOffset(verifier, VT_DATA) &&
           VerifyData(verifier, data(), data_type()) &&
           verifier.EndTable();
  }
};

template<> inline const Filepaths *Root::data_as<Filepaths>() const {
  return data_as_Filepaths();
}

template<> inline const Array3Meta *Root::data_as<Array3Meta>() const {
  return data_as_Array3Meta();
}

template<> inline const Array3DataChunk *Root::data_as<Array3DataChunk>() const {
  return data_as_Array3DataChunk();
}

struct RootBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_data_type(Data data_type) {
    fbb_.AddElement<uint8_t>(Root::VT_DATA_TYPE, static_cast<uint8_t>(data_type), 0);
  }
  void add_data(flatbuffers::Offset<void> data) {
    fbb_.AddOffset(Root::VT_DATA, data);
  }
  explicit RootBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  RootBuilder &operator=(const RootBuilder &);
  flatbuffers::Offset<Root> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<Root>(end);
    return o;
  }
};

inline flatbuffers::Offset<Root> CreateRoot(
    flatbuffers::FlatBufferBuilder &_fbb,
    Data data_type = Data_NONE,
    flatbuffers::Offset<void> data = 0) {
  RootBuilder builder_(_fbb);
  builder_.add_data(data);
  builder_.add_data_type(data_type);
  return builder_.Finish();
}

inline bool VerifyData(flatbuffers::Verifier &verifier, const void *obj, Data type) {
  switch (type) {
    case Data_NONE: {
      return true;
    }
    case Data_Filepaths: {
      auto ptr = reinterpret_cast<const Filepaths *>(obj);
      return verifier.VerifyTable(ptr);
    }
    case Data_Array3Meta: {
      auto ptr = reinterpret_cast<const Array3Meta *>(obj);
      return verifier.VerifyTable(ptr);
    }
    case Data_Array3DataChunk: {
      auto ptr = reinterpret_cast<const Array3DataChunk *>(obj);
      return verifier.VerifyTable(ptr);
    }
    default: return false;
  }
}

inline bool VerifyDataVector(flatbuffers::Verifier &verifier, const flatbuffers::Vector<flatbuffers::Offset<void>> *values, const flatbuffers::Vector<uint8_t> *types) {
  if (!values || !types) return !values && !types;
  if (values->size() != types->size()) return false;
  for (flatbuffers::uoffset_t i = 0; i < values->size(); ++i) {
    if (!VerifyData(
        verifier,  values->Get(i), types->GetEnum<Data>(i))) {
      return false;
    }
  }
  return true;
}

inline const fbs::Root *GetRoot(const void *buf) {
  return flatbuffers::GetRoot<fbs::Root>(buf);
}

inline const fbs::Root *GetSizePrefixedRoot(const void *buf) {
  return flatbuffers::GetSizePrefixedRoot<fbs::Root>(buf);
}

inline bool VerifyRootBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<fbs::Root>(nullptr);
}

inline bool VerifySizePrefixedRootBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<fbs::Root>(nullptr);
}

inline void FinishRootBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<fbs::Root> root) {
  fbb.Finish(root);
}

inline void FinishSizePrefixedRootBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<fbs::Root> root) {
  fbb.FinishSizePrefixed(root);
}

}  // namespace fbs

#endif  // FLATBUFFERS_GENERATED_MESSAGE_FBS_H_
