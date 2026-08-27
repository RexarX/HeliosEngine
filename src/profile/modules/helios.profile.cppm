module;
#include <helios/assert.hpp>
#include <helios/compiler/compiler.hpp>
#include <helios/config.hpp>
#include <helios/platform/platform.hpp>
#include <helios/utils/macro.hpp>
#ifndef HELIOS_ENABLE_IMPORT_STD
#include "../src/pch.hpp"
#else
#ifdef HELIOS_PROFILE_BUNDLE_TRACY
#include <client/TracyProfiler.hpp>
#include <client/TracyScoped.hpp>
#include <client/TracyThread.hpp>
#endif
#include <concurrentqueue/moodycamel/concurrentqueue.h>
#endif

export module helios.profile;
#ifdef HELIOS_ENABLE_IMPORT_STD
import std;
#endif
import helios.compiler;
import helios.container;
import helios.core;
import helios.platform;
import helios.utils;
HELIOS_BEGIN_MODULE_EXPORT
#include <helios/profile/backends/flamegraph.hpp>
#include <helios/profile/profile.hpp>
#ifdef HELIOS_PROFILE_BUNDLE_TRACY
#include <helios/profile/backends/tracy.hpp>
#include <helios/profile/tracy/lock.hpp>
#endif
HELIOS_END_MODULE_EXPORT
