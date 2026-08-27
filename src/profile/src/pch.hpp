#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <new>
#include <optional>
#include <ostream>
#include <source_location>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <variant>

#include <concurrentqueue/moodycamel/concurrentqueue.h>

#ifdef HELIOS_PROFILE_BUNDLE_TRACY
#ifndef TRACY_CALLSTACK
#define TRACY_CALLSTACK 0
#endif
#include <client/TracyProfiler.hpp>
#include <client/TracyScoped.hpp>
#include <client/TracyThread.hpp>
#endif
