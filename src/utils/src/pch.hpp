#pragma once

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <type_traits>

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
