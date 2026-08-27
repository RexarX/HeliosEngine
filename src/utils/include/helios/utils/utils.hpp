#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.utils;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#include <helios/utils/common_traits.hpp>
#include <helios/utils/defer.hpp>
#include <helios/utils/dynamic_library.hpp>
#include <helios/utils/fast_pimpl.hpp>
#include <helios/utils/filesystem.hpp>
#include <helios/utils/format.hpp>
#include <helios/utils/functional_adapters.hpp>
#include <helios/utils/hash.hpp>
#include <helios/utils/macro.hpp>
#include <helios/utils/random.hpp>
#include <helios/utils/sleep.hpp>
#include <helios/utils/string_hash.hpp>
#include <helios/utils/timer.hpp>
#include <helios/utils/type_info.hpp>
#endif  // HELIOS_MODULE_CONSUMER_SHIM
