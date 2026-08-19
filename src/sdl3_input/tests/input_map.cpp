#include <doctest/doctest.h>

#include <helios/input/gamepad.hpp>
#include <helios/input/keyboard.hpp>
#include <helios/input/mouse.hpp>
#include <helios/input/pen.hpp>
#include <helios/sdl3/input/details/input_map.hpp>

#include <SDL3/SDL.h>

#include <cstdint>
#include <optional>
#include <utility>

using namespace helios;
using namespace helios::sdl3::input;

TEST_SUITE("helios::sdl3::input::KeyFromSdl") {
  TEST_CASE("helios::sdl3::input::KeyFromSdl") {
    SUBCASE("Maps representative SDL scancodes") {
      CHECK_EQ(KeyFromSdl(SDL_SCANCODE_A), input::Key::kA);
      CHECK_EQ(KeyFromSdl(SDL_SCANCODE_ESCAPE), input::Key::kEscape);
      CHECK_EQ(KeyFromSdl(SDL_SCANCODE_SPACE), input::Key::kSpace);
      CHECK_EQ(KeyFromSdl(SDL_SCANCODE_F12), input::Key::kF12);
      CHECK_EQ(KeyFromSdl(SDL_SCANCODE_KP_ENTER), input::Key::kNumPadEnter);
      CHECK_EQ(KeyFromSdl(SDL_SCANCODE_LSHIFT), input::Key::kLeftShift);
      CHECK_EQ(KeyFromSdl(SDL_SCANCODE_RETURN2), input::Key::kEnter);
      CHECK_EQ(KeyFromSdl(SDL_SCANCODE_APPLICATION), input::Key::kMenu);
    }

    SUBCASE("Unknown and unmapped scancodes become Key::kUnknown") {
      CHECK_EQ(KeyFromSdl(SDL_SCANCODE_UNKNOWN), input::Key::kUnknown);
      CHECK_EQ(KeyFromSdl(static_cast<SDL_Scancode>(-2)), input::Key::kUnknown);
    }

    SUBCASE(
        "Round-trips every contiguous key except kUnknown, kF25, and kCount") {
      for (uint8_t i = 0; i < std::to_underlying(input::Key::kCount); ++i) {
        const auto key = static_cast<input::Key>(i);
        if (key == input::Key::kUnknown || key == input::Key::kF25) {
          CHECK_EQ(SdlFromKey(key), SDL_SCANCODE_UNKNOWN);
          continue;
        }
        CHECK_EQ(KeyFromSdl(SdlFromKey(key)), key);
      }
    }
  }
}

TEST_SUITE("helios::sdl3::input::SdlFromKey") {
  TEST_CASE("helios::sdl3::input::SdlFromKey") {
    SUBCASE("Maps kUnknown, kF25, and kCount to SDL_SCANCODE_UNKNOWN") {
      CHECK_EQ(SdlFromKey(input::Key::kUnknown), SDL_SCANCODE_UNKNOWN);
      CHECK_EQ(SdlFromKey(input::Key::kF25), SDL_SCANCODE_UNKNOWN);
      CHECK_EQ(SdlFromKey(input::Key::kCount), SDL_SCANCODE_UNKNOWN);
    }
  }
}

TEST_SUITE("helios::sdl3::input::MouseButtonFromSdl") {
  TEST_CASE("helios::sdl3::input::MouseButtonFromSdl") {
    SUBCASE("Maps SDL mouse buttons 1 through 5") {
      CHECK_EQ(MouseButtonFromSdl(SDL_BUTTON_LEFT), input::MouseButton::kLeft);
      CHECK_EQ(MouseButtonFromSdl(SDL_BUTTON_RIGHT),
               input::MouseButton::kRight);
      CHECK_EQ(MouseButtonFromSdl(SDL_BUTTON_MIDDLE),
               input::MouseButton::kMiddle);
      CHECK_EQ(MouseButtonFromSdl(SDL_BUTTON_X1), input::MouseButton::kExtra1);
      CHECK_EQ(MouseButtonFromSdl(SDL_BUTTON_X2), input::MouseButton::kExtra2);
    }

    SUBCASE("Returns empty for out-of-range buttons") {
      CHECK_FALSE(MouseButtonFromSdl(0).has_value());
      CHECK_FALSE(MouseButtonFromSdl(SDL_BUTTON_X2 + 4).has_value());
    }
  }
}

TEST_SUITE("helios::sdl3::input::ModifiersFromSdl") {
  TEST_CASE("helios::sdl3::input::ModifiersFromSdl") {
    SUBCASE("Maps no modifiers") {
      CHECK_EQ(ModifiersFromSdl(SDL_KMOD_NONE), input::Modifiers::kNone);
    }

    SUBCASE("Maps combined SDL modifier bits") {
      CHECK_EQ(ModifiersFromSdl(SDL_KMOD_SHIFT | SDL_KMOD_CTRL),
               input::Modifiers::kShift | input::Modifiers::kControl);
      CHECK_EQ(ModifiersFromSdl(SDL_KMOD_ALT | SDL_KMOD_GUI | SDL_KMOD_CAPS |
                                SDL_KMOD_NUM),
               input::Modifiers::kAlt | input::Modifiers::kSuper |
                   input::Modifiers::kCapsLock | input::Modifiers::kNumLock);
    }
  }
}

TEST_SUITE("helios::sdl3::input::ButtonStateFromSdl") {
  TEST_CASE("helios::sdl3::input::ButtonStateFromSdl") {
    SUBCASE("Maps press, release, and repeat") {
      CHECK_EQ(ButtonStateFromSdl(true, false), input::ButtonState::kPressed);
      CHECK_EQ(ButtonStateFromSdl(false, false), input::ButtonState::kReleased);
      CHECK_EQ(ButtonStateFromSdl(true, true), input::ButtonState::kRepeat);
    }

    SUBCASE("Released wins over a repeat flag") {
      CHECK_EQ(ButtonStateFromSdl(false, true), input::ButtonState::kReleased);
    }
  }
}

TEST_SUITE("helios::sdl3::input::GamepadButtonFromSdl") {
  TEST_CASE("helios::sdl3::input::GamepadButtonFromSdl") {
    SUBCASE("Maps SDL gamepad buttons") {
      CHECK_EQ(GamepadButtonFromSdl(SDL_GAMEPAD_BUTTON_SOUTH),
               input::GamepadButton::kA);
      CHECK_EQ(GamepadButtonFromSdl(SDL_GAMEPAD_BUTTON_START),
               input::GamepadButton::kStart);
      CHECK_EQ(GamepadButtonFromSdl(SDL_GAMEPAD_BUTTON_DPAD_LEFT),
               input::GamepadButton::kDpadLeft);
      CHECK_EQ(GamepadButtonFromSdl(SDL_GAMEPAD_BUTTON_MISC1),
               input::GamepadButton::kMisc1);
      CHECK_EQ(GamepadButtonFromSdl(SDL_GAMEPAD_BUTTON_TOUCHPAD),
               input::GamepadButton::kTouchpad);
    }

    SUBCASE("Invalid buttons become empty") {
      CHECK_FALSE(GamepadButtonFromSdl(SDL_GAMEPAD_BUTTON_INVALID).has_value());
    }
  }
}

TEST_SUITE("helios::sdl3::input::GamepadAxisFromSdl") {
  TEST_CASE("helios::sdl3::input::GamepadAxisFromSdl") {
    SUBCASE("Maps SDL gamepad axes in order") {
      CHECK_EQ(GamepadAxisFromSdl(SDL_GAMEPAD_AXIS_LEFTX),
               input::GamepadAxis::kLeftX);
      CHECK_EQ(GamepadAxisFromSdl(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER),
               input::GamepadAxis::kRightTrigger);
    }

    SUBCASE("Out-of-range axes fall back to LeftX") {
      CHECK_EQ(GamepadAxisFromSdl(SDL_GAMEPAD_AXIS_INVALID),
               input::GamepadAxis::kLeftX);
      CHECK_EQ(GamepadAxisFromSdl(static_cast<SDL_GamepadAxis>(
                   std::to_underlying(input::GamepadAxis::kCount))),
               input::GamepadAxis::kLeftX);
    }
  }
}

TEST_SUITE("helios::sdl3::input::SdlFromCursorIcon") {
  TEST_CASE("helios::sdl3::input::SdlFromCursorIcon") {
    SUBCASE("Maps standard cursor icons") {
      CHECK_EQ(SdlFromCursorIcon(input::CursorIcon::kDefault),
               SDL_SYSTEM_CURSOR_DEFAULT);
      CHECK_EQ(SdlFromCursorIcon(input::CursorIcon::kArrow),
               SDL_SYSTEM_CURSOR_DEFAULT);
      CHECK_EQ(SdlFromCursorIcon(input::CursorIcon::kIBeam),
               SDL_SYSTEM_CURSOR_TEXT);
      CHECK_EQ(SdlFromCursorIcon(input::CursorIcon::kCrosshair),
               SDL_SYSTEM_CURSOR_CROSSHAIR);
      CHECK_EQ(SdlFromCursorIcon(input::CursorIcon::kPointingHand),
               SDL_SYSTEM_CURSOR_POINTER);
      CHECK_EQ(SdlFromCursorIcon(input::CursorIcon::kResizeEw),
               SDL_SYSTEM_CURSOR_EW_RESIZE);
      CHECK_EQ(SdlFromCursorIcon(input::CursorIcon::kResizeNs),
               SDL_SYSTEM_CURSOR_NS_RESIZE);
      CHECK_EQ(SdlFromCursorIcon(input::CursorIcon::kResizeNwse),
               SDL_SYSTEM_CURSOR_NWSE_RESIZE);
      CHECK_EQ(SdlFromCursorIcon(input::CursorIcon::kResizeNesw),
               SDL_SYSTEM_CURSOR_NESW_RESIZE);
      CHECK_EQ(SdlFromCursorIcon(input::CursorIcon::kResizeAll),
               SDL_SYSTEM_CURSOR_MOVE);
      CHECK_EQ(SdlFromCursorIcon(input::CursorIcon::kNotAllowed),
               SDL_SYSTEM_CURSOR_NOT_ALLOWED);
      CHECK_EQ(SdlFromCursorIcon(input::CursorIcon::kCount),
               SDL_SYSTEM_CURSOR_DEFAULT);
    }
  }
}

TEST_SUITE("helios::sdl3::input::NormalizeStickAxis") {
  TEST_CASE("helios::sdl3::input::NormalizeStickAxis") {
    SUBCASE("Maps SDL stick samples onto [-1, 1]") {
      CHECK_EQ(NormalizeStickAxis(0), doctest::Approx(0.0F));
      CHECK_EQ(NormalizeStickAxis(32767), doctest::Approx(1.0F));
      CHECK_EQ(NormalizeStickAxis(-32767), doctest::Approx(-1.0F));
    }
  }
}

TEST_SUITE("helios::sdl3::input::NormalizeTriggerAxis") {
  TEST_CASE("helios::sdl3::input::NormalizeTriggerAxis") {
    SUBCASE("Maps a released trigger to -1 and a fully pressed trigger to 1") {
      CHECK_EQ(NormalizeTriggerAxis(0), doctest::Approx(-1.0F));
      CHECK_EQ(NormalizeTriggerAxis(32767), doctest::Approx(1.0F));
    }
  }
}

TEST_SUITE("helios::sdl3::input::PenButtonFromSdl") {
  TEST_CASE("helios::sdl3::input::PenButtonFromSdl") {
    SUBCASE("Maps barrel buttons 1 through 5") {
      CHECK_EQ(PenButtonFromSdl(1), input::PenButton::kBarrel1);
      CHECK_EQ(PenButtonFromSdl(5), input::PenButton::kBarrel5);
    }

    SUBCASE("Returns empty outside 1..5") {
      CHECK_FALSE(PenButtonFromSdl(0).has_value());
      CHECK_FALSE(PenButtonFromSdl(6).has_value());
    }
  }
}

TEST_SUITE("helios::sdl3::input::PenAxisFromSdl") {
  TEST_CASE("helios::sdl3::input::PenAxisFromSdl") {
    SUBCASE("Maps SDL pen axes in order") {
      CHECK_EQ(PenAxisFromSdl(SDL_PEN_AXIS_PRESSURE),
               input::PenAxis::kPressure);
      CHECK_EQ(PenAxisFromSdl(SDL_PEN_AXIS_XTILT), input::PenAxis::kXTilt);
    }

    SUBCASE("Returns empty for invalid axes") {
      CHECK_FALSE(PenAxisFromSdl(static_cast<SDL_PenAxis>(-1)).has_value());
      CHECK_FALSE(PenAxisFromSdl(SDL_PEN_AXIS_COUNT).has_value());
    }
  }
}

TEST_SUITE("helios::sdl3::input::PenDeviceTypeFromSdl") {
  TEST_CASE("helios::sdl3::input::PenDeviceTypeFromSdl") {
    SUBCASE("Maps direct, indirect, and unknown device types") {
      CHECK_EQ(PenDeviceTypeFromSdl(SDL_PEN_DEVICE_TYPE_DIRECT),
               input::PenDeviceType::kDirect);
      CHECK_EQ(PenDeviceTypeFromSdl(SDL_PEN_DEVICE_TYPE_INDIRECT),
               input::PenDeviceType::kIndirect);
      CHECK_EQ(PenDeviceTypeFromSdl(SDL_PEN_DEVICE_TYPE_UNKNOWN),
               input::PenDeviceType::kUnknown);
      CHECK_EQ(PenDeviceTypeFromSdl(SDL_PEN_DEVICE_TYPE_INVALID),
               input::PenDeviceType::kUnknown);
    }
  }
}
