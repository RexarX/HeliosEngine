#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <ios>
#include <iterator>
#include <limits>
#include <memory>
#include <memory_resource>
#include <new>
#include <optional>
#include <ostream>
#include <random>
#include <ranges>
#include <ratio>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

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
#include <cerrno>
#include <ctime>
#endif
