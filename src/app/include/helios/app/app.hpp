#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.app;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#include <helios/app/application.hpp>
#include <helios/app/builtin/app_exit.hpp>
#include <helios/app/builtin/executor.hpp>
#include <helios/app/builtin/frame_count.hpp>
#include <helios/app/builtin/frame_limiter.hpp>
#include <helios/app/builtin/time.hpp>
#include <helios/app/dynamic_plugin.hpp>
#include <helios/app/frame_order.hpp>
#include <helios/app/plugin.hpp>
#include <helios/app/plugin_group.hpp>
#include <helios/app/runners.hpp>
#include <helios/app/scheduler.hpp>
#include <helios/app/schedules.hpp>
#include <helios/app/sub_app.hpp>
#endif  // HELIOS_MODULE_CONSUMER_SHIM
