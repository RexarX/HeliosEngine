#include <doctest/doctest.h>

#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/schedule/system_local_data.hpp>
#include <helios/ecs/system/param.hpp>
#include <helios/ecs/system/param_policy.hpp>
#include <helios/ecs/world.hpp>
#include <helios/input/systems/keyboard.hpp>

using namespace helios::ecs;
using namespace helios::input;

namespace {

void AddKeyboardMessages(World& world) {
  world.AddMessages<KeyboardInputMsg, TextInputMsg, TextEditingMsg,
                    TextEditingCandidatesMsg, KeyboardConnectionMsg>();
}

}  // namespace

TEST_SUITE("helios::input::UpdateKeyboardState") {
  TEST_CASE("helios::input::UpdateKeyboardState::operator()") {
    SUBCASE("Applies press, release, repeat, and modifiers") {
      World world;
      world.InsertResources(Keyboard{});
      AddKeyboardMessages(world);

      world.WriteMessages<KeyboardInputMsg>().Write({
          .key = Key::kA,
          .state = ButtonState::kPressed,
          .modifiers = Modifiers::kShift,
      });
      world.WriteMessages<KeyboardInputMsg>().Write({
          .key = Key::kA,
          .state = ButtonState::kRepeat,
          .modifiers = Modifiers::kShift | Modifiers::kControl,
      });

      auto local_data = SystemLocalData::From();
      const AccessPolicy policy =
          BuildPolicyFromParams<Res<Keyboard>, KeyboardMessages>();
      UpdateKeyboardState{}(
          SystemParamTraits<Res<Keyboard>>::Make(world, local_data, policy),
          SystemParamTraits<KeyboardMessages>::Make(world, local_data, policy));

      const auto& keyboard = world.ReadResource<Keyboard>();
      CHECK(keyboard.keys.Pressed(Key::kA));
      CHECK(keyboard.keys.JustPressed(Key::kA));
      CHECK_EQ(keyboard.modifiers, Modifiers::kShift | Modifiers::kControl);
    }

    SUBCASE("Applies a release after a prior press") {
      World world;
      Keyboard keyboard;
      keyboard.keys.Press(Key::kB);
      keyboard.keys.Clear();
      world.InsertResources(std::move(keyboard));
      AddKeyboardMessages(world);

      world.WriteMessages<KeyboardInputMsg>().Write({
          .key = Key::kB,
          .state = ButtonState::kReleased,
      });

      auto local_data = SystemLocalData::From();
      const AccessPolicy policy =
          BuildPolicyFromParams<Res<Keyboard>, KeyboardMessages>();
      UpdateKeyboardState{}(
          SystemParamTraits<Res<Keyboard>>::Make(world, local_data, policy),
          SystemParamTraits<KeyboardMessages>::Make(world, local_data, policy));

      const auto& updated = world.ReadResource<Keyboard>();
      CHECK_FALSE(updated.keys.Pressed(Key::kB));
      CHECK(updated.keys.JustReleased(Key::kB));
    }

    SUBCASE("Applies IME composition and clears it on text commit") {
      World world;
      world.InsertResources(Keyboard{});
      AddKeyboardMessages(world);

      world.WriteMessages<TextEditingMsg>().Write({
          .composition = "ni",
          .start = 0,
          .length = 2,
      });

      auto local_data = SystemLocalData::From();
      const AccessPolicy policy =
          BuildPolicyFromParams<Res<Keyboard>, KeyboardMessages>();
      UpdateKeyboardState{}(
          SystemParamTraits<Res<Keyboard>>::Make(world, local_data, policy),
          SystemParamTraits<KeyboardMessages>::Make(world, local_data, policy));

      CHECK_EQ(world.ReadResource<Keyboard>().composition, "ni");
      CHECK_EQ(world.ReadResource<Keyboard>().composition_start, 0);
      CHECK_EQ(world.ReadResource<Keyboard>().composition_length, 2);

      world.WriteMessages<TextInputMsg>().Write({.codepoint = U'你'});
      UpdateKeyboardState{}(
          SystemParamTraits<Res<Keyboard>>::Make(world, local_data, policy),
          SystemParamTraits<KeyboardMessages>::Make(world, local_data, policy));

      CHECK(world.ReadResource<Keyboard>().composition.empty());
      CHECK_EQ(world.ReadResource<Keyboard>().composition_start, 0);
      CHECK_EQ(world.ReadResource<Keyboard>().composition_length, 0);
    }
  }
}
