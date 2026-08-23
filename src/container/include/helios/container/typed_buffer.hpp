#pragma once

#include <helios/assert.hpp>
#include <helios/container/details/typed_buffer_common.hpp>
#include <helios/utils/type_info.hpp>

#include <concepts>
#include <cstddef>
#include <cstring>
#include <memory>
#include <memory_resource>
#include <new>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

namespace helios::container {

/**
 * @brief Type-erased single-instance byte storage.
 * @details Stores exactly one instance of any `TypedBufferStorable` type in a
 * `std::pmr::vector<std::byte>` backing buffer. The stored type is not fixed at
 * class instantiation; it is set at runtime and verified on every access.
 * Object lifetime is properly managed. Trivially-copyable types are
 * fast-pathed.
 */
class TypedBuffer {
public:
  using size_type = size_t;
  using TypeIndex = utils::TypeIndex;

  using StorageType = std::pmr::vector<std::byte>;

  /// @brief Default constructor. Creates an empty buffer with no associated
  /// type.
  constexpr TypedBuffer() noexcept(
      std::is_nothrow_default_constructible_v<StorageType>) = default;

  /**
   * @brief Constructs an empty buffer with a PMR memory resource.
   * @param resource Memory resource used for internal storage
   */
  explicit constexpr TypedBuffer(std::pmr::memory_resource* resource) noexcept
      : storage_(resource) {}

  TypedBuffer(std::nullptr_t) = delete;

  /**
   * @brief Constructs a buffer holding a value of type `T`.
   * @tparam T The type to store
   * @tparam Args Constructor argument types
   * @param tag In-place type construction tag (use `std::in_place_type<T>`)
   * @param args Arguments forwarded to `T`'s constructor
   *
   * @code
   * TypedBuffer buf(std::in_place_type<int>, 42);
   * @endcode
   */
  template <TypedBufferStorable T, typename... Args>
    requires std::constructible_from<T, Args...>
  explicit constexpr TypedBuffer(std::in_place_type_t<T> tag, Args&&... args);

  /**
   * @brief Constructs a buffer holding a value of type `T` with a custom
   * allocator.
   * @tparam T The type to store
   * @tparam Args Constructor argument types
   * @param tag In-place type construction tag (use `std::in_place_type<T>`)
   * @param alloc Allocator instance to use
   * @param args Arguments forwarded to `T`'s constructor
   */
  template <TypedBufferStorable T, typename... Args>
    requires std::constructible_from<T, Args...>
  constexpr TypedBuffer(std::in_place_type_t<T> tag,
                        std::pmr::memory_resource* resource, Args&&... args);
  constexpr TypedBuffer(const TypedBuffer& other);

  /**
   * @brief Copy constructor with a custom memory resource.
   * @param other Buffer to copy from
   * @param resource Memory resource used for internal storage
   */
  constexpr TypedBuffer(const TypedBuffer& other,
                        std::pmr::memory_resource* resource);
  constexpr TypedBuffer(TypedBuffer&& other) noexcept(
      std::is_nothrow_move_constructible_v<StorageType>);

  /**
   * @brief Move constructor with a custom memory resource.
   * @param other Buffer to move from
   * @param resource Memory resource used for internal storage
   */
  constexpr TypedBuffer(TypedBuffer&& other,
                        std::pmr::memory_resource* resource);
  constexpr ~TypedBuffer() noexcept { Destroy(); }

  constexpr TypedBuffer& operator=(const TypedBuffer& other);
  constexpr TypedBuffer& operator=(TypedBuffer&& other) noexcept;

  /**
   * @brief Changes the stored type, destroying the current value if any.
   * @details After this call `Empty()` is true and `IsType<T>()` is true.
   * The underlying memory is retained for reuse.
   * @tparam T The new type
   */
  template <TypedBufferStorable T>
  constexpr void ChangeType() noexcept;

  /**
   * @brief Destroys the stored value and resets type information.
   * @details After this call both `Empty()` and `!HasType()` are true.
   */
  constexpr void Reset() noexcept;

  /**
   * @brief Reserves at least `bytes` of backing storage.
   * @details Empty or trivially copyable values use `vector::reserve`. A stored
   * non-trivial value is relocated if reallocation is required.
   * @param bytes Minimum capacity in bytes
   */
  constexpr void ReserveBytes(size_type bytes);

  /**
   * @brief Constructs (or replaces) the stored value in-place.
   * @details If a value of a different type is already stored it is destroyed
   * first. The type is updated to `T` before construction.
   * @tparam T The type to store
   * @tparam Args Constructor argument types
   * @param args Arguments forwarded to `T`'s constructor
   * @return Reference to the newly constructed value
   */
  template <TypedBufferStorable T, typename... Args>
    requires std::constructible_from<T, Args...>
  constexpr T& Set(Args&&... args);

  /**
   * @brief Swaps contents with another TypedBuffer.
   * @param other Buffer to swap with
   */
  constexpr void Swap(TypedBuffer& other) noexcept;
  friend constexpr void swap(TypedBuffer& lhs, TypedBuffer& rhs) noexcept {
    lhs.Swap(rhs);
  }

  /**
   * @brief Returns true if no value is currently stored.
   * @return True if empty, false if a value is stored
   */
  [[nodiscard]] constexpr bool Empty() const noexcept { return !has_value_; }

  /**
   * @brief Returns true if a type has been associated with this buffer.
   * @return True if a type is set, false if no type information is present
   */
  [[nodiscard]] constexpr bool HasType() const noexcept {
    return type_info_.IsValid();
  }

  /**
   * @brief Checks if the stored type matches `T`.
   * @tparam T The type to check against
   * @return true if types match or no type is set
   */
  template <TypedBufferStorable T>
  [[nodiscard]] constexpr bool IsType() const noexcept {
    return !type_info_.IsValid() || type_info_.type_index == TypeIndexOf<T>();
  }

  /**
   * @brief Gets the compile-time type index for `T`.
   * @tparam T The type to get index for
   * @return Type index
   */
  template <TypedBufferStorable T>
  [[nodiscard]] static consteval TypeIndex TypeIndexOf() noexcept {
    return utils::TypeIndex::From<T>();
  }

  /**
   * @brief Gets the current stored type index.
   * @return Type index of the stored type, or an invalid index if no type is
   * set
   */
  [[nodiscard]] constexpr TypeIndex StoredTypeId() const noexcept {
    return type_info_.type_index;
  }

  /**
   * @brief Returns the size in bytes of the stored element type, or 0 if no
   * type is set.
   * @return Size of the stored type in bytes
   */
  [[nodiscard]] constexpr size_type ElementSize() const noexcept {
    return type_info_.element_size;
  }

  /**
   * @brief Returns the current capacity of the backing buffer in bytes.
   * @return Capacity in bytes
   */
  [[nodiscard]] constexpr size_type CapacityBytes() const noexcept {
    return storage_.capacity();
  }

  /**
   * @brief Returns the memory resource used for internal storage.
   * @return Memory resource passed to the constructor, or the default resource
   */
  [[nodiscard]] constexpr std::pmr::memory_resource* GetMemoryResource()
      const noexcept {
    return storage_.get_allocator().resource();
  }

  /**
   * @brief Accesses the stored value.
   * @warning Triggers assertion if the buffer is empty or the stored type
   * doesn't match `T`.
   * @tparam T The expected type
   * @return Reference to the stored value
   */
  template <TypedBufferStorable T>
  [[nodiscard]] constexpr T& Value() noexcept;

  /**
   * @brief Accesses the stored value (const).
   * @warning Triggers assertion if the buffer is empty or the stored type
   * doesn't match `T`.
   * @tparam T The expected type
   * @return Const reference to the stored value
   */
  template <TypedBufferStorable T>
  [[nodiscard]] constexpr const T& Value() const noexcept;

  /**
   * @brief Returns a const byte span of the stored value (empty if no value is
   * stored).
   * @return Byte span of the stored value
   */
  [[nodiscard]] constexpr auto Bytes() const noexcept
      -> std::span<const std::byte>;

private:
  using TypeInfo = details::TypeBufferInfo;

  template <TypedBufferStorable T>
  [[nodiscard]] constexpr T* DataPtr() noexcept;

  template <TypedBufferStorable T>
  [[nodiscard]] constexpr const T* DataPtr() const noexcept;

  [[nodiscard]] constexpr void* RawDataPtr() noexcept {
    return storage_.empty() ? nullptr : static_cast<void*>(storage_.data());
  }

  [[nodiscard]] constexpr const void* RawDataPtr() const noexcept {
    return storage_.empty() ? nullptr
                            : static_cast<const void*>(storage_.data());
  }

  /// @brief Destroys the stored value if present; sets has_value_ to false.
  constexpr void Destroy() noexcept;

  TypeInfo type_info_;
  StorageType storage_;
  bool has_value_ = false;
};

template <TypedBufferStorable T, typename... Args>
  requires std::constructible_from<T, Args...>
constexpr TypedBuffer::TypedBuffer(std::in_place_type_t<T> /*tag*/,
                                   Args&&... args)
    : type_info_(TypeInfo::template From<T>()), has_value_(true) {
  storage_.resize(sizeof(T));
  std::construct_at(DataPtr<T>(), std::forward<Args>(args)...);
}

template <TypedBufferStorable T, typename... Args>
  requires std::constructible_from<T, Args...>
constexpr TypedBuffer::TypedBuffer(std::in_place_type_t<T> /*tag*/,
                                   std::pmr::memory_resource* resource,
                                   Args&&... args)
    : type_info_(TypeInfo::template From<T>()),
      storage_(resource),
      has_value_(true) {
  storage_.resize(sizeof(T));
  std::construct_at(DataPtr<T>(), std::forward<Args>(args)...);
}

constexpr TypedBuffer::TypedBuffer(const TypedBuffer& other)
    : storage_(std::pmr::get_default_resource()) {
  if (!other.has_value_) {
    type_info_ = other.type_info_;
    return;
  }

  type_info_ = other.type_info_;
  HELIOS_ASSERT(
      type_info_.is_copy_constructible,
      "Cannot copy TypedBuffer: stored type is not copy constructible!");

  storage_.resize(type_info_.element_size);

  if (type_info_.is_trivially_copyable) {
    std::memcpy(storage_.data(), other.storage_.data(),
                type_info_.element_size);
  } else {
    type_info_.copy_construct(storage_.data(), other.storage_.data(), 1);
  }

  has_value_ = true;
}

constexpr TypedBuffer::TypedBuffer(const TypedBuffer& other,
                                   std::pmr::memory_resource* resource)
    : storage_(resource) {
  if (!other.has_value_) {
    type_info_ = other.type_info_;
    return;
  }

  type_info_ = other.type_info_;
  HELIOS_ASSERT(
      type_info_.is_copy_constructible,
      "Cannot copy TypedBuffer: stored type is not copy constructible!");

  storage_.resize(type_info_.element_size);

  if (type_info_.is_trivially_copyable) {
    std::memcpy(storage_.data(), other.storage_.data(),
                type_info_.element_size);
  } else {
    type_info_.copy_construct(storage_.data(), other.storage_.data(), 1);
  }

  has_value_ = true;
}

constexpr TypedBuffer::TypedBuffer(TypedBuffer&& other) noexcept(
    std::is_nothrow_move_constructible_v<StorageType>)
    : type_info_(other.type_info_),
      storage_(std::move(other.storage_)),
      has_value_(other.has_value_) {
  other.has_value_ = false;
  other.type_info_.Reset();
}

constexpr TypedBuffer::TypedBuffer(TypedBuffer&& other,
                                   std::pmr::memory_resource* resource)
    : type_info_(other.type_info_), storage_(resource), has_value_(false) {
  if (GetMemoryResource() == other.GetMemoryResource()) {
    storage_ = std::move(other.storage_);
    has_value_ = other.has_value_;
  } else if (other.has_value_) {
    HELIOS_ASSERT(
        type_info_.is_trivially_copyable ||
            type_info_.move_construct != nullptr ||
            type_info_.copy_construct != nullptr,
        "Cannot relocate TypedBuffer across memory resources: stored type is "
        "not relocatable!");
    storage_.resize(type_info_.element_size);
    if (type_info_.is_trivially_copyable) {
      std::memcpy(storage_.data(), other.storage_.data(),
                  type_info_.element_size);
    } else if (type_info_.move_construct != nullptr) {
      type_info_.move_construct(storage_.data(), other.storage_.data(), 1);
    } else {
      type_info_.copy_construct(storage_.data(), other.storage_.data(), 1);
    }
    has_value_ = true;
    other.Destroy();
  }

  other.has_value_ = false;
  other.type_info_.Reset();
}

constexpr auto TypedBuffer::operator=(const TypedBuffer& other)
    -> TypedBuffer& {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  Destroy();

  if (!other.has_value_) {
    type_info_ = other.type_info_;
    return *this;
  }

  type_info_ = other.type_info_;
  HELIOS_ASSERT(
      type_info_.is_copy_constructible,
      "Cannot copy-assign TypedBuffer: stored type is not copy constructible!");

  if (storage_.size() < type_info_.element_size) {
    storage_.resize(type_info_.element_size);
  }

  if (type_info_.is_trivially_copyable) {
    std::memcpy(storage_.data(), other.storage_.data(),
                type_info_.element_size);
  } else {
    type_info_.copy_construct(storage_.data(), other.storage_.data(), 1);
  }

  has_value_ = true;

  return *this;
}

constexpr auto TypedBuffer::operator=(TypedBuffer&& other) noexcept
    -> TypedBuffer& {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  TypedBuffer relocated(std::move(other), GetMemoryResource());
  Swap(relocated);

  return *this;
}

template <TypedBufferStorable T>
constexpr void TypedBuffer::ChangeType() noexcept {
  Destroy();
  type_info_ = TypeInfo::template From<T>();
}

constexpr void TypedBuffer::Reset() noexcept {
  Destroy();
  storage_.clear();
  type_info_.Reset();
}

constexpr void TypedBuffer::ReserveBytes(size_type bytes) {
  if (bytes <= storage_.capacity()) {
    return;
  }

  if (!has_value_ || type_info_.is_trivially_copyable) {
    storage_.reserve(bytes);
    return;
  }

  StorageType new_storage(storage_.get_allocator());
  new_storage.reserve(bytes);
  new_storage.resize(storage_.size());

  if (type_info_.move_construct != nullptr) {
    type_info_.move_construct(new_storage.data(), storage_.data(), 1);
    if (type_info_.destroy != nullptr) {
      type_info_.destroy(storage_.data(), 1);
    }
  } else if (type_info_.copy_construct != nullptr) {
    type_info_.copy_construct(new_storage.data(), storage_.data(), 1);
    if (type_info_.destroy != nullptr) {
      type_info_.destroy(storage_.data(), 1);
    }
  } else {
    HELIOS_ASSERT(
        false, "Cannot reserve TypedBuffer: stored type is not relocatable!");
  }

  storage_ = std::move(new_storage);
}

template <TypedBufferStorable T, typename... Args>
  requires std::constructible_from<T, Args...>
constexpr T& TypedBuffer::Set(Args&&... args) {
  Destroy();

  type_info_ = TypeInfo::template From<T>();
  if (storage_.size() < sizeof(T)) {
    storage_.resize(sizeof(T));
  }

  T* ptr = DataPtr<T>();
  std::construct_at(ptr, std::forward<Args>(args)...);
  has_value_ = true;
  return *ptr;
}

constexpr void TypedBuffer::Swap(TypedBuffer& other) noexcept {
  if (this == &other) [[unlikely]] {
    return;
  }

  if (GetMemoryResource() != other.GetMemoryResource()) {
    TypedBuffer lhs(std::move(*this), other.GetMemoryResource());
    TypedBuffer rhs(std::move(other), GetMemoryResource());
    Swap(rhs);
    other.Swap(lhs);
    return;
  }

  std::swap(type_info_, other.type_info_);
  std::swap(storage_, other.storage_);
  std::swap(has_value_, other.has_value_);
}

template <TypedBufferStorable T>
constexpr T& TypedBuffer::Value() noexcept {
  HELIOS_ASSERT(has_value_, "TypedBuffer::Value: buffer is empty!");
  HELIOS_ASSERT(type_info_.type_index == TypeIndexOf<T>(),
                "TypedBuffer::Value: type mismatch!");
  return *DataPtr<T>();
}

template <TypedBufferStorable T>
constexpr const T& TypedBuffer::Value() const noexcept {
  HELIOS_ASSERT(has_value_, "TypedBuffer::Value: buffer is empty!");
  HELIOS_ASSERT(type_info_.type_index == TypeIndexOf<T>(),
                "TypedBuffer::Value: type mismatch!");
  return *DataPtr<T>();
}

constexpr auto TypedBuffer::Bytes() const noexcept
    -> std::span<const std::byte> {
  if (!has_value_) {
    return {};
  }
  return {storage_.data(), type_info_.element_size};
}

template <TypedBufferStorable T>
constexpr T* TypedBuffer::DataPtr() noexcept {
  HELIOS_ASSERT(
      !type_info_.IsValid() || type_info_.type_index == TypeIndexOf<T>(),
      "TypedBuffer::DataPtr: type mismatch!");
  if (storage_.empty()) [[unlikely]] {
    return nullptr;
  }
  return std::launder(static_cast<T*>(RawDataPtr()));
}

template <TypedBufferStorable T>
constexpr const T* TypedBuffer::DataPtr() const noexcept {
  HELIOS_ASSERT(
      !type_info_.IsValid() || type_info_.type_index == TypeIndexOf<T>(),
      "TypedBuffer::DataPtr: type mismatch!");
  if (storage_.empty()) [[unlikely]] {
    return nullptr;
  }
  return std::launder(static_cast<const T*>(RawDataPtr()));
}

constexpr void TypedBuffer::Destroy() noexcept {
  if (has_value_) {
    if (type_info_.destroy != nullptr) {
      type_info_.destroy(storage_.data(), 1);
    }
    has_value_ = false;
  }
}

}  // namespace helios::container
