// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_MESSAGE_FBS_H_
#define FLATBUFFERS_GENERATED_MESSAGE_FBS_H_

#include "flatbuffers/flatbuffers.h"

namespace fbs {

struct DictEntry;
struct DictEntryBuilder;

struct Array3Meta;
struct Array3MetaBuilder;

struct Array3MetaFlow;
struct Array3MetaFlowBuilder;

struct Array3DataChunkf;
struct Array3DataChunkfBuilder;

struct Array3DataChunku16;
struct Array3DataChunku16Builder;

struct Filepaths;
struct FilepathsBuilder;

struct Root;
struct RootBuilder;

enum BitRange {
  BitRange_AUTODETECT = 0,
  BitRange_UINT8 = 1,
  BitRange_UINT10 = 2,
  BitRange_UINT12 = 3,
  BitRange_UINT16 = 4,
  BitRange_FLOAT = 5,
  BitRange_DIFF = 6,
  BitRange_PHASE = 7,
  BitRange_PHASE_DIFF = 8,
  BitRange_MIN = BitRange_AUTODETECT,
  BitRange_MAX = BitRange_PHASE_DIFF
};

inline const BitRange (&EnumValuesBitRange())[9] {
  static const BitRange values[] = {
    BitRange_AUTODETECT,
    BitRange_UINT8,
    BitRange_UINT10,
    BitRange_UINT12,
    BitRange_UINT16,
    BitRange_FLOAT,
    BitRange_DIFF,
    BitRange_PHASE,
    BitRange_PHASE_DIFF
  };
  return values;
}

inline const char * const *EnumNamesBitRange() {
  static const char * const names[10] = {
    "AUTODETECT",
    "UINT8",
    "UINT10",
    "UINT12",
    "UINT16",
    "FLOAT",
    "DIFF",
    "PHASE",
    "PHASE_DIFF",
    nullptr
  };
  return names;
}

inline const char *EnumNameBitRange(BitRange e) {
  if (flatbuffers::IsOutRange(e, BitRange_AUTODETECT, BitRange_PHASE_DIFF)) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNamesBitRange()[index];
}

enum ColorMap {
  ColorMap_DEFAULT = 0,
  ColorMap_GRAY = 1,
  ColorMap_DIFF = 2,
  ColorMap_HSV = 3,
  ColorMap_BLACKBODY = 4,
  ColorMap_MIN = ColorMap_DEFAULT,
  ColorMap_MAX = ColorMap_BLACKBODY
};

inline const ColorMap (&EnumValuesColorMap())[5] {
  static const ColorMap values[] = {
    ColorMap_DEFAULT,
    ColorMap_GRAY,
    ColorMap_DIFF,
    ColorMap_HSV,
    ColorMap_BLACKBODY
  };
  return values;
}

inline const char * const *EnumNamesColorMap() {
  static const char * const names[6] = {
    "DEFAULT",
    "GRAY",
    "DIFF",
    "HSV",
    "BLACKBODY",
    nullptr
  };
  return names;
}

inline const char *EnumNameColorMap(ColorMap e) {
  if (flatbuffers::IsOutRange(e, ColorMap_DEFAULT, ColorMap_BLACKBODY)) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNamesColorMap()[index];
}

enum ArrayDataType {
  ArrayDataType_FLOAT = 0,
  ArrayDataType_UINT16 = 1,
  ArrayDataType_MIN = ArrayDataType_FLOAT,
  ArrayDataType_MAX = ArrayDataType_UINT16
};

inline const ArrayDataType (&EnumValuesArrayDataType())[2] {
  static const ArrayDataType values[] = {
    ArrayDataType_FLOAT,
    ArrayDataType_UINT16
  };
  return values;
}

inline const char * const *EnumNamesArrayDataType() {
  static const char * const names[3] = {
    "FLOAT",
    "UINT16",
    nullptr
  };
  return names;
}

inline const char *EnumNameArrayDataType(ArrayDataType e) {
  if (flatbuffers::IsOutRange(e, ArrayDataType_FLOAT, ArrayDataType_UINT16)) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNamesArrayDataType()[index];
}

enum TransferFunction {
  TransferFunction_NONE = 0,
  TransferFunction_LINEAR = 1,
  TransferFunction_DIFF = 2,
  TransferFunction_DIFF_POS = 3,
  TransferFunction_DIFF_NEG = 4,
  TransferFunction_MIN = TransferFunction_NONE,
  TransferFunction_MAX = TransferFunction_DIFF_NEG
};

inline const TransferFunction (&EnumValuesTransferFunction())[5] {
  static const TransferFunction values[] = {
    TransferFunction_NONE,
    TransferFunction_LINEAR,
    TransferFunction_DIFF,
    TransferFunction_DIFF_POS,
    TransferFunction_DIFF_NEG
  };
  return values;
}

inline const char * const *EnumNamesTransferFunction() {
  static const char * const names[6] = {
    "NONE",
    "LINEAR",
    "DIFF",
    "DIFF_POS",
    "DIFF_NEG",
    nullptr
  };
  return names;
}

inline const char *EnumNameTransferFunction(TransferFunction e) {
  if (flatbuffers::IsOutRange(e, TransferFunction_NONE, TransferFunction_DIFF_NEG)) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNamesTransferFunction()[index];
}

enum Data {
  Data_NONE = 0,
  Data_Filepaths = 1,
  Data_Array3Meta = 2,
  Data_Array3MetaFlow = 3,
  Data_Array3DataChunkf = 4,
  Data_Array3DataChunku16 = 5,
  Data_MIN = Data_NONE,
  Data_MAX = Data_Array3DataChunku16
};

inline const Data (&EnumValuesData())[6] {
  static const Data values[] = {
    Data_NONE,
    Data_Filepaths,
    Data_Array3Meta,
    Data_Array3MetaFlow,
    Data_Array3DataChunkf,
    Data_Array3DataChunku16
  };
  return values;
}

inline const char * const *EnumNamesData() {
  static const char * const names[7] = {
    "NONE",
    "Filepaths",
    "Array3Meta",
    "Array3MetaFlow",
    "Array3DataChunkf",
    "Array3DataChunku16",
    nullptr
  };
  return names;
}

inline const char *EnumNameData(Data e) {
  if (flatbuffers::IsOutRange(e, Data_NONE, Data_Array3DataChunku16)) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNamesData()[index];
}

template<typename T> struct DataTraits {
  static const Data enum_value = Data_NONE;
};

template<> struct DataTraits<fbs::Filepaths> {
  static const Data enum_value = Data_Filepaths;
};

template<> struct DataTraits<fbs::Array3Meta> {
  static const Data enum_value = Data_Array3Meta;
};

template<> struct DataTraits<fbs::Array3MetaFlow> {
  static const Data enum_value = Data_Array3MetaFlow;
};

template<> struct DataTraits<fbs::Array3DataChunkf> {
  static const Data enum_value = Data_Array3DataChunkf;
};

template<> struct DataTraits<fbs::Array3DataChunku16> {
  static const Data enum_value = Data_Array3DataChunku16;
};

bool VerifyData(flatbuffers::Verifier &verifier, const void *obj, Data type);
bool VerifyDataVector(flatbuffers::Verifier &verifier, const flatbuffers::Vector<flatbuffers::Offset<void>> *values, const flatbuffers::Vector<uint8_t> *types);

struct DictEntry FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef DictEntryBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_KEY = 4,
    VT_VAL = 6
  };
  const flatbuffers::String *key() const {
    return GetPointer<const flatbuffers::String *>(VT_KEY);
  }
  const flatbuffers::String *val() const {
    return GetPointer<const flatbuffers::String *>(VT_VAL);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_KEY) &&
           verifier.VerifyString(key()) &&
           VerifyOffset(verifier, VT_VAL) &&
           verifier.VerifyString(val()) &&
           verifier.EndTable();
  }
};

struct DictEntryBuilder {
  typedef DictEntry Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_key(flatbuffers::Offset<flatbuffers::String> key) {
    fbb_.AddOffset(DictEntry::VT_KEY, key);
  }
  void add_val(flatbuffers::Offset<flatbuffers::String> val) {
    fbb_.AddOffset(DictEntry::VT_VAL, val);
  }
  explicit DictEntryBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  DictEntryBuilder &operator=(const DictEntryBuilder &);
  flatbuffers::Offset<DictEntry> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<DictEntry>(end);
    return o;
  }
};

inline flatbuffers::Offset<DictEntry> CreateDictEntry(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::String> key = 0,
    flatbuffers::Offset<flatbuffers::String> val = 0) {
  DictEntryBuilder builder_(_fbb);
  builder_.add_val(val);
  builder_.add_key(key);
  return builder_.Finish();
}

inline flatbuffers::Offset<DictEntry> CreateDictEntryDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const char *key = nullptr,
    const char *val = nullptr) {
  auto key__ = key ? _fbb.CreateString(key) : 0;
  auto val__ = val ? _fbb.CreateString(val) : 0;
  return fbs::CreateDictEntry(
      _fbb,
      key__,
      val__);
}

struct Array3Meta FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef Array3MetaBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_TYPE = 4,
    VT_NX = 6,
    VT_NY = 8,
    VT_NT = 10,
    VT_NAME = 12,
    VT_DURATION = 14,
    VT_FPS = 16,
    VT_DATE = 18,
    VT_COMMENT = 20,
    VT_BITRANGE = 22,
    VT_CMAP = 24,
    VT_PARENTNAME = 26,
    VT_ALPHATRANSFERFCT = 28,
    VT_METADATA = 30
  };
  fbs::ArrayDataType type() const {
    return static_cast<fbs::ArrayDataType>(GetField<int32_t>(VT_TYPE, 0));
  }
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
  float duration() const {
    return GetField<float>(VT_DURATION, 0.0f);
  }
  float fps() const {
    return GetField<float>(VT_FPS, 0.0f);
  }
  const flatbuffers::String *date() const {
    return GetPointer<const flatbuffers::String *>(VT_DATE);
  }
  const flatbuffers::String *comment() const {
    return GetPointer<const flatbuffers::String *>(VT_COMMENT);
  }
  fbs::BitRange bitrange() const {
    return static_cast<fbs::BitRange>(GetField<int32_t>(VT_BITRANGE, 0));
  }
  fbs::ColorMap cmap() const {
    return static_cast<fbs::ColorMap>(GetField<int32_t>(VT_CMAP, 0));
  }
  const flatbuffers::String *parentName() const {
    return GetPointer<const flatbuffers::String *>(VT_PARENTNAME);
  }
  fbs::TransferFunction alphaTransferFct() const {
    return static_cast<fbs::TransferFunction>(GetField<int32_t>(VT_ALPHATRANSFERFCT, 0));
  }
  const flatbuffers::Vector<flatbuffers::Offset<fbs::DictEntry>> *metaData() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<fbs::DictEntry>> *>(VT_METADATA);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int32_t>(verifier, VT_TYPE) &&
           VerifyField<int32_t>(verifier, VT_NX) &&
           VerifyField<int32_t>(verifier, VT_NY) &&
           VerifyField<int32_t>(verifier, VT_NT) &&
           VerifyOffset(verifier, VT_NAME) &&
           verifier.VerifyString(name()) &&
           VerifyField<float>(verifier, VT_DURATION) &&
           VerifyField<float>(verifier, VT_FPS) &&
           VerifyOffset(verifier, VT_DATE) &&
           verifier.VerifyString(date()) &&
           VerifyOffset(verifier, VT_COMMENT) &&
           verifier.VerifyString(comment()) &&
           VerifyField<int32_t>(verifier, VT_BITRANGE) &&
           VerifyField<int32_t>(verifier, VT_CMAP) &&
           VerifyOffset(verifier, VT_PARENTNAME) &&
           verifier.VerifyString(parentName()) &&
           VerifyField<int32_t>(verifier, VT_ALPHATRANSFERFCT) &&
           VerifyOffset(verifier, VT_METADATA) &&
           verifier.VerifyVector(metaData()) &&
           verifier.VerifyVectorOfTables(metaData()) &&
           verifier.EndTable();
  }
};

struct Array3MetaBuilder {
  typedef Array3Meta Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_type(fbs::ArrayDataType type) {
    fbb_.AddElement<int32_t>(Array3Meta::VT_TYPE, static_cast<int32_t>(type), 0);
  }
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
  void add_duration(float duration) {
    fbb_.AddElement<float>(Array3Meta::VT_DURATION, duration, 0.0f);
  }
  void add_fps(float fps) {
    fbb_.AddElement<float>(Array3Meta::VT_FPS, fps, 0.0f);
  }
  void add_date(flatbuffers::Offset<flatbuffers::String> date) {
    fbb_.AddOffset(Array3Meta::VT_DATE, date);
  }
  void add_comment(flatbuffers::Offset<flatbuffers::String> comment) {
    fbb_.AddOffset(Array3Meta::VT_COMMENT, comment);
  }
  void add_bitrange(fbs::BitRange bitrange) {
    fbb_.AddElement<int32_t>(Array3Meta::VT_BITRANGE, static_cast<int32_t>(bitrange), 0);
  }
  void add_cmap(fbs::ColorMap cmap) {
    fbb_.AddElement<int32_t>(Array3Meta::VT_CMAP, static_cast<int32_t>(cmap), 0);
  }
  void add_parentName(flatbuffers::Offset<flatbuffers::String> parentName) {
    fbb_.AddOffset(Array3Meta::VT_PARENTNAME, parentName);
  }
  void add_alphaTransferFct(fbs::TransferFunction alphaTransferFct) {
    fbb_.AddElement<int32_t>(Array3Meta::VT_ALPHATRANSFERFCT, static_cast<int32_t>(alphaTransferFct), 0);
  }
  void add_metaData(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<fbs::DictEntry>>> metaData) {
    fbb_.AddOffset(Array3Meta::VT_METADATA, metaData);
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
    fbs::ArrayDataType type = fbs::ArrayDataType_FLOAT,
    int32_t nx = 0,
    int32_t ny = 0,
    int32_t nt = 0,
    flatbuffers::Offset<flatbuffers::String> name = 0,
    float duration = 0.0f,
    float fps = 0.0f,
    flatbuffers::Offset<flatbuffers::String> date = 0,
    flatbuffers::Offset<flatbuffers::String> comment = 0,
    fbs::BitRange bitrange = fbs::BitRange_AUTODETECT,
    fbs::ColorMap cmap = fbs::ColorMap_DEFAULT,
    flatbuffers::Offset<flatbuffers::String> parentName = 0,
    fbs::TransferFunction alphaTransferFct = fbs::TransferFunction_NONE,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<fbs::DictEntry>>> metaData = 0) {
  Array3MetaBuilder builder_(_fbb);
  builder_.add_metaData(metaData);
  builder_.add_alphaTransferFct(alphaTransferFct);
  builder_.add_parentName(parentName);
  builder_.add_cmap(cmap);
  builder_.add_bitrange(bitrange);
  builder_.add_comment(comment);
  builder_.add_date(date);
  builder_.add_fps(fps);
  builder_.add_duration(duration);
  builder_.add_name(name);
  builder_.add_nt(nt);
  builder_.add_ny(ny);
  builder_.add_nx(nx);
  builder_.add_type(type);
  return builder_.Finish();
}

inline flatbuffers::Offset<Array3Meta> CreateArray3MetaDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    fbs::ArrayDataType type = fbs::ArrayDataType_FLOAT,
    int32_t nx = 0,
    int32_t ny = 0,
    int32_t nt = 0,
    const char *name = nullptr,
    float duration = 0.0f,
    float fps = 0.0f,
    const char *date = nullptr,
    const char *comment = nullptr,
    fbs::BitRange bitrange = fbs::BitRange_AUTODETECT,
    fbs::ColorMap cmap = fbs::ColorMap_DEFAULT,
    const char *parentName = nullptr,
    fbs::TransferFunction alphaTransferFct = fbs::TransferFunction_NONE,
    const std::vector<flatbuffers::Offset<fbs::DictEntry>> *metaData = nullptr) {
  auto name__ = name ? _fbb.CreateString(name) : 0;
  auto date__ = date ? _fbb.CreateString(date) : 0;
  auto comment__ = comment ? _fbb.CreateString(comment) : 0;
  auto parentName__ = parentName ? _fbb.CreateString(parentName) : 0;
  auto metaData__ = metaData ? _fbb.CreateVector<flatbuffers::Offset<fbs::DictEntry>>(*metaData) : 0;
  return fbs::CreateArray3Meta(
      _fbb,
      type,
      nx,
      ny,
      nt,
      name__,
      duration,
      fps,
      date__,
      comment__,
      bitrange,
      cmap,
      parentName__,
      alphaTransferFct,
      metaData__);
}

struct Array3MetaFlow FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef Array3MetaFlowBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_NX = 4,
    VT_NY = 6,
    VT_NT = 8,
    VT_NAME = 10,
    VT_PARENTNAME = 12
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
  const flatbuffers::String *parentName() const {
    return GetPointer<const flatbuffers::String *>(VT_PARENTNAME);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int32_t>(verifier, VT_NX) &&
           VerifyField<int32_t>(verifier, VT_NY) &&
           VerifyField<int32_t>(verifier, VT_NT) &&
           VerifyOffset(verifier, VT_NAME) &&
           verifier.VerifyString(name()) &&
           VerifyOffset(verifier, VT_PARENTNAME) &&
           verifier.VerifyString(parentName()) &&
           verifier.EndTable();
  }
};

struct Array3MetaFlowBuilder {
  typedef Array3MetaFlow Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_nx(int32_t nx) {
    fbb_.AddElement<int32_t>(Array3MetaFlow::VT_NX, nx, 0);
  }
  void add_ny(int32_t ny) {
    fbb_.AddElement<int32_t>(Array3MetaFlow::VT_NY, ny, 0);
  }
  void add_nt(int32_t nt) {
    fbb_.AddElement<int32_t>(Array3MetaFlow::VT_NT, nt, 0);
  }
  void add_name(flatbuffers::Offset<flatbuffers::String> name) {
    fbb_.AddOffset(Array3MetaFlow::VT_NAME, name);
  }
  void add_parentName(flatbuffers::Offset<flatbuffers::String> parentName) {
    fbb_.AddOffset(Array3MetaFlow::VT_PARENTNAME, parentName);
  }
  explicit Array3MetaFlowBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  Array3MetaFlowBuilder &operator=(const Array3MetaFlowBuilder &);
  flatbuffers::Offset<Array3MetaFlow> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<Array3MetaFlow>(end);
    return o;
  }
};

inline flatbuffers::Offset<Array3MetaFlow> CreateArray3MetaFlow(
    flatbuffers::FlatBufferBuilder &_fbb,
    int32_t nx = 0,
    int32_t ny = 0,
    int32_t nt = 0,
    flatbuffers::Offset<flatbuffers::String> name = 0,
    flatbuffers::Offset<flatbuffers::String> parentName = 0) {
  Array3MetaFlowBuilder builder_(_fbb);
  builder_.add_parentName(parentName);
  builder_.add_name(name);
  builder_.add_nt(nt);
  builder_.add_ny(ny);
  builder_.add_nx(nx);
  return builder_.Finish();
}

inline flatbuffers::Offset<Array3MetaFlow> CreateArray3MetaFlowDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    int32_t nx = 0,
    int32_t ny = 0,
    int32_t nt = 0,
    const char *name = nullptr,
    const char *parentName = nullptr) {
  auto name__ = name ? _fbb.CreateString(name) : 0;
  auto parentName__ = parentName ? _fbb.CreateString(parentName) : 0;
  return fbs::CreateArray3MetaFlow(
      _fbb,
      nx,
      ny,
      nt,
      name__,
      parentName__);
}

struct Array3DataChunkf FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef Array3DataChunkfBuilder Builder;
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

struct Array3DataChunkfBuilder {
  typedef Array3DataChunkf Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_startidx(uint64_t startidx) {
    fbb_.AddElement<uint64_t>(Array3DataChunkf::VT_STARTIDX, startidx, 0);
  }
  void add_data(flatbuffers::Offset<flatbuffers::Vector<float>> data) {
    fbb_.AddOffset(Array3DataChunkf::VT_DATA, data);
  }
  explicit Array3DataChunkfBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  Array3DataChunkfBuilder &operator=(const Array3DataChunkfBuilder &);
  flatbuffers::Offset<Array3DataChunkf> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<Array3DataChunkf>(end);
    return o;
  }
};

inline flatbuffers::Offset<Array3DataChunkf> CreateArray3DataChunkf(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t startidx = 0,
    flatbuffers::Offset<flatbuffers::Vector<float>> data = 0) {
  Array3DataChunkfBuilder builder_(_fbb);
  builder_.add_startidx(startidx);
  builder_.add_data(data);
  return builder_.Finish();
}

inline flatbuffers::Offset<Array3DataChunkf> CreateArray3DataChunkfDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t startidx = 0,
    const std::vector<float> *data = nullptr) {
  auto data__ = data ? _fbb.CreateVector<float>(*data) : 0;
  return fbs::CreateArray3DataChunkf(
      _fbb,
      startidx,
      data__);
}

struct Array3DataChunku16 FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef Array3DataChunku16Builder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_STARTIDX = 4,
    VT_DATA = 6
  };
  uint64_t startidx() const {
    return GetField<uint64_t>(VT_STARTIDX, 0);
  }
  const flatbuffers::Vector<uint16_t> *data() const {
    return GetPointer<const flatbuffers::Vector<uint16_t> *>(VT_DATA);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint64_t>(verifier, VT_STARTIDX) &&
           VerifyOffset(verifier, VT_DATA) &&
           verifier.VerifyVector(data()) &&
           verifier.EndTable();
  }
};

struct Array3DataChunku16Builder {
  typedef Array3DataChunku16 Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_startidx(uint64_t startidx) {
    fbb_.AddElement<uint64_t>(Array3DataChunku16::VT_STARTIDX, startidx, 0);
  }
  void add_data(flatbuffers::Offset<flatbuffers::Vector<uint16_t>> data) {
    fbb_.AddOffset(Array3DataChunku16::VT_DATA, data);
  }
  explicit Array3DataChunku16Builder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  Array3DataChunku16Builder &operator=(const Array3DataChunku16Builder &);
  flatbuffers::Offset<Array3DataChunku16> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<Array3DataChunku16>(end);
    return o;
  }
};

inline flatbuffers::Offset<Array3DataChunku16> CreateArray3DataChunku16(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t startidx = 0,
    flatbuffers::Offset<flatbuffers::Vector<uint16_t>> data = 0) {
  Array3DataChunku16Builder builder_(_fbb);
  builder_.add_startidx(startidx);
  builder_.add_data(data);
  return builder_.Finish();
}

inline flatbuffers::Offset<Array3DataChunku16> CreateArray3DataChunku16Direct(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t startidx = 0,
    const std::vector<uint16_t> *data = nullptr) {
  auto data__ = data ? _fbb.CreateVector<uint16_t>(*data) : 0;
  return fbs::CreateArray3DataChunku16(
      _fbb,
      startidx,
      data__);
}

struct Filepaths FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef FilepathsBuilder Builder;
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
  typedef Filepaths Table;
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
  typedef RootBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_DATA_TYPE = 4,
    VT_DATA = 6
  };
  fbs::Data data_type() const {
    return static_cast<fbs::Data>(GetField<uint8_t>(VT_DATA_TYPE, 0));
  }
  const void *data() const {
    return GetPointer<const void *>(VT_DATA);
  }
  template<typename T> const T *data_as() const;
  const fbs::Filepaths *data_as_Filepaths() const {
    return data_type() == fbs::Data_Filepaths ? static_cast<const fbs::Filepaths *>(data()) : nullptr;
  }
  const fbs::Array3Meta *data_as_Array3Meta() const {
    return data_type() == fbs::Data_Array3Meta ? static_cast<const fbs::Array3Meta *>(data()) : nullptr;
  }
  const fbs::Array3MetaFlow *data_as_Array3MetaFlow() const {
    return data_type() == fbs::Data_Array3MetaFlow ? static_cast<const fbs::Array3MetaFlow *>(data()) : nullptr;
  }
  const fbs::Array3DataChunkf *data_as_Array3DataChunkf() const {
    return data_type() == fbs::Data_Array3DataChunkf ? static_cast<const fbs::Array3DataChunkf *>(data()) : nullptr;
  }
  const fbs::Array3DataChunku16 *data_as_Array3DataChunku16() const {
    return data_type() == fbs::Data_Array3DataChunku16 ? static_cast<const fbs::Array3DataChunku16 *>(data()) : nullptr;
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint8_t>(verifier, VT_DATA_TYPE) &&
           VerifyOffset(verifier, VT_DATA) &&
           VerifyData(verifier, data(), data_type()) &&
           verifier.EndTable();
  }
};

template<> inline const fbs::Filepaths *Root::data_as<fbs::Filepaths>() const {
  return data_as_Filepaths();
}

template<> inline const fbs::Array3Meta *Root::data_as<fbs::Array3Meta>() const {
  return data_as_Array3Meta();
}

template<> inline const fbs::Array3MetaFlow *Root::data_as<fbs::Array3MetaFlow>() const {
  return data_as_Array3MetaFlow();
}

template<> inline const fbs::Array3DataChunkf *Root::data_as<fbs::Array3DataChunkf>() const {
  return data_as_Array3DataChunkf();
}

template<> inline const fbs::Array3DataChunku16 *Root::data_as<fbs::Array3DataChunku16>() const {
  return data_as_Array3DataChunku16();
}

struct RootBuilder {
  typedef Root Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_data_type(fbs::Data data_type) {
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
    fbs::Data data_type = fbs::Data_NONE,
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
      auto ptr = reinterpret_cast<const fbs::Filepaths *>(obj);
      return verifier.VerifyTable(ptr);
    }
    case Data_Array3Meta: {
      auto ptr = reinterpret_cast<const fbs::Array3Meta *>(obj);
      return verifier.VerifyTable(ptr);
    }
    case Data_Array3MetaFlow: {
      auto ptr = reinterpret_cast<const fbs::Array3MetaFlow *>(obj);
      return verifier.VerifyTable(ptr);
    }
    case Data_Array3DataChunkf: {
      auto ptr = reinterpret_cast<const fbs::Array3DataChunkf *>(obj);
      return verifier.VerifyTable(ptr);
    }
    case Data_Array3DataChunku16: {
      auto ptr = reinterpret_cast<const fbs::Array3DataChunku16 *>(obj);
      return verifier.VerifyTable(ptr);
    }
    default: return true;
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
