module;
#include <helios/compiler/compiler.hpp>
#include <helios/config.hpp>
#include <helios/platform/platform.hpp>
#include <helios/utils/macro.hpp>
#ifndef HELIOS_ENABLE_IMPORT_STD
#include "../src/pch.hpp"
#else
#ifdef HELIOS_USE_STL_STACKTRACE
#include <stacktrace>
#else
#include <boost/stacktrace.hpp>
#endif
#include <uuid.h>
#endif

export module helios.core;
#ifdef HELIOS_ENABLE_IMPORT_STD
import std;
#endif
import helios.compiler;
import helios.platform;
import helios.utils;
HELIOS_BEGIN_MODULE_EXPORT
#include <helios/core/core.hpp>
HELIOS_END_MODULE_EXPORT
