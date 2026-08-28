module;
#include <helios/compiler/compiler.hpp>
#include <helios/config.hpp>
#ifndef HELIOS_ENABLE_IMPORT_STD
#include "../src/pch.hpp"
#endif
#include <taskflow/core/async_task.hpp>
#include <taskflow/core/executor.hpp>
#include <taskflow/core/flow_builder.hpp>
#include <taskflow/core/graph.hpp>
#include <taskflow/core/task.hpp>
#include <taskflow/core/taskflow.hpp>
#include <taskflow/observer/tfprof.hpp>
#ifndef HELIOS_ENABLE_IMPORT_STD
#include <ranges>
#include <utility>
#endif
#if defined(HELIOS_APP_ENABLE_PROFILE) &&       \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE) && \
    defined(HELIOS_PROFILE_BUNDLE_TRACY)
#ifndef TRACY_CALLSTACK
#define TRACY_CALLSTACK 0
#endif
#define HELIOS_ENABLE_PROFILE
#include <helios/profile/tracy/lock.hpp>
#undef HELIOS_ENABLE_PROFILE
#endif

export module helios.app;
#ifdef HELIOS_ENABLE_IMPORT_STD
import std;
#endif
import helios.async;
import helios.compiler;
import helios.container;
import helios.core;
import helios.ecs;
import helios.log;
import helios.memory;
import helios.utils;
#ifdef HELIOS_MODULE_PROFILE_AVAILABLE
import helios.profile;
#endif
HELIOS_BEGIN_MODULE_EXPORT
#include <helios/app/app.hpp>
HELIOS_END_MODULE_EXPORT
