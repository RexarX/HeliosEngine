#pragma once

#include <cstddef>
#include <memory_resource>

namespace helios::mem {

/**
 * @brief Per-thread bump allocator for frame and task-scoped allocations.
 * Same idea as Jai's `Temporary_Storage`.
 * @details Any thread that calls `Instance()` (job workers, third-party
 * threads, test threads) lazily claims one slot from a fixed-size pool on
 * first use. No setup call is required. The hot allocation path is a private
 * bump pointer with no atomics: once a slot is claimed, no other thread
 * allocates from it.
 *
 * The `thread_local` destructor never calls malloc/free/new/delete. It only
 * CAS-es slot state (`kActive` -> `kRetired`, or `kBusy` -> `kBusyRetired` if
 * `ResetAll()` currently holds the slot). Memory a retired slot holds is
 * released lazily by whichever thread later claims that slot, never by the
 * exiting thread.
 *
 * @warning `Reset()` / `ResetAll()` invalidate every pointer previously
 * handed out by the affected resource(s). Only call them at a point where
 * nothing still holds a temporary allocation (e.g. end of frame, full
 * barrier).
 */
class TemporaryStorage {
public:
  static constexpr size_t kInitialBlockSize = 4UZ * (1U << 10U);  // 4 KB
  static constexpr size_t kMaxThreads = 256;

  TemporaryStorage() = delete;
  TemporaryStorage(const TemporaryStorage&) = delete;
  TemporaryStorage(TemporaryStorage&&) = delete;
  ~TemporaryStorage() = delete;
  TemporaryStorage& operator=(const TemporaryStorage&) = delete;
  TemporaryStorage& operator=(TemporaryStorage&&) = delete;

  /**
   * @brief This calling thread's resource, claiming a slot on first call.
   * @return The thread's `monotonic_buffer_resource`
   */
  [[nodiscard]] static std::pmr::memory_resource& Instance() noexcept;

  /**
   * @brief Rewind this thread's own storage.
   * @warning Invalidates prior allocations made on this thread only.
   */
  static void Reset() noexcept;

  /**
   * @brief Rewind every currently live thread's storage.
   * @details Concurrent `Instance()` / allocate on another thread is a
   * caller error (frame-barrier contract). Thread exit concurrent with
   * this call is handled: a dying thread never frees. `ResetAll()` keeps
   * the slot `kBusy` for the duration of `release()` so it cannot be
   * reclaimed mid-reset; if the owner left, the restoring CAS fails and
   * the slot is published as `kRetired`.
   * @warning Invalidates all outstanding allocations on every live slot
   * this call successfully claims.
   */
  static void ResetAll() noexcept;

private:
  struct ThreadBinding;

  [[nodiscard]] static ThreadBinding& LocalBinding() noexcept;
  [[nodiscard]] static size_t ClaimSlot() noexcept;
};

/**
 * @brief The calling thread's temporary memory resource.
 * @return A reference to the thread's bump resource
 */
[[nodiscard]] inline std::pmr::memory_resource& GetTemporaryStorage() noexcept {
  return TemporaryStorage::Instance();
}

/**
 * @brief Creates a `polymorphic_allocator<T>` bound to the calling thread's
 * temporary storage.
 * @tparam T Element type for the allocator
 * @return A `polymorphic_allocator<T>` bound to this thread's temporary
 * resource
 * @code
 *   std::pmr::vector<int> scratch{helios::mem::GetTemporaryAllocator<int>()};
 * @endcode
 */
template <typename T = std::byte>
[[nodiscard]] inline auto GetTemporaryAllocator() noexcept
    -> std::pmr::polymorphic_allocator<T> {
  return {&GetTemporaryStorage()};
}

/**
 * @brief Releases the calling thread's temporary storage.
 * @warning Invalidates all outstanding allocations made on this thread.
 * See `TemporaryStorage`'s class-level @warning.
 */
inline void ResetTemporaryStorage() noexcept {
  TemporaryStorage::Reset();
}

/**
 * @brief Releases every currently live thread's temporary storage.
 * @warning Caller must guarantee no thread is concurrently allocating from
 * temporary storage. See `TemporaryStorage::ResetAll()`.
 */
inline void ResetAllTemporaryStorage() noexcept {
  TemporaryStorage::ResetAll();
}

}  // namespace helios::mem
