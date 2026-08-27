module;
#include <helios/assert.hpp>
#include <helios/compiler/compiler.hpp>
#include <helios/config.hpp>
#ifndef HELIOS_ENABLE_IMPORT_STD
#include "../src/pch.hpp"
#else
#include <concurrentqueue/moodycamel/concurrentqueue.h>
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

export module helios.ecs;
#ifdef HELIOS_ENABLE_IMPORT_STD
import std;
#endif
import helios.async;
import helios.compiler;
import helios.container;
import helios.core;
import helios.log;
import helios.memory;
import helios.utils;
#ifdef HELIOS_MODULE_PROFILE_AVAILABLE
import helios.profile;
#endif
HELIOS_BEGIN_MODULE_EXPORT
#include <helios/ecs/ecs.hpp>
HELIOS_END_MODULE_EXPORT
