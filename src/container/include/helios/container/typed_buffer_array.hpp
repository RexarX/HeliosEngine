#pragma once

#include <helios/assert.hpp>
#include <helios/container/details/typed_buffer_common.hpp>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <memory_resource>
#include <new>
#include <ranges>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

namespace helios::container {

/**
 * @brief Type-erased sequential byte storage for a single type (array variant).
 * @details Stores multiple instances of a type in a contiguous
 * `std::vector<std::byte>` buffer. The stored type is not fixed at class
 * instantiation but is verified at runtime. Object lifetimes are properly
 * managed. Trivially-copyable types are fast-pathed.
 * @tparam Allocator The allocator type for the byte storage (default:
 * `std::allocator<std::byte>`)
 */
template <typename Allocator = std::allocator<std::byte>>
class TypedBufferArray {
public:
  using allocator_type = Allocator;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using TypeIndex = utils::TypeIndex;

  using ContainerType = std::vector<std::byte, allocator_type>;

private:
  using TypeInfo = details::TypeBufferInfo;

public:
  /**
   * @brief Default constructor.
   * @details Creates an empty storage with no associated type.
   */
  constexpr TypedBufferArray() noexcept(
      std::is_nothrow_default_constructible_v<ContainerType>) = default;

  /**
   * @brief Constructs an empty storage with a custom allocator.
   * @param alloc Allocator instance to use for the underlying container
   */
  explicit constexpr TypedBufferArray(const allocator_type& alloc) noexcept(
      std::is_nothrow_constructible_v<ContainerType, const allocator_type&>)
      : storage_(alloc) {}

  /**
   * @brief Constructs an empty storage from a PMR memory resource.
   * @details Enabled only when `allocator_type` is constructible from
   * `std::pmr::memory_resource*`.
   * @param resource Memory resource used to construct allocator
   */
  explicit constexpr TypedBufferArray(
      std::pmr::memory_resource*
          resource) noexcept(std::
                                 is_nothrow_constructible_v<
                                     allocator_type,
                                     std::pmr::memory_resource*>)
    requires std::constructible_from<allocator_type, std::pmr::memory_resource*>
      : TypedBufferArray(allocator_type{resource}) {}

  TypedBufferArray(std::nullptr_t) = delete;

  /**
   * @brief Constructs storage with count default-inserted elements of type `T`.
   * @tparam T The type to store
   * @param count Number of elements to default-construct
   */
  template <TypedBufferStorable T>
    requires std::default_initializable<T>
  explicit constexpr TypedBufferArray(size_type count);

  /**
   * @brief Constructs storage with count copies of value.
   * @tparam T The type to store
   * @param count Number of elements
   * @param value Value to copy
   */
  template <TypedBufferStorable T>
    requires std::copy_constructible<T>
  constexpr TypedBufferArray(size_type count, const T& value);

  /**
   * @brief Constructs storage from iterator range.
   * @tparam InputIt Input iterator type
   * @param first Beginning of range
   * @param last End of range
   */
  template <std::input_iterator InputIt>
    requires TypedBufferStorable<std::iter_value_t<InputIt>>
  constexpr TypedBufferArray(InputIt first, InputIt last);

  /**
   * @brief Constructs storage from initializer list.
   * @tparam T The type to store
   * @param init Initializer list
   */
  template <TypedBufferStorable T>
    requires std::copy_constructible<T>
  constexpr TypedBufferArray(std::initializer_list<T> init)
      : TypedBufferArray(init.begin(), init.end()) {}

  constexpr TypedBufferArray(const TypedBufferArray& other);
  constexpr TypedBufferArray(
      const TypedBufferArray& other,
      const allocator_type&
          alloc) noexcept(std::
                              is_nothrow_constructible_v<
                                  ContainerType, const allocator_type&>);
  constexpr TypedBufferArray(TypedBufferArray&& other) noexcept(
      std::is_nothrow_move_constructible_v<ContainerType>);
  constexpr TypedBufferArray(
      TypedBufferArray&& other,
      const allocator_type&
          alloc) noexcept(std::
                              is_nothrow_constructible_v<
                                  ContainerType, const allocator_type&>);
  constexpr ~TypedBufferArray() noexcept { DestroyAll(); }

  constexpr TypedBufferArray& operator=(const TypedBufferArray& other);
  constexpr TypedBufferArray& operator=(TypedBufferArray&& other) noexcept(
      std::is_nothrow_move_assignable_v<ContainerType>);

  /**
   * @brief Assignment from initializer list.
   * @tparam T The type to store
   * @param init Initializer list
   * @return Reference to this
   */
  template <TypedBufferStorable T>
    requires std::copy_constructible<T>
  constexpr TypedBufferArray& operator=(std::initializer_list<T> init);

  /**
   * @brief Changes the stored type, destroying current elements but keeping
   * memory.
   * @details Destroys all current elements and resets size to 0. Memory is
   * retained.
   * @tparam T The new type to store
   */
  template <TypedBufferStorable T>
  constexpr void ChangeType() noexcept;

  /**
   * @brief Resets storage to empty state with no associated type.
   * @details Destroys all elements and clears type information.
   */
  constexpr void Reset() noexcept;

  /**
   * @brief Appends a value to the end.
   * @warning Triggers assertion if type mismatch and storage is not empty.
   * @tparam T The type of value (must match stored type)
   * @param value The value to push
   */
  template <TypedBufferStorable T>
  void PushBack(T&& value);

  /**
   * @brief Constructs an element in-place at the end.
   * @warning Triggers assertion if type mismatch and storage is not empty.
   * @tparam T The type to construct (must match stored type)
   * @tparam Args Constructor argument types
   * @param args Arguments forwarded to the constructor
   * @return Reference to the emplaced element
   */
  template <TypedBufferStorable T, typename... Args>
    requires std::constructible_from<T, Args...>
  T& EmplaceBack(Args&&... args);

  /**
   * @brief Constructs an element in-place at the specified position.
   * @warning Triggers assertion if pos > `Size()` or type mismatch and storage
   * is not empty.
   * @tparam T The type to construct (must match stored type)
   * @tparam Args Constructor argument types
   * @param pos Position (element index)
   * @param args Arguments forwarded to the constructor
   * @return Reference to the emplaced element
   */
  template <TypedBufferStorable T, typename... Args>
    requires std::constructible_from<T, Args...>
  T& Emplace(size_type pos, Args&&... args);

  /**
   * @brief Inserts a value at the specified position.
   * @warning Triggers assertion if pos > `Size()` or type mismatch and storage
   * is not empty.
   * @tparam T The type to insert (must match stored type)
   * @param pos Position (element index)
   * @param value Value to insert
   * @return Reference to the inserted element
   */
  template <TypedBufferStorable T>
  T& Insert(size_type pos, T&& value) {
    return Emplace<T>(pos, std::forward<T>(value));
  }

  /**
   * @brief Inserts count copies of value at the specified position.
   * @warning Triggers assertion if pos > `Size()` or type mismatch and storage
   * is not empty.
   * @tparam T The type to insert (must match stored type)
   * @param pos Position (element index)
   * @param count Number of copies
   * @param value Value to copy
   * @return Pointer to the first inserted element, or `nullptr` if count == 0
   */
  template <TypedBufferStorable T>
    requires std::copy_constructible<T>
  T* Insert(size_type pos, size_type count, const T& value);

  /**
   * @brief Inserts elements from iterator range at the specified position.
   * @warning Triggers assertion if pos > `Size()` or type mismatch and storage
   * is not empty.
   * @tparam InputIt Input iterator type
   * @param pos Position (element index)
   * @param first Beginning of range
   * @param last End of range
   * @return Pointer to the first inserted element, or `nullptr` if range is
   * empty
   */
  template <std::input_iterator InputIt>
    requires TypedBufferStorable<std::iter_value_t<InputIt>>
  auto Insert(size_type pos, InputIt first, InputIt last)
      -> std::iter_value_t<InputIt>*;

  /**
   * @brief Inserts elements from initializer list at the specified position.
   * @warning Triggers assertion if pos > `Size()` or type mismatch and storage
   * is not empty.
   * @tparam T The type to insert (must match stored type)
   * @param pos Position (element index)
   * @param init Initializer list
   * @return Pointer to the first inserted element, or `nullptr` if list is
   * empty
   */
  template <TypedBufferStorable T>
    requires std::copy_constructible<T>
  T* Insert(size_type pos, std::initializer_list<T> init) {
    return Insert(pos, init.begin(), init.end());
  }

  /**
   * @brief Appends elements from a range to the end.
   * @warning Triggers assertion if type mismatch and storage is not empty.
   * @tparam Range Range type
   * @param range The range to append
   */
  template <std::ranges::input_range Range>
    requires TypedBufferStorable<std::ranges::range_value_t<Range>>
  void AppendRange(Range&& range);

  /**
   * @brief Inserts elements from a range at the specified position.
   * @warning Triggers assertion if pos > `Size()` or type mismatch and storage
   * is not empty.
   * @tparam Range Range type
   * @param pos Position (element index)
   * @param range The range to insert
   * @return Pointer to the first inserted element, or `nullptr` if range is
   * empty
   */
  template <std::ranges::input_range Range>
    requires TypedBufferStorable<std::ranges::range_value_t<Range>>
  auto InsertRange(size_type pos, Range&& range)
      -> std::ranges::range_value_t<Range>*;

  /**
   * @brief Removes the last stored value.
   * @warning Triggers assertion if storage is empty.
   */
  constexpr void PopBack() noexcept;

  /**
   * @brief Erases the element at the specified position.
   * @warning Triggers assertion if pos >= `Size()` or storage has no type.
   * @param pos Position (element index) to erase
   */
  constexpr void Erase(size_type pos);

  /**
   * @brief Erases elements in the range [first_pos, last_pos).
   * @warning Triggers assertion if range is invalid or storage has no type.
   * @param first_pos Beginning of range (element index)
   * @param last_pos End of range (element index, exclusive)
   */
  constexpr void Erase(size_type first_pos, size_type last_pos);

  /// @brief Clears all stored data, but retains type information.
  constexpr void Clear() noexcept;

  /**
   * @brief Reserves space for at least the specified number of elements.
   * @warning Triggers assertion if no type is set and count > 0.
   * @param count Number of elements to reserve space for
   */
  constexpr void Reserve(size_type count);

  /// @brief Reduces capacity to fit the current size.
  constexpr void ShrinkToFit();

  /**
   * @brief Resizes the storage to contain count elements (default-initialized).
   * @warning Triggers assertion if type mismatch and storage is not empty.
   * @tparam T The type of elements (must match stored type)
   * @param count New size
   */
  template <TypedBufferStorable T>
    requires std::default_initializable<T>
  void Resize(size_type count);

  /**
   * @brief Resizes the storage to contain count elements.
   * @warning Triggers assertion if type mismatch and storage is not empty.
   * @tparam T The type of elements (must match stored type)
   * @param count New size
   * @param value Value to fill new elements with
   */
  template <TypedBufferStorable T>
    requires std::copy_constructible<T>
  void Resize(size_type count, const T& value);

  /**
   * @brief Access element at index.
   * @warning Triggers assertion if index >= `Size()` or type mismatch.
   * @tparam T The type of elements (must match stored type)
   * @param index Element index
   * @return Reference to element
   */
  template <TypedBufferStorable T>
  [[nodiscard]] T& At(size_type index) noexcept;

  /**
   * @brief Access element at index (const).
   * @warning Triggers assertion if index >= `Size()` or type mismatch.
   * @tparam T The type of elements (must match stored type)
   * @param index Element index
   * @return Const reference to element
   */
  template <TypedBufferStorable T>
  [[nodiscard]] const T& At(size_type index) const noexcept;

  /**
   * @brief Access the first element.
   * @warning Triggers assertion if storage is empty or type mismatch.
   * @tparam T The type of elements (must match stored type)
   * @return Reference to the first element
   */
  template <TypedBufferStorable T>
  [[nodiscard]] T& Front() noexcept;

  /**
   * @brief Access the first element (const).
   * @warning Triggers assertion if storage is empty or type mismatch.
   * @tparam T The type of elements (must match stored type)
   * @return Const reference to the first element
   */
  template <TypedBufferStorable T>
  [[nodiscard]] const T& Front() const noexcept;

  /**
   * @brief Access the last element.
   * @warning Triggers assertion if storage is empty or type mismatch.
   * @tparam T The type of elements (must match stored type)
   * @return Reference to the last element
   */
  template <TypedBufferStorable T>
  [[nodiscard]] T& Back() noexcept;

  /**
   * @brief Access the last element (const).
   * @warning Triggers assertion if storage is empty or type mismatch.
   * @tparam T The type of elements (must match stored type)
   * @return Const reference to the last element
   */
  template <TypedBufferStorable T>
  [[nodiscard]] const T& Back() const noexcept;

  /**
   * @brief Merges all elements from another `TypedBufferArray` into this one.
   * @details Appends all elements from `other` to the end of this buffer.
   * If this buffer is empty and has no type, it takes ownership of `other`'s
   * state. After merging, `other` is left in a valid but empty state.
   * @warning Triggers assertion if both buffers have types set and they don't
   * match.
   * @tparam OtherAllocator The allocator type of the other buffer
   * @param other The buffer to merge from (will be left empty)
   */
  template <typename OtherAllocator>
  constexpr void Merge(const TypedBufferArray<OtherAllocator>& other);

  /**
   * @brief Merges all elements from another `TypedBufferArray` into this one.
   * @details Rvalue overload that consumes the source buffer.
   * @tparam OtherAllocator The allocator type of the other buffer
   * @param other The buffer to merge from (will be left empty)
   */
  template <typename OtherAllocator>
  constexpr void Merge(TypedBufferArray<OtherAllocator>&& other);

  /**
   * @brief Swaps contents with another storage.
   * @param other Storage to swap with
   */
  constexpr void Swap(TypedBufferArray& other) noexcept(
      std::is_nothrow_swappable_v<ContainerType>);

  /**
   * @brief Swaps two elements within the buffer at the given indices
   * (type-erased).
   * @details For trivially copyable types, performs a direct byte-level swap.
   * For non-trivially copyable types, performs a 3-step move-construct swap via
   * temporary storage.
   * @warning Triggers assertion if storage has no type, or index >= `Size()`.
   * @param index Index of the first element
   * @param other_index Index of the second element
   */
  void Swap(size_type index, size_type other_index);

  /**
   * @brief Swaps two elements within the buffer at the given indices (typed).
   * @details Uses `std::swap`.
   * @warning Triggers assertion if storage has no type, stored type doesn't
   * match `T`, or index >= `Size()`.
   * @tparam T The type of stored elements (must match stored type)
   * @param index Index of the first element
   * @param other_index Index of the second element
   */
  template <TypedBufferStorable T>
  void Swap(size_type index, size_type other_index);

  friend constexpr void
  swap(TypedBufferArray& lhs, TypedBufferArray& rhs) noexcept(
      std::is_nothrow_swappable_v<ContainerType>) {
    lhs.Swap(rhs);
  }

  /**
   * @brief Checks if the storage is empty.
   * @return True if empty, false if a value is stored
   */
  [[nodiscard]] constexpr bool Empty() const noexcept { return size_ == 0; }

  /**
   * @brief Checks if a type is currently associated with this storage.
   * @return True if a type is set, false if no type is set
   */
  [[nodiscard]] constexpr bool HasType() const noexcept {
    return type_info_.IsValid();
  }

  /**
   * @brief Checks if the stored type matches `T`.
   * @tparam T The type to check
   * @return true if types match or no type is set
   */
  template <TypedBufferStorable T>
  [[nodiscard]] constexpr bool IsType() const noexcept {
    return !type_info_.IsValid() || type_info_.type_index == TypeIndexOf<T>();
  }

  /**
   * @brief Gets the type index for type `T`.
   * @tparam T The type to get index for
   * @return Type index
   */
  template <TypedBufferStorable T>
  [[nodiscard]] static consteval TypeIndex TypeIndexOf() noexcept {
    return utils::TypeIndex::From<T>();
  }

  /**
   * @brief Gets the current stored type index.
   * @return Type index of the stored type, or invalid index if no type is set
   */
  [[nodiscard]] constexpr TypeIndex StoredTypeId() const noexcept {
    return type_info_.type_index;
  }

  /** @brief Returns the number of stored elements.
   * @return Number of elements currently stored, or 0 if no type is set (even
   * if bytes are present)
   */
  [[nodiscard]] constexpr size_type Size() const noexcept { return size_; }

  /**
   * @brief Returns the number of bytes occupied by stored elements.
   * @warning If no type is set, this will return 0 even if there are bytes in
   * the storage.
   */
  [[nodiscard]] constexpr size_type SizeBytes() const noexcept {
    return size_ * type_info_.element_size;
  }

  /**
   * @brief Returns the size of each element in bytes.
   * @return Size of each element in bytes, or 0 if no type is set
   */
  [[nodiscard]] constexpr size_type ElementSize() const noexcept {
    return type_info_.element_size;
  }

  /**
   * @brief Returns the capacity in number of elements.
   * @return Capacity in number of elements, or 0 if no type is set
   */
  [[nodiscard]] constexpr size_type Capacity() const noexcept;

  /**
   * @brief Retrieves a span of all stored values.
   * @warning Triggers assertion if type mismatch.
   * @tparam T The type of elements (must match stored type)
   * @return A span containing all values, or empty span if no values stored
   */
  template <TypedBufferStorable T>
  [[nodiscard]] auto Data() noexcept -> std::span<T>;

  /**
   * @brief Retrieves a const span of all stored values.
   * @warning Triggers assertion if type mismatch.
   * @tparam T The type of elements (must match stored type)
   * @return A const span containing all values, or empty span if no values
   * stored
   */
  template <TypedBufferStorable T>
  [[nodiscard]] auto Data() const noexcept -> std::span<const T>;

  /**
   * @brief Retrieves a const byte span of all stored data (type-erased).
   * @return A const byte span containing all stored bytes, or empty span if no
   * type is set (even if bytes are present)
   */
  [[nodiscard]] constexpr auto Bytes() const noexcept
      -> std::span<const std::byte> {
    return {storage_.data(), SizeBytes()};
  }

  /**
   * @brief Returns iterator to the beginning.
   * @tparam T The type of elements (must match stored type)
   * @return Iterator to the first element, or `nullptr` if no elements stored
   */
  template <TypedBufferStorable T>
  [[nodiscard]] T* begin() noexcept {
    return DataPtr<T>();
  }

  /**
   * @brief Returns const iterator to the beginning.
   * @tparam T The type of elements (must match stored type)
   * @return Const iterator to the first element, or `nullptr` if no elements
   * stored
   */
  template <TypedBufferStorable T>
  [[nodiscard]] const T* begin() const noexcept {
    return DataPtr<T>();
  }

  /**
   * @brief Returns const iterator to the beginning.
   * @tparam T The type of elements (must match stored type)
   * @return Const iterator to the first element, or `nullptr` if no elements
   * stored
   */
  template <TypedBufferStorable T>
  [[nodiscard]] const T* cbegin() const noexcept {
    return DataPtr<T>();
  }

  /**
   * @brief Returns iterator past the last element.
   * @tparam T The type of elements (must match stored type)
   * @return Iterator past the last element, or `nullptr` if no elements stored
   */
  template <TypedBufferStorable T>
  [[nodiscard]] T* end() noexcept {
    return DataPtr<T>() + size_;
  }

  /**
   * @brief Returns const iterator past the last element.
   * @tparam T The type of elements (must match stored type)
   * @return Const iterator past the last element, or `nullptr` if no elements
   * stored
   */
  template <TypedBufferStorable T>
  [[nodiscard]] const T* end() const noexcept {
    return DataPtr<T>() + size_;
  }

  /**
   * @brief Returns const iterator past the last element.
   * @tparam T The type of elements (must match stored type)
   * @return Const iterator past the last element, or `nullptr` if no elements
   * stored
   */
  template <TypedBufferStorable T>
  [[nodiscard]] const T* cend() const noexcept {
    return DataPtr<T>() + size_;
  }

  /**
   * @brief Returns reverse iterator to the last element.
   * @tparam T The type of elements (must match stored type)
   * @return Reverse iterator to the last element, or `nullptr` if no elements
   * stored
   */
  template <TypedBufferStorable T>
  [[nodiscard]] auto rbegin() noexcept -> std::reverse_iterator<T*> {
    return std::reverse_iterator<T*>(end<T>());
  }

  /**
   * @brief Returns const reverse iterator to the last element.
   * @tparam T The type of elements (must match stored type)
   * @return Const reverse iterator to the last element, or `nullptr` if no
   * elements stored
   */
  template <TypedBufferStorable T>
  [[nodiscard]] auto rbegin() const noexcept
      -> std::reverse_iterator<const T*> {
    return std::reverse_iterator<const T*>(end<T>());
  }

  /**
   * @brief Returns const reverse iterator to the last element.
   * @tparam T The type of elements (must match stored type)
   * @return Const reverse iterator to the last element, or `nullptr` if no
   * elements stored
   */
  template <TypedBufferStorable T>
  [[nodiscard]] auto crbegin() const noexcept
      -> std::reverse_iterator<const T*> {
    return std::reverse_iterator<const T*>(cend<T>());
  }

  /**
   * @brief Returns reverse iterator before the first element.
   * @tparam T The type of elements (must match stored type)
   * @return Reverse iterator before the first element, or `nullptr` if no
   * elements stored
   */
  template <TypedBufferStorable T>
  [[nodiscard]] auto rend() noexcept -> std::reverse_iterator<T*> {
    return std::reverse_iterator<T*>(begin<T>());
  }

  /**
   * @brief Returns const reverse iterator before the first element.
   * @tparam T The type of elements (must match stored type)
   * @return Const reverse iterator before the first element, or `nullptr` if no
   * elements stored
   */
  template <TypedBufferStorable T>
  [[nodiscard]] auto rend() const noexcept -> std::reverse_iterator<const T*> {
    return std::reverse_iterator<const T*>(begin<T>());
  }

  /**
   * @brief Returns const reverse iterator before the first element.
   * @tparam T The type of elements (must match stored type)
   * @return Const reverse iterator before the first element, or `nullptr` if no
   * elements stored
   */
  template <TypedBufferStorable T>
  [[nodiscard]] auto crend() const noexcept -> std::reverse_iterator<const T*> {
    return std::reverse_iterator<const T*>(cbegin<T>());
  }

private:
  template <typename OtherA>
  friend class TypedBufferArray;

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

  constexpr void DestroyAll() noexcept;
  constexpr void DestroyRange(size_type first_idx, size_type last_idx) noexcept;
  constexpr void GrowIfNeeded(size_type additional_count);
  constexpr void ShiftRight(size_type pos, size_type count);
  constexpr void ShiftLeft(size_type pos, size_type count);

  /// @brief Verifies or sets type for operations.
  template <TypedBufferStorable T>
  constexpr void EnsureType();

  TypeInfo type_info_{};
  ContainerType storage_;
  size_type size_ = 0;
};

template <typename Allocator>
template <TypedBufferStorable T>
  requires std::default_initializable<T>
constexpr TypedBufferArray<Allocator>::TypedBufferArray(size_type count) {
  if (count == 0) [[unlikely]] {
    return;
  }

  type_info_ = TypeInfo::template From<T>();
  storage_.resize(count * sizeof(T));

  auto* ptr = static_cast<T*>(RawDataPtr());
  std::uninitialized_default_construct_n(ptr, count);
  size_ = count;
}

template <typename Allocator>
template <TypedBufferStorable T>
  requires std::copy_constructible<T>
constexpr TypedBufferArray<Allocator>::TypedBufferArray(size_type count,
                                                        const T& value) {
  using DecayedT = std::remove_cvref_t<T>;

  if (count == 0) [[unlikely]] {
    return;
  }

  type_info_ = TypeInfo::template From<DecayedT>();
  storage_.resize(count * sizeof(DecayedT));

  auto* ptr = static_cast<DecayedT*>(RawDataPtr());
  std::uninitialized_fill_n(ptr, count, value);
  size_ = count;
}

template <typename Allocator>
template <std::input_iterator InputIt>
  requires TypedBufferStorable<std::iter_value_t<InputIt>>
constexpr TypedBufferArray<Allocator>::TypedBufferArray(InputIt first,
                                                        InputIt last) {
  using T = std::iter_value_t<InputIt>;

  if (first == last) [[unlikely]] {
    return;
  }

  type_info_ = TypeInfo::template From<T>();

  if constexpr (std::forward_iterator<InputIt>) {
    const auto count = static_cast<size_type>(std::distance(first, last));
    storage_.resize(count * sizeof(T));
    auto* ptr = static_cast<T*>(RawDataPtr());
    std::uninitialized_copy(first, last, ptr);
    size_ = count;
  } else {
    for (; first != last; ++first) {
      EmplaceBack<T>(*first);
    }
  }
}

template <typename Allocator>
constexpr TypedBufferArray<Allocator>::TypedBufferArray(
    const TypedBufferArray& other) {
  if (other.Empty()) [[unlikely]] {
    type_info_ = other.type_info_;
    return;
  }

  type_info_ = other.type_info_;
  HELIOS_ASSERT(
      type_info_.is_copy_constructible,
      "Cannot copy TypedBufferArray: stored type is not copy constructible!");

  storage_.resize(other.SizeBytes());

  if (type_info_.is_trivially_copyable) {
    std::memcpy(storage_.data(), other.storage_.data(), other.SizeBytes());
  } else {
    type_info_.copy_construct(RawDataPtr(), other.RawDataPtr(), other.size_);
  }
  size_ = other.size_;
}

template <typename Allocator>
constexpr TypedBufferArray<Allocator>::TypedBufferArray(
    const TypedBufferArray& other,
    const allocator_type&
        alloc) noexcept(std::is_nothrow_constructible_v<ContainerType,
                                                        const allocator_type&>)
    : storage_(alloc) {
  if (other.Empty()) [[unlikely]] {
    type_info_ = other.type_info_;
    return;
  }

  type_info_ = other.type_info_;
  HELIOS_ASSERT(
      type_info_.is_copy_constructible,
      "Cannot copy TypedBufferArray: stored type is not copy constructible!");

  storage_.resize(other.SizeBytes());

  if (type_info_.is_trivially_copyable) {
    std::memcpy(storage_.data(), other.storage_.data(), other.SizeBytes());
  } else {
    type_info_.copy_construct(RawDataPtr(), other.RawDataPtr(), other.size_);
  }
  size_ = other.size_;
}

template <typename Allocator>
constexpr TypedBufferArray<Allocator>::TypedBufferArray(
    TypedBufferArray&&
        other) noexcept(std::is_nothrow_move_constructible_v<ContainerType>)
    : type_info_(other.type_info_),
      storage_(std::move(other.storage_)),
      size_(other.size_) {
  other.size_ = 0;
  other.type_info_ = TypeInfo{};
}

template <typename Allocator>
constexpr TypedBufferArray<Allocator>::TypedBufferArray(
    TypedBufferArray&& other,
    const allocator_type&
        alloc) noexcept(std::is_nothrow_constructible_v<ContainerType,
                                                        const allocator_type&>)
    : type_info_(other.type_info_), storage_(alloc), size_(0) {
  if (other.size_ == 0) [[unlikely]] {
    other.type_info_.Reset();
    return;
  }

  if (storage_.get_allocator() == other.storage_.get_allocator()) {
    storage_ = std::move(other.storage_);
    size_ = other.size_;
    other.size_ = 0;
    other.type_info_.Reset();
    return;
  }

  storage_.resize(other.SizeBytes());

  if (type_info_.is_trivially_copyable) {
    std::memcpy(storage_.data(), other.storage_.data(), other.SizeBytes());
  } else if (type_info_.move_construct != nullptr) {
    type_info_.move_construct(RawDataPtr(), other.RawDataPtr(), other.size_);
    other.DestroyAll();
  } else if (type_info_.copy_construct != nullptr) {
    type_info_.copy_construct(RawDataPtr(), other.RawDataPtr(), other.size_);
    other.DestroyAll();
  }

  size_ = other.size_;
  other.storage_.clear();
  other.size_ = 0;
  other.type_info_.Reset();
}

template <typename Allocator>
constexpr auto TypedBufferArray<Allocator>::operator=(
    const TypedBufferArray& other) -> TypedBufferArray& {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  Clear();

  if (other.Empty()) [[unlikely]] {
    type_info_ = other.type_info_;
    return *this;
  }

  type_info_ = other.type_info_;
  HELIOS_ASSERT(
      type_info_.is_copy_constructible,
      "Cannot copy TypedBufferArray: stored type is not copy constructible!");

  storage_.resize(other.SizeBytes());

  if (type_info_.is_trivially_copyable) {
    std::memcpy(storage_.data(), other.storage_.data(), other.SizeBytes());
  } else {
    type_info_.copy_construct(RawDataPtr(), other.RawDataPtr(), other.size_);
  }
  size_ = other.size_;

  return *this;
}

template <typename Allocator>
constexpr auto
TypedBufferArray<Allocator>::operator=(TypedBufferArray&& other) noexcept(
    std::is_nothrow_move_assignable_v<ContainerType>) -> TypedBufferArray& {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  DestroyAll();
  type_info_ = other.type_info_;
  storage_ = std::move(other.storage_);
  size_ = other.size_;
  other.size_ = 0;
  other.type_info_.Reset();

  return *this;
}

template <typename Allocator>
template <TypedBufferStorable T>
  requires std::copy_constructible<T>
constexpr auto TypedBufferArray<Allocator>::operator=(
    std::initializer_list<T> init) -> TypedBufferArray& {
  Clear();

  if (init.size() == 0) [[unlikely]] {
    return *this;
  }

  type_info_ = TypeInfo::template From<T>();
  storage_.resize(init.size() * sizeof(T));

  auto* ptr = static_cast<T*>(RawDataPtr());
  std::uninitialized_copy(init.begin(), init.end(), ptr);
  size_ = init.size();

  return *this;
}

template <typename Allocator>
template <TypedBufferStorable T>
constexpr void TypedBufferArray<Allocator>::ChangeType() noexcept {
  DestroyAll();
  size_ = 0;
  type_info_ = TypeInfo::template From<T>();
}

template <typename Allocator>
constexpr void TypedBufferArray<Allocator>::Reset() noexcept {
  DestroyAll();
  storage_.clear();
  size_ = 0;
  type_info_.Reset();
}

template <typename Allocator>
template <TypedBufferStorable T>
inline void TypedBufferArray<Allocator>::PushBack(T&& value) {
  using DecayedT = std::remove_cvref_t<T>;

  EnsureType<DecayedT>();
  GrowIfNeeded(1);
  std::construct_at(DataPtr<DecayedT>() + size_, std::forward<T>(value));
  ++size_;
}

template <typename Allocator>
template <TypedBufferStorable T, typename... Args>
  requires std::constructible_from<T, Args...>
inline T& TypedBufferArray<Allocator>::EmplaceBack(Args&&... args) {
  EnsureType<T>();
  GrowIfNeeded(1);
  auto* ptr = DataPtr<T>() + size_;
  std::construct_at(ptr, std::forward<Args>(args)...);
  ++size_;
  return *ptr;
}

template <typename Allocator>
template <TypedBufferStorable T, typename... Args>
  requires std::constructible_from<T, Args...>
inline T& TypedBufferArray<Allocator>::Emplace(size_type pos, Args&&... args) {
  HELIOS_ASSERT(pos <= size_, "Emplace position '{}' is out of bounds!", pos);
  EnsureType<T>();

  if (pos == size_) {
    return EmplaceBack<T>(std::forward<Args>(args)...);
  }

  GrowIfNeeded(1);
  ShiftRight(pos, 1);
  auto* ptr = DataPtr<T>() + pos;
  std::construct_at(ptr, std::forward<Args>(args)...);
  ++size_;
  return *ptr;
}

template <typename Allocator>
template <TypedBufferStorable T>
  requires std::copy_constructible<T>
inline T* TypedBufferArray<Allocator>::Insert(size_type pos, size_type count,
                                              const T& value) {
  using DecayedT = std::remove_cvref_t<T>;

  HELIOS_ASSERT(pos <= size_, "Insert position '{}' is out of bounds!", pos);

  if (count == 0) [[unlikely]] {
    return nullptr;
  }

  EnsureType<DecayedT>();
  GrowIfNeeded(count);

  if (pos < size_) {
    ShiftRight(pos, count);
  }

  auto* ptr = DataPtr<DecayedT>() + pos;
  std::uninitialized_fill_n(ptr, count, value);
  size_ += count;

  return ptr;
}

template <typename Allocator>
template <std::input_iterator InputIt>
  requires TypedBufferStorable<std::iter_value_t<InputIt>>
inline auto TypedBufferArray<Allocator>::Insert(size_type pos, InputIt first,
                                                InputIt last)
    -> std::iter_value_t<InputIt>* {
  using T = std::iter_value_t<InputIt>;

  HELIOS_ASSERT(pos <= size_, "Insert position '{}' is out of bounds!", pos);

  if (first == last) [[unlikely]] {
    return nullptr;
  }

  EnsureType<T>();

  if constexpr (std::forward_iterator<InputIt>) {
    const auto count = static_cast<size_type>(std::distance(first, last));
    GrowIfNeeded(count);

    if (pos < size_) {
      ShiftRight(pos, count);
    }

    auto* ptr = DataPtr<T>() + pos;
    std::uninitialized_copy(first, last, ptr);
    size_ += count;
    return ptr;
  } else {
    auto current_pos = pos;
    for (; first != last; ++first, ++current_pos) {
      GrowIfNeeded(1);
      if (current_pos < size_) {
        ShiftRight(current_pos, 1);
      }
      std::construct_at(DataPtr<T>() + current_pos, *first);
      ++size_;
    }
    return DataPtr<T>() + pos;
  }
}

template <typename Allocator>
template <std::ranges::input_range Range>
  requires TypedBufferStorable<std::ranges::range_value_t<Range>>
inline void TypedBufferArray<Allocator>::AppendRange(Range&& range) {
  using T = std::ranges::range_value_t<Range>;
  EnsureType<T>();

  if constexpr (std::ranges::sized_range<Range>) {
    const size_type count = std::ranges::size(range);
    if (count == 0) [[unlikely]] {
      return;
    }
    GrowIfNeeded(count);
  }

  for (auto&& elem : std::forward<Range>(range)) {
    GrowIfNeeded(1);
    std::construct_at(DataPtr<T>() + size_, std::forward<decltype(elem)>(elem));
    ++size_;
  }
}

template <typename Allocator>
template <std::ranges::input_range Range>
  requires TypedBufferStorable<std::ranges::range_value_t<Range>>
inline auto TypedBufferArray<Allocator>::InsertRange(size_type pos,
                                                     Range&& range)
    -> std::ranges::range_value_t<Range>* {
  using T = std::ranges::range_value_t<Range>;

  HELIOS_ASSERT(pos <= size_, "InsertRange position '{}' is out of bounds!",
                pos);
  EnsureType<T>();

  if constexpr (std::ranges::sized_range<Range>) {
    const size_type count = std::ranges::size(range);
    if (count == 0) [[unlikely]] {
      return nullptr;
    }

    GrowIfNeeded(count);
    if (pos < size_) {
      ShiftRight(pos, count);
    }

    size_type idx = 0;
    for (auto&& elem : std::forward<Range>(range)) {
      std::construct_at(DataPtr<T>() + pos + idx,
                        std::forward<decltype(elem)>(elem));
      ++idx;
    }
    size_ += count;
    return DataPtr<T>() + pos;
  } else {
    auto current_pos = pos;
    for (auto&& elem : std::forward<Range>(range)) {
      GrowIfNeeded(1);
      if (current_pos < size_) {
        ShiftRight(current_pos, 1);
      }
      std::construct_at(DataPtr<T>() + current_pos,
                        std::forward<decltype(elem)>(elem));
      ++size_;
      ++current_pos;
    }
    return pos < current_pos ? DataPtr<T>() + pos : nullptr;
  }
}

template <typename Allocator>
constexpr void TypedBufferArray<Allocator>::PopBack() noexcept {
  HELIOS_ASSERT(!Empty(), "Cannot pop from empty TypedBufferArray!");
  --size_;
  DestroyRange(size_, size_ + 1);
}

template <typename Allocator>
constexpr void TypedBufferArray<Allocator>::Erase(size_type pos) {
  HELIOS_ASSERT(pos < size_, "Erase position '{}' out of bounds!", pos);
  HELIOS_ASSERT(HasType(), "Cannot erase from storage with no type!");

  DestroyRange(pos, pos + 1);
  if (pos + 1 < size_) {
    ShiftLeft(pos + 1, 1);
  }
  --size_;
}

template <typename Allocator>
constexpr void TypedBufferArray<Allocator>::Erase(size_type first_pos,
                                                  size_type last_pos) {
  HELIOS_ASSERT(first_pos <= size_, "Erase range start '{}' is out of bounds!",
                first_pos);
  HELIOS_ASSERT(last_pos <= size_, "Erase range end '{}' is out of bounds!",
                last_pos);
  HELIOS_ASSERT(first_pos <= last_pos, "Invalid erase range!");

  if (first_pos == last_pos) [[unlikely]] {
    return;
  }

  HELIOS_ASSERT(HasType(), "Cannot erase from storage with no type!");

  const size_type count = last_pos - first_pos;
  DestroyRange(first_pos, last_pos);

  if (last_pos < size_) {
    ShiftLeft(last_pos, count);
  }
  size_ -= count;
}

template <typename Allocator>
constexpr void TypedBufferArray<Allocator>::Clear() noexcept {
  DestroyAll();
  storage_.clear();
  size_ = 0;
}

template <typename Allocator>
constexpr void TypedBufferArray<Allocator>::Reserve(size_type count) {
  if (count == 0) [[unlikely]] {
    return;
  }

  HELIOS_ASSERT(HasType(), "Cannot reserve without a type set!");

  const size_type required_bytes = count * type_info_.element_size;
  if (required_bytes <= storage_.size()) {
    return;
  }

  if (!type_info_.is_trivially_copyable && size_ > 0) {
    ContainerType new_storage(storage_.get_allocator());
    new_storage.resize(required_bytes);

    if (type_info_.relocate != nullptr) {
      type_info_.relocate(new_storage.data(), storage_.data(), size_);
    } else if (type_info_.move_construct != nullptr) {
      type_info_.move_construct(new_storage.data(), storage_.data(), size_);
      DestroyAll();
    } else if (type_info_.copy_construct != nullptr) {
      type_info_.copy_construct(new_storage.data(), storage_.data(), size_);
      DestroyAll();
    }

    storage_ = std::move(new_storage);
  } else {
    storage_.resize(required_bytes);
  }
}

template <typename Allocator>
constexpr void TypedBufferArray<Allocator>::ShrinkToFit() {
  if (!HasType() || size_ == 0) {
    storage_.clear();
    storage_.shrink_to_fit();
    return;
  }

  const auto used_bytes = SizeBytes();
  if (used_bytes < storage_.size()) {
    ContainerType new_storage(storage_.get_allocator());
    new_storage.resize(used_bytes);

    if (type_info_.is_trivially_copyable) {
      std::memcpy(new_storage.data(), storage_.data(), used_bytes);
    } else if (type_info_.move_construct != nullptr) {
      type_info_.move_construct(new_storage.data(), storage_.data(), size_);
      DestroyAll();
    } else if (type_info_.copy_construct != nullptr) {
      type_info_.copy_construct(new_storage.data(), storage_.data(), size_);
      DestroyAll();
    }

    storage_ = std::move(new_storage);
  }
  storage_.shrink_to_fit();
}

template <typename Allocator>
template <TypedBufferStorable T>
  requires std::default_initializable<T>
inline void TypedBufferArray<Allocator>::Resize(size_type count) {
  EnsureType<T>();
  if (count < size_) {
    DestroyRange(count, size_);
    size_ = count;
  } else if (count > size_) {
    GrowIfNeeded(count - size_);
    auto* ptr = DataPtr<T>() + size_;
    std::uninitialized_default_construct_n(ptr, count - size_);
    size_ = count;
  }
}

template <typename Allocator>
template <TypedBufferStorable T>
  requires std::copy_constructible<T>
inline void TypedBufferArray<Allocator>::Resize(size_type count,
                                                const T& value) {
  using DecayedT = std::remove_cvref_t<T>;

  EnsureType<DecayedT>();
  if (count < size_) {
    DestroyRange(count, size_);
    size_ = count;
  } else if (count > size_) {
    GrowIfNeeded(count - size_);
    auto* ptr = DataPtr<DecayedT>() + size_;
    std::uninitialized_fill_n(ptr, count - size_, value);
    size_ = count;
  }
}

template <typename Allocator>
template <TypedBufferStorable T>
inline T& TypedBufferArray<Allocator>::At(size_type index) noexcept {
  HELIOS_ASSERT(index < size_, "Index '{}' is out of bounds!", index);
  HELIOS_ASSERT(
      !type_info_.IsValid() || type_info_.type_index == TypeIndexOf<T>(),
      "Type mismatch: storage contains different type!");
  return *(DataPtr<T>() + index);
}

template <typename Allocator>
template <TypedBufferStorable T>
inline const T& TypedBufferArray<Allocator>::At(
    size_type index) const noexcept {
  HELIOS_ASSERT(index < size_, "Index '{}' is out of bounds!", index);
  HELIOS_ASSERT(
      !type_info_.IsValid() || type_info_.type_index == TypeIndexOf<T>(),
      "Type mismatch: storage contains different type!");
  return *(DataPtr<T>() + index);
}

template <typename Allocator>
template <TypedBufferStorable T>
inline T& TypedBufferArray<Allocator>::Front() noexcept {
  HELIOS_ASSERT(!Empty(), "Cannot access front of empty TypedBufferArray!");
  HELIOS_ASSERT(
      !type_info_.IsValid() || type_info_.type_index == TypeIndexOf<T>(),
      "Type mismatch: storage contains different type!");
  return *DataPtr<T>();
}

template <typename Allocator>
template <TypedBufferStorable T>
inline const T& TypedBufferArray<Allocator>::Front() const noexcept {
  HELIOS_ASSERT(!Empty(), "Cannot access front of empty TypedBufferArray!");
  HELIOS_ASSERT(
      !type_info_.IsValid() || type_info_.type_index == TypeIndexOf<T>(),
      "Type mismatch: storage contains different type!");
  return *DataPtr<T>();
}

template <typename Allocator>
template <TypedBufferStorable T>
inline T& TypedBufferArray<Allocator>::Back() noexcept {
  HELIOS_ASSERT(!Empty(), "Cannot access back of empty TypedBufferArray!");
  HELIOS_ASSERT(
      !type_info_.IsValid() || type_info_.type_index == TypeIndexOf<T>(),
      "Type mismatch: storage contains different type!");
  return *(DataPtr<T>() + size_ - 1);
}

template <typename Allocator>
template <TypedBufferStorable T>
inline const T& TypedBufferArray<Allocator>::Back() const noexcept {
  HELIOS_ASSERT(!Empty(), "Cannot access back of empty TypedBufferArray!");
  HELIOS_ASSERT(
      !type_info_.IsValid() || type_info_.type_index == TypeIndexOf<T>(),
      "Type mismatch: storage contains different type!");
  return *(DataPtr<T>() + size_ - 1);
}

template <typename Allocator>
template <typename OtherAllocator>
constexpr void TypedBufferArray<Allocator>::Merge(
    const TypedBufferArray<OtherAllocator>& other) {
  if (other.Empty()) [[unlikely]] {
    if (!other.type_info_.IsValid()) {
      return;
    }
    if (!type_info_.IsValid()) {
      type_info_ = other.type_info_;
    }
    return;
  }

  if (!HasType()) {
    type_info_ = other.type_info_;
  }

  HELIOS_ASSERT(
      type_info_.type_index == other.type_info_.type_index,
      "Type mismatch: Cannot merge TypedBufferArray of different types!");

  const size_type other_size = other.size_;
  GrowIfNeeded(other_size);

  if (type_info_.is_trivially_copyable) {
    std::memcpy(static_cast<std::byte*>(RawDataPtr()) + SizeBytes(),
                other.RawDataPtr(), other_size * type_info_.element_size);
  } else if (type_info_.copy_construct) {
    type_info_.copy_construct(
        static_cast<std::byte*>(RawDataPtr()) + SizeBytes(), other.RawDataPtr(),
        other_size);
  } else {
    HELIOS_ASSERT(false,
                  "Cannot merge from const source: stored type is not copy "
                  "constructible!");
  }

  size_ += other_size;
}

template <typename Allocator>
template <typename OtherAllocator>
constexpr void TypedBufferArray<Allocator>::Merge(
    TypedBufferArray<OtherAllocator>&& other) {
  if (other.Empty()) [[unlikely]] {
    if (!other.type_info_.IsValid()) {
      return;
    }
    if (!type_info_.IsValid()) {
      type_info_ = other.type_info_;
    }
    other.type_info_.Reset();
    return;
  }

  if constexpr (std::same_as<Allocator, OtherAllocator>) {
    if (!HasType()) {
      *this = std::move(other);
      return;
    }
  } else {
    if (!HasType()) {
      type_info_ = other.type_info_;
    }
  }

  HELIOS_ASSERT(
      type_info_.type_index == other.type_info_.type_index,
      "Type mismatch: Cannot merge TypedBufferArray of different types!");

  const size_type other_size = other.size_;
  GrowIfNeeded(other_size);

  if (type_info_.is_trivially_copyable) {
    std::memcpy(static_cast<std::byte*>(RawDataPtr()) + SizeBytes(),
                other.RawDataPtr(), other_size * type_info_.element_size);
  } else if (type_info_.move_construct) {
    type_info_.move_construct(
        static_cast<std::byte*>(RawDataPtr()) + SizeBytes(), other.RawDataPtr(),
        other_size);
  } else if (type_info_.copy_construct) {
    type_info_.copy_construct(
        static_cast<std::byte*>(RawDataPtr()) + SizeBytes(), other.RawDataPtr(),
        other_size);
  } else {
    HELIOS_ASSERT(
        false,
        "Cannot merge: stored type is neither move nor copy constructible!");
  }

  size_ += other_size;
  other.DestroyAll();
  other.storage_.clear();
  other.size_ = 0;
  other.type_info_.Reset();
}

template <typename Allocator>
constexpr void
TypedBufferArray<Allocator>::Swap(TypedBufferArray& other) noexcept(
    std::is_nothrow_swappable_v<ContainerType>) {
  std::swap(type_info_, other.type_info_);
  std::swap(storage_, other.storage_);
  std::swap(size_, other.size_);
}

template <typename Allocator>
inline void TypedBufferArray<Allocator>::Swap(size_type index,
                                              size_type other_index) {
  HELIOS_ASSERT(HasType(),
                "Cannot swap elements in storage without a type set!");
  HELIOS_ASSERT(index < size_, "Swap index '{}' is out of bounds!", index);
  HELIOS_ASSERT(other_index < size_, "Swap other_index '{}' is out of bounds!",
                other_index);

  if (index == other_index) [[unlikely]] {
    return;
  }

  auto* base = static_cast<std::byte*>(RawDataPtr());
  auto* first = base + (index * type_info_.element_size);
  auto* second = base + (other_index * type_info_.element_size);

  if (type_info_.is_trivially_copyable) {
    std::swap_ranges(first, first + type_info_.element_size, second);
  } else {
    HELIOS_ASSERT(type_info_.move_construct,
                  "Type must be move constructible to swap elements!");

    auto alloc = storage_.get_allocator();
    using alloc_traits = std::allocator_traits<allocator_type>;

    auto* temp = alloc_traits::allocate(alloc, type_info_.element_size);

    try {
      type_info_.move_construct(temp, first, 1);
      if (type_info_.destroy != nullptr) {
        type_info_.destroy(first, 1);
      }

      type_info_.move_construct(first, second, 1);
      if (type_info_.destroy != nullptr) {
        type_info_.destroy(second, 1);
      }

      type_info_.move_construct(second, temp, 1);
      if (type_info_.destroy != nullptr) {
        type_info_.destroy(temp, 1);
      }
    } catch (...) {
      alloc_traits::deallocate(alloc, temp, type_info_.element_size);
      throw;
    }

    alloc_traits::deallocate(alloc, temp, type_info_.element_size);
  }
}

template <typename Allocator>
template <TypedBufferStorable T>
inline void TypedBufferArray<Allocator>::Swap(size_type index,
                                              size_type other_index) {
  HELIOS_ASSERT(HasType(),
                "Cannot swap elements in storage without a type set!");
  HELIOS_ASSERT(type_info_.type_index == TypeIndexOf<T>(),
                "Type mismatch: stored type does not match!");
  HELIOS_ASSERT(index < size_, "Swap index '{}' is out of bounds!", index);
  HELIOS_ASSERT(other_index < size_, "Swap other_index '{}' is out of bounds!",
                other_index);

  if (index == other_index) [[unlikely]] {
    return;
  }

  std::swap(At<T>(index), At<T>(other_index));
}

template <typename Allocator>
constexpr auto TypedBufferArray<Allocator>::Capacity() const noexcept
    -> size_type {
  if (!HasType()) [[unlikely]] {
    return 0;
  }
  return storage_.capacity() / type_info_.element_size;
}

template <typename Allocator>
template <TypedBufferStorable T>
inline auto TypedBufferArray<Allocator>::Data() noexcept -> std::span<T> {
  HELIOS_ASSERT(
      !type_info_.IsValid() || type_info_.type_index == TypeIndexOf<T>(),
      "Type mismatch: storage contains different type!");
  return {DataPtr<T>(), size_};
}

template <typename Allocator>
template <TypedBufferStorable T>
inline auto TypedBufferArray<Allocator>::Data() const noexcept
    -> std::span<const T> {
  HELIOS_ASSERT(
      !type_info_.IsValid() || type_info_.type_index == TypeIndexOf<T>(),
      "Type mismatch: storage contains different type!");
  return {DataPtr<T>(), size_};
}

template <typename Allocator>
template <TypedBufferStorable T>
inline T* TypedBufferArray<Allocator>::DataPtr() noexcept {
  HELIOS_ASSERT(
      !type_info_.IsValid() || type_info_.type_index == TypeIndexOf<T>(),
      "Type mismatch: storage contains different type!");
  if (storage_.empty()) [[unlikely]] {
    return nullptr;
  }
  return std::launder(reinterpret_cast<T*>(storage_.data()));
}

template <typename Allocator>
template <TypedBufferStorable T>
inline const T* TypedBufferArray<Allocator>::DataPtr() const noexcept {
  HELIOS_ASSERT(
      !type_info_.IsValid() || type_info_.type_index == TypeIndexOf<T>(),
      "Type mismatch: storage contains different type!");
  if (storage_.empty()) [[unlikely]] {
    return nullptr;
  }
  return std::launder(reinterpret_cast<const T*>(storage_.data()));
}

template <typename Allocator>
constexpr void TypedBufferArray<Allocator>::DestroyAll() noexcept {
  if (size_ > 0 && type_info_.destroy != nullptr) {
    type_info_.destroy(RawDataPtr(), size_);
  }
}

template <typename Allocator>
constexpr void TypedBufferArray<Allocator>::DestroyRange(
    size_type first, size_type last) noexcept {
  if (first >= last || type_info_.destroy == nullptr) {
    return;
  }

  auto* ptr =
      static_cast<std::byte*>(RawDataPtr()) + (first * type_info_.element_size);
  type_info_.destroy(ptr, last - first);
}

template <typename Allocator>
constexpr void TypedBufferArray<Allocator>::GrowIfNeeded(
    size_type additional_count) {
  HELIOS_ASSERT(HasType(), "Cannot grow storage without a type set!");

  const size_type required_bytes =
      (size_ + additional_count) * type_info_.element_size;
  if (required_bytes <= storage_.size()) {
    return;
  }

  size_type new_capacity = storage_.size();
  if (new_capacity == 0) {
    new_capacity = type_info_.element_size;
  }
  while (new_capacity < required_bytes) {
    new_capacity =
        new_capacity +
        std::max(new_capacity / 2, size_type{1});  // 1.5x growth, min +1
  }

  if (!type_info_.is_trivially_copyable && size_ > 0) {
    ContainerType new_storage(storage_.get_allocator());
    new_storage.resize(new_capacity);

    if (type_info_.relocate != nullptr) {
      type_info_.relocate(new_storage.data(), storage_.data(), size_);
    } else if (type_info_.move_construct != nullptr) {
      type_info_.move_construct(new_storage.data(), storage_.data(), size_);
      DestroyAll();
    } else if (type_info_.copy_construct != nullptr) {
      type_info_.copy_construct(new_storage.data(), storage_.data(), size_);
      DestroyAll();
    }

    storage_ = std::move(new_storage);
  } else {
    storage_.resize(new_capacity);
  }
}

template <typename Allocator>
constexpr void TypedBufferArray<Allocator>::ShiftRight(size_type pos,
                                                       size_type count) {
  HELIOS_ASSERT(HasType(), "Cannot shift in storage without a type set!");
  HELIOS_ASSERT(pos <= size_, "ShiftRight position '{}' is out of bounds!",
                pos);

  const size_type required_bytes = (size_ + count) * type_info_.element_size;
  if (required_bytes > storage_.size()) {
    storage_.resize(required_bytes);
  }

  const size_type elements_to_move = size_ - pos;
  if (elements_to_move == 0) {
    return;
  }

  auto* src =
      static_cast<std::byte*>(RawDataPtr()) + (pos * type_info_.element_size);
  auto* dest = src + (count * type_info_.element_size);

  if (type_info_.is_trivially_copyable) {
    std::memmove(dest, src, elements_to_move * type_info_.element_size);
  } else if (type_info_.relocate != nullptr) {
    for (size_type idx = elements_to_move; idx > 0; --idx) {
      auto* elem_src = src + ((idx - 1) * type_info_.element_size);
      auto* elem_dest = dest + ((idx - 1) * type_info_.element_size);
      type_info_.relocate(elem_dest, elem_src, 1);
    }
  } else {
    HELIOS_ASSERT(type_info_.move_construct,
                  "Type must be move constructible for shift operations!");
    for (size_type idx = elements_to_move; idx > 0; --idx) {
      auto* elem_src = src + ((idx - 1) * type_info_.element_size);
      auto* elem_dest = dest + ((idx - 1) * type_info_.element_size);
      type_info_.move_construct(elem_dest, elem_src, 1);
      if (type_info_.destroy != nullptr) {
        type_info_.destroy(elem_src, 1);
      }
    }
  }
}

template <typename Allocator>
constexpr void TypedBufferArray<Allocator>::ShiftLeft(size_type pos,
                                                      size_type count) {
  HELIOS_ASSERT(HasType(), "Cannot shift in storage without a type set!");
  HELIOS_ASSERT(pos >= count, "ShiftLeft position '{}' is invalid!", pos);
  HELIOS_ASSERT(pos <= size_, "ShiftLeft position '{}' is out of bounds!", pos);

  const size_type elements_to_move = size_ - pos;
  if (elements_to_move == 0) {
    return;
  }

  auto* src =
      static_cast<std::byte*>(RawDataPtr()) + (pos * type_info_.element_size);
  auto* dest = src - (count * type_info_.element_size);

  if (type_info_.is_trivially_copyable) {
    std::memmove(dest, src, elements_to_move * type_info_.element_size);
  } else if (type_info_.relocate != nullptr) {
    for (size_type idx = 0; idx < elements_to_move; ++idx) {
      auto* elem_src = src + (idx * type_info_.element_size);
      auto* elem_dest = dest + (idx * type_info_.element_size);
      type_info_.relocate(elem_dest, elem_src, 1);
    }
  } else {
    HELIOS_ASSERT(type_info_.move_construct,
                  "Type must be move constructible for shift operations!");
    for (size_type idx = 0; idx < elements_to_move; ++idx) {
      auto* elem_src = src + (idx * type_info_.element_size);
      auto* elem_dest = dest + (idx * type_info_.element_size);
      type_info_.move_construct(elem_dest, elem_src, 1);
      if (type_info_.destroy != nullptr) {
        type_info_.destroy(elem_src, 1);
      }
    }
  }
}

template <typename Allocator>
template <TypedBufferStorable T>
constexpr void TypedBufferArray<Allocator>::EnsureType() {
  if (!type_info_.IsValid()) {
    type_info_ = TypeInfo::template From<T>();
  } else {
    HELIOS_ASSERT(type_info_.type_index == TypeIndexOf<T>(),
                  "Type mismatch: TypedBufferArray contains different type!");
  }
}

using PmrTypedBufferArray =
    TypedBufferArray<std::pmr::polymorphic_allocator<std::byte>>;

/**
 * @brief Erases all elements equal to value from the TypedBufferArray.
 * @details Uses the swap-then-pop pattern for each match. Element order is not
 * preserved.
 * @tparam T The element type
 * @tparam Allocator The allocator type
 * @param storage The buffer to erase from
 * @param value The value to erase
 * @return Number of elements erased
 */
template <TypedBufferStorable T, typename Allocator>
  requires(std::is_class_v<Allocator> &&
           !std::same_as<std::remove_cvref_t<T>,
                         typename TypedBufferArray<Allocator>::size_type>)
inline auto erase(TypedBufferArray<Allocator>& storage, const T& value) ->
    typename TypedBufferArray<Allocator>::size_type {
  using DecayedT = std::remove_cvref_t<T>;
  using size_type = typename TypedBufferArray<Allocator>::size_type;

  size_type erased = 0;
  size_type i = 0;
  while (i < storage.Size()) {
    if (storage.template At<DecayedT>(i) == value) {
      storage.template Swap<DecayedT>(i, storage.Size() - 1);
      storage.PopBack();
      ++erased;
    } else {
      ++i;
    }
  }

  return erased;
}

/**
 * @brief Erases the element at position pos from the TypedBufferArray
 * (swap-then-pop, O(1), unstable).
 * @tparam Allocator The allocator type
 * @param storage The buffer to erase from
 * @param pos Index of the element to erase
 */
template <typename Allocator>
  requires std::is_class_v<Allocator>
inline void erase(TypedBufferArray<Allocator>& storage,
                  typename TypedBufferArray<Allocator>::size_type pos) {
  storage.Swap(pos, storage.Size() - 1);
  storage.PopBack();
}

/**
 * @brief Erases all elements satisfying the predicate from the
 * TypedBufferArray.
 * @details Uses the swap-then-pop pattern. Element order is not preserved.
 * @tparam T The element type
 * @tparam Allocator The allocator type
 * @tparam Pred Predicate type
 * @param storage The buffer to erase from
 * @param pred Predicate to test elements
 * @return Number of elements erased
 */
template <TypedBufferStorable T, typename Allocator, typename Pred>
  requires(std::is_class_v<Allocator> && std::predicate<Pred, const T&>)
inline auto erase_if(TypedBufferArray<Allocator>& storage, Pred pred) ->
    typename TypedBufferArray<Allocator>::size_type {
  using size_type = typename TypedBufferArray<Allocator>::size_type;

  size_type erased = 0;
  size_type idx = 0;
  while (idx < storage.Size()) {
    if (pred(storage.template At<T>(idx))) {
      storage.template Swap<T>(idx, storage.Size() - 1);
      storage.PopBack();
      ++erased;
    } else {
      ++idx;
    }
  }

  return erased;
}

}  // namespace helios::container
