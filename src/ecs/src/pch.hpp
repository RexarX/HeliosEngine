#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <expected>
#include <format>
#include <functional>
#include <future>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <memory_resource>
#include <optional>
#include <ostream>
#include <queue>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef HELIOS_USE_STL_FLAT_MAP
#include <flat_map>
#else
#include <boost/container/flat_map.hpp>
#endif

#include <concurrentqueue/moodycamel/concurrentqueue.h>
