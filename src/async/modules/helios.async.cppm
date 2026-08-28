module;
#include <helios/assert.hpp>
#include <helios/config.hpp>
#ifndef HELIOS_ENABLE_IMPORT_STD
#include <algorithm>
#include <array>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <future>
#include <optional>
#include <ostream>
#include <ranges>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>
#endif

#include <taskflow/algorithm/for_each.hpp>
#include <taskflow/algorithm/reduce.hpp>
#include <taskflow/algorithm/sort.hpp>
#include <taskflow/algorithm/transform.hpp>
#include <taskflow/core/async_task.hpp>
#include <taskflow/core/executor.hpp>
#include <taskflow/core/flow_builder.hpp>
#include <taskflow/core/graph.hpp>
#include <taskflow/core/task.hpp>
#include <taskflow/core/taskflow.hpp>
#include <taskflow/observer/tfprof.hpp>

export module helios.async;
#ifdef HELIOS_ENABLE_IMPORT_STD
import std;
#endif
import helios.core;
#ifdef HELIOS_MODULE_PROFILE_AVAILABLE
import helios.profile;
#endif
HELIOS_BEGIN_MODULE_EXPORT
#include <helios/async/async.hpp>
HELIOS_END_MODULE_EXPORT
