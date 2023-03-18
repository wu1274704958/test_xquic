// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: msg_wrapper.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_msg_5fwrapper_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_msg_5fwrapper_2eproto

#include <limits>
#include "mqas/macro.h"
#include <string>

#include <google/protobuf/port_def.inc>
#if PROTOBUF_VERSION < 3021000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers. Please update
#error your headers.
#endif
#if 3021008 < PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers. Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/port_undef.inc>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/metadata_lite.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_msg_5fwrapper_2eproto MQAS_EXTERN
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct MQAS_EXTERN TableStruct_msg_5fwrapper_2eproto {
  static const uint32_t offsets[];
};
MQAS_EXTERN extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_msg_5fwrapper_2eproto;
namespace mqas {
namespace core {
namespace proto {
class MsgWrapper;
struct MsgWrapperDefaultTypeInternal;
MQAS_EXTERN extern MsgWrapperDefaultTypeInternal _MsgWrapper_default_instance_;
}  // namespace proto
}  // namespace core
}  // namespace mqas
PROTOBUF_NAMESPACE_OPEN
template<> MQAS_EXTERN ::mqas::core::proto::MsgWrapper* Arena::CreateMaybeMessage<::mqas::core::proto::MsgWrapper>(Arena*);
PROTOBUF_NAMESPACE_CLOSE
namespace mqas {
namespace core {
namespace proto {

// ===================================================================

class MQAS_EXTERN MsgWrapper final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:mqas.core.proto.MsgWrapper) */ {
 public:
  inline MsgWrapper() : MsgWrapper(nullptr) {}
  ~MsgWrapper() override;
  explicit PROTOBUF_CONSTEXPR MsgWrapper(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  MsgWrapper(const MsgWrapper& from);
  MsgWrapper(MsgWrapper&& from) noexcept
    : MsgWrapper() {
    *this = ::std::move(from);
  }

  inline MsgWrapper& operator=(const MsgWrapper& from) {
    CopyFrom(from);
    return *this;
  }
  inline MsgWrapper& operator=(MsgWrapper&& from) noexcept {
    if (this == &from) return *this;
    if (GetOwningArena() == from.GetOwningArena()
  #ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        && GetOwningArena() != nullptr
  #endif  // !PROTOBUF_FORCE_COPY_IN_MOVE
    ) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const MsgWrapper& default_instance() {
    return *internal_default_instance();
  }
  static inline const MsgWrapper* internal_default_instance() {
    return reinterpret_cast<const MsgWrapper*>(
               &_MsgWrapper_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(MsgWrapper& a, MsgWrapper& b) {
    a.Swap(&b);
  }
  inline void Swap(MsgWrapper* other) {
    if (other == this) return;
  #ifdef PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() != nullptr &&
        GetOwningArena() == other->GetOwningArena()) {
   #else  // PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() == other->GetOwningArena()) {
  #endif  // !PROTOBUF_FORCE_COPY_IN_SWAP
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(MsgWrapper* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  MsgWrapper* New(::PROTOBUF_NAMESPACE_ID::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<MsgWrapper>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const MsgWrapper& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom( const MsgWrapper& from) {
    MsgWrapper::MergeImpl(*this, from);
  }
  private:
  static void MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  uint8_t* _InternalSerialize(
      uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _impl_._cached_size_.Get(); }

  private:
  void SharedCtor(::PROTOBUF_NAMESPACE_ID::Arena* arena, bool is_message_owned);
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(MsgWrapper* other);

  private:
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "mqas.core.proto.MsgWrapper";
  }
  protected:
  explicit MsgWrapper(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kMsgBodyFieldNumber = 3,
    kMsgIdFieldNumber = 1,
    kBodyLenFieldNumber = 2,
  };
  // bytes msg_body = 3;
  void clear_msg_body();
  const std::string& msg_body() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_msg_body(ArgT0&& arg0, ArgT... args);
  std::string* mutable_msg_body();
  PROTOBUF_NODISCARD std::string* release_msg_body();
  void set_allocated_msg_body(std::string* msg_body);
  private:
  const std::string& _internal_msg_body() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_msg_body(const std::string& value);
  std::string* _internal_mutable_msg_body();
  public:

  // uint32 msg_id = 1;
  void clear_msg_id();
  uint32_t msg_id() const;
  void set_msg_id(uint32_t value);
  private:
  uint32_t _internal_msg_id() const;
  void _internal_set_msg_id(uint32_t value);
  public:

  // uint32 body_len = 2;
  void clear_body_len();
  uint32_t body_len() const;
  void set_body_len(uint32_t value);
  private:
  uint32_t _internal_body_len() const;
  void _internal_set_body_len(uint32_t value);
  public:

  // @@protoc_insertion_point(class_scope:mqas.core.proto.MsgWrapper)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  struct Impl_ {
    ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr msg_body_;
    uint32_t msg_id_;
    uint32_t body_len_;
    mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_msg_5fwrapper_2eproto;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// MsgWrapper

// uint32 msg_id = 1;
inline void MsgWrapper::clear_msg_id() {
  _impl_.msg_id_ = 0u;
}
inline uint32_t MsgWrapper::_internal_msg_id() const {
  return _impl_.msg_id_;
}
inline uint32_t MsgWrapper::msg_id() const {
  // @@protoc_insertion_point(field_get:mqas.core.proto.MsgWrapper.msg_id)
  return _internal_msg_id();
}
inline void MsgWrapper::_internal_set_msg_id(uint32_t value) {
  
  _impl_.msg_id_ = value;
}
inline void MsgWrapper::set_msg_id(uint32_t value) {
  _internal_set_msg_id(value);
  // @@protoc_insertion_point(field_set:mqas.core.proto.MsgWrapper.msg_id)
}

// uint32 body_len = 2;
inline void MsgWrapper::clear_body_len() {
  _impl_.body_len_ = 0u;
}
inline uint32_t MsgWrapper::_internal_body_len() const {
  return _impl_.body_len_;
}
inline uint32_t MsgWrapper::body_len() const {
  // @@protoc_insertion_point(field_get:mqas.core.proto.MsgWrapper.body_len)
  return _internal_body_len();
}
inline void MsgWrapper::_internal_set_body_len(uint32_t value) {
  
  _impl_.body_len_ = value;
}
inline void MsgWrapper::set_body_len(uint32_t value) {
  _internal_set_body_len(value);
  // @@protoc_insertion_point(field_set:mqas.core.proto.MsgWrapper.body_len)
}

// bytes msg_body = 3;
inline void MsgWrapper::clear_msg_body() {
  _impl_.msg_body_.ClearToEmpty();
}
inline const std::string& MsgWrapper::msg_body() const {
  // @@protoc_insertion_point(field_get:mqas.core.proto.MsgWrapper.msg_body)
  return _internal_msg_body();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE
void MsgWrapper::set_msg_body(ArgT0&& arg0, ArgT... args) {
 
 _impl_.msg_body_.SetBytes(static_cast<ArgT0 &&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:mqas.core.proto.MsgWrapper.msg_body)
}
inline std::string* MsgWrapper::mutable_msg_body() {
  std::string* _s = _internal_mutable_msg_body();
  // @@protoc_insertion_point(field_mutable:mqas.core.proto.MsgWrapper.msg_body)
  return _s;
}
inline const std::string& MsgWrapper::_internal_msg_body() const {
  return _impl_.msg_body_.Get();
}
inline void MsgWrapper::_internal_set_msg_body(const std::string& value) {
  
  _impl_.msg_body_.Set(value, GetArenaForAllocation());
}
inline std::string* MsgWrapper::_internal_mutable_msg_body() {
  
  return _impl_.msg_body_.Mutable(GetArenaForAllocation());
}
inline std::string* MsgWrapper::release_msg_body() {
  // @@protoc_insertion_point(field_release:mqas.core.proto.MsgWrapper.msg_body)
  return _impl_.msg_body_.Release();
}
inline void MsgWrapper::set_allocated_msg_body(std::string* msg_body) {
  if (msg_body != nullptr) {
    
  } else {
    
  }
  _impl_.msg_body_.SetAllocated(msg_body, GetArenaForAllocation());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.msg_body_.IsDefault()) {
    _impl_.msg_body_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:mqas.core.proto.MsgWrapper.msg_body)
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__

// @@protoc_insertion_point(namespace_scope)

}  // namespace proto
}  // namespace core
}  // namespace mqas

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_msg_5fwrapper_2eproto
