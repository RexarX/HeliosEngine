#pragma once

#include <helios/cstring_view.hpp>
#include <helios/profile/common.hpp>
#include <helios/utils/type_info.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <source_location>
#include <span>
#include <string_view>
#include <type_traits>

namespace helios::profile {

/**
 * @brief Abstract profiler backend interface.
 * @details Each backend receives a slice of `ScopedZone` storage sized by
 * `ZoneStorageSize()`. Backends adapt Helios profiling concepts (zones,
 * frames, counters, memory events) to their native representation.
 *
 * Thread-safety contract:
 * - `Startup()` and `Shutdown()` are called only during single-threaded
 *   lifecycle phases.
 * - All other dispatch hooks must be safe to call concurrently from multiple
 *   threads after `Profiler::Finalize()`.
 */
class Backend {
public:
  virtual ~Backend() noexcept = default;

  /// @brief Called once after finalization (single-threaded).
  virtual void Startup() noexcept {}

  /// @brief Called during shutdown
  /// (single-threaded).
  virtual void Shutdown() noexcept {}

  /**
   * @brief Returns the per-zone storage required by this backend.
   * @return Storage size in bytes
   */
  [[nodiscard]] virtual size_t ZoneStorageSize() const noexcept = 0;

  /**
   * @brief Begins a profiling zone.
   * @param spec Zone specification
   * @param storage Backend-specific storage slice
   */
  virtual void BeginZone(const ZoneSpec& spec,
                         std::span<std::byte> storage) noexcept = 0;

  /**
   * @brief Ends a profiling zone.
   * @param storage Backend-specific storage slice
   */
  virtual void EndZone(std::span<std::byte> storage) noexcept = 0;

  /**
   * @brief Attaches text to the active zone.
   * @param storage Backend-specific storage slice
   * @param text Text to attach
   */
  virtual void ZoneText(std::span<std::byte> storage,
                        std::string_view text) noexcept = 0;

  /**
   * @brief Attaches a numeric value to the active zone.
   * @param storage Backend-specific storage slice
   * @param value Value to attach
   */
  virtual void ZoneValue(std::span<std::byte> storage,
                         uint64_t value) noexcept = 0;

  /**
   * @brief Renames the active zone.
   * @param storage Backend-specific storage slice
   * @param name New zone name
   */
  virtual void ZoneName(std::span<std::byte> storage,
                        std::string_view name) noexcept = 0;

  /// @brief Marks the end of the current frame.
  virtual void FrameMark() noexcept = 0;

  /**
   * @brief Marks the end of a named frame.
   * @param name Frame name
   */
  virtual void FrameMark(CStringView name) noexcept = 0;

  /**
   * @brief Marks the start of a named frame region.
   * @param name Frame region name
   */
  virtual void FrameMarkStart(CStringView name) noexcept = 0;

  /**
   * @brief Marks the end of a named frame region.
   * @param name Frame region name
   */
  virtual void FrameMarkEnd(CStringView name) noexcept = 0;

  /**
   * @brief Emits a timeline message.
   * @param text Message text
   * @param color Optional message color
   */
  virtual void Message(std::string_view text, uint32_t color) noexcept = 0;

  /**
   * @brief Sets the current thread name.
   * @param name Thread name
   */
  virtual void SetThreadName(CStringView name) noexcept = 0;

  /**
   * @brief Records a plot sample.
   * @param name Plot channel name
   * @param value Sample value
   */
  virtual void Plot(CStringView name, double value) noexcept = 0;

  /**
   * @brief Configures a plot channel.
   * @param name Plot channel name
   * @param type Plot value format
   * @param step Whether the plot is step-wise
   * @param fill Whether the plot is filled
   * @param color Plot color
   */
  virtual void PlotConfig(CStringView name, PlotFormat type, bool step,
                          bool fill, uint32_t color) noexcept = 0;

  /**
   * @brief Records a memory allocation.
   * @param ptr Allocated pointer
   * @param size Allocation size
   * @param name Optional allocation pool name
   * @param depth Callstack depth
   * @param loc Source location
   */
  virtual void Alloc(const void* ptr, size_t size,
                     std::optional<CStringView> name, int depth,
                     std::source_location loc) noexcept = 0;

  /**
   * @brief Records a memory deallocation.
   * @param ptr Deallocated pointer
   * @param name Optional allocation pool name
   * @param depth Callstack depth
   * @param loc Source location
   */
  virtual void Free(const void* ptr, std::optional<CStringView> name, int depth,
                    std::source_location loc) noexcept = 0;

  /**
   * @brief Discards tracked memory for a named pool.
   * @param name Pool name
   */
  virtual void MemoryDiscard(CStringView name) noexcept = 0;

  /**
   * @brief Discards tracked memory for a named pool with callstack depth.
   * @param name Pool name
   * @param depth Callstack depth
   */
  virtual void MemoryDiscard(CStringView name, int depth) noexcept = 0;

  /**
   * @brief Returns the backend name.
   * @return Backend identifier
   */
  [[nodiscard]] virtual std::string_view Name() const noexcept = 0;
};

/// @brief Type alias for profiler backend type IDs.
using BackendTypeId = utils::TypeId;

/// @brief Type alias for profiler backend type indices.
using BackendTypeIndex = utils::TypeIndex;

/**
 * @brief Trait to identify concrete profiler backend types.
 * @tparam T Type to check
 */
template <typename T>
concept ProfilerBackendTrait =
    std::derived_from<std::remove_cvref_t<T>, Backend> &&
    !std::is_abstract_v<std::remove_cvref_t<T>>;

}  // namespace helios::profile
