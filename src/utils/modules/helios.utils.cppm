module;
#include <helios/compiler/compiler.hpp>
#include <helios/config.hpp>
#include <helios/platform/platform.hpp>
#ifndef HELIOS_ENABLE_IMPORT_STD
#include "../src/pch.hpp"
#else
#include <xoshiro.h>

#ifdef HELIOS_PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(HELIOS_PLATFORM_LINUX) || defined(HELIOS_PLATFORM_MACOS)
#include <dlfcn.h>
#endif
#endif

export module helios.utils;
#ifdef HELIOS_ENABLE_IMPORT_STD
import std;
#endif
import helios.compiler;
import helios.platform;
HELIOS_BEGIN_MODULE_EXPORT
#include <helios/utils/utils.hpp>
HELIOS_END_MODULE_EXPORT
