#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.async;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#include <helios/async/async_task.hpp>
#include <helios/async/common.hpp>
#include <helios/async/executor.hpp>
#include <helios/async/future.hpp>
#include <helios/async/sub_task_graph.hpp>
#include <helios/async/task.hpp>
#include <helios/async/task_graph.hpp>
#endif  // HELIOS_MODULE_CONSUMER_SHIM
