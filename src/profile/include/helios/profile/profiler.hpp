#pragma once

#include <helios/assert.hpp>
#include <helios/container/multi_type_map.hpp>
#include <helios/cstring_view.hpp>
#include <helios/profile/backend.hpp>
#include <helios/profile/common.hpp>
#include <helios/profile/config.hpp>
#include <helios/profile/details/memory_dispatch.hpp>
#include <helios/utils/type_info.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <source_location>
#include <span>
#include <string_view>

namespace helios::profile {

/**
 * @brief Global profiler registry and multi-backend fan-out dispatcher.
 * @details Backends are registered before `Finalize()` and keyed by concrete
 * type via `MultiTypeMap`.
 */
class Profiler {
public:
  /**
   * @brief Returns the singleton profiler instance.
   * @return Reference to the profiler
   */
  [[nodiscard]] static Profiler& Instance() noexcept;

  /**
   * @brief Clears all registered backends and resets finalization state.
   * @warning Not thread-safe. Call only when no zones are active.
   */
  void Clear() noexcept;

  /**
   * @brief Partitions zone storage and marks the profiler ready for use.
   * @details Backends must be explicitly registered via `AddBackend()`
   * before calling `Finalize()`. No backend is added automatically.
   * @warning Not thread-safe.
   * Call before worker threads start.
   */
  void Finalize() noexcept;

  /**
   * @brief Calls `Startup()` on all registered backends.
   * @warning Not thread-safe.
   * Call only during single-threaded startup.
   */
  void Startup() noexcept;

  /**
   * @brief Calls `Shutdown()` on all registered backends.
   * @warning Not thread-safe.
   * Call only during single-threaded startup.
   */
  void Shutdown() noexcept;

  /**
   * @brief Registers and owns a backend of type `T`.
   * @warning Not thread-safe.
   * Call only during single-threaded startup.
   * @tparam T Concrete backend type
   * @tparam Args Constructor argument types
   * @param args Arguments forwarded to `T`'s constructor
   * @return Reference to the registered backend
   */
  template <ProfilerBackendTrait T, typename... Args>
  T& AddBackend(Args&&... args) noexcept;

  /**
   * @brief Removes a backend by type.
   * @warning Not thread-safe.
   * Triggers assertion if finalize has been called yet.
   * Call only when no zones are active.
   * @tparam T Concrete backend type
   */
  template <ProfilerBackendTrait T>
  void RemoveBackend() noexcept;

  /**
   * @brief Removes a backend by type index.
   * @warning Not thread-safe.
   * Triggers assertion if finalize has been called yet.
   * Call only when no zones are active.
   * @param index Backend type index
   */
  void RemoveBackend(BackendTypeIndex index) noexcept;

  /**
   * @brief Gets a backend by type.
   * @warning Not thread-safe.
   * Triggers assertion if the backend is not registered.
   * @tparam T Concrete backend type
   * @return Reference to the backend
   */
  template <ProfilerBackendTrait T>
  [[nodiscard]] T& Get() noexcept {
    return static_cast<T&>(*backends_.Get<T>().backend);
  }

  /**
   * @brief Gets a backend by type (const).
   * @warning Not thread-safe.
   * Triggers assertion if the backend is not registered.
   * @tparam T Concrete backend type
   * @return Const reference to the backend
   */
  template <ProfilerBackendTrait T>
  [[nodiscard]] const T& Get() const noexcept {
    return static_cast<const T&>(*backends_.Get<T>().backend);
  }

  /**
   * @brief Gets a backend by type index.
   * @warning Not thread-safe.
   * Triggers assertion if the backend is not registered.
   * @param index Backend type index
   * @return Reference to the backend
   */
  [[nodiscard]] Backend& Get(BackendTypeIndex index) noexcept;

  /**
   * @brief Gets a backend by type index (const).
   * @warning Not thread-safe.
   * Triggers assertion if the backend is not registered.
   * @param index Backend type index
   * @return Const reference to the backend
   */
  [[nodiscard]] const Backend& Get(BackendTypeIndex index) const noexcept;

  /**
   * @brief Tries to get a backend by type.
   * @warning Not thread-safe.
   * @tparam T Concrete backend type
   * @return Pointer to the backend, or `nullptr` if not registered
   */
  template <ProfilerBackendTrait T>
  [[nodiscard]] T* TryGet() noexcept;

  /**
   * @brief Tries to get a backend by type (const).
   * @warning Not thread-safe.
   * @tparam T Concrete backend type
   * @return Const pointer to the backend, or `nullptr` if not registered
   */
  template <ProfilerBackendTrait T>
  [[nodiscard]] const T* TryGet() const noexcept;

  /**
   * @brief Tries to get a backend by type index.
   * @warning Not thread-safe.
   * @param index Backend type index
   * @return Pointer to the backend, or `nullptr` if not registered
   */
  [[nodiscard]] Backend* TryGet(BackendTypeIndex index) noexcept;

  /**
   * @brief Tries to get a backend by type index (const).
   * @warning Not thread-safe.
   * @param index Backend type index
   * @return Const pointer to the backend, or `nullptr` if not registered
   */
  [[nodiscard]] const Backend* TryGet(BackendTypeIndex index) const noexcept;

  /**
   * @brief Checks whether a backend type is registered.
   * @warning Not thread-safe.
   * @tparam T Concrete backend type
   * @return True if registered
   */
  template <ProfilerBackendTrait T>
  [[nodiscard]] bool Contains() const noexcept {
    return backends_.Contains<T>();
  }

  /**
   * @brief Checks whether a backend type index is registered.
   * @warning Not thread-safe.
   * @param index Backend type index
   * @return True if registered
   */
  [[nodiscard]] bool Contains(BackendTypeIndex index) const noexcept {
    return backends_.Contains(index);
  }

  /**
   * @brief Returns whether the profiler has been finalized.
   * @warning Not thread-safe.
   * @return True after `Finalize()`
   */
  [[nodiscard]] bool Finalized() const noexcept { return finalized_; }

  /**
   * @brief Returns the zone storage byte offset for backend type `T`.
   * @warning Not thread-safe.
   * Triggers assertion if the backend is not registered or not finalized.
   * @tparam T Concrete backend type
   * @return Storage offset in bytes
   */
  template <ProfilerBackendTrait T>
  [[nodiscard]] size_t StorageOffset() const noexcept {
    return backends_.Get<T>().storage_offset;
  }

  /**
   * @brief Returns the number of registered backends.
   * @warning Not thread-safe.
   * @return Backend count
   */
  [[nodiscard]] size_t BackendCount() const noexcept {
    return backends_.TypeCount();
  }

  /**
   * @brief Dispatches a zone begin event to all registered backends.
   * @details Thread-safe after `Finalize()` when backends honor the dispatch
   * contract.
   * @param spec Zone specification
   * @param storage Contiguous storage for backend-specific zone data
   */
  void BeginZone(const ZoneSpec& spec, std::span<std::byte> storage) noexcept;

  /**
   * @brief Dispatches a zone end event to all registered backends.
   * @details Thread-safe after `Finalize()` when backends honor the dispatch
   * contract.
   * @param storage Contiguous storage for backend-specific zone data
   */
  void EndZone(std::span<std::byte> storage) noexcept;

  /**
   * @brief Dispatches a zone text event to all registered backends.
   * @details Thread-safe after `Finalize()` when backends honor the dispatch
   * contract.
   * @param storage Contiguous storage for backend-specific zone data
   * @param text Text to attach to the zone
   */
  void ZoneText(std::span<std::byte> storage, std::string_view text) noexcept;

  /**
   * @brief Dispatches a zone value event to all registered backends.
   * @details Thread-safe after `Finalize()` when backends honor the dispatch
   * contract.
   * @param storage Contiguous storage for backend-specific zone data
   * @param value Numeric value to attach to the zone
   */
  void ZoneValue(std::span<std::byte> storage, uint64_t value) noexcept;

  /**
   * @brief Dispatches a zone name event to all registered backends.
   * @details Thread-safe after `Finalize()` when backends honor the dispatch
   * contract.
   * @param storage Contiguous storage for backend-specific zone data
   * @param name Name to attach to the zone
   */
  void ZoneName(std::span<std::byte> storage, std::string_view name) noexcept;

  /**
   * @brief Dispatches a frame mark event to all registered backends.
   * @details Thread-safe after `Finalize()` when backends honor the dispatch
   * contract.
   */
  void FrameMark() noexcept;

  /**
   * @brief Dispatches a frame mark event with a name to all registered
   * backends.
   * @details Thread-safe after `Finalize()` when backends honor the dispatch
   * contract.
   * @param name Name of the frame mark
   */
  void FrameMark(CStringView name) noexcept;

  /**
   * @brief Dispatches a frame mark start event to all registered backends.
   * @details Thread-safe after `Finalize()` when backends honor the dispatch
   * contract.
   * @param name Name of the frame mark
   */
  void FrameMarkStart(CStringView name) noexcept;

  /**
   * @brief Dispatches a frame mark end event to all registered backends.
   * @details Thread-safe after `Finalize()` when backends honor the dispatch
   * contract.
   * @param name Name of the frame mark
   */
  void FrameMarkEnd(CStringView name) noexcept;

  /**
   * @brief Dispatches a message event to all registered backends.
   * @details Thread-safe after `Finalize()` when backends honor the dispatch
   * contract.
   * @param text Text of the message
   * @param color Color of the message
   */
  void Message(std::string_view text, uint32_t color) noexcept;

  /**
   * @brief Dispatches a thread name event to all registered backends.
   * @details Thread-safe after `Finalize()` when backends honor the dispatch
   * contract.
   * @param name Name of the thread
   */
  void SetThreadName(CStringView name) noexcept;

  /**
   * @brief Dispatches a plot event to all registered backends.
   * @details Thread-safe after `Finalize()` when backends honor the dispatch
   * contract.
   * @param name Name of the plot
   * @param value Value to plot
   */
  void Plot(CStringView name, double value) noexcept;

  /**
   * @brief Dispatches a plot configuration event to all registered backends.
   * @details Thread-safe after `Finalize()` when backends honor the dispatch
   * contract.
   * @param name Name of the plot
   * @param type Plot format type
   * @param step Whether to use step interpolation
   * @param fill Whether to fill the area under the plot line
   * @param color Color of the plot
   */
  void PlotConfig(CStringView name, PlotFormat type, bool step, bool fill,
                  uint32_t color) noexcept;

  /**
   * @brief Dispatches an allocation event to all registered backends.
   * @details Thread-safe after `Finalize()` when backends honor the dispatch
   * contract.
   * @param ptr Pointer to the allocated memory
   * @param size Size of the allocated memory in bytes
   * @param name Optional name of the allocation
   * @param depth Call stack depth of the allocation event
   * @param loc Source location of the allocation event
   */
  void Alloc(const void* ptr, size_t size, std::optional<CStringView> name,
             int depth, std::source_location loc) noexcept;

  /**
   * @brief Dispatches a deallocation event to all registered backends.
   * @details Thread-safe after `Finalize()` when backends honor the dispatch
   * contract.
   * @param ptr Pointer to the deallocated memory
   * @param name Optional name of the deallocation
   * @param depth Call stack depth of the deallocation event
   * @param loc Source location of the deallocation event
   */
  void Free(const void* ptr, std::optional<CStringView> name, int depth,
            std::source_location loc) noexcept;

  /**
   * @brief Dispatches a memory discard event to all registered backends.
   * @details Thread-safe after `Finalize()` when backends honor the dispatch
   * contract.
   * @param name Optional name of the discard event
   */
  void MemoryDiscard(CStringView name) noexcept;

  /**
   * @brief Dispatches a memory discard event to all registered backends.
   * @details Thread-safe after `Finalize()` when backends honor the dispatch
   * contract.
   * @param name Optional name of the discard event
   * @param depth Call stack depth of the discard event
   */
  void MemoryDiscard(CStringView name, int depth) noexcept;

private:
  /// @brief Owned backend slot stored in the profiler registry.
  struct BackendEntry {
    std::unique_ptr<Backend> backend;
    size_t storage_offset = 0;
    size_t storage_size = 0;
  };

  using BackendMap = container::MultiTypeMap<BackendEntry>;

  Profiler() noexcept = default;
  ~Profiler() noexcept;

  [[nodiscard]] BackendEntry* FindEntry(BackendTypeIndex index) noexcept {
    return backends_.TryGet(index);
  }

  [[nodiscard]] const BackendEntry* FindEntry(
      BackendTypeIndex index) const noexcept {
    return backends_.TryGet(index);
  }

  BackendMap backends_;
  bool finalized_ = false;
};

inline Profiler& Profiler::Instance() noexcept {
  static Profiler instance;
  return instance;
}

inline void Profiler::Clear() noexcept {
  details::ScopedMemoryDispatchSuspend suspend;
  backends_.ResetAll();
  finalized_ = false;
  details::ResetProfilerFinalized();
}

inline void Profiler::RemoveBackend(BackendTypeIndex index) noexcept {
  HELIOS_ASSERT(!finalized_,
                "Cannot remove profiling backends after Finalize()!");
  details::ScopedMemoryDispatchSuspend suspend;
  backends_.Remove(index);
}

template <ProfilerBackendTrait T, typename... Args>
inline T& Profiler::AddBackend(Args&&... args) noexcept {
  HELIOS_ASSERT(!finalized_, "Cannot add profiling backends after Finalize()!");
  details::ScopedMemoryDispatchSuspend suspend;
  const auto [it, inserted] = backends_.TryEmplace<T>(
      BackendEntry{std::make_unique<T>(std::forward<Args>(args)...), 0});
  HELIOS_ASSERT(inserted, "Backend type '{}' already registered!",
                utils::TypeNameOf<T>());
  return static_cast<T&>(*it->second.backend);
}

template <ProfilerBackendTrait T>
inline void Profiler::RemoveBackend() noexcept {
  RemoveBackend(BackendTypeIndex::From<T>());
}

inline Backend& Profiler::Get(BackendTypeIndex index) noexcept {
  BackendEntry* const entry = backends_.TryGet(index);
  HELIOS_ASSERT(entry != nullptr, "Backend not found!");
  return *entry->backend;
}

inline const Backend& Profiler::Get(BackendTypeIndex index) const noexcept {
  const BackendEntry* const entry = backends_.TryGet(index);
  HELIOS_ASSERT(entry != nullptr, "Backend not found!");
  return *entry->backend;
}

inline Backend* Profiler::TryGet(BackendTypeIndex index) noexcept {
  const BackendEntry* const entry = FindEntry(index);
  return entry != nullptr ? entry->backend.get() : nullptr;
}

inline const Backend* Profiler::TryGet(BackendTypeIndex index) const noexcept {
  const BackendEntry* const entry = FindEntry(index);
  return entry != nullptr ? entry->backend.get() : nullptr;
}

template <ProfilerBackendTrait T>
inline T* Profiler::TryGet() noexcept {
  const BackendEntry* const entry = backends_.TryGet<T>();
  return entry != nullptr ? static_cast<T*>(entry->backend.get()) : nullptr;
}

template <ProfilerBackendTrait T>
inline const T* Profiler::TryGet() const noexcept {
  const BackendEntry* const entry = backends_.TryGet<T>();
  return entry != nullptr ? static_cast<const T*>(entry->backend.get())
                          : nullptr;
}

namespace details {

/**
 * @brief Returns the zone storage byte offset assigned to backend type `T`.
 * @details Valid after `Finalize()`. For testing and diagnostics only.
 * @tparam T Concrete backend type
 * @return Storage offset in bytes
 */
template <ProfilerBackendTrait T>
[[nodiscard]] inline size_t BackendStorageOffset() noexcept {
  return Profiler::Instance().StorageOffset<T>();
}

/**
 * @brief Returns the number of active backend slots after finalization.
 * @return Number of registered backends
 */
[[nodiscard]] inline size_t ActiveBackendCount() noexcept {
  return Profiler::Instance().BackendCount();
}

}  // namespace details

}  // namespace helios::profile
