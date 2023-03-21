// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: send_file.proto

#include "mqas/tools/proto/send_file.pb.h"

#include <algorithm>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>

PROTOBUF_PRAGMA_INIT_SEG

namespace _pb = ::PROTOBUF_NAMESPACE_ID;
namespace _pbi = _pb::internal;

namespace mqas {
namespace tools {
namespace proto {
PROTOBUF_CONSTEXPR ReqSendFile::ReqSendFile(
    ::_pbi::ConstantInitialized): _impl_{
    /*decltype(_impl_.name_)*/{&::_pbi::fixed_address_empty_string, ::_pbi::ConstantInitialized{}}
  , /*decltype(_impl_.size_)*/uint64_t{0u}
  , /*decltype(_impl_.buf_size_)*/0u
  , /*decltype(_impl_.overlay_)*/false
  , /*decltype(_impl_._cached_size_)*/{}} {}
struct ReqSendFileDefaultTypeInternal {
  PROTOBUF_CONSTEXPR ReqSendFileDefaultTypeInternal()
      : _instance(::_pbi::ConstantInitialized{}) {}
  ~ReqSendFileDefaultTypeInternal() {}
  union {
    ReqSendFile _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 ReqSendFileDefaultTypeInternal _ReqSendFile_default_instance_;
PROTOBUF_CONSTEXPR ReqSendFileRet::ReqSendFileRet(
    ::_pbi::ConstantInitialized): _impl_{
    /*decltype(_impl_._has_bits_)*/{}
  , /*decltype(_impl_._cached_size_)*/{}
  , /*decltype(_impl_.code_)*/0
  , /*decltype(_impl_.error_code_)*/0} {}
struct ReqSendFileRetDefaultTypeInternal {
  PROTOBUF_CONSTEXPR ReqSendFileRetDefaultTypeInternal()
      : _instance(::_pbi::ConstantInitialized{}) {}
  ~ReqSendFileRetDefaultTypeInternal() {}
  union {
    ReqSendFileRet _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 ReqSendFileRetDefaultTypeInternal _ReqSendFileRet_default_instance_;
PROTOBUF_CONSTEXPR SendFileEnd::SendFileEnd(
    ::_pbi::ConstantInitialized): _impl_{
    /*decltype(_impl_.name_)*/{&::_pbi::fixed_address_empty_string, ::_pbi::ConstantInitialized{}}
  , /*decltype(_impl_.md5_)*/{&::_pbi::fixed_address_empty_string, ::_pbi::ConstantInitialized{}}
  , /*decltype(_impl_._cached_size_)*/{}} {}
struct SendFileEndDefaultTypeInternal {
  PROTOBUF_CONSTEXPR SendFileEndDefaultTypeInternal()
      : _instance(::_pbi::ConstantInitialized{}) {}
  ~SendFileEndDefaultTypeInternal() {}
  union {
    SendFileEnd _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 SendFileEndDefaultTypeInternal _SendFileEnd_default_instance_;
}  // namespace proto
}  // namespace tools
}  // namespace mqas
static ::_pb::Metadata file_level_metadata_send_5ffile_2eproto[3];
static const ::_pb::EnumDescriptor* file_level_enum_descriptors_send_5ffile_2eproto[1];
static constexpr ::_pb::ServiceDescriptor const** file_level_service_descriptors_send_5ffile_2eproto = nullptr;

const uint32_t TableStruct_send_5ffile_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::mqas::tools::proto::ReqSendFile, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  ~0u,  // no _inlined_string_donated_
  PROTOBUF_FIELD_OFFSET(::mqas::tools::proto::ReqSendFile, _impl_.name_),
  PROTOBUF_FIELD_OFFSET(::mqas::tools::proto::ReqSendFile, _impl_.size_),
  PROTOBUF_FIELD_OFFSET(::mqas::tools::proto::ReqSendFile, _impl_.buf_size_),
  PROTOBUF_FIELD_OFFSET(::mqas::tools::proto::ReqSendFile, _impl_.overlay_),
  PROTOBUF_FIELD_OFFSET(::mqas::tools::proto::ReqSendFileRet, _impl_._has_bits_),
  PROTOBUF_FIELD_OFFSET(::mqas::tools::proto::ReqSendFileRet, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  ~0u,  // no _inlined_string_donated_
  PROTOBUF_FIELD_OFFSET(::mqas::tools::proto::ReqSendFileRet, _impl_.code_),
  PROTOBUF_FIELD_OFFSET(::mqas::tools::proto::ReqSendFileRet, _impl_.error_code_),
  ~0u,
  0,
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::mqas::tools::proto::SendFileEnd, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  ~0u,  // no _inlined_string_donated_
  PROTOBUF_FIELD_OFFSET(::mqas::tools::proto::SendFileEnd, _impl_.name_),
  PROTOBUF_FIELD_OFFSET(::mqas::tools::proto::SendFileEnd, _impl_.md5_),
};
static const ::_pbi::MigrationSchema schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  { 0, -1, -1, sizeof(::mqas::tools::proto::ReqSendFile)},
  { 10, 18, -1, sizeof(::mqas::tools::proto::ReqSendFileRet)},
  { 20, -1, -1, sizeof(::mqas::tools::proto::SendFileEnd)},
};

static const ::_pb::Message* const file_default_instances[] = {
  &::mqas::tools::proto::_ReqSendFile_default_instance_._instance,
  &::mqas::tools::proto::_ReqSendFileRet_default_instance_._instance,
  &::mqas::tools::proto::_SendFileEnd_default_instance_._instance,
};

const char descriptor_table_protodef_send_5ffile_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) =
  "\n\017send_file.proto\022\020mqas.tools.proto\"L\n\013R"
  "eqSendFile\022\014\n\004name\030\001 \001(\t\022\014\n\004size\030\002 \001(\004\022\020"
  "\n\010buf_size\030\003 \001(\r\022\017\n\007overlay\030\004 \001(\010\"\330\001\n\016Re"
  "qSendFileRet\0223\n\004code\030\001 \001(\0162%.mqas.tools."
  "proto.ReqSendFileRet.Code\022\027\n\nerror_code\030"
  "\002 \001(\021H\000\210\001\001\"i\n\004Code\022\006\n\002ok\020\000\022\022\n\016already_ex"
  "ists\020\001\022\017\n\013open_failed\020\002\022\020\n\014write_failed\020"
  "\003\022\020\n\014close_failed\020\004\022\020\n\014md5_not_same\020\005B\r\n"
  "\013_error_code\"(\n\013SendFileEnd\022\014\n\004name\030\001 \001("
  "\t\022\013\n\003md5\030\002 \001(\014b\006proto3"
  ;
static ::_pbi::once_flag descriptor_table_send_5ffile_2eproto_once;
const ::_pbi::DescriptorTable descriptor_table_send_5ffile_2eproto = {
    false, false, 382, descriptor_table_protodef_send_5ffile_2eproto,
    "send_file.proto",
    &descriptor_table_send_5ffile_2eproto_once, nullptr, 0, 3,
    schemas, file_default_instances, TableStruct_send_5ffile_2eproto::offsets,
    file_level_metadata_send_5ffile_2eproto, file_level_enum_descriptors_send_5ffile_2eproto,
    file_level_service_descriptors_send_5ffile_2eproto,
};
PROTOBUF_ATTRIBUTE_WEAK const ::_pbi::DescriptorTable* descriptor_table_send_5ffile_2eproto_getter() {
  return &descriptor_table_send_5ffile_2eproto;
}

// Force running AddDescriptors() at dynamic initialization time.
PROTOBUF_ATTRIBUTE_INIT_PRIORITY2 static ::_pbi::AddDescriptorsRunner dynamic_init_dummy_send_5ffile_2eproto(&descriptor_table_send_5ffile_2eproto);
namespace mqas {
namespace tools {
namespace proto {
const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* ReqSendFileRet_Code_descriptor() {
  ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(&descriptor_table_send_5ffile_2eproto);
  return file_level_enum_descriptors_send_5ffile_2eproto[0];
}
bool ReqSendFileRet_Code_IsValid(int value) {
  switch (value) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
      return true;
    default:
      return false;
  }
}

#if (__cplusplus < 201703) && (!defined(_MSC_VER) || (_MSC_VER >= 1900 && _MSC_VER < 1912))
constexpr ReqSendFileRet_Code ReqSendFileRet::ok;
constexpr ReqSendFileRet_Code ReqSendFileRet::already_exists;
constexpr ReqSendFileRet_Code ReqSendFileRet::open_failed;
constexpr ReqSendFileRet_Code ReqSendFileRet::write_failed;
constexpr ReqSendFileRet_Code ReqSendFileRet::close_failed;
constexpr ReqSendFileRet_Code ReqSendFileRet::md5_not_same;
constexpr ReqSendFileRet_Code ReqSendFileRet::Code_MIN;
constexpr ReqSendFileRet_Code ReqSendFileRet::Code_MAX;
constexpr int ReqSendFileRet::Code_ARRAYSIZE;
#endif  // (__cplusplus < 201703) && (!defined(_MSC_VER) || (_MSC_VER >= 1900 && _MSC_VER < 1912))

// ===================================================================

class ReqSendFile::_Internal {
 public:
};

ReqSendFile::ReqSendFile(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                         bool is_message_owned)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena, is_message_owned) {
  SharedCtor(arena, is_message_owned);
  // @@protoc_insertion_point(arena_constructor:mqas.tools.proto.ReqSendFile)
}
ReqSendFile::ReqSendFile(const ReqSendFile& from)
  : ::PROTOBUF_NAMESPACE_ID::Message() {
  ReqSendFile* const _this = this; (void)_this;
  new (&_impl_) Impl_{
      decltype(_impl_.name_){}
    , decltype(_impl_.size_){}
    , decltype(_impl_.buf_size_){}
    , decltype(_impl_.overlay_){}
    , /*decltype(_impl_._cached_size_)*/{}};

  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  _impl_.name_.InitDefault();
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    _impl_.name_.Set("", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (!from._internal_name().empty()) {
    _this->_impl_.name_.Set(from._internal_name(), 
      _this->GetArenaForAllocation());
  }
  ::memcpy(&_impl_.size_, &from._impl_.size_,
    static_cast<size_t>(reinterpret_cast<char*>(&_impl_.overlay_) -
    reinterpret_cast<char*>(&_impl_.size_)) + sizeof(_impl_.overlay_));
  // @@protoc_insertion_point(copy_constructor:mqas.tools.proto.ReqSendFile)
}

inline void ReqSendFile::SharedCtor(
    ::_pb::Arena* arena, bool is_message_owned) {
  (void)arena;
  (void)is_message_owned;
  new (&_impl_) Impl_{
      decltype(_impl_.name_){}
    , decltype(_impl_.size_){uint64_t{0u}}
    , decltype(_impl_.buf_size_){0u}
    , decltype(_impl_.overlay_){false}
    , /*decltype(_impl_._cached_size_)*/{}
  };
  _impl_.name_.InitDefault();
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    _impl_.name_.Set("", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
}

ReqSendFile::~ReqSendFile() {
  // @@protoc_insertion_point(destructor:mqas.tools.proto.ReqSendFile)
  if (auto *arena = _internal_metadata_.DeleteReturnArena<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>()) {
  (void)arena;
    return;
  }
  SharedDtor();
}

inline void ReqSendFile::SharedDtor() {
  GOOGLE_DCHECK(GetArenaForAllocation() == nullptr);
  _impl_.name_.Destroy();
}

void ReqSendFile::SetCachedSize(int size) const {
  _impl_._cached_size_.Set(size);
}

void ReqSendFile::Clear() {
// @@protoc_insertion_point(message_clear_start:mqas.tools.proto.ReqSendFile)
  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  _impl_.name_.ClearToEmpty();
  ::memset(&_impl_.size_, 0, static_cast<size_t>(
      reinterpret_cast<char*>(&_impl_.overlay_) -
      reinterpret_cast<char*>(&_impl_.size_)) + sizeof(_impl_.overlay_));
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* ReqSendFile::_InternalParse(const char* ptr, ::_pbi::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    uint32_t tag;
    ptr = ::_pbi::ReadTag(ptr, &tag);
    switch (tag >> 3) {
      // string name = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 10)) {
          auto str = _internal_mutable_name();
          ptr = ::_pbi::InlineGreedyStringParser(str, ptr, ctx);
          CHK_(ptr);
          CHK_(::_pbi::VerifyUTF8(str, "mqas.tools.proto.ReqSendFile.name"));
        } else
          goto handle_unusual;
        continue;
      // uint64 size = 2;
      case 2:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 16)) {
          _impl_.size_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint64(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // uint32 buf_size = 3;
      case 3:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 24)) {
          _impl_.buf_size_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint32(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // bool overlay = 4;
      case 4:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 32)) {
          _impl_.overlay_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint64(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      default:
        goto handle_unusual;
    }  // switch
  handle_unusual:
    if ((tag == 0) || ((tag & 7) == 4)) {
      CHK_(ptr);
      ctx->SetLastTag(tag);
      goto message_done;
    }
    ptr = UnknownFieldParse(
        tag,
        _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(),
        ptr, ctx);
    CHK_(ptr != nullptr);
  }  // while
message_done:
  return ptr;
failure:
  ptr = nullptr;
  goto message_done;
#undef CHK_
}

uint8_t* ReqSendFile::_InternalSerialize(
    uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:mqas.tools.proto.ReqSendFile)
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  // string name = 1;
  if (!this->_internal_name().empty()) {
    ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::VerifyUtf8String(
      this->_internal_name().data(), static_cast<int>(this->_internal_name().length()),
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::SERIALIZE,
      "mqas.tools.proto.ReqSendFile.name");
    target = stream->WriteStringMaybeAliased(
        1, this->_internal_name(), target);
  }

  // uint64 size = 2;
  if (this->_internal_size() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteUInt64ToArray(2, this->_internal_size(), target);
  }

  // uint32 buf_size = 3;
  if (this->_internal_buf_size() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteUInt32ToArray(3, this->_internal_buf_size(), target);
  }

  // bool overlay = 4;
  if (this->_internal_overlay() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteBoolToArray(4, this->_internal_overlay(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:mqas.tools.proto.ReqSendFile)
  return target;
}

size_t ReqSendFile::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:mqas.tools.proto.ReqSendFile)
  size_t total_size = 0;

  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // string name = 1;
  if (!this->_internal_name().empty()) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::StringSize(
        this->_internal_name());
  }

  // uint64 size = 2;
  if (this->_internal_size() != 0) {
    total_size += ::_pbi::WireFormatLite::UInt64SizePlusOne(this->_internal_size());
  }

  // uint32 buf_size = 3;
  if (this->_internal_buf_size() != 0) {
    total_size += ::_pbi::WireFormatLite::UInt32SizePlusOne(this->_internal_buf_size());
  }

  // bool overlay = 4;
  if (this->_internal_overlay() != 0) {
    total_size += 1 + 1;
  }

  return MaybeComputeUnknownFieldsSize(total_size, &_impl_._cached_size_);
}

const ::PROTOBUF_NAMESPACE_ID::Message::ClassData ReqSendFile::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::Message::CopyWithSourceCheck,
    ReqSendFile::MergeImpl
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*ReqSendFile::GetClassData() const { return &_class_data_; }


void ReqSendFile::MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg) {
  auto* const _this = static_cast<ReqSendFile*>(&to_msg);
  auto& from = static_cast<const ReqSendFile&>(from_msg);
  // @@protoc_insertion_point(class_specific_merge_from_start:mqas.tools.proto.ReqSendFile)
  GOOGLE_DCHECK_NE(&from, _this);
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  if (!from._internal_name().empty()) {
    _this->_internal_set_name(from._internal_name());
  }
  if (from._internal_size() != 0) {
    _this->_internal_set_size(from._internal_size());
  }
  if (from._internal_buf_size() != 0) {
    _this->_internal_set_buf_size(from._internal_buf_size());
  }
  if (from._internal_overlay() != 0) {
    _this->_internal_set_overlay(from._internal_overlay());
  }
  _this->_internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
}

void ReqSendFile::CopyFrom(const ReqSendFile& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:mqas.tools.proto.ReqSendFile)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool ReqSendFile::IsInitialized() const {
  return true;
}

void ReqSendFile::InternalSwap(ReqSendFile* other) {
  using std::swap;
  auto* lhs_arena = GetArenaForAllocation();
  auto* rhs_arena = other->GetArenaForAllocation();
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::InternalSwap(
      &_impl_.name_, lhs_arena,
      &other->_impl_.name_, rhs_arena
  );
  ::PROTOBUF_NAMESPACE_ID::internal::memswap<
      PROTOBUF_FIELD_OFFSET(ReqSendFile, _impl_.overlay_)
      + sizeof(ReqSendFile::_impl_.overlay_)
      - PROTOBUF_FIELD_OFFSET(ReqSendFile, _impl_.size_)>(
          reinterpret_cast<char*>(&_impl_.size_),
          reinterpret_cast<char*>(&other->_impl_.size_));
}

::PROTOBUF_NAMESPACE_ID::Metadata ReqSendFile::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_send_5ffile_2eproto_getter, &descriptor_table_send_5ffile_2eproto_once,
      file_level_metadata_send_5ffile_2eproto[0]);
}

// ===================================================================

class ReqSendFileRet::_Internal {
 public:
  using HasBits = decltype(std::declval<ReqSendFileRet>()._impl_._has_bits_);
  static void set_has_error_code(HasBits* has_bits) {
    (*has_bits)[0] |= 1u;
  }
};

ReqSendFileRet::ReqSendFileRet(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                         bool is_message_owned)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena, is_message_owned) {
  SharedCtor(arena, is_message_owned);
  // @@protoc_insertion_point(arena_constructor:mqas.tools.proto.ReqSendFileRet)
}
ReqSendFileRet::ReqSendFileRet(const ReqSendFileRet& from)
  : ::PROTOBUF_NAMESPACE_ID::Message() {
  ReqSendFileRet* const _this = this; (void)_this;
  new (&_impl_) Impl_{
      decltype(_impl_._has_bits_){from._impl_._has_bits_}
    , /*decltype(_impl_._cached_size_)*/{}
    , decltype(_impl_.code_){}
    , decltype(_impl_.error_code_){}};

  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  ::memcpy(&_impl_.code_, &from._impl_.code_,
    static_cast<size_t>(reinterpret_cast<char*>(&_impl_.error_code_) -
    reinterpret_cast<char*>(&_impl_.code_)) + sizeof(_impl_.error_code_));
  // @@protoc_insertion_point(copy_constructor:mqas.tools.proto.ReqSendFileRet)
}

inline void ReqSendFileRet::SharedCtor(
    ::_pb::Arena* arena, bool is_message_owned) {
  (void)arena;
  (void)is_message_owned;
  new (&_impl_) Impl_{
      decltype(_impl_._has_bits_){}
    , /*decltype(_impl_._cached_size_)*/{}
    , decltype(_impl_.code_){0}
    , decltype(_impl_.error_code_){0}
  };
}

ReqSendFileRet::~ReqSendFileRet() {
  // @@protoc_insertion_point(destructor:mqas.tools.proto.ReqSendFileRet)
  if (auto *arena = _internal_metadata_.DeleteReturnArena<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>()) {
  (void)arena;
    return;
  }
  SharedDtor();
}

inline void ReqSendFileRet::SharedDtor() {
  GOOGLE_DCHECK(GetArenaForAllocation() == nullptr);
}

void ReqSendFileRet::SetCachedSize(int size) const {
  _impl_._cached_size_.Set(size);
}

void ReqSendFileRet::Clear() {
// @@protoc_insertion_point(message_clear_start:mqas.tools.proto.ReqSendFileRet)
  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  _impl_.code_ = 0;
  _impl_.error_code_ = 0;
  _impl_._has_bits_.Clear();
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* ReqSendFileRet::_InternalParse(const char* ptr, ::_pbi::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  _Internal::HasBits has_bits{};
  while (!ctx->Done(&ptr)) {
    uint32_t tag;
    ptr = ::_pbi::ReadTag(ptr, &tag);
    switch (tag >> 3) {
      // .mqas.tools.proto.ReqSendFileRet.Code code = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 8)) {
          uint64_t val = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint64(&ptr);
          CHK_(ptr);
          _internal_set_code(static_cast<::mqas::tools::proto::ReqSendFileRet_Code>(val));
        } else
          goto handle_unusual;
        continue;
      // optional sint32 error_code = 2;
      case 2:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 16)) {
          _Internal::set_has_error_code(&has_bits);
          _impl_.error_code_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarintZigZag32(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      default:
        goto handle_unusual;
    }  // switch
  handle_unusual:
    if ((tag == 0) || ((tag & 7) == 4)) {
      CHK_(ptr);
      ctx->SetLastTag(tag);
      goto message_done;
    }
    ptr = UnknownFieldParse(
        tag,
        _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(),
        ptr, ctx);
    CHK_(ptr != nullptr);
  }  // while
message_done:
  _impl_._has_bits_.Or(has_bits);
  return ptr;
failure:
  ptr = nullptr;
  goto message_done;
#undef CHK_
}

uint8_t* ReqSendFileRet::_InternalSerialize(
    uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:mqas.tools.proto.ReqSendFileRet)
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  // .mqas.tools.proto.ReqSendFileRet.Code code = 1;
  if (this->_internal_code() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteEnumToArray(
      1, this->_internal_code(), target);
  }

  // optional sint32 error_code = 2;
  if (_internal_has_error_code()) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteSInt32ToArray(2, this->_internal_error_code(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:mqas.tools.proto.ReqSendFileRet)
  return target;
}

size_t ReqSendFileRet::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:mqas.tools.proto.ReqSendFileRet)
  size_t total_size = 0;

  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // .mqas.tools.proto.ReqSendFileRet.Code code = 1;
  if (this->_internal_code() != 0) {
    total_size += 1 +
      ::_pbi::WireFormatLite::EnumSize(this->_internal_code());
  }

  // optional sint32 error_code = 2;
  cached_has_bits = _impl_._has_bits_[0];
  if (cached_has_bits & 0x00000001u) {
    total_size += ::_pbi::WireFormatLite::SInt32SizePlusOne(this->_internal_error_code());
  }

  return MaybeComputeUnknownFieldsSize(total_size, &_impl_._cached_size_);
}

const ::PROTOBUF_NAMESPACE_ID::Message::ClassData ReqSendFileRet::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::Message::CopyWithSourceCheck,
    ReqSendFileRet::MergeImpl
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*ReqSendFileRet::GetClassData() const { return &_class_data_; }


void ReqSendFileRet::MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg) {
  auto* const _this = static_cast<ReqSendFileRet*>(&to_msg);
  auto& from = static_cast<const ReqSendFileRet&>(from_msg);
  // @@protoc_insertion_point(class_specific_merge_from_start:mqas.tools.proto.ReqSendFileRet)
  GOOGLE_DCHECK_NE(&from, _this);
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  if (from._internal_code() != 0) {
    _this->_internal_set_code(from._internal_code());
  }
  if (from._internal_has_error_code()) {
    _this->_internal_set_error_code(from._internal_error_code());
  }
  _this->_internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
}

void ReqSendFileRet::CopyFrom(const ReqSendFileRet& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:mqas.tools.proto.ReqSendFileRet)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool ReqSendFileRet::IsInitialized() const {
  return true;
}

void ReqSendFileRet::InternalSwap(ReqSendFileRet* other) {
  using std::swap;
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  swap(_impl_._has_bits_[0], other->_impl_._has_bits_[0]);
  ::PROTOBUF_NAMESPACE_ID::internal::memswap<
      PROTOBUF_FIELD_OFFSET(ReqSendFileRet, _impl_.error_code_)
      + sizeof(ReqSendFileRet::_impl_.error_code_)
      - PROTOBUF_FIELD_OFFSET(ReqSendFileRet, _impl_.code_)>(
          reinterpret_cast<char*>(&_impl_.code_),
          reinterpret_cast<char*>(&other->_impl_.code_));
}

::PROTOBUF_NAMESPACE_ID::Metadata ReqSendFileRet::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_send_5ffile_2eproto_getter, &descriptor_table_send_5ffile_2eproto_once,
      file_level_metadata_send_5ffile_2eproto[1]);
}

// ===================================================================

class SendFileEnd::_Internal {
 public:
};

SendFileEnd::SendFileEnd(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                         bool is_message_owned)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena, is_message_owned) {
  SharedCtor(arena, is_message_owned);
  // @@protoc_insertion_point(arena_constructor:mqas.tools.proto.SendFileEnd)
}
SendFileEnd::SendFileEnd(const SendFileEnd& from)
  : ::PROTOBUF_NAMESPACE_ID::Message() {
  SendFileEnd* const _this = this; (void)_this;
  new (&_impl_) Impl_{
      decltype(_impl_.name_){}
    , decltype(_impl_.md5_){}
    , /*decltype(_impl_._cached_size_)*/{}};

  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  _impl_.name_.InitDefault();
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    _impl_.name_.Set("", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (!from._internal_name().empty()) {
    _this->_impl_.name_.Set(from._internal_name(), 
      _this->GetArenaForAllocation());
  }
  _impl_.md5_.InitDefault();
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    _impl_.md5_.Set("", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (!from._internal_md5().empty()) {
    _this->_impl_.md5_.Set(from._internal_md5(), 
      _this->GetArenaForAllocation());
  }
  // @@protoc_insertion_point(copy_constructor:mqas.tools.proto.SendFileEnd)
}

inline void SendFileEnd::SharedCtor(
    ::_pb::Arena* arena, bool is_message_owned) {
  (void)arena;
  (void)is_message_owned;
  new (&_impl_) Impl_{
      decltype(_impl_.name_){}
    , decltype(_impl_.md5_){}
    , /*decltype(_impl_._cached_size_)*/{}
  };
  _impl_.name_.InitDefault();
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    _impl_.name_.Set("", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  _impl_.md5_.InitDefault();
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    _impl_.md5_.Set("", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
}

SendFileEnd::~SendFileEnd() {
  // @@protoc_insertion_point(destructor:mqas.tools.proto.SendFileEnd)
  if (auto *arena = _internal_metadata_.DeleteReturnArena<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>()) {
  (void)arena;
    return;
  }
  SharedDtor();
}

inline void SendFileEnd::SharedDtor() {
  GOOGLE_DCHECK(GetArenaForAllocation() == nullptr);
  _impl_.name_.Destroy();
  _impl_.md5_.Destroy();
}

void SendFileEnd::SetCachedSize(int size) const {
  _impl_._cached_size_.Set(size);
}

void SendFileEnd::Clear() {
// @@protoc_insertion_point(message_clear_start:mqas.tools.proto.SendFileEnd)
  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  _impl_.name_.ClearToEmpty();
  _impl_.md5_.ClearToEmpty();
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* SendFileEnd::_InternalParse(const char* ptr, ::_pbi::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    uint32_t tag;
    ptr = ::_pbi::ReadTag(ptr, &tag);
    switch (tag >> 3) {
      // string name = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 10)) {
          auto str = _internal_mutable_name();
          ptr = ::_pbi::InlineGreedyStringParser(str, ptr, ctx);
          CHK_(ptr);
          CHK_(::_pbi::VerifyUTF8(str, "mqas.tools.proto.SendFileEnd.name"));
        } else
          goto handle_unusual;
        continue;
      // bytes md5 = 2;
      case 2:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 18)) {
          auto str = _internal_mutable_md5();
          ptr = ::_pbi::InlineGreedyStringParser(str, ptr, ctx);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      default:
        goto handle_unusual;
    }  // switch
  handle_unusual:
    if ((tag == 0) || ((tag & 7) == 4)) {
      CHK_(ptr);
      ctx->SetLastTag(tag);
      goto message_done;
    }
    ptr = UnknownFieldParse(
        tag,
        _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(),
        ptr, ctx);
    CHK_(ptr != nullptr);
  }  // while
message_done:
  return ptr;
failure:
  ptr = nullptr;
  goto message_done;
#undef CHK_
}

uint8_t* SendFileEnd::_InternalSerialize(
    uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:mqas.tools.proto.SendFileEnd)
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  // string name = 1;
  if (!this->_internal_name().empty()) {
    ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::VerifyUtf8String(
      this->_internal_name().data(), static_cast<int>(this->_internal_name().length()),
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::SERIALIZE,
      "mqas.tools.proto.SendFileEnd.name");
    target = stream->WriteStringMaybeAliased(
        1, this->_internal_name(), target);
  }

  // bytes md5 = 2;
  if (!this->_internal_md5().empty()) {
    target = stream->WriteBytesMaybeAliased(
        2, this->_internal_md5(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:mqas.tools.proto.SendFileEnd)
  return target;
}

size_t SendFileEnd::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:mqas.tools.proto.SendFileEnd)
  size_t total_size = 0;

  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // string name = 1;
  if (!this->_internal_name().empty()) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::StringSize(
        this->_internal_name());
  }

  // bytes md5 = 2;
  if (!this->_internal_md5().empty()) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::BytesSize(
        this->_internal_md5());
  }

  return MaybeComputeUnknownFieldsSize(total_size, &_impl_._cached_size_);
}

const ::PROTOBUF_NAMESPACE_ID::Message::ClassData SendFileEnd::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::Message::CopyWithSourceCheck,
    SendFileEnd::MergeImpl
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*SendFileEnd::GetClassData() const { return &_class_data_; }


void SendFileEnd::MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg) {
  auto* const _this = static_cast<SendFileEnd*>(&to_msg);
  auto& from = static_cast<const SendFileEnd&>(from_msg);
  // @@protoc_insertion_point(class_specific_merge_from_start:mqas.tools.proto.SendFileEnd)
  GOOGLE_DCHECK_NE(&from, _this);
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  if (!from._internal_name().empty()) {
    _this->_internal_set_name(from._internal_name());
  }
  if (!from._internal_md5().empty()) {
    _this->_internal_set_md5(from._internal_md5());
  }
  _this->_internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
}

void SendFileEnd::CopyFrom(const SendFileEnd& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:mqas.tools.proto.SendFileEnd)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool SendFileEnd::IsInitialized() const {
  return true;
}

void SendFileEnd::InternalSwap(SendFileEnd* other) {
  using std::swap;
  auto* lhs_arena = GetArenaForAllocation();
  auto* rhs_arena = other->GetArenaForAllocation();
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::InternalSwap(
      &_impl_.name_, lhs_arena,
      &other->_impl_.name_, rhs_arena
  );
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::InternalSwap(
      &_impl_.md5_, lhs_arena,
      &other->_impl_.md5_, rhs_arena
  );
}

::PROTOBUF_NAMESPACE_ID::Metadata SendFileEnd::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_send_5ffile_2eproto_getter, &descriptor_table_send_5ffile_2eproto_once,
      file_level_metadata_send_5ffile_2eproto[2]);
}

// @@protoc_insertion_point(namespace_scope)
}  // namespace proto
}  // namespace tools
}  // namespace mqas
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE ::mqas::tools::proto::ReqSendFile*
Arena::CreateMaybeMessage< ::mqas::tools::proto::ReqSendFile >(Arena* arena) {
  return Arena::CreateMessageInternal< ::mqas::tools::proto::ReqSendFile >(arena);
}
template<> PROTOBUF_NOINLINE ::mqas::tools::proto::ReqSendFileRet*
Arena::CreateMaybeMessage< ::mqas::tools::proto::ReqSendFileRet >(Arena* arena) {
  return Arena::CreateMessageInternal< ::mqas::tools::proto::ReqSendFileRet >(arena);
}
template<> PROTOBUF_NOINLINE ::mqas::tools::proto::SendFileEnd*
Arena::CreateMaybeMessage< ::mqas::tools::proto::SendFileEnd >(Arena* arena) {
  return Arena::CreateMessageInternal< ::mqas::tools::proto::SendFileEnd >(arena);
}
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>
