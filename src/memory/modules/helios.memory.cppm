module;
#include <helios/assert.hpp>
#include <helios/config.hpp>
#ifndef HELIOS_ENABLE_IMPORT_STD
#include "../src/pch.hpp"
#else
#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif
#if defined(HELIOS_MEMORY_USE_MIMALLOC) && !defined(__SANITIZE_ADDRESS__)
#include <mimalloc.h>
#endif
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

export module helios.memory;
#ifdef HELIOS_ENABLE_IMPORT_STD
import std;
#endif
import helios.core;
import helios.platform;
#ifdef HELIOS_MODULE_PROFILE_AVAILABLE
import helios.profile;
#endif
HELIOS_BEGIN_MODULE_EXPORT
#include <helios/memory/memory.hpp>
HELIOS_END_MODULE_EXPORT
