#include <doctest/doctest.h>

#include <helios/ecs/world.hpp>
#include <helios/input/keyboard.hpp>

using namespace helios::ecs;
using namespace helios::input;

TEST_SUITE("helios::input::HasFlag") {
  TEST_CASE("helios::input::HasFlag") {
    SUBCASE("Detects a set modifier") {
      CHECK(HasFlag(Modifiers::kShift, Modifiers::kShift));
      CHECK(HasFlag(Modifiers::kShift | Modifiers::kControl,
                    Modifiers::kControl));
    }

    SUBCASE("Returns false for an unset modifier") {
      CHECK_FALSE(HasFlag(Modifiers::kShift, Modifiers::kAlt));
    }

    SUBCASE("kNone never matches a concrete flag") {
      CHECK_FALSE(HasFlag(Modifiers::kNone, Modifiers::kShift));
    }
  }
}

TEST_SUITE("helios::input::Keyboard") {
  TEST_CASE("helios::input::Keyboard") {
    SUBCASE(
        "Inserts as a resource and stores keys, modifiers, and composition") {
      World world;
      world.InsertResources(Keyboard{});
      CHECK(world.HasResource<Keyboard>());

      auto& keyboard = world.WriteResource<Keyboard>();
      keyboard.keys.Press(Key::kA);
      keyboard.modifiers = Modifiers::kShift;
      keyboard.composition = "ni";
      keyboard.composition_start = 0;
      keyboard.composition_length = 2;

      const auto& view = world.ReadResource<Keyboard>();
      CHECK(view.keys.Pressed(Key::kA));
      CHECK(view.keys.JustPressed(Key::kA));
      CHECK_EQ(view.modifiers, Modifiers::kShift);
      CHECK_EQ(view.composition, "ni");
      CHECK_EQ(view.composition_start, 0);
      CHECK_EQ(view.composition_length, 2);
    }
  }
}

TEST_SUITE("helios::input::KeyboardInputMsg") {
  TEST_CASE("helios::input::KeyboardInputMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<KeyboardInputMsg>();
      CHECK(world.HasMessage<KeyboardInputMsg>());

      world.WriteMessages<KeyboardInputMsg>().Write({
          .key = Key::kA,
          .state = ButtonState::kPressed,
          .modifiers = Modifiers::kControl,
      });
    }
  }
}

TEST_SUITE("helios::input::TextInputMsg") {
  TEST_CASE("helios::input::TextInputMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<TextInputMsg>();
      CHECK(world.HasMessage<TextInputMsg>());
      world.WriteMessages<TextInputMsg>().Write({.codepoint = U'a'});
    }
  }
}

TEST_SUITE("helios::input::TextEditingMsg") {
  TEST_CASE("helios::input::TextEditingMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<TextEditingMsg>();
      CHECK(world.HasMessage<TextEditingMsg>());
      world.WriteMessages<TextEditingMsg>().Write(
          {.composition = "pre", .start = 0, .length = 3});
    }
  }
}

TEST_SUITE("helios::input::TextEditingCandidatesMsg") {
  TEST_CASE("helios::input::TextEditingCandidatesMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<TextEditingCandidatesMsg>();
      CHECK(world.HasMessage<TextEditingCandidatesMsg>());
      world.WriteMessages<TextEditingCandidatesMsg>().Write({
          .candidates = {"a", "b"},
          .selected = 1,
          .horizontal = true,
      });
    }
  }
}

TEST_SUITE("helios::input::KeyboardConnectionMsg") {
  TEST_CASE("helios::input::KeyboardConnectionMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<KeyboardConnectionMsg>();
      CHECK(world.HasMessage<KeyboardConnectionMsg>());
      world.WriteMessages<KeyboardConnectionMsg>().Write(
          {.name = "kbd", .id = 0, .connected = true});
    }
  }
}
