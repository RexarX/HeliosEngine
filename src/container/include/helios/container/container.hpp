#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.container;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#include <helios/container/callable_buffer.hpp>
#include <helios/container/callable_buffer_array.hpp>
#include <helios/container/flat_map.hpp>
#include <helios/container/multi_type_map.hpp>
#include <helios/container/sparse_set.hpp>
#include <helios/container/static_string.hpp>
#include <helios/container/typed_buffer.hpp>
#include <helios/container/typed_buffer_array.hpp>
#endif  // HELIOS_MODULE_CONSUMER_SHIM
