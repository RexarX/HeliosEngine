#include <pch.hpp>

#include <helios/profile/zone.hpp>

#include <helios/profile/profiler.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace helios::profile {

namespace {

thread_local std::span<std::byte> g_current_zone_storage = {};
thread_local bool g_zone_annotations_suppressed = false;

}  // namespace

ScopedZone::ScopedZone(const ZoneSpec& spec) noexcept
    : ScopedZone(spec, spec.active) {}

ScopedZone::ScopedZone(const ZoneSpec& spec, bool active) noexcept
    : active_(active && Profiler::Instance().Finalized()) {
  previous_storage_ = g_current_zone_storage;
  previous_annotations_suppressed_ = g_zone_annotations_suppressed;
  if (!active_) [[unlikely]] {
    g_current_zone_storage = {};
    g_zone_annotations_suppressed = true;
    return;
  }

  const auto storage = std::span<std::byte>{storage_};
  Profiler::Instance().BeginZone(spec, storage);
  g_current_zone_storage = storage;
}

ScopedZone::~ScopedZone() noexcept {
  if (!active_) [[unlikely]] {
    g_current_zone_storage = previous_storage_;
    g_zone_annotations_suppressed = previous_annotations_suppressed_;
    return;
  }

  const auto storage = std::span<std::byte>{storage_};
  Profiler::Instance().EndZone(storage);
  g_current_zone_storage = previous_storage_;
  g_zone_annotations_suppressed = previous_annotations_suppressed_;
}

void ScopedZone::SetText(std::string_view text) noexcept {
  if (!active_) [[unlikely]] {
    return;
  }
  Profiler::Instance().ZoneText(storage_, text);
}

void ScopedZone::SetValue(uint64_t value) noexcept {
  if (!active_) [[unlikely]] {
    return;
  }
  Profiler::Instance().ZoneValue(storage_, value);
}

void ScopedZone::SetName(std::string_view name) noexcept {
  if (!active_) [[unlikely]] {
    return;
  }
  Profiler::Instance().ZoneName(storage_, name);
}

auto ScopedZone::CurrentZoneStorage() noexcept -> std::span<std::byte> {
  if (g_zone_annotations_suppressed) {
    return {};
  }
  return g_current_zone_storage;
}

}  // namespace helios::profile
