module;
#include <helios/assert.hpp>
#include <helios/config.hpp>
#ifndef HELIOS_ENABLE_IMPORT_STD
#include "../src/pch.hpp"
#else
#include <GLFW/glfw3.h>
#ifdef HELIOS_PLATFORM_WINDOWS
#include <windows.h>
#endif
#endif

export module helios.glfw;
#ifdef HELIOS_ENABLE_IMPORT_STD
import std;
#endif
import helios.app;
import helios.ecs;
import helios.log;
import helios.memory;
import helios.window;
#ifdef HELIOS_MODULE_INPUT_AVAILABLE
import helios.input;
#endif
HELIOS_BEGIN_MODULE_EXPORT
#include <helios/glfw/glfw.hpp>
HELIOS_END_MODULE_EXPORT
