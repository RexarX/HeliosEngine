#pragma once

#if defined(HELIOS_MEMORY_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
#define HELIOS_ENABLE_PROFILE
#include <helios/profile/details/memory_dispatch.hpp>
#include <helios/profile/macros.hpp>

#ifdef HELIOS_PROFILE_BUNDLE_TRACY
#include <helios/profile/tracy/lock.hpp>
#endif

#define HELIOS_MEMORY_PROFILE_SCOPE() HELIOS_PROFILE_SCOPE()
#define HELIOS_MEMORY_PROFILE_SCOPE_N(name) HELIOS_PROFILE_SCOPE_N(name)
#define HELIOS_MEMORY_PROFILE_SCOPE_IF(name, active) \
  HELIOS_PROFILE_SCOPE_IF(name, active)
#define HELIOS_MEMORY_PROFILE_ALLOC(ptr, size, name) \
  HELIOS_PROFILE_ALLOC_N((ptr), (size), (name))
#define HELIOS_MEMORY_PROFILE_FREE(ptr, name) \
  HELIOS_PROFILE_FREE_N((ptr), (name))
#define HELIOS_MEMORY_PROFILE_DISCARD(name) HELIOS_PROFILE_MEMORY_DISCARD(name)
#define HELIOS_MEMORY_PROFILE_ZONE_VALUE(value) HELIOS_PROFILE_ZONE_VALUE(value)

#define HELIOS_MEMORY_GLOBAL_ALLOC_PROFILE_ALLOC(ptr, size)              \
  do {                                                                   \
    if ((ptr) != nullptr &&                                              \
        ::helios::profile::details::IsProfilerFinalized() &&             \
        ::helios::profile::details::IsProfilerMemoryDispatchEnabled() && \
        !::helios::profile::details::IsMemoryDispatchSuspended()) {      \
      HELIOS_MEMORY_PROFILE_ALLOC((ptr), (size), "global");              \
    }                                                                    \
  } while (false)

#define HELIOS_MEMORY_GLOBAL_ALLOC_PROFILE_FREE(ptr)                     \
  do {                                                                   \
    if ((ptr) != nullptr &&                                              \
        ::helios::profile::details::IsProfilerFinalized() &&             \
        ::helios::profile::details::IsProfilerMemoryDispatchEnabled() && \
        !::helios::profile::details::IsMemoryDispatchSuspended()) {      \
      HELIOS_MEMORY_PROFILE_FREE((ptr), "global");                       \
    }                                                                    \
  } while (false)

#ifdef HELIOS_PROFILE_BUNDLE_TRACY

#define HELIOS_MEMORY_PROFILE_LOCKABLE(type, var) \
  HELIOS_PROFILE_LOCKABLE(type, var)
#define HELIOS_MEMORY_PROFILE_LOCKABLE_BASE(type) \
  HELIOS_PROFILE_LOCKABLE_BASE(type)
#define HELIOS_MEMORY_PROFILE_SHARED_LOCKABLE(type, var) \
  HELIOS_PROFILE_SHARED_LOCKABLE(type, var)
#define HELIOS_MEMORY_PROFILE_SHARED_LOCKABLE_BASE(type) \
  HELIOS_PROFILE_SHARED_LOCKABLE_BASE(type)
#define HELIOS_MEMORY_PROFILE_LOCK_NAME(lock, name) \
  HELIOS_PROFILE_LOCK_NAME(lock, name)
#define HELIOS_MEMORY_PROFILE_LOCK_MARK(lock) HELIOS_PROFILE_LOCK_MARK(lock)

#else

#define HELIOS_MEMORY_PROFILE_LOCKABLE(type, var) type var
#define HELIOS_MEMORY_PROFILE_LOCKABLE_BASE(type) type
#define HELIOS_MEMORY_PROFILE_SHARED_LOCKABLE(type, var) type var
#define HELIOS_MEMORY_PROFILE_SHARED_LOCKABLE_BASE(type) type
#define HELIOS_MEMORY_PROFILE_LOCK_NAME(lock, name)
#define HELIOS_MEMORY_PROFILE_LOCK_MARK(lock)

#endif

#undef HELIOS_ENABLE_PROFILE

#else

#define HELIOS_MEMORY_PROFILE_SCOPE()
#define HELIOS_MEMORY_PROFILE_SCOPE_N(name)
#define HELIOS_MEMORY_PROFILE_SCOPE_IF(name, active)
#define HELIOS_MEMORY_PROFILE_ALLOC(ptr, size, name)
#define HELIOS_MEMORY_PROFILE_FREE(ptr, name)
#define HELIOS_MEMORY_PROFILE_DISCARD(name)
#define HELIOS_MEMORY_PROFILE_ZONE_VALUE(value)
#define HELIOS_MEMORY_GLOBAL_ALLOC_PROFILE_ALLOC(ptr, size)
#define HELIOS_MEMORY_GLOBAL_ALLOC_PROFILE_FREE(ptr)
#define HELIOS_MEMORY_PROFILE_LOCKABLE(type, var) type var
#define HELIOS_MEMORY_PROFILE_LOCKABLE_BASE(type) type
#define HELIOS_MEMORY_PROFILE_SHARED_LOCKABLE(type, var) type var
#define HELIOS_MEMORY_PROFILE_SHARED_LOCKABLE_BASE(type) type
#define HELIOS_MEMORY_PROFILE_LOCK_NAME(lock, name)
#define HELIOS_MEMORY_PROFILE_LOCK_MARK(lock)

#endif
