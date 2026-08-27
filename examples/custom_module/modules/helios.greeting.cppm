module;
#include <helios/compiler/compiler.hpp>
#include <helios/config.hpp>
#include <helios/platform/platform.hpp>
#include <helios/utils/macro.hpp>
#ifndef HELIOS_ENABLE_IMPORT_STD
#include <string>
#endif

export module helios.greeting;
#ifdef HELIOS_ENABLE_IMPORT_STD
import std;
#endif
import helios.core;
import helios.utils;
HELIOS_BEGIN_MODULE_EXPORT
#include <helios/greeting/greeting.hpp>
HELIOS_END_MODULE_EXPORT
