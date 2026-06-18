#pragma once

#include <helios/profile/common.hpp>
#include <helios/profile/config.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace helios::profile {

/// @brief RAII scope zone that delegates to the active profiling backends.
class ScopedZone {
public:
  static constexpr size_t kStorage = kZoneStorageBytes;

  /**
   * @brief Creates a new profiling zone with the given specification.
   * @param spec Zone specification
   */
  explicit ScopedZone(const ZoneSpec& spec) noexcept;

  /**
   * @brief Creates a profiling zone with a runtime activation flag.
   * @param spec Zone specification
   * @param active Whether the zone is active
   */
  ScopedZone(const ZoneSpec& spec, bool active) noexcept;
  ScopedZone(const ScopedZone&) = delete;
  ScopedZone(ScopedZone&&) = delete;
  ~ScopedZone() noexcept;

  ScopedZone& operator=(const ScopedZone&) = delete;
  ScopedZone& operator=(ScopedZone&&) = delete;

  /**
   * @brief Updates the zone's text.
   * @param text The new text to associate with the zone
   */
  void SetText(std::string_view text) noexcept;

  /**
   * @brief Updates the zone's value.
   * @param value The new value to associate with the zone
   */
  void SetValue(uint64_t value) noexcept;

  /**
   * @brief Updates the zone's name.
   * @param name The new name to associate with the zone
   */
  void SetName(std::string_view name) noexcept;

  /**
   * @brief Retrieves the current zone storage span.
   * @return Storage span for the active zone, or empty if none
   */
  [[nodiscard]] static auto CurrentZoneStorage() noexcept
      -> std::span<std::byte>;

private:
  alignas(std::max_align_t) std::array<std::byte, kStorage> storage_;
  std::span<std::byte> previous_storage_;
  bool previous_annotations_suppressed_ = false;
  bool active_ = true;
};

}  // namespace helios::profile
