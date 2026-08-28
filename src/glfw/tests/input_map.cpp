#ifndef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

#ifdef HELIOS_MODULE_INPUT_AVAILABLE

#include <helios/glfw/details/input_map.hpp>
#include <helios/glfw/systems/input.hpp>
#include <helios/input/gamepad.hpp>
#include <helios/input/keyboard.hpp>
#include <helios/input/mouse.hpp>

#include <GLFW/glfw3.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

using namespace helios;
using namespace helios::glfw;

TEST_SUITE("helios::glfw::KeyFromGlfw") {
  TEST_CASE("helios::glfw::KeyFromGlfw") {
    SUBCASE("Maps representative GLFW keys") {
      CHECK_EQ(KeyFromGlfw(GLFW_KEY_A), input::Key::kA);
      CHECK_EQ(KeyFromGlfw(GLFW_KEY_ESCAPE), input::Key::kEscape);
      CHECK_EQ(KeyFromGlfw(GLFW_KEY_SPACE), input::Key::kSpace);
      CHECK_EQ(KeyFromGlfw(GLFW_KEY_F12), input::Key::kF12);
      CHECK_EQ(KeyFromGlfw(GLFW_KEY_KP_ENTER), input::Key::kNumPadEnter);
      CHECK_EQ(KeyFromGlfw(GLFW_KEY_LEFT_SHIFT), input::Key::kLeftShift);
    }

    SUBCASE("Unknown and unmapped keys become input::Key::kUnknown") {
      CHECK_EQ(KeyFromGlfw(GLFW_KEY_UNKNOWN), input::Key::kUnknown);
      CHECK_EQ(KeyFromGlfw(-2), input::Key::kUnknown);
    }

    SUBCASE("Round-trips every contiguous key except kUnknown and kCount") {
      for (uint8_t i = 0; i < std::to_underlying(input::Key::kCount); ++i) {
        const auto key = static_cast<input::Key>(i);
        if (key == input::Key::kUnknown) {
          CHECK_EQ(GlfwFromKey(key), GLFW_KEY_UNKNOWN);
          continue;
        }
        CHECK_EQ(KeyFromGlfw(GlfwFromKey(key)), key);
      }
    }
  }
}

TEST_SUITE("helios::glfw::GlfwFromKey") {
  TEST_CASE("helios::glfw::GlfwFromKey") {
    SUBCASE("Maps kUnknown and kCount to GLFW_KEY_UNKNOWN") {
      CHECK_EQ(GlfwFromKey(input::Key::kUnknown), GLFW_KEY_UNKNOWN);
      CHECK_EQ(GlfwFromKey(input::Key::kCount), GLFW_KEY_UNKNOWN);
    }
  }
}

TEST_SUITE("helios::glfw::MouseButtonFromGlfw") {
  TEST_CASE("helios::glfw::MouseButtonFromGlfw") {
    SUBCASE("Maps GLFW mouse buttons 1 through 8") {
      CHECK_EQ(MouseButtonFromGlfw(GLFW_MOUSE_BUTTON_1),
               input::MouseButton::kLeft);
      CHECK_EQ(MouseButtonFromGlfw(GLFW_MOUSE_BUTTON_2),
               input::MouseButton::kRight);
      CHECK_EQ(MouseButtonFromGlfw(GLFW_MOUSE_BUTTON_3),
               input::MouseButton::kMiddle);
      CHECK_EQ(MouseButtonFromGlfw(GLFW_MOUSE_BUTTON_8),
               input::MouseButton::kExtra5);
    }

    SUBCASE("Returns empty for out-of-range buttons") {
      CHECK_FALSE(MouseButtonFromGlfw(GLFW_MOUSE_BUTTON_1 - 1).has_value());
      CHECK_FALSE(MouseButtonFromGlfw(GLFW_MOUSE_BUTTON_8 + 1).has_value());
    }
  }
}

TEST_SUITE("helios::glfw::ModifiersFromGlfw") {
  TEST_CASE("helios::glfw::ModifiersFromGlfw") {
    SUBCASE("Maps no modifiers") {
      CHECK_EQ(ModifiersFromGlfw(0), input::Modifiers::kNone);
    }

    SUBCASE("Maps combined GLFW modifier bits") {
      CHECK_EQ(ModifiersFromGlfw(GLFW_MOD_SHIFT | GLFW_MOD_CONTROL),
               input::Modifiers::kShift | input::Modifiers::kControl);
      CHECK_EQ(ModifiersFromGlfw(GLFW_MOD_ALT | GLFW_MOD_SUPER |
                                 GLFW_MOD_CAPS_LOCK | GLFW_MOD_NUM_LOCK),
               input::Modifiers::kAlt | input::Modifiers::kSuper |
                   input::Modifiers::kCapsLock | input::Modifiers::kNumLock);
    }
  }
}

TEST_SUITE("helios::glfw::ButtonStateFromGlfw") {
  TEST_CASE("helios::glfw::ButtonStateFromGlfw") {
    SUBCASE("Maps press, release, and repeat") {
      CHECK_EQ(ButtonStateFromGlfw(GLFW_PRESS), input::ButtonState::kPressed);
      CHECK_EQ(ButtonStateFromGlfw(GLFW_RELEASE),
               input::ButtonState::kReleased);
      CHECK_EQ(ButtonStateFromGlfw(GLFW_REPEAT), input::ButtonState::kRepeat);
    }

    SUBCASE("Unknown actions become released") {
      CHECK_EQ(ButtonStateFromGlfw(99), input::ButtonState::kReleased);
    }
  }
}

TEST_SUITE("helios::glfw::GamepadButtonFromGlfw") {
  TEST_CASE("helios::glfw::GamepadButtonFromGlfw") {
    SUBCASE("Maps GLFW gamepad buttons in order") {
      CHECK_EQ(GamepadButtonFromGlfw(GLFW_GAMEPAD_BUTTON_A),
               input::GamepadButton::kA);
      CHECK_EQ(GamepadButtonFromGlfw(GLFW_GAMEPAD_BUTTON_START),
               input::GamepadButton::kStart);
      CHECK_EQ(GamepadButtonFromGlfw(GLFW_GAMEPAD_BUTTON_DPAD_LEFT),
               input::GamepadButton::kDpadLeft);
    }

    SUBCASE("Out-of-range buttons fall back to A") {
      CHECK_EQ(GamepadButtonFromGlfw(-1), input::GamepadButton::kA);
      CHECK_EQ(GamepadButtonFromGlfw(GLFW_GAMEPAD_BUTTON_LAST + 1),
               input::GamepadButton::kA);
      CHECK_EQ(
          GamepadButtonFromGlfw(static_cast<int>(input::GamepadButton::kMisc1)),
          input::GamepadButton::kA);
    }
  }
}

TEST_SUITE("helios::glfw::GamepadAxisFromGlfw") {
  TEST_CASE("helios::glfw::GamepadAxisFromGlfw") {
    SUBCASE("Maps GLFW gamepad axes in order") {
      CHECK_EQ(GamepadAxisFromGlfw(GLFW_GAMEPAD_AXIS_LEFT_X),
               input::GamepadAxis::kLeftX);
      CHECK_EQ(GamepadAxisFromGlfw(GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER),
               input::GamepadAxis::kRightTrigger);
    }

    SUBCASE("Out-of-range axes fall back to LeftX") {
      CHECK_EQ(GamepadAxisFromGlfw(-1), input::GamepadAxis::kLeftX);
      CHECK_EQ(
          GamepadAxisFromGlfw(static_cast<int>(input::GamepadAxis::kCount)),
          input::GamepadAxis::kLeftX);
    }
  }
}

TEST_SUITE("helios::glfw::GlfwFromCursorIcon") {
  TEST_CASE("helios::glfw::GlfwFromCursorIcon") {
    SUBCASE("Maps standard cursor icons") {
      CHECK_EQ(GlfwFromCursorIcon(input::CursorIcon::kDefault),
               GLFW_ARROW_CURSOR);
      CHECK_EQ(GlfwFromCursorIcon(input::CursorIcon::kArrow),
               GLFW_ARROW_CURSOR);
      CHECK_EQ(GlfwFromCursorIcon(input::CursorIcon::kIBeam),
               GLFW_IBEAM_CURSOR);
      CHECK_EQ(GlfwFromCursorIcon(input::CursorIcon::kCrosshair),
               GLFW_CROSSHAIR_CURSOR);
      CHECK_EQ(GlfwFromCursorIcon(input::CursorIcon::kPointingHand),
               GLFW_POINTING_HAND_CURSOR);
      CHECK_EQ(GlfwFromCursorIcon(input::CursorIcon::kResizeEw),
               GLFW_RESIZE_EW_CURSOR);
      CHECK_EQ(GlfwFromCursorIcon(input::CursorIcon::kResizeNs),
               GLFW_RESIZE_NS_CURSOR);
      CHECK_EQ(GlfwFromCursorIcon(input::CursorIcon::kResizeNwse),
               GLFW_RESIZE_NWSE_CURSOR);
      CHECK_EQ(GlfwFromCursorIcon(input::CursorIcon::kResizeNesw),
               GLFW_RESIZE_NESW_CURSOR);
      CHECK_EQ(GlfwFromCursorIcon(input::CursorIcon::kResizeAll),
               GLFW_RESIZE_ALL_CURSOR);
      CHECK_EQ(GlfwFromCursorIcon(input::CursorIcon::kNotAllowed),
               GLFW_NOT_ALLOWED_CURSOR);
      CHECK_EQ(GlfwFromCursorIcon(input::CursorIcon::kCount),
               GLFW_ARROW_CURSOR);
    }
  }
}

TEST_SUITE("helios::glfw::GamepadSlotCache") {
  TEST_CASE("helios::glfw::GamepadSlotCache") {
    SUBCASE("Packed field order matches the cache layout") {
      struct Packed {
        std::string name;
        std::array<float, kGlfwMappedAxisCount> axes = {};
        std::array<unsigned char, kGlfwMappedButtonCount> buttons = {};
        bool connected = false;
      };

      CHECK_EQ(sizeof(GamepadSlotCache), sizeof(Packed));
      CHECK_EQ(offsetof(GamepadSlotCache, name), offsetof(Packed, name));
      CHECK_EQ(offsetof(GamepadSlotCache, axes), offsetof(Packed, axes));
      CHECK_EQ(offsetof(GamepadSlotCache, buttons), offsetof(Packed, buttons));
      CHECK_EQ(offsetof(GamepadSlotCache, connected),
               offsetof(Packed, connected));
    }
  }
}

#endif  // HELIOS_MODULE_INPUT_AVAILABLE
#endif
