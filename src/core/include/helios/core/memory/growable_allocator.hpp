#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/assert.hpp>
#include <helios/core/logger.hpp>
#include <helios/core/memory/allocator_traits.hpp>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <type_traits>
#include <vector>

namespace helios::memory {

/**
 * @brief Growable allocator adapter that automatically expands capacity.
 * @details Wraps another allocator and automatically creates additional allocator instances when capacity is exceeded.
 * Manages multiple allocator instances and delegates allocations to them.
 *
 * When an allocation fails due to insufficient capacity,
 * a new allocator instance is created with a configurable growth factor applied to the initial capacity.
 *
 * Supports deallocation by tracking which allocator owns each pointer.
 *
 * The GrowableAllocator itself is thread-safe, using shared_mutex for optimal concurrent access.
 * Underlying allocators are already thread-safe.
 *
 * @note Thread-safe with optimized locking.
 * Growth occurs only when an allocation fails due to capacity constraints.
 * Each growth creates a new allocator instance with expanded capacity.
 * Read operations (Stats, queries) use shared locks for concurrent access.
 * Write operations (Allocate with growth, Deallocate, Reset) use exclusive locks.
 *
 * The wrapped allocator must be constructible with a single capacity argument.
 * Compatible allocators: FrameAllocator, StackAllocator, FreeListAllocator.
 * Not compatible with: PoolAllocator (requires additional construction parameters).
 *
 * Copy operations are conditionally available based on the underlying allocator.
 * If a custom copyable allocator is provided,
 * GrowableAllocator will automatically support copy construction and copy assignment.
 *
 * @tparam Allocator The underlying allocator type to wrap
 */
template <typename Allocator>
class GrowableAllocator final {
public:
  static constexpr double kDefaultGrowthFactor = 2.0;

  /**
   * @brief Constructs a growable allocator with initial capacity.
   * @warning Triggers assertion in next cases:
   * - Initial capacity is 0.
   * - Growth factor is less than or equal to 1.0.
   * @param initial_capacity Initial capacity for the first allocator instance
   * @param growth_factor Factor to multiply capacity by when growing (default: kDefaultGrowthFactor)
   * @param max_allocators Maximum number of allocator instances to create (0 = unlimited)
   */
  explicit GrowableAllocator(size_t initial_capacity, double growth_factor = kDefaultGrowthFactor,
                             size_t max_allocators = 0);

  /**
   * @brief Copy constructor.
   * @note Only available if Allocator is copy constructible.
   * All built-in Helios allocators are non-copyable, so this will not be available
   * for standard use cases. Provided for custom copyable allocator types.
   */
  GrowableAllocator(const GrowableAllocator& other)
    requires std::is_copy_constructible_v<Allocator>;

  /**
   * @brief Move constructor.
   * @note Only available if Allocator is move constructible.
   */
  GrowableAllocator(GrowableAllocator&& other) noexcept
    requires std::is_move_constructible_v<Allocator>;

  ~GrowableAllocator() noexcept = default;

  /**
   * @brief Copy assignment operator.
   * @note Only available if Allocator is copy assignable.
   * All built-in Helios allocators are non-copyable, so this will not be available
   * for standard use cases. Provided for custom copyable allocator types.
   */
  GrowableAllocator& operator=(const GrowableAllocator& other)
    requires std::is_copy_assignable_v<Allocator>;

  /**
   * @brief Move assignment operator.
   * @note Only available if Allocator is move assignable.
   */
  GrowableAllocator& operator=(GrowableAllocator&& other) noexcept
    requires std::is_move_assignable_v<Allocator>;

  /**
   * @brief Allocates memory with specified size and alignment.
   * @details Attempts allocation from existing allocators. If all fail,
   * creates a new allocator instance with expanded capacity.
   * @warning Triggers assertion in next cases:
   * - Alignment is not a power of 2.
   * - Alignment is less than kMinAlignment.
   * @param size Number of bytes to allocate
   * @param alignment Alignment requirement (must be power of 2)
   * @return AllocationResult with pointer and actual allocated size, or {nullptr, 0} on failure
   */
  [[nodiscard]] AllocationResult Allocate(size_t size, size_t alignment = kDefaultAlignment) noexcept;

  /**
   * @brief Allocates memory for a single object of type T.
   * @details Convenience function that calculates size and alignment from the type.
   * The returned memory is uninitialized - use placement new to construct the object.
   * @tparam T Type to allocate memory for
   * @return Pointer to allocated memory, or nullptr on failure
   *
   * @example
   * @code
   * GrowableAllocator<FrameAllocator> alloc(1024);
   * int* ptr = alloc.Allocate<int>();
   * if (ptr != nullptr) {
   *   new (ptr) int(42);
   * }
   * @endcode
   */
  template <typename T>
  [[nodiscard]] T* Allocate() noexcept;

  /**
   * @brief Allocates memory for an array of objects of type T.
   * @details Convenience function that calculates size and alignment from the type.
   * The returned memory is uninitialized - use placement new to construct objects.
   * @tparam T Type to allocate memory for
   * @param count Number of objects to allocate space for
   * @return Pointer to allocated memory, or nullptr on failure
   *
   * @example
   * @code
   * GrowableAllocator<FrameAllocator> alloc(1024);
   * int* arr = alloc.Allocate<int>(10);
   * @endcode
   */
  template <typename T>
  [[nodiscard]] T* Allocate(size_t count) noexcept;

  /**
   * @brief Allocates and constructs a single object of type T.
   * @details Convenience function that allocates memory and constructs the object in-place.
   * @tparam T Type to allocate and construct
   * @tparam Args Constructor argument types
   * @param args Arguments to forward to T's constructor
   * @return Pointer to constructed object, or nullptr on allocation failure
   *
   * @example
   * @code
   * GrowableAllocator<FrameAllocator> alloc(1024);
   * auto* vec = alloc.AllocateAndConstruct<MyVec3>(1.0f, 2.0f, 3.0f);
   * @endcode
   */
  template <typename T, typename... Args>
    requires std::constructible_from<T, Args...>
  [[nodiscard]] T* AllocateAndConstruct(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>);

  /**
   * @brief Allocates and default-constructs an array of objects of type T.
   * @details Convenience function that allocates memory and default-constructs objects in-place.
   * @tparam T Type to allocate and construct (must be default constructible)
   * @param count Number of objects to allocate and construct
   * @return Pointer to first constructed object, or nullptr on allocation failure
   *
   * @example
   * @code
   * GrowableAllocator<FrameAllocator> alloc(1024);
   * auto* arr = alloc.AllocateAndConstructArray<MyType>(10);
   * @endcode
   */
  template <typename T>
    requires std::default_initializable<T>
  [[nodiscard]] T* AllocateAndConstructArray(size_t count) noexcept(std::is_nothrow_default_constructible_v<T>);

  /**
   * @brief Deallocates memory.
   * @details Finds the allocator that owns the pointer and delegates deallocation.
   * @warning Triggers assertion if pointer does not belong to any allocator instance.
   * @param ptr Pointer to deallocate
   * @param size Size of allocation (for allocators that require it)
   */
  void Deallocate(void* ptr, size_t size = 0) noexcept;

  /**
   * @brief Resets all allocator instances.
   * @details Resets all allocators and removes all but the first one.
   */
  void Reset() noexcept;

  /**
   * @brief Checks if the allocator can grow.
   * @return True if more allocator instances can be created
   */
  [[nodiscard]] bool CanGrow() const noexcept;

  /**
   * @brief Gets the number of allocator instances.
   * @return Number of allocator instances
   */
  [[nodiscard]] size_t AllocatorCount() const noexcept;

  /**
   * @brief Gets combined statistics for all allocator instances.
   * @return AllocatorStats with combined usage information
   */
  [[nodiscard]] AllocatorStats Stats() const noexcept;

  /**
   * @brief Gets the total capacity across all allocator instances.
   * @return Total capacity in bytes
   */
  [[nodiscard]] size_t TotalCapacity() const noexcept;

  /**
   * @brief Gets the initial capacity.
   * @return Initial capacity in bytes
   */
  [[nodiscard]] size_t InitialCapacity() const noexcept { return initial_capacity_; }

  /**
   * @brief Gets the growth factor.
   * @return Growth factor
   */
  [[nodiscard]] double GrowthFactor() const noexcept { return growth_factor_; }

  /**
   * @brief Gets the maximum number of allocators.
   * @return Maximum allocator count (0 = unlimited)
   */
  [[nodiscard]] size_t MaxAllocators() const noexcept { return max_allocators_; }

private:
  /**
   * @brief Creates a new allocator instance with specified capacity.
   * @param capacity Capacity for the new allocator
   */
  void CreateAllocator(size_t capacity);

  /**
   * @brief Finds which allocator owns a pointer.
   * @param ptr Pointer to search for
   * @return Pointer to the owning allocator or nullptr
   */
  [[nodiscard]] Allocator* FindOwningAllocator(const void* ptr) noexcept;

  std::vector<Allocator> allocators_;            ///< Vector of allocator instances
  size_t initial_capacity_ = 0;                  ///< Initial capacity
  double growth_factor_ = kDefaultGrowthFactor;  ///< Growth factor for new allocators
  size_t max_allocators_ = 0;                    ///< Maximum number of allocators (0 = unlimited)
  size_t next_capacity_ = 0;                     ///< Next capacity to use for growth
  mutable std::shared_mutex mutex_;              ///< Shared mutex for optimized thread-safety
};

template <typename Allocator>
inline GrowableAllocator<Allocator>::GrowableAllocator(size_t initial_capacity, double growth_factor,
                                                       size_t max_allocators)
    : initial_capacity_(initial_capacity),
      growth_factor_(growth_factor),
      max_allocators_(max_allocators),
      next_capacity_(initial_capacity) {
  HELIOS_ASSERT(initial_capacity > 0,
                "Failed to construct GrowableAllocator: initial_capacity must be greater than 0!");
  HELIOS_ASSERT(growth_factor > 1.0,
                "Failed to construct GrowableAllocator: growth_factor must be greater than 1.0, got '{}'!",
                growth_factor);

  // Create the first allocator
  allocators_.reserve(max_allocators > 0 ? max_allocators : 4);
  CreateAllocator(initial_capacity_);
}

template <typename Allocator>
inline GrowableAllocator<Allocator>::GrowableAllocator(const GrowableAllocator& other)
  requires std::is_copy_constructible_v<Allocator>
    : initial_capacity_(other.initial_capacity_),
      growth_factor_(other.growth_factor_),
      max_allocators_(other.max_allocators_),
      next_capacity_(other.next_capacity_) {
  const std::shared_lock lock(other.mutex_);
  allocators_ = other.allocators_;
}

template <typename Allocator>
inline GrowableAllocator<Allocator>::GrowableAllocator(GrowableAllocator&& other) noexcept
  requires std::is_move_constructible_v<Allocator>
    : allocators_(std::move(other.allocators_)),
      initial_capacity_(other.initial_capacity_),
      growth_factor_(other.growth_factor_),
      max_allocators_(other.max_allocators_),
      next_capacity_(other.next_capacity_) {
  other.initial_capacity_ = 0;
  other.growth_factor_ = kDefaultGrowthFactor;
  other.max_allocators_ = 0;
  other.next_capacity_ = 0;
}

template <typename Allocator>
inline auto GrowableAllocator<Allocator>::operator=(const GrowableAllocator& other) -> GrowableAllocator<Allocator>&
  requires std::is_copy_assignable_v<Allocator>
{
  if (this == &other) [[unlikely]] {
    return *this;
  }

  const std::scoped_lock lock(mutex_);
  const std::shared_lock other_lock(other.mutex_);

  allocators_ = other.allocators_;
  initial_capacity_ = other.initial_capacity_;
  growth_factor_ = other.growth_factor_;
  max_allocators_ = other.max_allocators_;
  next_capacity_ = other.next_capacity_;

  return *this;
}

template <typename Allocator>
inline auto GrowableAllocator<Allocator>::operator=(GrowableAllocator&& other) noexcept -> GrowableAllocator<Allocator>&
  requires std::is_move_assignable_v<Allocator>
{
  if (this == &other) [[unlikely]] {
    return *this;
  }

  const std::scoped_lock lock(mutex_);
  const std::scoped_lock other_lock(other.mutex_);

  allocators_ = std::move(other.allocators_);
  initial_capacity_ = other.initial_capacity_;
  growth_factor_ = other.growth_factor_;
  max_allocators_ = other.max_allocators_;
  next_capacity_ = other.next_capacity_;

  other.initial_capacity_ = 0;
  other.growth_factor_ = kDefaultGrowthFactor;
  other.max_allocators_ = 0;
  other.next_capacity_ = 0;

  return *this;
}

template <typename Allocator>
inline AllocationResult GrowableAllocator<Allocator>::Allocate(size_t size, size_t alignment) noexcept {
  HELIOS_ASSERT(IsPowerOfTwo(alignment), "Failed to allocate memory: alignment must be power of 2, got '{}'!",
                alignment);
  HELIOS_ASSERT(alignment >= kMinAlignment, "Failed to allocate memory: alignment must be at least '{}', got '{}'!",
                kMinAlignment, alignment);

  if (size == 0) [[unlikely]] {
    return {.ptr = nullptr, .allocated_size = 0};
  }

  // First, try to allocate from existing allocators with shared lock
  {
    const std::shared_lock lock(mutex_);

    for (auto& allocator : allocators_) {
      AllocationResult result{};
      if constexpr (requires { allocator.Allocate(size, alignment); }) {
        result = allocator.Allocate(size, alignment);
      } else if constexpr (requires { allocator.Allocate(size); }) {
        result = allocator.Allocate(size);
      }
      if (result.ptr != nullptr) {
        return result;
      }
    }
  }

  // All existing allocators are full, need exclusive lock for growth
  const std::scoped_lock lock(mutex_);

  // Try again with exclusive lock (another thread might have grown while we waited)
  for (auto& allocator : allocators_) {
    AllocationResult result{};
    if constexpr (requires { allocator.Allocate(size, alignment); }) {
      result = allocator.Allocate(size, alignment);
    } else if constexpr (requires { allocator.Allocate(size); }) {
      result = allocator.Allocate(size);
    }
    if (result.ptr != nullptr) {
      return result;
    }
  }

  // Still need to grow, check if we can
  if (max_allocators_ > 0 && allocators_.size() >= max_allocators_) {
    return {.ptr = nullptr, .allocated_size = 0};
  }

  // Calculate next capacity
  size_t new_capacity = static_cast<size_t>(next_capacity_ * growth_factor_);

  // Ensure new capacity is at least large enough for the requested allocation
  if (new_capacity < size) {
    new_capacity = size + (size / 2);  // Add 50% extra space
  }

  // Create new allocator
  CreateAllocator(new_capacity);
  next_capacity_ = new_capacity;

  // Try to allocate from the new allocator
  auto& new_allocator = allocators_.back();
  if constexpr (requires { new_allocator.Allocate(size, alignment); }) {
    return new_allocator.Allocate(size, alignment);
  } else if constexpr (requires { new_allocator.Allocate(size); }) {
    return new_allocator.Allocate(size);
  }
  return {.ptr = nullptr, .allocated_size = 0};
}

template <typename Allocator>
inline void GrowableAllocator<Allocator>::Deallocate(void* ptr, size_t size) noexcept {
  if (ptr == nullptr) [[unlikely]] {
    return;
  }

  // Check if allocator supports deallocation at compile time
  if constexpr (
      requires { std::declval<Allocator>().Deallocate(ptr, size); } ||
      requires { std::declval<Allocator>().Deallocate(ptr); }) {
    const std::shared_lock lock(mutex_);

    auto* allocator = FindOwningAllocator(ptr);
    HELIOS_ASSERT(allocator != nullptr, "Failed to deallocate memory: pointer does not belong to any allocator!");

    // Delegate deallocation to the owning allocator
    if constexpr (requires { allocator->Deallocate(ptr, size); }) {
      allocator->Deallocate(ptr, size);
    } else if constexpr (requires { allocator->Deallocate(ptr); }) {
      allocator->Deallocate(ptr);
    }
  }
  // else: No-op for allocators that don't support individual deallocation (e.g., FrameAllocator)
}

template <typename Allocator>
inline void GrowableAllocator<Allocator>::Reset() noexcept {
  const std::scoped_lock lock(mutex_);

  // Reset all allocators
  for (auto& allocator : allocators_) {
    allocator.Reset();
  }

  // Keep only the first allocator
  if (!allocators_.empty()) {
    auto first = std::move(allocators_.front());
    allocators_.clear();
    allocators_.push_back(std::move(first));
  }

  next_capacity_ = initial_capacity_;
}

template <typename Allocator>
inline bool GrowableAllocator<Allocator>::CanGrow() const noexcept {
  const std::shared_lock lock(mutex_);
  return max_allocators_ == 0 || allocators_.size() < max_allocators_;
}

template <typename Allocator>
inline size_t GrowableAllocator<Allocator>::AllocatorCount() const noexcept {
  const std::shared_lock lock(mutex_);
  return allocators_.size();
}

template <typename Allocator>
inline AllocatorStats GrowableAllocator<Allocator>::Stats() const noexcept {
  const std::shared_lock lock(mutex_);

  AllocatorStats combined{};

  for (const auto& allocator : allocators_) {
    const auto stats = allocator.Stats();
    combined.total_allocated += stats.total_allocated;
    combined.total_freed += stats.total_freed;
    combined.peak_usage = std::max(combined.peak_usage, stats.peak_usage);
    combined.allocation_count += stats.allocation_count;
    combined.total_allocations += stats.total_allocations;
    combined.total_deallocations += stats.total_deallocations;
    combined.alignment_waste += stats.alignment_waste;
  }

  return combined;
}

template <typename Allocator>
inline size_t GrowableAllocator<Allocator>::TotalCapacity() const noexcept {
  const std::shared_lock lock(mutex_);

  size_t total = 0;
  for (const auto& allocator : allocators_) {
    total += allocator.Capacity();
  }
  return total;
}

template <typename Allocator>
inline void GrowableAllocator<Allocator>::CreateAllocator(size_t capacity) {
  if constexpr (requires { Allocator(capacity); }) {
    allocators_.emplace_back(capacity);
  } else {
    HELIOS_ASSERT(false, "Failed to create allocator: Allocator type does not support construction with capacity!");
  }
}

template <typename Allocator>
inline Allocator* GrowableAllocator<Allocator>::FindOwningAllocator(const void* ptr) noexcept {
  for (auto& allocator : allocators_) {
    if constexpr (requires { allocator.Owns(ptr); }) {
      if (allocator.Owns(ptr)) {
        return &allocator;
      }
    }
  }
  return nullptr;
}

template <typename Allocator>
template <typename T>
inline T* GrowableAllocator<Allocator>::Allocate() noexcept {
  constexpr size_t size = sizeof(T);
  constexpr size_t alignment = std::max(alignof(T), kMinAlignment);
  auto result = Allocate(size, alignment);
  return static_cast<T*>(result.ptr);
}

template <typename Allocator>
template <typename T>
inline T* GrowableAllocator<Allocator>::Allocate(size_t count) noexcept {
  if (count == 0) [[unlikely]] {
    return nullptr;
  }
  constexpr size_t alignment = std::max(alignof(T), kMinAlignment);
  const size_t size = sizeof(T) * count;
  auto result = Allocate(size, alignment);
  return static_cast<T*>(result.ptr);
}

template <typename Allocator>
template <typename T, typename... Args>
  requires std::constructible_from<T, Args...>
inline T* GrowableAllocator<Allocator>::AllocateAndConstruct(Args&&... args) noexcept(
    std::is_nothrow_constructible_v<T, Args...>) {
  T* ptr = Allocate<T>();
  if (ptr != nullptr) [[likely]] {
    std::construct_at(ptr, std::forward<Args>(args)...);
  }
  return ptr;
}

template <typename Allocator>
template <typename T>
  requires std::default_initializable<T>
inline T* GrowableAllocator<Allocator>::AllocateAndConstructArray(size_t count) noexcept(
    std::is_nothrow_default_constructible_v<T>) {
  T* ptr = Allocate<T>(count);
  if (ptr != nullptr) [[likely]] {
    for (size_t i = 0; i < count; ++i) {
      std::construct_at(ptr + i);
    }
  }
  return ptr;
}

}  // namespace helios::memory
