#pragma once

namespace helios::profile::details {

/**
 * @brief Returns false until `Profiler::Finalize()` has completed.
 * @details Global allocation hooks use this to avoid touching
 * `Profiler::Instance()` during static initialization.
 */
[[nodiscard]] bool IsProfilerFinalized() noexcept;

/**
 * @brief Returns true while profiler registry mutations suppress memory
 * dispatch.
 * @details Global allocation hooks and other memory events are skipped while
 * suspended to avoid re-entering backend iteration during `MultiTypeMap`
 * updates.
 */
[[nodiscard]] bool IsMemoryDispatchSuspended() noexcept;

/**
 * @brief Returns false after profiler shutdown has begun.
 * @details Memory hooks must not touch `Profiler::Instance()` once shutdown
 * starts.
 */
[[nodiscard]] bool IsProfilerMemoryDispatchEnabled() noexcept;

/// @brief Disables profiler memory dispatch for process shutdown.
void DisableProfilerMemoryDispatch() noexcept;

/// @brief Clears the finalized flag used by global allocation hooks.
void ResetProfilerFinalized() noexcept;

/// @brief RAII guard that suspends profiler memory dispatch on the current
/// thread.
class ScopedMemoryDispatchSuspend {
public:
  ScopedMemoryDispatchSuspend() noexcept;
  ~ScopedMemoryDispatchSuspend() noexcept;

  ScopedMemoryDispatchSuspend(const ScopedMemoryDispatchSuspend&) = delete;
  ScopedMemoryDispatchSuspend& operator=(const ScopedMemoryDispatchSuspend&) =
      delete;
  ScopedMemoryDispatchSuspend(ScopedMemoryDispatchSuspend&&) = delete;
  ScopedMemoryDispatchSuspend& operator=(ScopedMemoryDispatchSuspend&&) =
      delete;
};

}  // namespace helios::profile::details
