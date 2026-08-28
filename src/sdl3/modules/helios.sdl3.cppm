module;
#include <helios/assert.hpp>
#include <helios/config.hpp>
#ifndef HELIOS_ENABLE_IMPORT_STD
#include "../src/pch.hpp"
#else
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#ifdef HELIOS_PLATFORM_WINDOWS
#include <windows.h>
#endif
#endif

export module helios.sdl3;
#ifdef HELIOS_ENABLE_IMPORT_STD
import std;
#endif
import helios.app;
import helios.async;
import helios.compiler;
import helios.core;
import helios.ecs;
import helios.log;
import helios.memory;
import helios.utils;
import helios.window;
HELIOS_BEGIN_MODULE_EXPORT
#include <helios/sdl3/sdl3.hpp>
HELIOS_END_MODULE_EXPORT
