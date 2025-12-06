#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/assert.hpp>
#include <helios/core/logger.hpp>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <limits>
#include <memory>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

namespace helios::container {

/**
 * @brief A sparse set data structure for efficient mapping of sparse indices to dense storage of values.
 * @details SparseSet provides O(1) insertion, deletion, and lookup operations using two arrays:
 * - sparse: maps element indices to dense indices
 * - dense: stores packed values of type T in contiguous memory
 *
 * The data structure is particularly useful for managing sparse collections where indices
 * may have large gaps but you want cache-friendly iteration over existing values.
 *
 * Memory complexity: O(max_index + num_elements)
 * Time complexity: O(1) for all operations (amortized for insertions)
 *
 * @tparam T Type of values stored in the dense array
 * @tparam IndexType Type used for element indices (default: size_t)
 * @tparam Allocator Allocator type for memory management (default: std::allocator<T>)
 */
template <typename T, typename IndexType = size_t, typename Allocator = std::allocator<T>>
class SparseSet {
private:
  using DenseIndexType = IndexType;
  using SparseAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<DenseIndexType>;
  using DenseAllocator = Allocator;

public:
  using value_type = T;
  using index_type = IndexType;
  using dense_index_type = DenseIndexType;
  using size_type = size_t;
  using reference = T&;
  using const_reference = const T&;
  using pointer = T*;
  using const_pointer = const T*;
  using allocator_type = Allocator;

  using sparse_container_type = std::vector<DenseIndexType, SparseAllocator>;
  using dense_container_type = std::vector<T, DenseAllocator>;

  using iterator = typename dense_container_type::iterator;
  using const_iterator = typename dense_container_type::const_iterator;
  using reverse_iterator = typename dense_container_type::reverse_iterator;
  using const_reverse_iterator = typename dense_container_type::const_reverse_iterator;

  static constexpr IndexType kInvalidIndex = std::numeric_limits<IndexType>::max();
  static constexpr DenseIndexType kInvalidDenseIndex = std::numeric_limits<DenseIndexType>::max();

  /**
   * @brief Default constructor.
   * @details Creates an empty sparse set.
   * Exception safety: No-throw guarantee if T and Allocator are nothrow default constructible.
   */
  constexpr SparseSet() noexcept(std::is_nothrow_default_constructible_v<Allocator>) = default;

  /**
   * @brief Constructor with custom allocator.
   * @details Creates an empty sparse set with the specified allocator.
   * Exception safety: Basic guarantee.
   * @param alloc The allocator to use for memory management
   * @throws May throw if allocator copy constructor throws
   */
  explicit SparseSet(const Allocator& alloc) noexcept(noexcept(SparseAllocator(alloc)))
      : sparse_(SparseAllocator(alloc)), dense_(alloc) {}

  /**
   * @brief Copy constructor.
   * @details Creates a copy of another sparse set.
   * Exception safety: Strong guarantee.
   * @param other The sparse set to copy from
   * @throws May throw if T copy constructor or memory allocation fails
   */
  constexpr SparseSet(const SparseSet& other) = default;
  /**
   * @brief Move constructor.
   * @details Transfers ownership of resources from other sparse set.
   * Exception safety: No-throw guarantee.
   */
  constexpr SparseSet(SparseSet&&) noexcept = default;

  /**
   * @brief Destructor.
   * @details Destroys the sparse set and releases all memory.
   */
  constexpr ~SparseSet() = default;

  /**
   * @brief Copy assignment operator.
   * @details Assigns the contents of another sparse set to this one.
   * Exception safety: Strong guarantee.
   * @param other The sparse set to copy from
   * @return Reference to this sparse set
   * @throws May throw if T copy assignment or memory allocation fails
   */
  constexpr SparseSet& operator=(const SparseSet& other) = default;

  /**
   * @brief Move assignment operator.
   * @details Transfers ownership of resources from other sparse set.
   * Exception safety: No-throw guarantee.
   * @return Reference to this sparse set
   */
  constexpr SparseSet& operator=(SparseSet&&) noexcept = default;

  /**
   * @brief Clears the set, removing all elements.
   * @details Removes all values from the set.
   * The sparse array is reset to invalid indices but its capacity is preserved for performance.
   * Time complexity: O(sparse_capacity).
   * Exception safety: No-throw guarantee.
   */
  constexpr void Clear() noexcept;

  /**
   * @brief Inserts a value at the specified index (copy version).
   * @details Adds the specified value to the set at the given index if it's not already present.
   * If the index already exists, the existing value is replaced.
   * The sparse array will be resized if necessary to accommodate the index.
   * Time complexity: O(1) amortized (O(index) worst case if sparse array needs resizing).
   * Exception safety: Strong guarantee.
   * @warning Triggers assertion if index is invalid or negative
   * @param index The index to insert at
   * @param value The value to move insert
   * @return The dense index where the value was stored
   * @throws std::bad_alloc If memory allocation fails during sparse array resize or dense array growth
   */
  DenseIndexType Insert(IndexType index, const T& value);

  /**
   * @brief Inserts a value at the specified index (move version).
   * @details Adds the specified value to the set at the given index if it's not already present.
   * If the index already exists, the existing value is replaced.
   * The sparse array will be resized if necessary to accommodate the index.
   * Time complexity: O(1) amortized (O(index) worst case if sparse array needs resizing).
   * Exception safety: Strong guarantee.
   * @warning Triggers assertion if index is invalid or negative
   * @param index The index to insert at
   * @param value The value to copy insert
   * @return The dense index where the value was stored
   * @throws std::bad_alloc If memory allocation fails during sparse array resize or dense array growth
   */
  DenseIndexType Insert(IndexType index, T&& value);

  /**
   * @brief Constructs a value in-place at the specified index.
   * @details Constructs a value directly in the dense array at the given index.
   * If the index already exists, the existing value is replaced.
   * The sparse array will be resized if necessary to accommodate the index.
   * Time complexity: O(1) amortized (O(index) worst case if sparse array needs resizing).
   * Exception safety: Strong guarantee.
   * @warning Triggers assertion if index is invalid or negative.
   * @param index The index to insert at
   * @param args Arguments to forward to T's constructor
   * @return The dense index where the value was stored
   * @throws std::bad_alloc If memory allocation fails during sparse array resize or dense array growth
   */
  template <typename... Args>
  DenseIndexType Emplace(IndexType index, Args&&... args);

  /**
   * @brief Removes an index from the set.
   * @details Removes the specified index from the set using swap-and-pop technique to maintain dense packing.
   * The last element in the dense array is moved to fill the gap left by the removed element.
   * Time complexity: O(1).
   * Exception safety: No-throw guarantee.
   * @warning Triggers assertion if index is invalid, negative, or doesn't exist.
   * @param index The index to remove
   */
  void Remove(IndexType index) noexcept;

  /**
   * @brief Reserves space for at least n elements in the dense array.
   * @details Ensures that the dense array can hold at least n elements without triggering a reallocation.
   * Does not affect the sparse array capacity.
   * Time complexity: O(1) if no reallocation, O(Size()) if reallocation occurs.
   * Exception safety: Strong guarantee.
   * @warning Triggers assertion if n is negative.
   * @param n The minimum capacity to reserve
   * @throws std::bad_alloc If memory allocation fails
   */
  void Reserve(size_type n) { dense_.reserve(n); }

  /**
   * @brief Reserves space for indices up to max_index in the sparse array.
   * @details Ensures that the sparse array can accommodate indices up to max_index without triggering a reallocation.
   * Time complexity: O(1) if no reallocation, O(max_index) if reallocation occurs.
   * Exception safety: Strong guarantee.
   * @warning Triggers assertion if max_index is invalid or negative.
   * @param max_index The maximum index to accommodate
   * @throws std::bad_alloc If memory allocation fails
   */
  void ReserveSparse(IndexType max_index);

  /**
   * @brief Shrinks the capacity of both arrays to fit their current size.
   * @details Reduces memory usage by shrinking both the dense and sparse arrays to their minimum required size.
   * Time complexity: O(Size() + SparseSize()).
   * Exception safety: Strong guarantee.
   * @throws std::bad_alloc If memory allocation fails during shrinking
   */
  void ShrinkToFit();

  /**
   * @brief Gets the value at the specified index.
   * @details Returns a reference to the value stored at the given index.
   * Time complexity: O(1).
   * Exception safety: No-throw guarantee.
   * @warning Triggers assertion if index is invalid, negative, or doesn't exist.
   * @param index The index to access
   * @return Reference to the value at the specified index
   */
  [[nodiscard]] constexpr T& Get(IndexType index) noexcept;

  /**
   * @brief Gets the value at the specified index (const version).
   * @details Returns a const reference to the value stored at the given index.
   * Time complexity: O(1).
   * Exception safety: No-throw guarantee.
   * @warning Triggers assertion if index is invalid, negative, or doesn't exist.
   * @param index The index to access
   * @return Const reference to the value at the specified index
   */
  [[nodiscard]] constexpr const T& Get(IndexType index) const noexcept;

  /**
   * @brief Gets the value at the specified dense index.
   * @details Returns a reference to the value stored at the given dense position.
   * Time complexity: O(1).
   * Exception safety: No-throw guarantee.
   * @warning Triggers assertion if dense_index is invalid, negative, or out of bounds.
   * @param dense_index The dense position to access
   * @return Reference to the value at the specified dense position
   */
  [[nodiscard]] constexpr T& GetByDenseIndex(DenseIndexType dense_index) noexcept;

  /**
   * @brief Gets the value at the specified dense index (const version).
   * @details Returns a const reference to the value stored at the given dense position.
   * Time complexity: O(1).
   * Exception safety: No-throw guarantee.
   * @warning Triggers assertion if dense_index is invalid, negative, or out of bounds.
   * @param dense_index The dense position to access
   * @return Const reference to the value at the specified dense position
   */
  [[nodiscard]] constexpr const T& GetByDenseIndex(DenseIndexType dense_index) const noexcept;

  /**
   * @brief Tries to get the value at the specified index.
   * @details Returns a pointer to the value stored at the given index, or nullptr if the index doesn't exist.
   * Time complexity: O(1).
   * Exception safety: No-throw guarantee.
   * @param index The index to access
   * @return Pointer to the value at the specified index, or nullptr if index doesn't exist
   */
  [[nodiscard]] constexpr T* TryGet(IndexType index) noexcept;

  /**
   * @brief Tries to get the value at the specified index (const version).
   * @details Returns a pointer to the value stored at the given index, or nullptr if the index doesn't exist.
   * Time complexity: O(1).
   * Exception safety: No-throw guarantee.
   * @param index The index to access
   * @return Pointer to the value at the specified index, or nullptr if index doesn't exist
   */
  [[nodiscard]] constexpr const T* TryGet(IndexType index) const noexcept;

  /**
   * @brief Swaps the contents of this sparse set with another.
   * @details Efficiently swaps all data between two sparse sets.
   * Time complexity: O(1).
   * Exception safety: No-throw guarantee.
   * @param other The sparse set to swap with
   */
  void Swap(SparseSet& other) noexcept;

  /**
   * @brief Checks if two sparse sets are equal.
   * @details Two sparse sets are equal if they contain the same index-value pairs.
   * Time complexity: O(Size() + other.Size()).
   * Exception safety: No-throw guarantee.
   * @param other The sparse set to compare with
   * @return True if both sets contain the same index-value pairs
   */
  [[nodiscard]] bool operator==(const SparseSet& other) const noexcept
    requires std::equality_comparable<T>;

  /**
   * @brief Checks if the set is empty.
   * @details Returns true if no values are stored in the set.
   * Time complexity: O(1).
   * Exception safety: No-throw guarantee.
   * @return True if the set contains no elements, false otherwise
   */
  [[nodiscard]] constexpr bool Empty() const noexcept { return dense_.empty(); }

  /**
   * @brief Checks if an index value is valid for this sparse set.
   * @details Returns true if the index is not the reserved invalid value.
   * Time complexity: O(1).
   * Exception safety: No-throw guarantee.
   * @param index The index to validate
   * @return True if the index is valid, false if it's the reserved invalid value
   */
  [[nodiscard]] static constexpr bool IsValidIndex(IndexType index) noexcept { return index != kInvalidIndex; }

  /**
   * @brief Checks if an index exists in the set.
   * @details Performs a comprehensive check to ensure the index is valid and exists in the set.
   * Time complexity: O(1).
   * Exception safety: No-throw guarantee.
   * @warning Triggers assertion if index is invalid or negative.
   * @param index The index to check
   * @return True if the index exists in the set, false otherwise
   */
  [[nodiscard]] constexpr bool Contains(IndexType index) const noexcept;

  /**
   * @brief Returns the allocator associated with the container.
   * @details Gets the allocator used for the dense array.
   * Time complexity: O(1).
   * Exception safety: No-throw guarantee.
   * @return Copy of the allocator
   */
  [[nodiscard]] constexpr allocator_type GetAllocator() const noexcept { return dense_.get_allocator(); }

  /**
   * @brief Gets the dense index for a given index.
   * @details Returns the position of the value in the dense array for the specified index.
   * Time complexity: O(1).
   * Exception safety: No-throw guarantee.
   * @warning Triggers assertion if index is invalid, negative, or doesn't exist.
   * @param index The index to look up
   * @return The dense index
   */
  [[nodiscard]] constexpr DenseIndexType GetDenseIndex(IndexType index) const noexcept;

  /**
   * @brief Returns the number of values in the set.
   * @details Returns the count of values currently stored in the set.
   * Time complexity: O(1).
   * Exception safety: No-throw guarantee.
   * @return The number of values in the set
   */
  [[nodiscard]] constexpr size_type Size() const noexcept { return dense_.size(); }

  /**
   * @brief Returns the maximum possible size of the set.
   * @details Returns the theoretical maximum number of elements the set can hold.
   * Time complexity: O(1).
   * Exception safety: No-throw guarantee.
   * @return The maximum possible size
   */
  [[nodiscard]] constexpr size_type MaxSize() const noexcept { return dense_.max_size(); }

  /**
   * @brief Returns the capacity of the dense array.
   * @details Returns the number of elements that can be stored in the dense array without triggering a reallocation.
   * Time complexity: O(1).
   * Exception safety: No-throw guarantee.
   * @return The capacity of the dense array
   */
  [[nodiscard]] constexpr size_type Capacity() const noexcept { return dense_.capacity(); }

  /**
   * @brief Returns the capacity of the sparse array.
   * @details Returns the maximum index that can be stored without triggering a sparse array reallocation.
   * Time complexity: O(1).
   * Exception safety: No-throw guarantee.
   * @return The capacity of the sparse array
   */
  [[nodiscard]] constexpr size_type SparseCapacity() const noexcept { return sparse_.capacity(); }

  /**
   * @brief Returns a writable span of the packed values.
   * @details Provides direct access to the densely packed values in the order
   * they were inserted (with removal gaps filled by later insertions).
   * Time complexity: O(1).
   * Exception safety: No-throw guarantee.
   * @return A span containing all values in the set
   */
  [[nodiscard]] constexpr std::span<T> Data() noexcept { return dense_; }

  /**
   * @brief Returns a read-only span of the packed values.
   * @details Provides direct access to the densely packed values in the order
   * they were inserted (with removal gaps filled by later insertions).
   * Time complexity: O(1).
   * Exception safety: No-throw guarantee.
   * @return A span containing all values in the set
   */
  [[nodiscard]] constexpr std::span<const T> Data() const noexcept { return dense_; }

  [[nodiscard]] constexpr iterator begin() noexcept { return dense_.begin(); }
  [[nodiscard]] constexpr const_iterator begin() const noexcept { return dense_.begin(); }
  [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return dense_.cbegin(); }

  [[nodiscard]] constexpr iterator end() noexcept { return dense_.end(); }
  [[nodiscard]] constexpr const_iterator end() const noexcept { return dense_.end(); }
  [[nodiscard]] constexpr const_iterator cend() const noexcept { return dense_.cend(); }

  [[nodiscard]] constexpr reverse_iterator rbegin() noexcept { return dense_.rbegin(); }
  [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return dense_.rbegin(); }
  [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return dense_.crbegin(); }

  [[nodiscard]] constexpr reverse_iterator rend() noexcept { return dense_.rend(); }
  [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return dense_.rend(); }
  [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return dense_.crend(); }

private:
  sparse_container_type sparse_;        ///< Maps index -> dense index
  dense_container_type dense_;          ///< Packed values in insertion order
  std::vector<IndexType> reverse_map_;  ///< Maps dense index -> original index for efficient removal
};

template <typename T, typename IndexType, typename Allocator>
constexpr void SparseSet<T, IndexType, Allocator>::Clear() noexcept {
  dense_.clear();
  reverse_map_.clear();
  std::ranges::fill(sparse_, kInvalidDenseIndex);
}

template <typename T, typename IndexType, typename Allocator>
inline typename SparseSet<T, IndexType, Allocator>::DenseIndexType SparseSet<T, IndexType, Allocator>::Insert(
    IndexType index, const T& value) {
  HELIOS_ASSERT(IsValidIndex(index), "Failed to insert value: index is invalid!");
  if constexpr (std::is_signed_v<IndexType>) {
    HELIOS_ASSERT(index >= 0, "Failed to insert value: index cannot be negative!");
  }

  // Check if index already exists and replace the value
  if (Contains(index)) {
    const DenseIndexType dense_index = sparse_[index];
    dense_[dense_index] = value;
    return dense_index;
  }

  // Ensure sparse array can accommodate this index
  if (index >= sparse_.size()) {
    sparse_.resize(index + 1, kInvalidDenseIndex);
  }

  const auto dense_index = static_cast<DenseIndexType>(dense_.size());
  dense_.push_back(value);
  reverse_map_.push_back(index);
  sparse_[index] = dense_index;

  return dense_index;
}

template <typename T, typename IndexType, typename Allocator>
inline typename SparseSet<T, IndexType, Allocator>::DenseIndexType SparseSet<T, IndexType, Allocator>::Insert(
    IndexType index, T&& value) {
  HELIOS_ASSERT(IsValidIndex(index), "Failed to insert value: index is invalid!");
  if constexpr (std::is_signed_v<IndexType>) {
    HELIOS_ASSERT(index >= 0, "Failed to insert value: index cannot be negative!");
  }

  // Check if index already exists and replace the value
  if (Contains(index)) {
    const DenseIndexType dense_index = sparse_[index];
    dense_[dense_index] = std::move(value);
    return dense_index;
  }

  // Ensure sparse array is large enough
  if (static_cast<size_type>(index) >= sparse_.size()) {
    sparse_.resize(static_cast<size_type>(index) + 1, kInvalidDenseIndex);
  }

  const auto dense_index = static_cast<DenseIndexType>(dense_.size());
  sparse_[index] = dense_index;
  dense_.push_back(std::move(value));
  reverse_map_.push_back(index);

  return dense_index;
}

template <typename T, typename IndexType, typename Allocator>
template <typename... Args>
inline typename SparseSet<T, IndexType, Allocator>::DenseIndexType SparseSet<T, IndexType, Allocator>::Emplace(
    IndexType index, Args&&... args) {
  HELIOS_ASSERT(IsValidIndex(index), "Failed to emplace value: index is invalid!");
  if constexpr (std::is_signed_v<IndexType>) {
    HELIOS_ASSERT(index >= 0, "Failed to emplace value: index cannot be negative!");
  }

  // Check if index already exists and replace the value
  if (Contains(index)) {
    const DenseIndexType dense_index = sparse_[index];
    dense_[dense_index] = T(std::forward<Args>(args)...);
    return dense_index;
  }

  // Ensure sparse array is large enough
  if (static_cast<size_type>(index) >= sparse_.size()) {
    sparse_.resize(static_cast<size_type>(index) + 1, kInvalidDenseIndex);
  }

  const auto dense_index = static_cast<DenseIndexType>(dense_.size());
  sparse_[index] = dense_index;
  dense_.emplace_back(std::forward<Args>(args)...);
  reverse_map_.push_back(index);

  return dense_index;
}

template <typename T, typename IndexType, typename Allocator>
inline void SparseSet<T, IndexType, Allocator>::Remove(IndexType index) noexcept {
  HELIOS_ASSERT(IsValidIndex(index), "Failed to remove value: index is invalid!");
  if constexpr (std::is_signed_v<IndexType>) {
    HELIOS_ASSERT(index >= 0, "Failed to remove value: index cannot be negative!");
  }
  HELIOS_ASSERT(Contains(index), "Failed to remove value: index does not exist!");

  const DenseIndexType dense_index = sparse_[index];
  const auto last_dense_index = static_cast<DenseIndexType>(dense_.size() - 1);

  if (dense_index != last_dense_index) {
    // Swap with last element to maintain dense packing
    const IndexType last_index = reverse_map_[last_dense_index];
    dense_[dense_index] = std::move(dense_[last_dense_index]);
    reverse_map_[dense_index] = last_index;
    sparse_[last_index] = dense_index;
  }

  dense_.pop_back();
  reverse_map_.pop_back();
  sparse_[index] = kInvalidDenseIndex;
}

template <typename T, typename IndexType, typename Allocator>
inline void SparseSet<T, IndexType, Allocator>::ReserveSparse(IndexType max_index) {
  HELIOS_ASSERT(IsValidIndex(max_index), "Failed to reserve sparse: max_index is invalid!");
  if constexpr (std::is_signed_v<IndexType>) {
    HELIOS_ASSERT(max_index >= 0, "Failed to reserve sparse: max_index cannot be negative!");
  }
  if (static_cast<size_type>(max_index) + 1 > sparse_.size()) {
    sparse_.resize(static_cast<size_type>(max_index) + 1, kInvalidDenseIndex);
  }
}

template <typename T, typename IndexType, typename Allocator>
inline void SparseSet<T, IndexType, Allocator>::ShrinkToFit() {
  dense_.shrink_to_fit();
  reverse_map_.shrink_to_fit();

  // Find the maximum index in use to determine minimum sparse size needed
  if (reverse_map_.empty()) {
    sparse_.clear();
  } else {
    const IndexType max_index = *std::ranges::max_element(reverse_map_);
    sparse_.resize(static_cast<size_type>(max_index) + 1);
  }
  sparse_.shrink_to_fit();
}

template <typename T, typename IndexType, typename Allocator>
constexpr T& SparseSet<T, IndexType, Allocator>::Get(IndexType index) noexcept {
  HELIOS_ASSERT(IsValidIndex(index), "Failed to get value: index is invalid!");
  if constexpr (std::is_signed_v<IndexType>) {
    HELIOS_ASSERT(index >= 0, "Failed to get value: index cannot be negative!");
  }
  HELIOS_ASSERT(Contains(index), "Failed to get value: index does not exist!");
  return dense_[sparse_[index]];
}

template <typename T, typename IndexType, typename Allocator>
constexpr const T& SparseSet<T, IndexType, Allocator>::Get(IndexType index) const noexcept {
  HELIOS_ASSERT(IsValidIndex(index), "Failed to get value: index is invalid!");
  if constexpr (std::is_signed_v<IndexType>) {
    HELIOS_ASSERT(index >= 0, "Failed to get value: index cannot be negative!");
  }
  HELIOS_ASSERT(Contains(index), "Failed to get value: index does not exist!");
  return dense_[sparse_[index]];
}

template <typename T, typename IndexType, typename Allocator>
constexpr T& SparseSet<T, IndexType, Allocator>::GetByDenseIndex(DenseIndexType dense_index) noexcept {
  HELIOS_ASSERT(dense_index != kInvalidDenseIndex, "Failed to get value: dense_index is invalid!");
  if constexpr (std::is_signed_v<DenseIndexType>) {
    HELIOS_ASSERT(dense_index >= 0, "Failed to get value: dense_index cannot be negative!");
  }
  HELIOS_ASSERT(dense_index < dense_.size(), "Failed to get value: dense_index is out of bounds!");
  return dense_[dense_index];
}

template <typename T, typename IndexType, typename Allocator>
constexpr const T& SparseSet<T, IndexType, Allocator>::GetByDenseIndex(DenseIndexType dense_index) const noexcept {
  HELIOS_ASSERT(dense_index != kInvalidDenseIndex, "Failed to get value: dense_index is invalid!");
  if constexpr (std::is_signed_v<DenseIndexType>) {
    HELIOS_ASSERT(dense_index >= 0, "Failed to get value: dense_index cannot be negative!");
  }
  HELIOS_ASSERT(dense_index < dense_.size(), "Failed to get value: dense_index is out of bounds!");
  return dense_[dense_index];
}

template <typename T, typename IndexType, typename Allocator>
constexpr T* SparseSet<T, IndexType, Allocator>::TryGet(IndexType index) noexcept {
  HELIOS_ASSERT(index != kInvalidIndex, "Failed to try get value: index is invalid!");
  if constexpr (std::is_signed_v<IndexType>) {
    HELIOS_ASSERT(index >= 0, "Failed to try get value: index cannot be negative!");
  }

  if (!Contains(index)) {
    return nullptr;
  }
  return &dense_[sparse_[index]];
}

template <typename T, typename IndexType, typename Allocator>
constexpr const T* SparseSet<T, IndexType, Allocator>::TryGet(IndexType index) const noexcept {
  HELIOS_ASSERT(index != kInvalidIndex, "Failed to get value: index is invalid!");
  if constexpr (std::is_signed_v<IndexType>) {
    HELIOS_ASSERT(index >= 0, "Failed to try get value: index cannot be negative!");
  }

  if (!Contains(index)) {
    return nullptr;
  }
  return &dense_[sparse_[index]];
}

template <typename T, typename IndexType, typename Allocator>
inline void SparseSet<T, IndexType, Allocator>::Swap(SparseSet& other) noexcept {
  std::swap(sparse_, other.sparse_);
  std::swap(dense_, other.dense_);
  std::swap(reverse_map_, other.reverse_map_);
}

template <typename T, typename IndexType, typename Allocator>
inline bool SparseSet<T, IndexType, Allocator>::operator==(const SparseSet& other) const noexcept
  requires std::equality_comparable<T>
{
  if (Size() != other.Size()) {
    return false;
  }

  for (size_type i = 0; i < dense_.size(); ++i) {
    const IndexType index = reverse_map_[i];
    if (!other.Contains(index) || dense_[i] != other.Get(index)) {
      return false;
    }
  }

  return true;
}

template <typename T, typename IndexType, typename Allocator>
constexpr bool SparseSet<T, IndexType, Allocator>::Contains(IndexType index) const noexcept {
  HELIOS_ASSERT(IsValidIndex(index), "Failed to check if set contains index: index is invalid!");
  if constexpr (std::is_signed_v<IndexType>) {
    HELIOS_ASSERT(index >= 0, "Failed to check if set contains index: index cannot be negative!");
  }
  return static_cast<size_type>(index) < sparse_.size() && sparse_[index] != kInvalidDenseIndex &&
         static_cast<size_type>(sparse_[index]) < dense_.size() &&
         static_cast<size_type>(sparse_[index]) < reverse_map_.size() && reverse_map_[sparse_[index]] == index;
}

template <typename T, typename IndexType, typename Allocator>
constexpr typename SparseSet<T, IndexType, Allocator>::DenseIndexType SparseSet<T, IndexType, Allocator>::GetDenseIndex(
    IndexType index) const noexcept {
  HELIOS_ASSERT(IsValidIndex(index), "Failed to get dense index: index is invalid!");
  if constexpr (std::is_signed_v<IndexType>) {
    HELIOS_ASSERT(index >= 0, "Failed to get dense index: index cannot be negative!");
  }
  HELIOS_ASSERT(Contains(index), "Failed to get dense index: index does not exist!");
  return sparse_[index];
}

/**
 * @brief Swaps two sparse sets.
 * @details Non-member swap function for efficient swapping of sparse sets.
 * Time complexity: O(1).
 * Exception safety: No-throw guarantee.
 * @tparam T Type of values stored
 * @tparam IndexType Type used for indices
 * @tparam Allocator Allocator type
 * @param lhs First sparse set
 * @param rhs Second sparse set
 */
template <typename T, typename IndexType, typename Allocator>
void swap(SparseSet<T, IndexType, Allocator>& lhs, SparseSet<T, IndexType, Allocator>& rhs) noexcept {
  lhs.Swap(rhs);
}

}  // namespace helios::container
