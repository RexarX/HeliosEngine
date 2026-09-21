#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.profile;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/cstring_view.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <source_location>
#include <span>
#include <string_view>
#endif
#include <helios/profile/backend.hpp>
#include <helios/profile/common.hpp>

HELIOS_MODULE_EXPORT
namespace helios::profile {

/// @brief Tracy profiler backend adapter.
class TracyBackend final : public Backend {
public:
  [[nodiscard]] size_t ZoneStorageSize() const noexcept override;

  void BeginZone(const ZoneSpec& spec,
                 std::span<std::byte> storage) noexcept override;

  void EndZone(std::span<std::byte> storage) noexcept override;

  void ZoneText(std::span<std::byte> storage,
                std::string_view text) noexcept override;

  void ZoneValue(std::span<std::byte> storage,
                 uint64_t value) noexcept override;

  void ZoneName(std::span<std::byte> storage,
                std::string_view name) noexcept override;

  void FrameMark() noexcept override;

  void FrameMark(CStringView name) noexcept override;

  void FrameMarkStart(CStringView name) noexcept override;

  void FrameMarkEnd(CStringView name) noexcept override;

  void Message(std::string_view text, uint32_t color) noexcept override;

  void SetThreadName(CStringView name) noexcept override;

  void Plot(CStringView name, double value) noexcept override;

  void PlotConfig(CStringView name, PlotFormat type, bool step, bool fill,
                  uint32_t color) noexcept override;

  void Alloc(const void* ptr, size_t size, std::optional<CStringView> name,
             int depth,
             const std::source_location& loc =
                 std::source_location::current()) noexcept override;

  void Free(const void* ptr, std::optional<CStringView> name, int depth,
            const std::source_location& loc =
                std::source_location::current()) noexcept override;

  void MemoryDiscard(CStringView name) noexcept override;

  void MemoryDiscard(CStringView name, int depth) noexcept override;

  [[nodiscard]] std::string_view Name() const noexcept override {
    return "tracy";
  }
};

}  // namespace helios::profile
#endif  // HELIOS_MODULE_CONSUMER_SHIM
