#include <doctest/doctest.h>

#include <helios/input/button_input.hpp>
#include <helios/input/keyboard.hpp>
#include <helios/input/mouse.hpp>

using namespace helios::input;

TEST_SUITE("helios::input::ButtonInput") {
  TEST_CASE("helios::input::ButtonInput::Press") {
    SUBCASE("Pressing a released button sets pressed and just-pressed") {
      ButtonInput<Key> keys;
      keys.Press(Key::kA);

      CHECK(keys.Pressed(Key::kA));
      CHECK(keys.JustPressed(Key::kA));
      CHECK_FALSE(keys.JustReleased(Key::kA));
    }

    SUBCASE("Pressing an already pressed button keeps just-pressed") {
      ButtonInput<Key> keys;
      keys.Press(Key::kA);
      keys.Press(Key::kA);

      CHECK(keys.Pressed(Key::kA));
      CHECK(keys.JustPressed(Key::kA));
    }

    SUBCASE("Pressing after Clear does not set just-pressed again") {
      ButtonInput<Key> keys;
      keys.Press(Key::kA);
      keys.Clear();
      keys.Press(Key::kA);

      CHECK(keys.Pressed(Key::kA));
      CHECK_FALSE(keys.JustPressed(Key::kA));
    }
  }

  TEST_CASE("helios::input::ButtonInput::Release") {
    SUBCASE("Releasing a pressed button sets just-released") {
      ButtonInput<Key> keys;
      keys.Press(Key::kA);
      keys.Clear();
      keys.Release(Key::kA);

      CHECK_FALSE(keys.Pressed(Key::kA));
      CHECK(keys.JustReleased(Key::kA));
    }

    SUBCASE("Releasing a never-pressed button is a no-op") {
      ButtonInput<Key> keys;
      keys.Release(Key::kA);

      CHECK_FALSE(keys.Pressed(Key::kA));
      CHECK_FALSE(keys.JustReleased(Key::kA));
    }

    SUBCASE("Press and release in the same frame keep both edges") {
      ButtonInput<Key> keys;
      keys.Press(Key::kA);
      keys.Release(Key::kA);

      CHECK_FALSE(keys.Pressed(Key::kA));
      CHECK(keys.JustPressed(Key::kA));
      CHECK(keys.JustReleased(Key::kA));
    }
  }

  TEST_CASE("helios::input::ButtonInput::Clear") {
    SUBCASE("Clears edges and keeps held buttons") {
      ButtonInput<Key> keys;
      keys.Press(Key::kA);
      keys.Clear();

      CHECK(keys.Pressed(Key::kA));
      CHECK_FALSE(keys.JustPressed(Key::kA));
      CHECK_FALSE(keys.JustReleased(Key::kA));
    }

    SUBCASE("Clears just-released after a release") {
      ButtonInput<Key> keys;
      keys.Press(Key::kA);
      keys.Release(Key::kA);
      keys.Clear();

      CHECK_FALSE(keys.JustReleased(Key::kA));
    }
  }

  TEST_CASE("helios::input::ButtonInput::Reset") {
    SUBCASE("Clears pressed state and edges") {
      ButtonInput<Key> keys;
      keys.Press(Key::kW);
      keys.Reset();

      CHECK_FALSE(keys.Pressed(Key::kW));
      CHECK_FALSE(keys.JustPressed(Key::kW));
      CHECK_FALSE(keys.AnyPressed());
    }
  }

  TEST_CASE("helios::input::ButtonInput::Pressed") {
    SUBCASE("Returns false by default") {
      const ButtonInput<Key> keys;
      CHECK_FALSE(keys.Pressed(Key::kA));
    }
  }

  TEST_CASE("helios::input::ButtonInput::JustPressed") {
    SUBCASE("Returns false by default") {
      const ButtonInput<Key> keys;
      CHECK_FALSE(keys.JustPressed(Key::kA));
    }
  }

  TEST_CASE("helios::input::ButtonInput::JustReleased") {
    SUBCASE("Returns false by default") {
      const ButtonInput<Key> keys;
      CHECK_FALSE(keys.JustReleased(Key::kA));
    }
  }

  TEST_CASE("helios::input::ButtonInput::AnyPressed") {
    SUBCASE("Returns false when no buttons are held") {
      const ButtonInput<Key> keys;
      CHECK_FALSE(keys.AnyPressed());
    }

    SUBCASE("Returns true when any button is held") {
      ButtonInput<Key> keys;
      keys.Press(Key::kA);
      CHECK(keys.AnyPressed());
    }
  }

  TEST_CASE("helios::input::ButtonInput::AllPressed") {
    SUBCASE("Requires every listed button") {
      ButtonInput<Key> keys;
      keys.Press(Key::kA);
      keys.Press(Key::kB);

      CHECK(keys.AllPressed(Key::kA));
      CHECK(keys.AllPressed(Key::kA, Key::kB));
      CHECK_FALSE(keys.AllPressed(Key::kA, Key::kC));
    }

    SUBCASE("Empty pack is vacuously true") {
      const ButtonInput<Key> keys;
      CHECK(keys.AllPressed());
    }

    SUBCASE("Works for mouse buttons") {
      ButtonInput<MouseButton> buttons;
      buttons.Press(MouseButton::kLeft);
      buttons.Press(MouseButton::kRight);

      CHECK(buttons.AllPressed(MouseButton::kLeft, MouseButton::kRight));
      CHECK_FALSE(buttons.AllPressed(MouseButton::kLeft, MouseButton::kMiddle));
    }
  }
}

TEST_SUITE("helios::input::IndexedButtonInput") {
  TEST_CASE("helios::input::IndexedButtonInput::Press") {
    SUBCASE("Pressing a released button sets pressed and just-pressed") {
      IndexedButtonInput<8> buttons;
      buttons.Press(1);

      CHECK(buttons.Pressed(1));
      CHECK(buttons.JustPressed(1));
      CHECK_FALSE(buttons.JustReleased(1));
    }

    SUBCASE("Pressing an already pressed button keeps just-pressed") {
      IndexedButtonInput<8> buttons;
      buttons.Press(1);
      buttons.Press(1);

      CHECK(buttons.Pressed(1));
      CHECK(buttons.JustPressed(1));
    }
  }

  TEST_CASE("helios::input::IndexedButtonInput::Release") {
    SUBCASE("Releasing a pressed button sets just-released") {
      IndexedButtonInput<8> buttons;
      buttons.Press(2);
      buttons.Clear();
      buttons.Release(2);

      CHECK_FALSE(buttons.Pressed(2));
      CHECK(buttons.JustReleased(2));
    }

    SUBCASE("Releasing a never-pressed button is a no-op") {
      IndexedButtonInput<8> buttons;
      buttons.Release(2);

      CHECK_FALSE(buttons.Pressed(2));
      CHECK_FALSE(buttons.JustReleased(2));
    }
  }

  TEST_CASE("helios::input::IndexedButtonInput::Clear") {
    SUBCASE("Clears edges and keeps held buttons") {
      IndexedButtonInput<8> buttons;
      buttons.Press(3);
      buttons.Clear();

      CHECK(buttons.Pressed(3));
      CHECK_FALSE(buttons.JustPressed(3));
      CHECK_FALSE(buttons.JustReleased(3));
    }
  }

  TEST_CASE("helios::input::IndexedButtonInput::Reset") {
    SUBCASE("Clears pressed state and edges") {
      IndexedButtonInput<8> buttons;
      buttons.Press(3);
      buttons.Reset();

      CHECK_FALSE(buttons.Pressed(3));
      CHECK_FALSE(buttons.JustPressed(3));
      CHECK_FALSE(buttons.AnyPressed());
    }
  }

  TEST_CASE("helios::input::IndexedButtonInput::Pressed") {
    SUBCASE("Returns false by default") {
      const IndexedButtonInput<8> buttons;
      CHECK_FALSE(buttons.Pressed(0));
    }
  }

  TEST_CASE("helios::input::IndexedButtonInput::JustPressed") {
    SUBCASE("Returns false by default") {
      const IndexedButtonInput<8> buttons;
      CHECK_FALSE(buttons.JustPressed(0));
    }
  }

  TEST_CASE("helios::input::IndexedButtonInput::JustReleased") {
    SUBCASE("Returns false by default") {
      const IndexedButtonInput<8> buttons;
      CHECK_FALSE(buttons.JustReleased(0));
    }
  }

  TEST_CASE("helios::input::IndexedButtonInput::AnyPressed") {
    SUBCASE("Returns false when no buttons are held") {
      const IndexedButtonInput<8> buttons;
      CHECK_FALSE(buttons.AnyPressed());
    }

    SUBCASE("Returns true when any button is held") {
      IndexedButtonInput<8> buttons;
      buttons.Press(4);
      CHECK(buttons.AnyPressed());
    }
  }
}
