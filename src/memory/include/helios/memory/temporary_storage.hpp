#pragma once

#include <helios/memory/details/profile.hpp>
#include <helios/utils/format.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory_resource>

namespace helios::mem {

#if defined(HELIOS_MEMORY_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
namespace details {

/// @brief Sits between a `monotonic_buffer_resource` and its true upstream,
/// profiling only the allocations the monotonic resource makes when it grow.
class TemporaryStorageUpstream final : public std::pmr::memory_resource {
public:
  explicit TemporaryStorageUpstream(
      std::pmr::memory_resource* upstream) noexcept
      : upstream_(upstream) {}

private:
  void* do_allocate(size_t bytes, size_t alignment) override {
    void* const ptr = upstream_->allocate(bytes, alignment);
    HELIOS_MEMORY_PROFILE_ALLOC(ptr, bytes, "TemporaryStorage");
    return ptr;
  }

  void do_deallocate(void* ptr, size_t bytes, size_t alignment) override {
    HELIOS_MEMORY_PROFILE_FREE(ptr, "TemporaryStorage");
    upstream_->deallocate(ptr, bytes, alignment);
  }

  [[nodiscard]] bool do_is_equal(
      const std::pmr::memory_resource& other) const noexcept override {
    return this == &other;
  }

  std::pmr::memory_resource* upstream_ = std::pmr::new_delete_resource();
};

}  // namespace details
#endif

/**
 * @brief Per-thread linear allocator for short-lived, frame-scoped allocations.
 * Same idea as Jai's `Temporary_Storage`.
 * @details Wraps a `std::pmr::monotonic_buffer_resource` in thread-local
 * storage. Every thread gets its own instance lazily on first use (Meyer's
 * singleton). The hot allocation path is a plain bump pointer with no atomics:
 * thread-local storage is the only synchronization mechanism, and it's
 * sufficient because no other thread ever touches this instance.
 *
 * Self-registers into a global, lock-free, singly-linked registry on
 * construction so `ResetAllTemporaryStorage()` can reach every thread's
 * instance. Registration is a CAS push onto the registry head, of a small
 * heap-allocated, deliberately-leaked `RegistryNode`(NOT of the
 * `TemporaryStorage` object itself). This distinction matters: `this` is
 * `thread_local`, and its storage is reclaimed by the runtime as soon as
 * the owning thread exits (and may be reused for another thread's block
 * afterwards). A registry built by linking `this` directly would leave
 * `ResetAll()` walking into freed/reused memory the moment any registered
 * thread exits. The `RegistryNode` instead lives on the heap and is never
 * freed, so it's always safe to read; it holds a raw pointer back to the
 * (possibly already-destroyed) `TemporaryStorage` plus an atomic flag that the
 * destructor sets before the object goes away, and which `ResetAll()` must
 * check before dereferencing that pointer. There is deliberately no unlinking
 * of nodes from the list, so `ResetAll()` can walk it concurrently with new
 * threads registering without any locking or hazard-pointer/epoch reclamation
 * machinery. This is the right trade-off for a long-lived, roughly-fixed-size
 * worker thread pool, but a poor fit for workloads that spawn large numbers of
 * short-lived threads over the life of the process, since each one permanently
 * grows the registry.
 *
 * @warning `Reset()` invalidates every pointer and object previously handed out
 * by this thread's resource. Only call it at a point where nothing on this
 * thread still holds a temporary allocation (e.g. end of frame, end of task).
 */
class TemporaryStorage final : public std::pmr::memory_resource {
public:
  static constexpr size_t kInitialBlockSize = 4UZ * (1U << 10U);  // 4 KB

  TemporaryStorage(const TemporaryStorage&) = delete;
  TemporaryStorage(TemporaryStorage&&) = delete;

  /**
   * @brief Waits for any in-flight `ResetAll()`, then marks the registry
   * node retired so later walks no longer touch `this`.
   * @details Does not unlink the node from the registry. Must not yield or
   * sleep: this runs from a `thread_local` destructor.
   */
  ~TemporaryStorage() noexcept override {
    HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::TemporaryStorage::Destroy");
    Destroy();
  }

  TemporaryStorage& operator=(const TemporaryStorage&) = delete;
  TemporaryStorage& operator=(TemporaryStorage&&) = delete;

  /**
   * @brief Get this thread's `TemporaryStorage` instance, constructing it
   * on first call.
   * @return A reference to the thread-local `TemporaryStorage` instance
   */
  [[nodiscard]] static TemporaryStorage& Instance();

  /**
   * @brief Releases every block owned by this thread's resource back to
   * upstream.
   * @warning Invalidates all outstanding allocations made on this thread.
   */
  void Reset() noexcept {
    HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::TemporaryStorage::Reset");
    upstream_resource_.release();
  }

  /**
   * @brief Rewinds every registered, still-live thread's resource.
   * @details Performs a traversal of the registry's linked list, starting
   * from the current head and following each node's immutable `next`
   * pointer, so the traversal itself cannot race with a node being freed.
   *
   * This function makes no attempt to synchronize with threads that might
   * be concurrently allocating from (as opposed to merely registering or
   * exiting) the resources it resets, the caller is responsible for
   * guaranteeing that no other thread is allocating from temporary
   * storage while this runs (e.g. only call it from a single coordinating
   * thread at a full frame barrier, after every worker has quiesced). New
   * threads may safely register, and existing threads may safely exit,
   * concurrently with this call; those races are handled.
   * @warning Invalidates all outstanding allocations on every registered
   * thread whose node this call successfully claims.
   */
  static void ResetAll() noexcept {
    HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::TemporaryStorage::ResetAll");
    ResetAllImpl();
  }

  /**
   * @brief The memory resource backing this thread's temporary allocations.
   * @return A reference to the thread-local memory resource
   */
  [[nodiscard]] std::pmr::memory_resource& Resource() noexcept {
    return upstream_resource_;
  }

private:
  /**
   * @brief A registry entry, heap-allocated and intentionally never freed.
   * @details Kept separate from `TemporaryStorage` itself precisely because
   * `TemporaryStorage` is `thread_local` and does not outlive its thread,
   * whereas the registry (and anything `ResetAll()` might concurrently be
   * reading) must be safe to touch for the life of the process. See the
   * class-level note for the full rationale.
   *
   * `Reset()` mutates `monotonic_buffer_resource`'s internal chunk list and is
   * not thread-safe against a second concurrent call, and the owning thread's
   * teardown of `upstream_resource_` in `~TemporaryStorage()` is effectively
   * a second, concurrent call to the same non-thread-safe machinery if
   * `ResetAll()` races it.
   */
  enum class NodeState : uint8_t { kActive, kBusy, kRetired };

  struct RegistryNode {
    // Immutable after construction, set before this node is published via
    // the CAS in TemporaryStorage's constructor. Forms the intrusive
    // singly-linked list; never mutated again, so ResetAll() can traverse
    // it without synchronizing on anything but the head.
    RegistryNode* next = nullptr;

    // The thread-local instance this node describes. Only ever
    // dereferenced while `state` is held at kBusy by the dereferencing
    // side (see NodeState doc comment above); never dereferenced from the
    // node's own owner past that owner's destructor.
    TemporaryStorage* owner = nullptr;

    // kActive: owner is live and not currently being reset by anyone.
    // kBusy: exclusively claimed by ResetAll() while it calls Reset() on
    //   owner. Allocate/deallocate on the owning thread are not synchronized
    //   with this flag; the caller of ResetAll() must quiesce those paths.
    // kRetired: owner is destroyed; must never be dereferenced again.
    // Destroy waits until the node is not kBusy, then CAS kActive -> kRetired,
    // so it never holds kBusy itself (TLS destructors must not yield/sleep).
    std::atomic<NodeState> state{NodeState::kActive};
  };

  /**
   * @brief Constructs the resource and CAS-pushes a new `RegistryNode`
   * describing it onto the global registry.
   * @details The push is a standard Treiber-stack insertion on
   * `registry_head_`: read the current head into the new node's `next`,
   * then CAS the head from that value to the new node, retrying with the
   * updated head on failure. A node's `next` is never modified again after
   * a successful CAS, which is precisely what lets `ResetAll()` traverse
   * the list without synchronizing with concurrent pushes beyond a single
   * acquire load of the head.
   */
  TemporaryStorage() {
    HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::TemporaryStorage::Init");
    Init();
  }

  void Init();
  void Destroy() noexcept;
  static void ResetAllImpl() noexcept;

  void* do_allocate(size_t bytes, size_t alignment) override {
    return upstream_resource_.allocate(bytes, alignment);
  }

  void do_deallocate(void* ptr, size_t bytes, size_t alignment) override {
    upstream_resource_.deallocate(ptr, bytes, alignment);
  }

  [[nodiscard]] bool do_is_equal(
      const std::pmr::memory_resource& other) const noexcept override {
    return this == &other;
  }

  // Head of the lock-free registry list of RegistryNodes. Pushed to via CAS
  // in the constructor; never popped, and nodes are never freed (see
  // RegistryNode). `nullptr` means an empty registry.
  static inline std::atomic<RegistryNode*> registry_head_ = nullptr;

  // This instance's own entry in the registry above. Allocated once in the
  // constructor and never freed; only its `retired` flag is touched again
  // (by ~TemporaryStorage()).
  RegistryNode* registry_node_ = nullptr;

#if defined(HELIOS_MEMORY_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
  // Declared before upstream_resource_: member destruction runs in reverse
  // declaration order, so upstream_resource_'s release() (inside its own
  // destructor) runs first, while profiled_upstream_ is still alive to
  // receive the resulting do_deallocate calls.
  details::TemporaryStorageUpstream profiled_upstream_{
      std::pmr::new_delete_resource()};

  std::pmr::monotonic_buffer_resource upstream_resource_{kInitialBlockSize,
                                                         &profiled_upstream_};
#else
  std::pmr::monotonic_buffer_resource upstream_resource_{
      kInitialBlockSize, std::pmr::new_delete_resource()};
#endif
};

inline TemporaryStorage& TemporaryStorage::Instance() {
  thread_local TemporaryStorage instance;
  return instance;
}

/**
 * @brief The calling thread's temporary `TemporaryStorage`.
 * @return A reference to the thread-local `TemporaryStorage` instance
 */
[[nodiscard]] inline TemporaryStorage& GetTemporaryStorage() {
  return TemporaryStorage::Instance();
}

/**
 * @brief Creates a `polymorphic_allocator<T>` bound to the calling thread's
 * temporary storage.
 * @tparam T Element type for the allocator
 * @return A `polymorphic_allocator<T> `bound to the calling thread's temporary
 * @code
 *   std::pmr::vector<int> scratch{helios::mem::GetTemporaryStorage<int>()};
 * @endcode
 */
template <typename T = std::byte>
[[nodiscard]] inline auto GetTemporaryAllocator()
    -> std::pmr::polymorphic_allocator<T> {
  return {&GetTemporaryStorage()};
}

/**
 * @brief Releases the calling thread's temporary storage.
 * @warning Invalidates all outstanding allocations made on this thread.
 * See `TemporaryStorage`'s class-level @warning.
 */
inline void ResetTemporaryStorage() noexcept {
  TemporaryStorage::Instance().Reset();
}

/**
 * @brief Releases every registered thread's temporary storage.
 * @warning Caller must guarantee no thread is concurrently allocating from, or
 * holding, temporary storage. See `TemporaryStorage::ResetAll()`.
 */
inline void ResetAllTemporaryStorage() noexcept {
  TemporaryStorage::ResetAll();
}

}  // namespace helios::mem
