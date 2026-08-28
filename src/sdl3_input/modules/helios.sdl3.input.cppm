module;
#include <helios/config.hpp>
#ifndef HELIOS_ENABLE_IMPORT_STD
#include "../src/pch.hpp"
#else
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_guid.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_joystick.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_pen.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_sensor.h>
#include <SDL3/SDL_surface.h>
#ifdef HELIOS_PLATFORM_WINDOWS
#include <windows.h>
#endif
#endif

export module helios.sdl3.input;
#ifdef HELIOS_ENABLE_IMPORT_STD
import std;
#endif
import helios.app;
import helios.async;
import helios.compiler;
import helios.core;
import helios.ecs;
import helios.input;
import helios.log;
import helios.memory;
import helios.sdl3;
import helios.utils;
import helios.window;
#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
import helios.sdl3.window;
#endif
HELIOS_BEGIN_MODULE_EXPORT
#include <helios/sdl3/input/input.hpp>
HELIOS_END_MODULE_EXPORT
