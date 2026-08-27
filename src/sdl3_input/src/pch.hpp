#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <ostream>
#include <source_location>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_guid.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_joystick.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_pen.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_sensor.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_touch.h>

#ifdef HELIOS_PLATFORM_WINDOWS
#include <windows.h>
#endif
