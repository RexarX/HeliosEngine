#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <concepts>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <format>
#include <functional>
#include <future>
#include <memory>
#include <memory_resource>
#include <mutex>
#include <optional>
#include <ostream>
#include <ranges>
#include <ratio>
#include <source_location>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(HELIOS_APP_ENABLE_PROFILE) &&       \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE) && \
    defined(HELIOS_PROFILE_BUNDLE_TRACY)
#ifndef TRACY_CALLSTACK
#define TRACY_CALLSTACK 0
#endif
#define HELIOS_ENABLE_PROFILE
#include <helios/profile/tracy/lock.hpp>
#undef HELIOS_ENABLE_PROFILE
#endif
