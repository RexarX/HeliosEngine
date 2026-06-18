#pragma once

#include <helios/assert.hpp>
#include <helios/memory/arena_allocator.hpp>
#include <helios/memory/common.hpp>
#include <helios/memory/details/profile.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory_resource>
#include <utility>

namespace helios::mem {

/// @brief Configuration for `NFrameAllocator`.
struct FrameAllocatorOptions {
  size_t initial_capacity = 0;
  GrowthPolicy growth = GrowthPolicy::Fixed();
};

/**
 * @brief N-buffered PMR frame allocator.
 * @details Holds N arena resources and routes allocations to current frame.
 *
 * `Advance()` switches to the next frame and resets it, allowing memory reuse
 * with deterministic lifetime windows.
 *
 * @note Allocation is thread-safe as delegated to ArenaAllocator.
 * @warning `Advance()` and `Reset()` must not run concurrently with allocate.
 * @tparam N Number of frame buffers, must be >= 1
 */
template <size_t N>
  requires(N >= 1)
class NFrameAllocator final : public std::pmr::memory_resource {
public:
  /**
   * @brief Constructs N-frame allocator from options.
   * @warning Triggers assertion if `options.initial_capacity == 0`.
   * @param options Frame allocator options
   */
  explicit NFrameAllocator(FrameAllocatorOptions options) noexcept;

  /**
   * @brief Constructs N-frame allocator with fixed-growth arenas.
   * @warning Triggers assertion if `initial_capacity == 0`.
   * @param initial_capacity Initial per-frame arena capacity
   */
  explicit NFrameAllocator(size_t initial_capacity) noexcept;

  NFrameAllocator(const NFrameAllocator&) = delete;
  NFrameAllocator(NFrameAllocator&& other) noexcept;
  ~NFrameAllocator() noexcept override = default;

  NFrameAllocator& operator=(const NFrameAllocator&) = delete;
  NFrameAllocator& operator=(NFrameAllocator&& other) noexcept;

  /// @brief Advances frame index and resets new current arena.
  void Advance() noexcept;

  /// @brief Resets all arenas and sets frame index to zero.
  void Reset() noexcept;

  /**
   * @brief Returns true when current frame arena has no allocations.
   * @return True if current arena is empty, false otherwise
   */
  [[nodiscard]] bool Empty() const noexcept { return CurrentArena().Empty(); }

  /**
   * @brief Returns stats for current frame arena.
   * @return Allocator stats
   */
  [[nodiscard]] AllocatorStats Stats() const noexcept {
    return CurrentArena().Stats();
  }

  /**
   * @brief Returns aggregated stats across all frame arenas.
   * @return Summed allocator stats
   */
  [[nodiscard]] AllocatorStats AggregateStats() const noexcept;

  /**
   * @brief Returns current frame index.
   * @return Current frame in range [0, N)
   */
  [[nodiscard]] size_t FrameIndex() const noexcept { return frame_index_; }

  /**
   * @brief Returns number of frame buffers.
   * @return N
   */
  [[nodiscard]] static consteval size_t FrameCount() noexcept { return N; }

  /**
   * @brief Returns per-arena initial capacity.
   * @return Capacity in bytes
   */
  [[nodiscard]] size_t InitialCapacity() const noexcept {
    return initial_capacity_;
  }

  /**
   * @brief Returns growth policy.
   * @return Growth policy
   */
  [[nodiscard]] GrowthPolicy Growth() const noexcept { return growth_; }

  /**
   * @brief Returns current arena total capacity.
   * @return Capacity in bytes
   */
  [[nodiscard]] size_t TotalCapacity() const noexcept {
    return CurrentArena().TotalCapacity();
  }

  /**
   * @brief Returns current arena block count.
   * @return Block count
   */
  [[nodiscard]] size_t BlockCount() const noexcept {
    return CurrentArena().BlockCount();
  }

  /**
   * @brief Returns mutable arena by frame index.
   * @warning Triggers assertion if `index >= N`.
   * @param index Arena index
   * @return Arena reference
   */
  [[nodiscard]] ArenaAllocator& Arena(size_t index) noexcept;

  /**
   * @brief Returns const arena by frame index.
   * @warning Triggers assertion if `index >= N`.
   * @param index Arena index
   * @return Const arena reference
   */
  [[nodiscard]] const ArenaAllocator& Arena(size_t index) const noexcept;

private:
  [[nodiscard]] ArenaAllocator& CurrentArena() noexcept {
    return arenas_[frame_index_];
  }

  [[nodiscard]] const ArenaAllocator& CurrentArena() const noexcept {
    return arenas_[frame_index_];
  }

  template <size_t... Indexes>
  [[nodiscard]] static auto BuildArenas(const FrameAllocatorOptions& options,
                                        std::index_sequence<Indexes...> /*seq*/)
      -> std::array<ArenaAllocator, N>;

  [[nodiscard]] void* do_allocate(size_t bytes, size_t alignment) override {
    HELIOS_MEMORY_PROFILE_SCOPE();
    HELIOS_MEMORY_PROFILE_ZONE_VALUE(bytes);

    return CurrentArena().allocate(bytes, alignment);
  }

  void do_deallocate(void* ptr, size_t bytes, size_t alignment) override {
    HELIOS_MEMORY_PROFILE_SCOPE();
    HELIOS_MEMORY_PROFILE_ZONE_VALUE(bytes);

    CurrentArena().deallocate(ptr, bytes, alignment);
  }

  [[nodiscard]] bool do_is_equal(
      const std::pmr::memory_resource& other) const noexcept override {
    return this == &other;
  }

  std::array<ArenaAllocator, N> arenas_;
  size_t initial_capacity_ = 0;
  GrowthPolicy growth_;
  size_t frame_index_ = 0;
};

template <size_t N>
  requires(N >= 1)
inline NFrameAllocator<N>::NFrameAllocator(
    FrameAllocatorOptions options) noexcept
    : arenas_(BuildArenas(options, std::make_index_sequence<N>{})),
      initial_capacity_(options.initial_capacity),
      growth_(options.growth) {
  HELIOS_ASSERT(initial_capacity_ > 0,
                "initial_capacity must be greater than zero");
}

template <size_t N>
  requires(N >= 1)
inline NFrameAllocator<N>::NFrameAllocator(size_t initial_capacity) noexcept
    : NFrameAllocator(
          FrameAllocatorOptions{.initial_capacity = initial_capacity,
                                .growth = GrowthPolicy::Fixed()}) {}

template <size_t N>
  requires(N >= 1)
inline NFrameAllocator<N>::NFrameAllocator(NFrameAllocator&& other) noexcept
    : arenas_(std::move(other.arenas_)),
      initial_capacity_(std::exchange(other.initial_capacity_, 0)),
      growth_(other.growth_),
      frame_index_(std::exchange(other.frame_index_, 0)) {}

template <size_t N>
  requires(N >= 1)
inline NFrameAllocator<N>& NFrameAllocator<N>::operator=(
    NFrameAllocator&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  arenas_ = std::move(other.arenas_);
  initial_capacity_ = std::exchange(other.initial_capacity_, 0);
  growth_ = other.growth_;
  frame_index_ = std::exchange(other.frame_index_, 0);
  return *this;
}

template <size_t N>
  requires(N >= 1)
inline void NFrameAllocator<N>::Advance() noexcept {
  HELIOS_MEMORY_PROFILE_SCOPE();
  HELIOS_MEMORY_PROFILE_ZONE_VALUE(frame_index_);

  frame_index_ = (frame_index_ + 1) % N;
  CurrentArena().Reset();
}

template <size_t N>
  requires(N >= 1)
inline void NFrameAllocator<N>::Reset() noexcept {
  HELIOS_MEMORY_PROFILE_SCOPE();

  for (auto& arena : arenas_) {
    arena.Reset();
  }
  frame_index_ = 0;
}

template <size_t N>
  requires(N >= 1)
inline AllocatorStats NFrameAllocator<N>::AggregateStats() const noexcept {
  AllocatorStats aggregate{};
  for (const auto& arena : arenas_) {
    const AllocatorStats stats = arena.Stats();
    aggregate.total_allocated =
        SaturatingAdd(aggregate.total_allocated, stats.total_allocated);
    aggregate.peak_usage =
        SaturatingAdd(aggregate.peak_usage, stats.peak_usage);
    aggregate.allocation_count =
        SaturatingAdd(aggregate.allocation_count, stats.allocation_count);
    aggregate.total_allocations =
        SaturatingAdd(aggregate.total_allocations, stats.total_allocations);
    aggregate.total_deallocations =
        SaturatingAdd(aggregate.total_deallocations, stats.total_deallocations);
    aggregate.alignment_waste =
        SaturatingAdd(aggregate.alignment_waste, stats.alignment_waste);
  }
  return aggregate;
}

template <size_t N>
  requires(N >= 1)
inline ArenaAllocator& NFrameAllocator<N>::Arena(size_t index) noexcept {
  HELIOS_ASSERT(index < N, "index '{}' is out of range [0, {})!", index, N);
  return arenas_[index];
}

template <size_t N>
  requires(N >= 1)
inline const ArenaAllocator& NFrameAllocator<N>::Arena(
    size_t index) const noexcept {
  HELIOS_ASSERT(index < N, "index '{}' is out of range [0, {})!", index, N);
  return arenas_[index];
}

template <size_t N>
  requires(N >= 1)
template <size_t... Indexes>
inline auto NFrameAllocator<N>::BuildArenas(
    const FrameAllocatorOptions& options,
    std::index_sequence<Indexes...> /*seq*/) -> std::array<ArenaAllocator, N> {
  return {((void)Indexes, ArenaAllocator(ArenaOptions{
                              .initial_capacity = options.initial_capacity,
                              .growth = options.growth}))...};
}

using FrameAllocator = NFrameAllocator<1>;
using DoubleFrameAllocator = NFrameAllocator<2>;

}  // namespace helios::mem
