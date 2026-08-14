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

using namespace helios::glfw;
using namespace helios::input;

TEST_SUITE("helios::glfw::KeyFromGlfw") {
  TEST_CASE("helios::glfw::KeyFromGlfw") {
    SUBCASE("Maps representative GLFW keys") {
      CHECK_EQ(KeyFromGlfw(GLFW_KEY_A), Key::kA);
      CHECK_EQ(KeyFromGlfw(GLFW_KEY_ESCAPE), Key::kEscape);
      CHECK_EQ(KeyFromGlfw(GLFW_KEY_SPACE), Key::kSpace);
      CHECK_EQ(KeyFromGlfw(GLFW_KEY_F12), Key::kF12);
      CHECK_EQ(KeyFromGlfw(GLFW_KEY_KP_ENTER), Key::kNumPadEnter);
      CHECK_EQ(KeyFromGlfw(GLFW_KEY_LEFT_SHIFT), Key::kLeftShift);
    }

    SUBCASE("Unknown and unmapped keys become Key::kUnknown") {
      CHECK_EQ(KeyFromGlfw(GLFW_KEY_UNKNOWN), Key::kUnknown);
      CHECK_EQ(KeyFromGlfw(-2), Key::kUnknown);
    }

    SUBCASE("Round-trips every contiguous key except kUnknown and kCount") {
      for (uint8_t i = 0; i < std::to_underlying(Key::kCount); ++i) {
        const auto key = static_cast<Key>(i);
        if (key == Key::kUnknown) {
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
      CHECK_EQ(GlfwFromKey(Key::kUnknown), GLFW_KEY_UNKNOWN);
      CHECK_EQ(GlfwFromKey(Key::kCount), GLFW_KEY_UNKNOWN);
    }
  }
}

TEST_SUITE("helios::glfw::MouseButtonFromGlfw") {
  TEST_CASE("helios::glfw::MouseButtonFromGlfw") {
    SUBCASE("Maps GLFW mouse buttons 1 through 8") {
      CHECK_EQ(MouseButtonFromGlfw(GLFW_MOUSE_BUTTON_1), MouseButton::kLeft);
      CHECK_EQ(MouseButtonFromGlfw(GLFW_MOUSE_BUTTON_2), MouseButton::kRight);
      CHECK_EQ(MouseButtonFromGlfw(GLFW_MOUSE_BUTTON_3), MouseButton::kMiddle);
      CHECK_EQ(MouseButtonFromGlfw(GLFW_MOUSE_BUTTON_8), MouseButton::kExtra5);
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
      CHECK_EQ(ModifiersFromGlfw(0), Modifiers::kNone);
    }

    SUBCASE("Maps combined GLFW modifier bits") {
      CHECK_EQ(ModifiersFromGlfw(GLFW_MOD_SHIFT | GLFW_MOD_CONTROL),
               Modifiers::kShift | Modifiers::kControl);
      CHECK_EQ(ModifiersFromGlfw(GLFW_MOD_ALT | GLFW_MOD_SUPER |
                                 GLFW_MOD_CAPS_LOCK | GLFW_MOD_NUM_LOCK),
               Modifiers::kAlt | Modifiers::kSuper | Modifiers::kCapsLock |
                   Modifiers::kNumLock);
    }
  }
}

TEST_SUITE("helios::glfw::ButtonStateFromGlfw") {
  TEST_CASE("helios::glfw::ButtonStateFromGlfw") {
    SUBCASE("Maps press, release, and repeat") {
      CHECK_EQ(ButtonStateFromGlfw(GLFW_PRESS), ButtonState::kPressed);
      CHECK_EQ(ButtonStateFromGlfw(GLFW_RELEASE), ButtonState::kReleased);
      CHECK_EQ(ButtonStateFromGlfw(GLFW_REPEAT), ButtonState::kRepeat);
    }

    SUBCASE("Unknown actions become released") {
      CHECK_EQ(ButtonStateFromGlfw(99), ButtonState::kReleased);
    }
  }
}

TEST_SUITE("helios::glfw::GamepadButtonFromGlfw") {
  TEST_CASE("helios::glfw::GamepadButtonFromGlfw") {
    SUBCASE("Maps GLFW gamepad buttons in order") {
      CHECK_EQ(GamepadButtonFromGlfw(GLFW_GAMEPAD_BUTTON_A), GamepadButton::kA);
      CHECK_EQ(GamepadButtonFromGlfw(GLFW_GAMEPAD_BUTTON_START),
               GamepadButton::kStart);
      CHECK_EQ(GamepadButtonFromGlfw(GLFW_GAMEPAD_BUTTON_DPAD_LEFT),
               GamepadButton::kDpadLeft);
    }

    SUBCASE("Out-of-range buttons fall back to A") {
      CHECK_EQ(GamepadButtonFromGlfw(-1), GamepadButton::kA);
      CHECK_EQ(GamepadButtonFromGlfw(static_cast<int>(GamepadButton::kCount)),
               GamepadButton::kA);
    }
  }
}

TEST_SUITE("helios::glfw::GamepadAxisFromGlfw") {
  TEST_CASE("helios::glfw::GamepadAxisFromGlfw") {
    SUBCASE("Maps GLFW gamepad axes in order") {
      CHECK_EQ(GamepadAxisFromGlfw(GLFW_GAMEPAD_AXIS_LEFT_X),
               GamepadAxis::kLeftX);
      CHECK_EQ(GamepadAxisFromGlfw(GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER),
               GamepadAxis::kRightTrigger);
    }

    SUBCASE("Out-of-range axes fall back to LeftX") {
      CHECK_EQ(GamepadAxisFromGlfw(-1), GamepadAxis::kLeftX);
      CHECK_EQ(GamepadAxisFromGlfw(static_cast<int>(GamepadAxis::kCount)),
               GamepadAxis::kLeftX);
    }
  }
}

TEST_SUITE("helios::glfw::GlfwFromCursorIcon") {
  TEST_CASE("helios::glfw::GlfwFromCursorIcon") {
    SUBCASE("Maps standard cursor icons") {
      CHECK_EQ(GlfwFromCursorIcon(CursorIcon::kDefault), GLFW_ARROW_CURSOR);
      CHECK_EQ(GlfwFromCursorIcon(CursorIcon::kArrow), GLFW_ARROW_CURSOR);
      CHECK_EQ(GlfwFromCursorIcon(CursorIcon::kIBeam), GLFW_IBEAM_CURSOR);
      CHECK_EQ(GlfwFromCursorIcon(CursorIcon::kCrosshair),
               GLFW_CROSSHAIR_CURSOR);
      CHECK_EQ(GlfwFromCursorIcon(CursorIcon::kPointingHand),
               GLFW_POINTING_HAND_CURSOR);
      CHECK_EQ(GlfwFromCursorIcon(CursorIcon::kResizeEw),
               GLFW_RESIZE_EW_CURSOR);
      CHECK_EQ(GlfwFromCursorIcon(CursorIcon::kResizeNs),
               GLFW_RESIZE_NS_CURSOR);
      CHECK_EQ(GlfwFromCursorIcon(CursorIcon::kResizeNwse),
               GLFW_RESIZE_NWSE_CURSOR);
      CHECK_EQ(GlfwFromCursorIcon(CursorIcon::kResizeNesw),
               GLFW_RESIZE_NESW_CURSOR);
      CHECK_EQ(GlfwFromCursorIcon(CursorIcon::kResizeAll),
               GLFW_RESIZE_ALL_CURSOR);
      CHECK_EQ(GlfwFromCursorIcon(CursorIcon::kNotAllowed),
               GLFW_NOT_ALLOWED_CURSOR);
      CHECK_EQ(GlfwFromCursorIcon(CursorIcon::kCount), GLFW_ARROW_CURSOR);
    }
  }
}

TEST_SUITE("helios::glfw::GamepadSlotCache") {
  TEST_CASE("helios::glfw::GamepadSlotCache") {
    SUBCASE("Packed field order matches the cache layout") {
      struct Packed {
        std::string name;
        std::array<float, static_cast<size_t>(GamepadAxis::kCount)> axes = {};
        std::array<unsigned char, static_cast<size_t>(GamepadButton::kCount)>
            buttons = {};
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
