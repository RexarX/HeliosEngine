#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <format>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <random>
#include <source_location>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include <uuid.h>

#if defined(__cpp_lib_print) && (__cpp_lib_print >= 202302L)
#include <print>
#endif

#ifdef HELIOS_USE_STL_STACKTRACE
#include <stacktrace>
#else
#include <boost/stacktrace.hpp>
#endif
