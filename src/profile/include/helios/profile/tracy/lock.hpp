/**
 * @file tracy/lock.hpp
 * @brief Tracy-specific mutex instrumentation macros.
 *
 * IMPORTANT: Lock profiling is Tracy-specific and bypasses the Profiler
 * backend abstraction. These macros directly expand to Tracy's LockableBase /
 * TracyLockable wrappers and cannot fan-out to other backends. Include this
 * file only if Tracy is your primary profiling backend and you need lock
 * contention visualization.
 *
 * Do NOT include this from profile.hpp or any header included by default.
 */
#pragma once

#ifdef HELIOS_ENABLE_PROFILE

#include <tracy/Tracy.hpp>

// Tracy zone macros collide with helios::profile::Backend method names.
#ifdef ZoneText
#undef ZoneText
#endif
#ifdef ZoneValue
#undef ZoneValue
#endif
#ifdef ZoneName
#undef ZoneName
#endif
#ifdef ZoneColor
#undef ZoneColor
#endif
#ifdef FrameMark
#undef FrameMark
#endif
#ifdef FrameMarkNamed
#undef FrameMarkNamed
#endif
#ifdef FrameMarkStart
#undef FrameMarkStart
#endif
#ifdef FrameMarkEnd
#undef FrameMarkEnd
#endif

#define HELIOS_PROFILE_LOCKABLE(type, var) TracyLockable(type, var)
#define HELIOS_PROFILE_LOCKABLE_BASE(type) LockableBase(type)
#define HELIOS_PROFILE_SHARED_LOCKABLE(type, var) TracySharedLockable(type, var)
#define HELIOS_PROFILE_SHARED_LOCKABLE_BASE(type) SharedLockableBase(type)
#define HELIOS_PROFILE_LOCK_MARK(lock) LockMark(lock)
#define HELIOS_PROFILE_LOCK_NAME(lock, name) \
  LockableName(lock, (name).data(), (name).size())

#else

#include <helios/utils/macro.hpp>

#define HELIOS_PROFILE_LOCKABLE(type, var) type var
#define HELIOS_PROFILE_LOCKABLE_BASE(type) type
#define HELIOS_PROFILE_SHARED_LOCKABLE(type, var) type var
#define HELIOS_PROFILE_SHARED_LOCKABLE_BASE(type) type
#define HELIOS_PROFILE_LOCK_MARK(lock) \
  [[maybe_unused]] static constexpr int HELIOS_ANONYMOUS_VAR(_helios_lock) = 0
#define HELIOS_PROFILE_LOCK_NAME(lock, name) HELIOS_PROFILE_LOCK_MARK(lock)

#endif
