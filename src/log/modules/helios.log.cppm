module;
#include <helios/assert.hpp>
#include <helios/config.hpp>
#ifndef HELIOS_ENABLE_IMPORT_STD
#include "../src/pch.hpp"
#else
#include <spdlog/async.h>
#include <spdlog/logger.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#endif

export module helios.log;
#ifdef HELIOS_ENABLE_IMPORT_STD
import std;
#endif
import helios.container;
import helios.core;
import helios.utils;
HELIOS_BEGIN_MODULE_EXPORT
#include <helios/log/log.hpp>
HELIOS_END_MODULE_EXPORT
