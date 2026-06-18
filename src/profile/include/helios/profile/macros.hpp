#pragma once

#include <helios/cstring_view.hpp>
#include <helios/profile/common.hpp>
#include <helios/profile/frame.hpp>
#include <helios/profile/memory.hpp>
#include <helios/profile/message.hpp>
#include <helios/profile/plot.hpp>
#include <helios/profile/profiler.hpp>
#include <helios/profile/zone.hpp>

#include <helios/utils/macro.hpp>

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

#ifdef HELIOS_ENABLE_PROFILE

#include <source_location>
#include <string_view>

#define HELIOS_PROFILE_DETAIL_SCOPE(...)                                \
  const ::helios::profile::ZoneSpec HELIOS_ANONYMOUS_VAR(_helios_spec){ \
      __VA_ARGS__};                                                     \
  ::helios::profile::ScopedZone HELIOS_ANONYMOUS_VAR(_helios_zone)(     \
      HELIOS_ANONYMOUS_VAR(_helios_spec))

#define HELIOS_PROFILE_SCOPE()                    \
  HELIOS_PROFILE_DETAIL_SCOPE(std::string_view{}, \
                              std::source_location::current(), 0U, true, 0)

#define HELIOS_PROFILE_SCOPE_N(name)                                       \
  HELIOS_PROFILE_DETAIL_SCOPE((name), std::source_location::current(), 0U, \
                              true, 0)

#define HELIOS_PROFILE_SCOPE_C(color) \
  HELIOS_PROFILE_DETAIL_SCOPE(        \
      std::string_view{}, std::source_location::current(), (color), true, 0)

#define HELIOS_PROFILE_SCOPE_NC(name, color)                           \
  HELIOS_PROFILE_DETAIL_SCOPE((name), std::source_location::current(), \
                              (color), true, 0)

#define HELIOS_PROFILE_SCOPE_IF(name, active_expr)                      \
  const ::helios::profile::ZoneSpec HELIOS_ANONYMOUS_VAR(_helios_spec){ \
      (name), std::source_location::current(), 0U, true, 0};            \
  const bool HELIOS_ANONYMOUS_VAR(_helios_active) =                     \
      static_cast<bool>(active_expr);                                   \
  ::helios::profile::ScopedZone HELIOS_ANONYMOUS_VAR(_helios_zone)(     \
      HELIOS_ANONYMOUS_VAR(_helios_spec),                               \
      HELIOS_ANONYMOUS_VAR(_helios_active))

#define HELIOS_PROFILE_SCOPE_S(depth) \
  HELIOS_PROFILE_DETAIL_SCOPE(        \
      std::string_view{}, std::source_location::current(), 0U, true, (depth))

#define HELIOS_PROFILE_SCOPE_NS(name, depth)                               \
  HELIOS_PROFILE_DETAIL_SCOPE((name), std::source_location::current(), 0U, \
                              true, (depth))

#define HELIOS_PROFILE_ZONE_TEXT(text)                                    \
  do {                                                                    \
    if (const auto _storage =                                             \
            ::helios::profile::ScopedZone::CurrentZoneStorage();          \
        !_storage.empty()) {                                              \
      ::helios::profile::Profiler::Instance().ZoneText(_storage, (text)); \
    }                                                                     \
  } while (false)

#define HELIOS_PROFILE_ZONE_VALUE(value)                                    \
  do {                                                                      \
    if (const auto _storage =                                               \
            ::helios::profile::ScopedZone::CurrentZoneStorage();            \
        !_storage.empty()) {                                                \
      ::helios::profile::Profiler::Instance().ZoneValue(_storage, (value)); \
    }                                                                       \
  } while (false)

#define HELIOS_PROFILE_ZONE_NAME(name)                                    \
  do {                                                                    \
    if (const auto _storage =                                             \
            ::helios::profile::ScopedZone::CurrentZoneStorage();          \
        !_storage.empty()) {                                              \
      ::helios::profile::Profiler::Instance().ZoneName(_storage, (name)); \
    }                                                                     \
  } while (false)

#define HELIOS_PROFILE_FRAME() ::helios::profile::FrameMark()

#define HELIOS_PROFILE_FRAME_N(name) \
  ::helios::profile::FrameMarkNamed(::helios::CStringView{(name)})

#define HELIOS_PROFILE_FRAME_START(name) \
  ::helios::profile::FrameMarkStart(::helios::CStringView{(name)})

#define HELIOS_PROFILE_FRAME_END(name) \
  ::helios::profile::FrameMarkEnd(::helios::CStringView{(name)})

#define HELIOS_PROFILE_MESSAGE(msg) ::helios::profile::Message(msg)

#define HELIOS_PROFILE_SET_THREAD_NAME(name) \
  ::helios::profile::SetThreadName(::helios::CStringView{(name)})

#define HELIOS_PROFILE_PLOT(name, value) \
  ::helios::profile::Plot(::helios::CStringView{(name)}, (value))

#define HELIOS_PROFILE_PLOT_CONFIG(name, type, step, fill, color)              \
  ::helios::profile::PlotConfig(::helios::CStringView{(name)}, (type), (step), \
                                (fill), (color))

#define HELIOS_PROFILE_ALLOC(ptr, size) \
  ::helios::profile::Alloc((ptr), (size), std::nullopt, 0)

#define HELIOS_PROFILE_FREE(ptr) ::helios::profile::Free((ptr), std::nullopt, 0)

#define HELIOS_PROFILE_ALLOC_N(ptr, size, name) \
  ::helios::profile::Alloc((ptr), (size), ::helios::CStringView{(name)}, 0)

#define HELIOS_PROFILE_FREE_N(ptr, name) \
  ::helios::profile::Free((ptr), ::helios::CStringView{(name)}, 0)

#define HELIOS_PROFILE_ALLOC_S(ptr, size, depth) \
  ::helios::profile::Alloc((ptr), (size), std::nullopt, (depth))

#define HELIOS_PROFILE_FREE_S(ptr, depth) \
  ::helios::profile::Free((ptr), std::nullopt, (depth))

#define HELIOS_PROFILE_ALLOC_NS(ptr, size, depth, name)                  \
  ::helios::profile::Alloc((ptr), (size), ::helios::CStringView{(name)}, \
                           (depth))

#define HELIOS_PROFILE_FREE_NS(ptr, depth, name) \
  ::helios::profile::Free((ptr), ::helios::CStringView{(name)}, (depth))

#define HELIOS_PROFILE_MEMORY_DISCARD(name) \
  ::helios::profile::MemoryDiscard(::helios::CStringView{(name)})

#define HELIOS_PROFILE_MEMORY_DISCARD_S(name, depth) \
  ::helios::profile::MemoryDiscardS(::helios::CStringView{(name)}, (depth))

#else  // HELIOS_ENABLE_PROFILE

#define HELIOS_PROFILE_SCOPE() \
  [[maybe_unused]] static constexpr int HELIOS_ANONYMOUS_VAR(_helios_prof) = 0
#define HELIOS_PROFILE_SCOPE_N(name) HELIOS_PROFILE_SCOPE()
#define HELIOS_PROFILE_SCOPE_C(color) HELIOS_PROFILE_SCOPE()
#define HELIOS_PROFILE_SCOPE_NC(name, color) HELIOS_PROFILE_SCOPE()
#define HELIOS_PROFILE_SCOPE_IF(name, active) HELIOS_PROFILE_SCOPE()
#define HELIOS_PROFILE_SCOPE_S(depth) HELIOS_PROFILE_SCOPE()
#define HELIOS_PROFILE_SCOPE_NS(name, depth) HELIOS_PROFILE_SCOPE()
#define HELIOS_PROFILE_ZONE_TEXT(text) \
  [[maybe_unused]] static constexpr int HELIOS_ANONYMOUS_VAR(_helios_prof) = 0
#define HELIOS_PROFILE_ZONE_VALUE(value) HELIOS_PROFILE_ZONE_TEXT(value)
#define HELIOS_PROFILE_ZONE_NAME(name) HELIOS_PROFILE_ZONE_TEXT(name)
#define HELIOS_PROFILE_FRAME() HELIOS_PROFILE_SCOPE()
#define HELIOS_PROFILE_FRAME_N(name) HELIOS_PROFILE_SCOPE()
#define HELIOS_PROFILE_FRAME_START(name) HELIOS_PROFILE_SCOPE()
#define HELIOS_PROFILE_FRAME_END(name) HELIOS_PROFILE_SCOPE()
#define HELIOS_PROFILE_MESSAGE(msg) HELIOS_PROFILE_SCOPE()
#define HELIOS_PROFILE_SET_THREAD_NAME(name) HELIOS_PROFILE_SCOPE()
#define HELIOS_PROFILE_PLOT(name, value) HELIOS_PROFILE_SCOPE()
#define HELIOS_PROFILE_PLOT_CONFIG(name, type, step, fill, color) \
  HELIOS_PROFILE_SCOPE()
#define HELIOS_PROFILE_ALLOC(ptr, size) HELIOS_PROFILE_SCOPE()
#define HELIOS_PROFILE_FREE(ptr) HELIOS_PROFILE_SCOPE()
#define HELIOS_PROFILE_ALLOC_N(ptr, size, name) HELIOS_PROFILE_SCOPE()
#define HELIOS_PROFILE_FREE_N(ptr, name) HELIOS_PROFILE_SCOPE()
#define HELIOS_PROFILE_ALLOC_S(ptr, size, depth) HELIOS_PROFILE_SCOPE()
#define HELIOS_PROFILE_FREE_S(ptr, depth) HELIOS_PROFILE_SCOPE()
#define HELIOS_PROFILE_ALLOC_NS(ptr, size, depth, name) HELIOS_PROFILE_SCOPE()
#define HELIOS_PROFILE_FREE_NS(ptr, depth, name) HELIOS_PROFILE_SCOPE()
#define HELIOS_PROFILE_MEMORY_DISCARD(name) HELIOS_PROFILE_SCOPE()
#define HELIOS_PROFILE_MEMORY_DISCARD_S(name, depth) HELIOS_PROFILE_SCOPE()

#endif  // HELIOS_ENABLE_PROFILE

// NOLINTEND(cppcoreguidelines-macro-usage)
