#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/memory/allocator_traits.hpp>
#include <helios/core/memory/frame_allocator.hpp>
#include <helios/core/memory/free_list_allocator.hpp>
#include <helios/core/memory/pool_allocator.hpp>
#include <helios/core/memory/stack_allocator.hpp>

#include <cstddef>
#include <string_view>

namespace helios::app {

/**
 * @brief Resource wrapper for a per-World frame allocator.
 * @details Owns a frame allocator that can be reset per-frame.
 * Suitable for temporary per-frame allocations within a specific World.
 *
 * Uses lock-free operations.
 *
 * @note Thread-safe.
 *
 * @example
 * @code
 * world.AddResource<FrameAllocatorResource>(1024 * 1024); // 1MB
 *
 * void Update(SystemContext& ctx) {
 *   auto& resource = ctx.WriteResource<FrameAllocatorResource>();
 *   auto& allocator = resource.Get();
 *   // Use allocator for temporary data
 * }
 *
 * // Reset at frame end
 * world.WriteResource<FrameAllocatorResource>().Reset();
 * @endcode
 */
class FrameAllocatorResource {
public:
  /**
   * @brief Constructs resource with a frame allocator.
   * @param capacity Size of the frame allocator buffer in bytes (default: 16MB)
   */
  explicit FrameAllocatorResource(size_t capacity = 16 * 1024 * 1024) : allocator_(capacity) {}
  FrameAllocatorResource(const FrameAllocatorResource&) = delete;
  FrameAllocatorResource(FrameAllocatorResource&&) noexcept = default;
  ~FrameAllocatorResource() noexcept = default;

  FrameAllocatorResource& operator=(const FrameAllocatorResource&) = delete;
  FrameAllocatorResource& operator=(FrameAllocatorResource&&) noexcept = default;

  /**
   * @brief Resets the frame allocator, freeing all allocations.
   * @details Should be called at frame boundaries.
   */
  void Reset() noexcept { allocator_.Reset(); }

  /**
   * @brief Checks if the frame allocator is empty.
   * @return True if no allocations exist
   */
  [[nodiscard]] bool Empty() const noexcept { return allocator_.Empty(); }

  /**
   * @brief Gets reference to the frame allocator.
   * @return Reference to FrameAllocator
   */
  [[nodiscard]] memory::FrameAllocator& Get() noexcept { return allocator_; }

  /**
   * @brief Gets const reference to the frame allocator.
   * @return Const reference to FrameAllocator
   */
  [[nodiscard]] const memory::FrameAllocator& Get() const noexcept { return allocator_; }

  /**
   * @brief Gets frame allocator statistics.
   * @return AllocatorStats with current usage
   */
  [[nodiscard]] memory::AllocatorStats Stats() const noexcept { return allocator_.Stats(); }

  /**
   * @brief Gets the capacity of the frame allocator.
   * @return Capacity in bytes
   */
  [[nodiscard]] size_t Capacity() const noexcept { return allocator_.Capacity(); }

  /**
   * @brief Gets the resource name for registration.
   * @return Resource name
   */
  [[nodiscard]] static constexpr std::string_view Name() noexcept { return "FrameAllocatorResource"; }

private:
  memory::FrameAllocator allocator_;
};

/**
 * @brief Resource wrapper for a per-World free list allocator.
 * @details Owns a general-purpose allocator for arbitrary allocation patterns.
 *
 * Uses mutex for thread safety.
 *
 * @note Thread-safe.
 *
 * @example
 * @code
 * world.AddResource<FreeListAllocatorResource>(64 * 1024 * 1024); // 64MB
 *
 * void Update(SystemContext& ctx) {
 *   auto& resource = ctx.WriteResource<FreeListAllocatorResource>();
 *   auto& allocator = resource.Get();
 *   // Use allocator
 * }
 * @endcode
 */
class FreeListAllocatorResource {
public:
  /**
   * @brief Constructs resource with a free list allocator.
   * @param capacity Size of the allocator buffer in bytes (default: 64MB)
   */
  explicit FreeListAllocatorResource(size_t capacity = 64 * 1024 * 1024) : allocator_(capacity) {}
  FreeListAllocatorResource(const FreeListAllocatorResource&) = delete;
  FreeListAllocatorResource(FreeListAllocatorResource&&) noexcept = default;
  ~FreeListAllocatorResource() noexcept = default;

  FreeListAllocatorResource& operator=(const FreeListAllocatorResource&) = delete;
  FreeListAllocatorResource& operator=(FreeListAllocatorResource&&) noexcept = default;

  /**
   * @brief Gets reference to the free list allocator.
   * @return Reference to FreeListAllocator
   */
  [[nodiscard]] memory::FreeListAllocator& Get() noexcept { return allocator_; }

  /**
   * @brief Gets const reference to the free list allocator.
   * @return Const reference to FreeListAllocator
   */
  [[nodiscard]] const memory::FreeListAllocator& Get() const noexcept { return allocator_; }

  /**
   * @brief Resets the allocator, freeing all allocations.
   */
  void Reset() noexcept { allocator_.Reset(); }

  /**
   * @brief Checks if the allocator is empty.
   * @return True if no allocations exist
   */
  [[nodiscard]] bool Empty() const noexcept { return allocator_.Empty(); }

  /**
   * @brief Gets allocator statistics.
   * @return AllocatorStats with current usage
   */
  [[nodiscard]] memory::AllocatorStats Stats() const noexcept { return allocator_.Stats(); }

  /**
   * @brief Gets the capacity of the allocator.
   * @return Capacity in bytes
   */
  [[nodiscard]] size_t Capacity() const noexcept { return allocator_.Capacity(); }

  /**
   * @brief Gets the resource name for registration.
   * @return Resource name
   */
  [[nodiscard]] static constexpr std::string_view Name() noexcept { return "FreeListAllocatorResource"; }

private:
  memory::FreeListAllocator allocator_;
};

/**
 * @brief Resource wrapper for a per-World pool allocator.
 * @details Owns a pool allocator for fixed-size allocations.
 *
 * Uses lock-free operations.
 *
 * @note Thread-safe.
 *
 * @example
 * @code
 * // Create pool for Entity-sized allocations
 * auto pool = PoolAllocatorResource::ForType<Entity>(1000);
 * world.AddResource<PoolAllocatorResource>(std::move(pool));
 *
 * void Update(SystemContext& ctx) {
 *   auto& resource = ctx.WriteResource<PoolAllocatorResource>();
 *   auto& allocator = resource.Get();
 * }
 * @endcode
 */
class PoolAllocatorResource {
public:
  /**
   * @brief Creates a pool allocator resource sized for type T.
   * @tparam T Type to allocate
   * @param block_count Number of blocks to allocate
   * @return PoolAllocatorResource configured for type T
   */
  template <typename T>
  [[nodiscard]] static PoolAllocatorResource ForType(size_t block_count) {
    return PoolAllocatorResource(sizeof(T), block_count, alignof(T));
  }

  /**
   * @brief Constructs resource with a pool allocator.
   * @param block_size Size of each block in bytes
   * @param block_count Number of blocks to allocate
   * @param alignment Alignment for each block (must be power of 2)
   */
  explicit PoolAllocatorResource(size_t block_size, size_t block_count, size_t alignment = memory::kDefaultAlignment)
      : allocator_(block_size, block_count, alignment) {}

  PoolAllocatorResource(const PoolAllocatorResource&) = delete;
  PoolAllocatorResource(PoolAllocatorResource&&) noexcept = default;
  ~PoolAllocatorResource() noexcept = default;

  PoolAllocatorResource& operator=(const PoolAllocatorResource&) = delete;
  PoolAllocatorResource& operator=(PoolAllocatorResource&&) noexcept = default;

  /**
   * @brief Gets reference to the pool allocator.
   * @return Reference to PoolAllocator
   */
  [[nodiscard]] memory::PoolAllocator& Get() noexcept { return allocator_; }

  /**
   * @brief Gets const reference to the pool allocator.
   * @return Const reference to PoolAllocator
   */
  [[nodiscard]] const memory::PoolAllocator& Get() const noexcept { return allocator_; }

  /**
   * @brief Resets the pool, making all blocks available.
   */
  void Reset() noexcept { allocator_.Reset(); }

  /**
   * @brief Checks if the pool allocator is empty.
   * @return True if all blocks are free
   */
  [[nodiscard]] bool Empty() const noexcept { return allocator_.Empty(); }

  /**
   * @brief Checks if the pool allocator is full.
   * @return True if all blocks are allocated
   */
  [[nodiscard]] bool Full() const noexcept { return allocator_.Full(); }

  /**
   * @brief Gets pool allocator statistics.
   * @return AllocatorStats with current usage
   */
  [[nodiscard]] memory::AllocatorStats Stats() const noexcept { return allocator_.Stats(); }

  /**
   * @brief Gets the block size.
   * @return Block size in bytes
   */
  [[nodiscard]] size_t BlockSize() const noexcept { return allocator_.BlockSize(); }

  /**
   * @brief Gets the block count.
   * @return Total number of blocks
   */
  [[nodiscard]] size_t BlockCount() const noexcept { return allocator_.BlockCount(); }

  /**
   * @brief Gets the resource name for registration.
   * @return Resource name
   */
  [[nodiscard]] static constexpr std::string_view Name() noexcept { return "PoolAllocatorResource"; }

private:
  memory::PoolAllocator allocator_;
};

/**
 * @brief Resource wrapper for a per-World stack allocator.
 * @details Owns a stack allocator for LIFO allocation patterns.
 *
 * Uses mutex for thread safety.
 *
 * @warning Deallocations must follow LIFO order.
 * @note Thread-safe.
 *
 * @example
 * @code
 * world.AddResource<StackAllocatorResource>(1024 * 1024); // 1MB
 *
 * void Update(SystemContext& ctx) {
 *   auto& resource = ctx.WriteResource<StackAllocatorResource>();
 *   auto& allocator = resource.Get();
 * }
 * @endcode
 */
class StackAllocatorResource {
public:
  /**
   * @brief Constructs resource with a stack allocator.
   * @param capacity Size of the allocator buffer in bytes (default: 16MB)
   */
  explicit StackAllocatorResource(size_t capacity = 16 * 1024 * 1024) : allocator_(capacity) {}
  StackAllocatorResource(const StackAllocatorResource&) = delete;
  StackAllocatorResource(StackAllocatorResource&&) noexcept = default;
  ~StackAllocatorResource() noexcept = default;

  StackAllocatorResource& operator=(const StackAllocatorResource&) = delete;
  StackAllocatorResource& operator=(StackAllocatorResource&&) noexcept = default;

  /**
   * @brief Gets reference to the stack allocator.
   * @return Reference to StackAllocator
   */
  [[nodiscard]] memory::StackAllocator& Get() noexcept { return allocator_; }

  /**
   * @brief Gets const reference to the stack allocator.
   * @return Const reference to StackAllocator
   */
  [[nodiscard]] const memory::StackAllocator& Get() const noexcept { return allocator_; }

  /**
   * @brief Resets the stack allocator, freeing all allocations.
   */
  void Reset() noexcept { allocator_.Reset(); }

  /**
   * @brief Checks if the stack allocator is empty.
   * @return True if no allocations exist
   */
  [[nodiscard]] bool Empty() const noexcept { return allocator_.Empty(); }

  /**
   * @brief Checks if the stack allocator is full.
   * @return True if no more allocations can be made without reset
   */
  [[nodiscard]] bool Full() const noexcept { return allocator_.Full(); }

  /**
   * @brief Gets stack allocator statistics.
   * @return AllocatorStats with current usage
   */
  [[nodiscard]] memory::AllocatorStats Stats() const noexcept { return allocator_.Stats(); }

  /**
   * @brief Gets the capacity of the allocator.
   * @return Capacity in bytes
   */
  [[nodiscard]] size_t Capacity() const noexcept { return allocator_.Capacity(); }

  /**
   * @brief Gets the resource name for registration.
   * @return Resource name
   */
  [[nodiscard]] static constexpr std::string_view Name() noexcept { return "StackAllocatorResource"; }

private:
  memory::StackAllocator allocator_;
};

}  // namespace helios::app
