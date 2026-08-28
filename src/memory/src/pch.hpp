#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <expected>
#include <format>
#include <limits>
#include <memory>
#include <memory_resource>
#include <mutex>
#include <new>
#include <ostream>
#include <shared_mutex>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

#if defined(HELIOS_MEMORY_USE_MIMALLOC) && !defined(__SANITIZE_ADDRESS__)
#include <mimalloc.h>
#endif

#if defined(HELIOS_MEMORY_ENABLE_PROFILE) &&    \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE) && \
    defined(HELIOS_PROFILE_BUNDLE_TRACY)
#ifndef TRACY_CALLSTACK
#define TRACY_CALLSTACK 0
#endif
#define HELIOS_ENABLE_PROFILE
#include <helios/profile/tracy/lock.hpp>
#undef HELIOS_ENABLE_PROFILE
#endif
