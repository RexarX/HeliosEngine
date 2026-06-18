#include <pch.hpp>

#include <helios/profile/profiler.hpp>

#include <helios/assert.hpp>
#include <helios/profile/zone.hpp>

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <source_location>
#include <span>
#include <string_view>

namespace helios::profile {

namespace {

thread_local int g_memory_dispatch_suspend_count = 0;
std::atomic<bool> g_profiler_memory_dispatch_enabled{true};
std::atomic<bool> g_profiler_finalized{false};

[[nodiscard]] constexpr size_t AlignUp(size_t value,
                                       size_t alignment) noexcept {
  return (value + alignment - 1) & ~(alignment - 1);
}

void RegisterShutdownMemoryDispatchGuard() noexcept {
  static bool registered = false;
  if (registered) {
    return;
  }
  registered = true;
  std::atexit(+[]() noexcept { details::DisableProfilerMemoryDispatch(); });
}

}  // namespace

namespace details {

bool IsMemoryDispatchSuspended() noexcept {
  return g_memory_dispatch_suspend_count > 0;
}

bool IsProfilerFinalized() noexcept {
  return g_profiler_finalized.load(std::memory_order_acquire);
}

bool IsProfilerMemoryDispatchEnabled() noexcept {
  return g_profiler_memory_dispatch_enabled.load(std::memory_order_acquire);
}

void DisableProfilerMemoryDispatch() noexcept {
  g_profiler_memory_dispatch_enabled.store(false, std::memory_order_release);
}

void ResetProfilerFinalized() noexcept {
  g_profiler_finalized.store(false, std::memory_order_release);
}

ScopedMemoryDispatchSuspend::ScopedMemoryDispatchSuspend() noexcept {
  ++g_memory_dispatch_suspend_count;
}

ScopedMemoryDispatchSuspend::~ScopedMemoryDispatchSuspend() noexcept {
  --g_memory_dispatch_suspend_count;
}

}  // namespace details

Profiler::~Profiler() noexcept {
  details::DisableProfilerMemoryDispatch();
}

void Profiler::Startup() noexcept {
  if (!finalized_) [[unlikely]] {
    return;
  }

  for (auto&& [_, entry] : backends_) {
    entry.backend->Startup();
  }
}

void Profiler::Shutdown() noexcept {
  if (!finalized_) [[unlikely]] {
    return;
  }

  for (auto&& [_, entry] : backends_) {
    entry.backend->Shutdown();
  }
}

void Profiler::Finalize() noexcept {
  HELIOS_ASSERT(!finalized_, "Profiler already finalized!");

  size_t offset = 0;
  for (auto&& [_, entry] : backends_) {
    offset = AlignUp(offset, alignof(std::max_align_t));
    entry.storage_offset = offset;
    entry.storage_size = entry.backend->ZoneStorageSize();
    HELIOS_ASSERT(
        entry.storage_size <= kZoneStorageBytes,
        "Backend '{}' ZoneStorageSize ({}) exceeds kZoneStorageBytes ({})!",
        entry.backend->Name(), entry.storage_size, kZoneStorageBytes);
    offset += entry.storage_size;
  }

  HELIOS_ASSERT(
      offset <= ScopedZone::kStorage,
      "Combined backend storage {} bytes exceeds ScopedZone::kStorage {} bytes "
      "— raise HELIOS_PROFILE_ZONE_STORAGE_BYTES!",
      offset, ScopedZone::kStorage);

  finalized_ = true;
  g_profiler_finalized.store(true, std::memory_order_release);
  RegisterShutdownMemoryDispatchGuard();
}

void Profiler::BeginZone(const ZoneSpec& spec,
                         std::span<std::byte> storage) noexcept {
  if (!finalized_) [[unlikely]] {
    return;
  }

  for (auto&& [_, entry] : backends_) {
    entry.backend->BeginZone(
        spec, storage.subspan(entry.storage_offset, entry.storage_size));
  }
}

void Profiler::EndZone(std::span<std::byte> storage) noexcept {
  if (!finalized_) [[unlikely]] {
    return;
  }

  for (auto&& [_, entry] : backends_) {
    entry.backend->EndZone(
        storage.subspan(entry.storage_offset, entry.storage_size));
  }
}

void Profiler::ZoneText(std::span<std::byte> storage,
                        std::string_view text) noexcept {
  if (!finalized_) [[unlikely]] {
    return;
  }

  for (auto&& [_, entry] : backends_) {
    entry.backend->ZoneText(
        storage.subspan(entry.storage_offset, entry.storage_size), text);
  }
}

void Profiler::ZoneValue(std::span<std::byte> storage,
                         uint64_t value) noexcept {
  if (!finalized_) [[unlikely]] {
    return;
  }

  for (auto&& [_, entry] : backends_) {
    entry.backend->ZoneValue(
        storage.subspan(entry.storage_offset, entry.storage_size), value);
  }
}

void Profiler::ZoneName(std::span<std::byte> storage,
                        std::string_view name) noexcept {
  if (!finalized_) [[unlikely]] {
    return;
  }

  for (auto&& [_, entry] : backends_) {
    entry.backend->ZoneName(
        storage.subspan(entry.storage_offset, entry.storage_size), name);
  }
}

void Profiler::FrameMark() noexcept {
  if (!finalized_) [[unlikely]] {
    return;
  }

  for (auto&& [_, entry] : backends_) {
    entry.backend->FrameMark();
  }
}

void Profiler::FrameMark(CStringView name) noexcept {
  if (!finalized_) [[unlikely]] {
    return;
  }

  for (auto&& [_, entry] : backends_) {
    entry.backend->FrameMark(name);
  }
}

void Profiler::FrameMarkStart(CStringView name) noexcept {
  if (!finalized_) [[unlikely]] {
    return;
  }

  for (auto&& [_, entry] : backends_) {
    entry.backend->FrameMarkStart(name);
  }
}

void Profiler::FrameMarkEnd(CStringView name) noexcept {
  if (!finalized_) [[unlikely]] {
    return;
  }

  for (auto&& [_, entry] : backends_) {
    entry.backend->FrameMarkEnd(name);
  }
}

void Profiler::Message(std::string_view text, uint32_t color) noexcept {
  if (!finalized_) [[unlikely]] {
    return;
  }

  for (auto&& [_, entry] : backends_) {
    entry.backend->Message(text, color);
  }
}

void Profiler::SetThreadName(CStringView name) noexcept {
  if (!finalized_) [[unlikely]] {
    return;
  }

  for (auto&& [_, entry] : backends_) {
    entry.backend->SetThreadName(name);
  }
}

void Profiler::Plot(CStringView name, double value) noexcept {
  if (!finalized_) [[unlikely]] {
    return;
  }

  for (auto&& [_, entry] : backends_) {
    entry.backend->Plot(name, value);
  }
}

void Profiler::PlotConfig(CStringView name, PlotFormat type, bool step,
                          bool fill, uint32_t color) noexcept {
  if (!finalized_) [[unlikely]] {
    return;
  }

  for (auto&& [_, entry] : backends_) {
    entry.backend->PlotConfig(name, type, step, fill, color);
  }
}

void Profiler::Alloc(const void* ptr, size_t size,
                     std::optional<CStringView> name, int depth,
                     std::source_location location) noexcept {
  if (!finalized_ || !details::IsProfilerMemoryDispatchEnabled() ||
      details::IsMemoryDispatchSuspended()) [[unlikely]] {
    return;
  }

  for (auto&& [_, entry] : backends_) {
    entry.backend->Alloc(ptr, size, name, depth, location);
  }
}

void Profiler::Free(const void* ptr, std::optional<CStringView> name, int depth,
                    std::source_location location) noexcept {
  if (!finalized_ || !details::IsProfilerMemoryDispatchEnabled() ||
      details::IsMemoryDispatchSuspended()) [[unlikely]] {
    return;
  }

  for (auto&& [_, entry] : backends_) {
    entry.backend->Free(ptr, name, depth, location);
  }
}

void Profiler::MemoryDiscard(CStringView name) noexcept {
  if (!finalized_ || !details::IsProfilerMemoryDispatchEnabled()) [[unlikely]] {
    return;
  }

  for (auto&& [_, entry] : backends_) {
    entry.backend->MemoryDiscard(name);
  }
}

void Profiler::MemoryDiscard(CStringView name, int depth) noexcept {
  if (!finalized_ || !details::IsProfilerMemoryDispatchEnabled()) [[unlikely]] {
    return;
  }

  for (auto&& [_, entry] : backends_) {
    entry.backend->MemoryDiscard(name, depth);
  }
}

}  // namespace helios::profile
