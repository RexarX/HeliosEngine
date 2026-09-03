module;
#include <helios/assert.hpp>
#include <helios/config.hpp>
#ifndef HELIOS_ENABLE_IMPORT_STD
#include "../src/pch.hpp"
#else
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_video.h>
#ifdef HELIOS_PLATFORM_WINDOWS
#include <windows.h>
#endif
#endif

export module helios.sdl3.window;
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
import helios.sdl3;
import helios.utils;
import helios.window;
#ifdef HELIOS_MODULE_INPUT_AVAILABLE
import helios.input;
#endif
HELIOS_BEGIN_MODULE_EXPORT
#include <helios/sdl3/window/window.hpp>
HELIOS_END_MODULE_EXPORT
