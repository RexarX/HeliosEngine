#pragma once

#include <helios/assert.hpp>
#include <helios/container/details/typed_buffer_common.hpp>

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
 * `std::vector<std::byte>` backing buffer. The stored type is not fixed at
 * class instantiation; it is set at runtime and verified on every access.
 * Object lifetime is properly managed. Trivially-copyable types are
 * fast-pathed.
 * @tparam Allocator The allocator type for the byte storage (default:
 * `std::allocator<std::byte>`)
 */
template <typename Allocator = std::allocator<std::byte>>
class TypedBuffer {
public:
  using allocator_type = Allocator;
  using size_type = size_t;
  using TypeIndex = utils::TypeIndex;

  using StorageType = std::vector<std::byte, allocator_type>;

  /// @brief Default constructor. Creates an empty buffer with no associated
  /// type.
  constexpr TypedBuffer() noexcept(
      std::is_nothrow_default_constructible_v<StorageType>) = default;

  /**
   * @brief Constructs an empty buffer with a custom allocator.
   * @param alloc Allocator instance to use
   */
  explicit constexpr TypedBuffer(const allocator_type& alloc) noexcept(
      std::is_nothrow_constructible_v<StorageType, const allocator_type&>)
      : storage_(alloc) {}

  /**
   * @brief Constructs an empty buffer from a PMR memory resource.
   * @details Enabled only when `allocator_type` is constructible from
   * `std::pmr::memory_resource*`.
   * @param resource Memory resource used to construct allocator
   */
  explicit constexpr TypedBuffer(std::pmr::memory_resource* resource) noexcept(
      std::is_nothrow_constructible_v<allocator_type,
                                      std::pmr::memory_resource*>)
    requires std::constructible_from<allocator_type, std::pmr::memory_resource*>
      : TypedBuffer(allocator_type{resource}) {}

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
                        const allocator_type& alloc, Args&&... args);
  constexpr TypedBuffer(const TypedBuffer& other);
  constexpr TypedBuffer(TypedBuffer&& other) noexcept(
      std::is_nothrow_move_constructible_v<StorageType>);
  constexpr ~TypedBuffer() noexcept { Destroy(); }

  constexpr TypedBuffer& operator=(const TypedBuffer& other);
  constexpr TypedBuffer& operator=(TypedBuffer&& other) noexcept(
      std::is_nothrow_move_assignable_v<StorageType>);

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
  T& Set(Args&&... args);

  /**
   * @brief Swaps contents with another TypedBuffer.
   * @param other Buffer to swap with
   */
  constexpr void Swap(TypedBuffer& other) noexcept(
      std::is_nothrow_swappable_v<StorageType>);

  friend constexpr void swap(TypedBuffer& lhs, TypedBuffer& rhs) noexcept(
      std::is_nothrow_swappable_v<StorageType>) {
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
   * @brief Accesses the stored value.
   * @warning Triggers assertion if the buffer is empty or the stored type
   * doesn't match `T`.
   * @tparam T The expected type
   * @return Reference to the stored value
   */
  template <TypedBufferStorable T>
  [[nodiscard]] T& Value() noexcept;

  /**
   * @brief Accesses the stored value (const).
   * @warning Triggers assertion if the buffer is empty or the stored type
   * doesn't match `T`.
   * @tparam T The expected type
   * @return Const reference to the stored value
   */
  template <TypedBufferStorable T>
  [[nodiscard]] const T& Value() const noexcept;

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
  [[nodiscard]] T* DataPtr() noexcept;

  template <TypedBufferStorable T>
  [[nodiscard]] const T* DataPtr() const noexcept;

  [[nodiscard]] constexpr void* RawDataPtr() noexcept {
    return storage_.empty() ? nullptr : static_cast<void*>(storage_.data());
  }

  [[nodiscard]] constexpr const void* RawDataPtr() const noexcept {
    return storage_.empty() ? nullptr
                            : static_cast<const void*>(storage_.data());
  }

  /// @brief Destroys the stored value if present; sets has_value_ to false.
  constexpr void Destroy() noexcept;

  TypeInfo type_info_{};
  StorageType storage_;
  bool has_value_ = false;
};

template <typename Allocator>
template <TypedBufferStorable T, typename... Args>
  requires std::constructible_from<T, Args...>
constexpr TypedBuffer<Allocator>::TypedBuffer(std::in_place_type_t<T> /*tag*/,
                                              Args&&... args)
    : type_info_(TypeInfo::template From<T>()), has_value_(true) {
  storage_.resize(sizeof(T));
  std::construct_at(std::launder(reinterpret_cast<T*>(storage_.data())),
                    std::forward<Args>(args)...);
}

template <typename Allocator>
template <TypedBufferStorable T, typename... Args>
  requires std::constructible_from<T, Args...>
constexpr TypedBuffer<Allocator>::TypedBuffer(std::in_place_type_t<T> /*tag*/,
                                              const allocator_type& alloc,
                                              Args&&... args)
    : type_info_(TypeInfo::template From<T>()),
      storage_(alloc),
      has_value_(true) {
  storage_.resize(sizeof(T));
  std::construct_at(std::launder(reinterpret_cast<T*>(storage_.data())),
                    std::forward<Args>(args)...);
}

template <typename Allocator>
constexpr TypedBuffer<Allocator>::TypedBuffer(const TypedBuffer& other)
    : storage_(other.storage_.get_allocator()) {
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

template <typename Allocator>
constexpr TypedBuffer<Allocator>::TypedBuffer(TypedBuffer&& other) noexcept(
    std::is_nothrow_move_constructible_v<StorageType>)
    : type_info_(other.type_info_),
      storage_(std::move(other.storage_)),
      has_value_(other.has_value_) {
  other.has_value_ = false;
  other.type_info_.Reset();
}

template <typename Allocator>
constexpr auto TypedBuffer<Allocator>::operator=(const TypedBuffer& other)
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

template <typename Allocator>
constexpr auto TypedBuffer<Allocator>::operator=(TypedBuffer&& other) noexcept(
    std::is_nothrow_move_assignable_v<StorageType>) -> TypedBuffer& {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  Destroy();
  type_info_ = other.type_info_;
  storage_ = std::move(other.storage_);
  has_value_ = other.has_value_;
  other.has_value_ = false;
  other.type_info_.Reset();

  return *this;
}

template <typename Allocator>
template <TypedBufferStorable T>
constexpr void TypedBuffer<Allocator>::ChangeType() noexcept {
  Destroy();
  type_info_ = TypeInfo::template From<T>();
}

template <typename Allocator>
constexpr void TypedBuffer<Allocator>::Reset() noexcept {
  Destroy();
  storage_.clear();
  type_info_.Reset();
}

template <typename Allocator>
template <TypedBufferStorable T, typename... Args>
  requires std::constructible_from<T, Args...>
inline T& TypedBuffer<Allocator>::Set(Args&&... args) {
  Destroy();

  type_info_ = TypeInfo::template From<T>();
  if (storage_.size() < sizeof(T)) {
    storage_.resize(sizeof(T));
  }

  std::construct_at(std::launder(reinterpret_cast<T*>(storage_.data())),
                    std::forward<Args>(args)...);
  has_value_ = true;
  return *std::launder(std::launder(reinterpret_cast<T*>(storage_.data())));
}

template <typename Allocator>
constexpr void TypedBuffer<Allocator>::Swap(TypedBuffer& other) noexcept(
    std::is_nothrow_swappable_v<StorageType>) {
  std::swap(type_info_, other.type_info_);
  std::swap(storage_, other.storage_);
  std::swap(has_value_, other.has_value_);
}

template <typename Allocator>
template <TypedBufferStorable T>
inline T& TypedBuffer<Allocator>::Value() noexcept {
  HELIOS_ASSERT(has_value_, "TypedBuffer::Value: buffer is empty!");
  HELIOS_ASSERT(type_info_.type_index == TypeIndexOf<T>(),
                "TypedBuffer::Value: type mismatch!");
  return *std::launder(reinterpret_cast<T*>(storage_.data()));
}

template <typename Allocator>
template <TypedBufferStorable T>
inline const T& TypedBuffer<Allocator>::Value() const noexcept {
  HELIOS_ASSERT(has_value_, "TypedBuffer::Value: buffer is empty!");
  HELIOS_ASSERT(type_info_.type_index == TypeIndexOf<T>(),
                "TypedBuffer::Value: type mismatch!");
  return *std::launder(reinterpret_cast<const T*>(storage_.data()));
}

template <typename Allocator>
constexpr auto TypedBuffer<Allocator>::Bytes() const noexcept
    -> std::span<const std::byte> {
  if (!has_value_) {
    return {};
  }
  return {storage_.data(), type_info_.element_size};
}

template <typename Allocator>
template <TypedBufferStorable T>
inline T* TypedBuffer<Allocator>::DataPtr() noexcept {
  HELIOS_ASSERT(
      !type_info_.IsValid() || type_info_.type_index == TypeIndexOf<T>(),
      "TypedBuffer::DataPtr: type mismatch!");
  if (storage_.empty()) [[unlikely]] {
    return nullptr;
  }
  return std::launder(reinterpret_cast<T*>(storage_.data()));
}

template <typename Allocator>
template <TypedBufferStorable T>
inline const T* TypedBuffer<Allocator>::DataPtr() const noexcept {
  HELIOS_ASSERT(
      !type_info_.IsValid() || type_info_.type_index == TypeIndexOf<T>(),
      "TypedBuffer::DataPtr: type mismatch!");
  if (storage_.empty()) [[unlikely]] {
    return nullptr;
  }
  return std::launder(reinterpret_cast<const T*>(storage_.data()));
}

template <typename Allocator>
constexpr void TypedBuffer<Allocator>::Destroy() noexcept {
  if (has_value_) {
    if (type_info_.destroy != nullptr) {
      type_info_.destroy(storage_.data(), 1);
    }
    has_value_ = false;
  }
}

using PmrTypedBuffer = TypedBuffer<std::pmr::polymorphic_allocator<std::byte>>;

}  // namespace helios::container
