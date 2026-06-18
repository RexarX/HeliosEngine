#include <pch.hpp>

#include <helios/profile/backends/tracy.hpp>

#include <helios/profile/config.hpp>

#ifndef TRACY_CALLSTACK
#define TRACY_CALLSTACK 0
#endif

#include <client/TracyProfiler.hpp>
#include <client/TracyScoped.hpp>
#include <client/TracyThread.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <new>
#include <source_location>
#include <string_view>

namespace helios::profile {

namespace {

constexpr size_t kTracyZoneStorageSize = sizeof(tracy::ScopedZone);

static_assert(kTracyZoneStorageSize <= kZoneStorageBytes,
              "Tracy ScopedZone exceeds HELIOS_PROFILE_ZONE_STORAGE_BYTES");

[[nodiscard]] tracy::PlotFormatType ToTracyPlotFormat(
    PlotFormat format) noexcept {
  switch (format) {
    case PlotFormat::kNumber:
      return tracy::PlotFormatType::Number;
    case PlotFormat::kMemory:
      return tracy::PlotFormatType::Memory;
    case PlotFormat::kPercentage:
      return tracy::PlotFormatType::Percentage;
    case PlotFormat::kWatts:
      return tracy::PlotFormatType::Number;
  }
  return tracy::PlotFormatType::Number;
}

[[nodiscard]] tracy::ScopedZone* AsTracyZone(
    std::span<std::byte> storage) noexcept {
  return std::launder(reinterpret_cast<tracy::ScopedZone*>(storage.data()));
}

}  // namespace

size_t TracyBackend::ZoneStorageSize() const noexcept {
  return kTracyZoneStorageSize;
}

void TracyBackend::BeginZone(const ZoneSpec& spec,
                             std::span<std::byte> storage) noexcept {
  const char* const name_ptr = spec.name.empty() ? nullptr : spec.name.data();
  const size_t name_size = spec.name.size();
  const char* const function = spec.loc.function_name();
  const char* const file = spec.loc.file_name();
  const auto line = spec.loc.line();

  auto* zone = AsTracyZone(storage);
  std::construct_at(zone, line, file, std::strlen(file), function,
                    std::strlen(function), name_ptr, name_size, spec.color,
                    spec.callstack_depth, spec.active);
}

void TracyBackend::EndZone(std::span<std::byte> storage) noexcept {
  std::destroy_at(AsTracyZone(storage));
}

void TracyBackend::ZoneText(std::span<std::byte> storage,
                            std::string_view text) noexcept {
  AsTracyZone(storage)->Text(text.data(), text.size());
}

void TracyBackend::ZoneValue(std::span<std::byte> storage,
                             uint64_t value) noexcept {
  AsTracyZone(storage)->Value(value);
}

void TracyBackend::ZoneName(std::span<std::byte> storage,
                            std::string_view name) noexcept {
  AsTracyZone(storage)->Name(name.data(), name.size());
}

void TracyBackend::FrameMark() noexcept {
  tracy::Profiler::SendFrameMark(nullptr);
}

void TracyBackend::FrameMark(CStringView name) noexcept {
  tracy::Profiler::SendFrameMark(name.CStr());
}

void TracyBackend::FrameMarkStart(CStringView name) noexcept {
  tracy::Profiler::SendFrameMark(name.CStr(),
                                 tracy::QueueType::FrameMarkMsgStart);
}

void TracyBackend::FrameMarkEnd(CStringView name) noexcept {
  tracy::Profiler::SendFrameMark(name.CStr(),
                                 tracy::QueueType::FrameMarkMsgEnd);
}

void TracyBackend::Message(std::string_view text, uint32_t color) noexcept {
  if (color == 0) {
    tracy::Profiler::Message(text.data(), text.size(), TRACY_CALLSTACK);
  } else {
    tracy::Profiler::MessageColor(text.data(), text.size(), color,
                                  TRACY_CALLSTACK);
  }
}

void TracyBackend::SetThreadName(CStringView name) noexcept {
  tracy::SetThreadName(name.CStr());
}

void TracyBackend::Plot(CStringView name, double value) noexcept {
  tracy::Profiler::PlotData(name.CStr(), value);
}

void TracyBackend::PlotConfig(CStringView name, PlotFormat type, bool step,
                              bool fill, uint32_t color) noexcept {
  tracy::Profiler::ConfigurePlot(name.CStr(), ToTracyPlotFormat(type), step,
                                 fill, color);
}

void TracyBackend::Alloc(const void* ptr, size_t size,
                         std::optional<CStringView> name, int depth,
                         std::source_location /*loc*/) noexcept {
  if (!name.has_value()) {
    if (depth > 0) {
      tracy::Profiler::MemAllocCallstack(ptr, size, depth, false);
    } else {
      tracy::Profiler::MemAllocCallstack(ptr, size, TRACY_CALLSTACK, false);
    }
    return;
  }
  if (depth > 0) {
    tracy::Profiler::MemAllocCallstackNamed(ptr, size, depth, false,
                                            name->CStr());
  } else if (TRACY_CALLSTACK > 0) {
    tracy::Profiler::MemAllocCallstackNamed(ptr, size, TRACY_CALLSTACK, false,
                                            name->CStr());
  } else {
    tracy::Profiler::MemAllocNamed(ptr, size, false, name->CStr());
  }
}

void TracyBackend::Free(const void* ptr, std::optional<CStringView> name,
                        int depth, std::source_location /*loc*/) noexcept {
  if (!name.has_value()) {
    if (depth > 0) {
      tracy::Profiler::MemFreeCallstack(ptr, depth, false);
    } else {
      tracy::Profiler::MemFreeCallstack(ptr, TRACY_CALLSTACK, false);
    }
    return;
  }
  if (depth > 0) {
    tracy::Profiler::MemFreeCallstackNamed(ptr, depth, false, name->CStr());
  } else if (TRACY_CALLSTACK > 0) {
    tracy::Profiler::MemFreeCallstackNamed(ptr, TRACY_CALLSTACK, false,
                                           name->CStr());
  } else {
    tracy::Profiler::MemFreeNamed(ptr, false, name->CStr());
  }
}

void TracyBackend::MemoryDiscard(CStringView name) noexcept {
  if (TRACY_CALLSTACK > 0) {
    tracy::Profiler::MemDiscardCallstack(name.CStr(), false, TRACY_CALLSTACK);
  } else {
    tracy::Profiler::MemDiscard(name.CStr(), false);
  }
}

void TracyBackend::MemoryDiscard(CStringView name, int depth) noexcept {
  tracy::Profiler::MemDiscardCallstack(name.CStr(), false, depth);
}

}  // namespace helios::profile
