#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <format>
#include <functional>
#include <iterator>
#include <source_location>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#if defined(__cpp_lib_print) && (__cpp_lib_print >= 202302L)
#include <print>
#endif

#ifdef HELIOS_USE_STL_STACKTRACE
#include <stacktrace>
#else
#include <boost/stacktrace.hpp>
#endif
