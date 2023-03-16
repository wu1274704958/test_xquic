// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: say_hello.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_say_5fhello_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_say_5fhello_2eproto

#include <limits>
#include <string>

#include <google/protobuf/port_def.inc>
#if PROTOBUF_VERSION < 3021000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers. Please update
#error your headers.
#endif
#if 3021012 < PROTOBUF_MIN_PROTOC_VERSION
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
#define PROTOBUF_INTERNAL_EXPORT_say_5fhello_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_say_5fhello_2eproto {
  static const uint32_t offsets[];
};
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_say_5fhello_2eproto;
namespace proto {
class SayByeMsg;
struct SayByeMsgDefaultTypeInternal;
extern SayByeMsgDefaultTypeInternal _SayByeMsg_default_instance_;
class SayHelloMsg;
struct SayHelloMsgDefaultTypeInternal;
extern SayHelloMsgDefaultTypeInternal _SayHelloMsg_default_instance_;
}  // namespace proto
PROTOBUF_NAMESPACE_OPEN
template<> ::proto::SayByeMsg* Arena::CreateMaybeMessage<::proto::SayByeMsg>(Arena*);
template<> ::proto::SayHelloMsg* Arena::CreateMaybeMessage<::proto::SayHelloMsg>(Arena*);
PROTOBUF_NAMESPACE_CLOSE
namespace proto {

// ===================================================================

class SayHelloMsg final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:proto.SayHelloMsg) */ {
 public:
  inline SayHelloMsg() : SayHelloMsg(nullptr) {}
  ~SayHelloMsg() override;
  explicit PROTOBUF_CONSTEXPR SayHelloMsg(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  SayHelloMsg(const SayHelloMsg& from);
  SayHelloMsg(SayHelloMsg&& from) noexcept
    : SayHelloMsg() {
    *this = ::std::move(from);
  }

  inline SayHelloMsg& operator=(const SayHelloMsg& from) {
    CopyFrom(from);
    return *this;
  }
  inline SayHelloMsg& operator=(SayHelloMsg&& from) noexcept {
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
  static const SayHelloMsg& default_instance() {
    return *internal_default_instance();
  }
  static inline const SayHelloMsg* internal_default_instance() {
    return reinterpret_cast<const SayHelloMsg*>(
               &_SayHelloMsg_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(SayHelloMsg& a, SayHelloMsg& b) {
    a.Swap(&b);
  }
  inline void Swap(SayHelloMsg* other) {
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
  void UnsafeArenaSwap(SayHelloMsg* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  SayHelloMsg* New(::PROTOBUF_NAMESPACE_ID::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<SayHelloMsg>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const SayHelloMsg& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom( const SayHelloMsg& from) {
    SayHelloMsg::MergeImpl(*this, from);
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
  void InternalSwap(SayHelloMsg* other);

  private:
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "proto.SayHelloMsg";
  }
  protected:
  explicit SayHelloMsg(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kMsgFieldNumber = 1,
    kNumFieldNumber = 2,
  };
  // string msg = 1;
  void clear_msg();
  const std::string& msg() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_msg(ArgT0&& arg0, ArgT... args);
  std::string* mutable_msg();
  PROTOBUF_NODISCARD std::string* release_msg();
  void set_allocated_msg(std::string* msg);
  private:
  const std::string& _internal_msg() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_msg(const std::string& value);
  std::string* _internal_mutable_msg();
  public:

  // uint32 num = 2;
  void clear_num();
  uint32_t num() const;
  void set_num(uint32_t value);
  private:
  uint32_t _internal_num() const;
  void _internal_set_num(uint32_t value);
  public:

  // @@protoc_insertion_point(class_scope:proto.SayHelloMsg)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  struct Impl_ {
    ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr msg_;
    uint32_t num_;
    mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_say_5fhello_2eproto;
};
// -------------------------------------------------------------------

class SayByeMsg final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:proto.SayByeMsg) */ {
 public:
  inline SayByeMsg() : SayByeMsg(nullptr) {}
  ~SayByeMsg() override;
  explicit PROTOBUF_CONSTEXPR SayByeMsg(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  SayByeMsg(const SayByeMsg& from);
  SayByeMsg(SayByeMsg&& from) noexcept
    : SayByeMsg() {
    *this = ::std::move(from);
  }

  inline SayByeMsg& operator=(const SayByeMsg& from) {
    CopyFrom(from);
    return *this;
  }
  inline SayByeMsg& operator=(SayByeMsg&& from) noexcept {
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
  static const SayByeMsg& default_instance() {
    return *internal_default_instance();
  }
  static inline const SayByeMsg* internal_default_instance() {
    return reinterpret_cast<const SayByeMsg*>(
               &_SayByeMsg_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    1;

  friend void swap(SayByeMsg& a, SayByeMsg& b) {
    a.Swap(&b);
  }
  inline void Swap(SayByeMsg* other) {
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
  void UnsafeArenaSwap(SayByeMsg* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  SayByeMsg* New(::PROTOBUF_NAMESPACE_ID::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<SayByeMsg>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const SayByeMsg& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom( const SayByeMsg& from) {
    SayByeMsg::MergeImpl(*this, from);
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
  void InternalSwap(SayByeMsg* other);

  private:
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "proto.SayByeMsg";
  }
  protected:
  explicit SayByeMsg(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kNumFieldNumber = 1,
    kNum2FieldNumber = 2,
  };
  // uint32 num = 1;
  void clear_num();
  uint32_t num() const;
  void set_num(uint32_t value);
  private:
  uint32_t _internal_num() const;
  void _internal_set_num(uint32_t value);
  public:

  // uint32 num2 = 2;
  void clear_num2();
  uint32_t num2() const;
  void set_num2(uint32_t value);
  private:
  uint32_t _internal_num2() const;
  void _internal_set_num2(uint32_t value);
  public:

  // @@protoc_insertion_point(class_scope:proto.SayByeMsg)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  struct Impl_ {
    uint32_t num_;
    uint32_t num2_;
    mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_say_5fhello_2eproto;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// SayHelloMsg

// string msg = 1;
inline void SayHelloMsg::clear_msg() {
  _impl_.msg_.ClearToEmpty();
}
inline const std::string& SayHelloMsg::msg() const {
  // @@protoc_insertion_point(field_get:proto.SayHelloMsg.msg)
  return _internal_msg();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE
void SayHelloMsg::set_msg(ArgT0&& arg0, ArgT... args) {
 
 _impl_.msg_.Set(static_cast<ArgT0 &&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:proto.SayHelloMsg.msg)
}
inline std::string* SayHelloMsg::mutable_msg() {
  std::string* _s = _internal_mutable_msg();
  // @@protoc_insertion_point(field_mutable:proto.SayHelloMsg.msg)
  return _s;
}
inline const std::string& SayHelloMsg::_internal_msg() const {
  return _impl_.msg_.Get();
}
inline void SayHelloMsg::_internal_set_msg(const std::string& value) {
  
  _impl_.msg_.Set(value, GetArenaForAllocation());
}
inline std::string* SayHelloMsg::_internal_mutable_msg() {
  
  return _impl_.msg_.Mutable(GetArenaForAllocation());
}
inline std::string* SayHelloMsg::release_msg() {
  // @@protoc_insertion_point(field_release:proto.SayHelloMsg.msg)
  return _impl_.msg_.Release();
}
inline void SayHelloMsg::set_allocated_msg(std::string* msg) {
  if (msg != nullptr) {
    
  } else {
    
  }
  _impl_.msg_.SetAllocated(msg, GetArenaForAllocation());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.msg_.IsDefault()) {
    _impl_.msg_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:proto.SayHelloMsg.msg)
}

// uint32 num = 2;
inline void SayHelloMsg::clear_num() {
  _impl_.num_ = 0u;
}
inline uint32_t SayHelloMsg::_internal_num() const {
  return _impl_.num_;
}
inline uint32_t SayHelloMsg::num() const {
  // @@protoc_insertion_point(field_get:proto.SayHelloMsg.num)
  return _internal_num();
}
inline void SayHelloMsg::_internal_set_num(uint32_t value) {
  
  _impl_.num_ = value;
}
inline void SayHelloMsg::set_num(uint32_t value) {
  _internal_set_num(value);
  // @@protoc_insertion_point(field_set:proto.SayHelloMsg.num)
}

// -------------------------------------------------------------------

// SayByeMsg

// uint32 num = 1;
inline void SayByeMsg::clear_num() {
  _impl_.num_ = 0u;
}
inline uint32_t SayByeMsg::_internal_num() const {
  return _impl_.num_;
}
inline uint32_t SayByeMsg::num() const {
  // @@protoc_insertion_point(field_get:proto.SayByeMsg.num)
  return _internal_num();
}
inline void SayByeMsg::_internal_set_num(uint32_t value) {
  
  _impl_.num_ = value;
}
inline void SayByeMsg::set_num(uint32_t value) {
  _internal_set_num(value);
  // @@protoc_insertion_point(field_set:proto.SayByeMsg.num)
}

// uint32 num2 = 2;
inline void SayByeMsg::clear_num2() {
  _impl_.num2_ = 0u;
}
inline uint32_t SayByeMsg::_internal_num2() const {
  return _impl_.num2_;
}
inline uint32_t SayByeMsg::num2() const {
  // @@protoc_insertion_point(field_get:proto.SayByeMsg.num2)
  return _internal_num2();
}
inline void SayByeMsg::_internal_set_num2(uint32_t value) {
  
  _impl_.num2_ = value;
}
inline void SayByeMsg::set_num2(uint32_t value) {
  _internal_set_num2(value);
  // @@protoc_insertion_point(field_set:proto.SayByeMsg.num2)
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__
// -------------------------------------------------------------------


// @@protoc_insertion_point(namespace_scope)

}  // namespace proto

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_say_5fhello_2eproto
