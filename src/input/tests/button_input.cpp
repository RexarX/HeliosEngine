#include <doctest/doctest.h>

#include <helios/input/button_input.hpp>

#include <cstdint>

using namespace helios::input;

namespace {

enum class TestButton : uint8_t {
  kA,
  kB,
  kC,
  kCount,
};

}  // namespace

TEST_SUITE("helios::input::ButtonInput") {
  TEST_CASE("helios::input::ButtonInput::Press") {
    SUBCASE("Pressing a released button sets pressed and just-pressed") {
      ButtonInput<TestButton> keys;
      keys.Press(TestButton::kA);

      CHECK(keys.Pressed(TestButton::kA));
      CHECK(keys.JustPressed(TestButton::kA));
      CHECK_FALSE(keys.JustReleased(TestButton::kA));
    }

    SUBCASE("Pressing an already pressed button keeps just-pressed") {
      ButtonInput<TestButton> keys;
      keys.Press(TestButton::kA);
      keys.Press(TestButton::kA);

      CHECK(keys.Pressed(TestButton::kA));
      CHECK(keys.JustPressed(TestButton::kA));
    }

    SUBCASE("Pressing after Clear does not set just-pressed again") {
      ButtonInput<TestButton> keys;
      keys.Press(TestButton::kA);
      keys.Clear();
      keys.Press(TestButton::kA);

      CHECK(keys.Pressed(TestButton::kA));
      CHECK_FALSE(keys.JustPressed(TestButton::kA));
    }
  }

  TEST_CASE("helios::input::ButtonInput::Release") {
    SUBCASE("Releasing a pressed button sets just-released") {
      ButtonInput<TestButton> keys;
      keys.Press(TestButton::kA);
      keys.Clear();
      keys.Release(TestButton::kA);

      CHECK_FALSE(keys.Pressed(TestButton::kA));
      CHECK(keys.JustReleased(TestButton::kA));
    }

    SUBCASE("Releasing a never-pressed button is a no-op") {
      ButtonInput<TestButton> keys;
      keys.Release(TestButton::kA);

      CHECK_FALSE(keys.Pressed(TestButton::kA));
      CHECK_FALSE(keys.JustReleased(TestButton::kA));
    }

    SUBCASE("Press and release in the same frame keep both edges") {
      ButtonInput<TestButton> keys;
      keys.Press(TestButton::kA);
      keys.Release(TestButton::kA);

      CHECK_FALSE(keys.Pressed(TestButton::kA));
      CHECK(keys.JustPressed(TestButton::kA));
      CHECK(keys.JustReleased(TestButton::kA));
    }
  }

  TEST_CASE("helios::input::ButtonInput::Clear") {
    SUBCASE("Clears edges and keeps held buttons") {
      ButtonInput<TestButton> keys;
      keys.Press(TestButton::kA);
      keys.Clear();

      CHECK(keys.Pressed(TestButton::kA));
      CHECK_FALSE(keys.JustPressed(TestButton::kA));
      CHECK_FALSE(keys.JustReleased(TestButton::kA));
    }

    SUBCASE("Clears just-released after a release") {
      ButtonInput<TestButton> keys;
      keys.Press(TestButton::kA);
      keys.Release(TestButton::kA);
      keys.Clear();

      CHECK_FALSE(keys.JustReleased(TestButton::kA));
    }
  }

  TEST_CASE("helios::input::ButtonInput::Reset") {
    SUBCASE("Clears pressed state and edges") {
      ButtonInput<TestButton> keys;
      keys.Press(TestButton::kB);
      keys.Reset();

      CHECK_FALSE(keys.Pressed(TestButton::kB));
      CHECK_FALSE(keys.JustPressed(TestButton::kB));
      CHECK_FALSE(keys.AnyPressed());
    }
  }

  TEST_CASE("helios::input::ButtonInput::Pressed") {
    SUBCASE("Returns false by default") {
      const ButtonInput<TestButton> keys;
      CHECK_FALSE(keys.Pressed(TestButton::kA));
    }
  }

  TEST_CASE("helios::input::ButtonInput::JustPressed") {
    SUBCASE("Returns false by default") {
      const ButtonInput<TestButton> keys;
      CHECK_FALSE(keys.JustPressed(TestButton::kA));
    }
  }

  TEST_CASE("helios::input::ButtonInput::JustReleased") {
    SUBCASE("Returns false by default") {
      const ButtonInput<TestButton> keys;
      CHECK_FALSE(keys.JustReleased(TestButton::kA));
    }
  }

  TEST_CASE("helios::input::ButtonInput::AnyPressed") {
    SUBCASE("Returns false when no buttons are held") {
      const ButtonInput<TestButton> keys;
      CHECK_FALSE(keys.AnyPressed());
    }

    SUBCASE("Returns true when any button is held") {
      ButtonInput<TestButton> keys;
      keys.Press(TestButton::kA);
      CHECK(keys.AnyPressed());
    }
  }

  TEST_CASE("helios::input::ButtonInput::AllPressed") {
    SUBCASE("Requires every listed button") {
      ButtonInput<TestButton> keys;
      keys.Press(TestButton::kA);
      keys.Press(TestButton::kB);

      CHECK(keys.AllPressed(TestButton::kA));
      CHECK(keys.AllPressed(TestButton::kA, TestButton::kB));
      CHECK_FALSE(keys.AllPressed(TestButton::kA, TestButton::kC));
    }

    SUBCASE("Empty pack is vacuously true") {
      const ButtonInput<TestButton> keys;
      CHECK(keys.AllPressed());
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
