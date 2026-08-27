#ifndef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

#include <helios/app/application.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/schedule/system_local_data.hpp>
#include <helios/ecs/system/param.hpp>
#include <helios/ecs/system/param_policy.hpp>
#include <helios/input/input.hpp>
#include <helios/sdl3/input/input.hpp>
#include <helios/sdl3/input/state.hpp>
#include <helios/sdl3/input/systems/poll_gamepads.hpp>
#include <helios/sdl3/plugin.hpp>
#include <helios/window/window.hpp>

#include "available.hpp"

#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
#include <helios/sdl3/window/plugin.hpp>
#endif

using namespace helios;
using namespace helios::sdl3::input;

namespace {

void AddInputPlugins(app::App& app) {
  app.AddPlugins(sdl3::Plugin{}, window::Plugin{},
#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
                 sdl3::window::Plugin{},
#endif
                 input::Plugin{}, Plugin{});
}

}  // namespace

TEST_SUITE("helios::sdl3::input::DestroyGamepadCache") {
  TEST_CASE("helios::sdl3::input::DestroyGamepadCache") {
    SUBCASE("Clears pending device lists and enumeration state") {
      GamepadCache cache;
      cache.pending_added.push_back(1);
      cache.pending_removed.push_back(2);
      cache.pending_remapped.push_back(3);
      cache.enumerated = true;
      cache.slots[0].connected = true;
      cache.slots[0].name = "pad";

      DestroyGamepadCache(cache);

      CHECK(cache.pending_added.empty());
      CHECK(cache.pending_removed.empty());
      CHECK(cache.pending_remapped.empty());
      CHECK_FALSE(cache.enumerated);
      CHECK_FALSE(cache.slots[0].connected);
      CHECK(cache.slots[0].name.empty());
    }
  }
}

TEST_SUITE("helios::sdl3::input::ApplyGamepadMappings") {
  TEST_CASE("helios::sdl3::input::ApplyGamepadMappings::operator()") {
    SUBCASE("Consumes queued mapping lines") {
      HELIOS_SKIP_IF_NO_SDL_RUNTIME();

      app::App app;
      AddInputPlugins(app);
      app.Initialize();
      test::ScopedShutdown shutdown{app};

      auto& mappings = app.GetWorld().WriteResource<input::GamepadMappings>();
      mappings.Add(
          "03000000000000000000000000000000,Test "
          "Pad,a:b0,b:b1,back:b6,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0."
          "1,guide:b8,leftshoulder:b4,leftstick:b9,lefttrigger:a2,leftx:a0,"
          "lefty:a1,rightshoulder:b5,rightstick:b10,righttrigger:a5,rightx:a3,"
          "righty:a4,start:b7,x:b2,y:b3,platform:Windows,");

      auto local_data = ecs::SystemLocalData::From();
      const ecs::AccessPolicy policy =
          ecs::BuildPolicyFromParams<ecs::Res<const Context>,
                                     ecs::Res<input::GamepadMappings>>();
      ApplyGamepadMappings{}(
          ecs::SystemParamTraits<ecs::Res<const Context>>::Make(
              app.GetWorld(), local_data, policy),
          ecs::SystemParamTraits<ecs::Res<input::GamepadMappings>>::Make(
              app.GetWorld(), local_data, policy));

      CHECK_FALSE(app.GetWorld().ReadResource<input::GamepadMappings>().dirty);
      CHECK(app.GetWorld()
                .ReadResource<input::GamepadMappings>()
                .pending_lines.empty());
    }
  }
}

TEST_SUITE("helios::sdl3::input::PollGamepads") {
  TEST_CASE("helios::sdl3::input::PollGamepads::operator()") {
    SUBCASE("Runs without connected gamepads") {
      HELIOS_SKIP_IF_NO_SDL_RUNTIME();

      app::App app;
      AddInputPlugins(app);
      app.Initialize();
      test::ScopedShutdown shutdown{app};
      app.Update();

      CHECK(app.GetWorld().HasResource<GamepadCache>());
      CHECK(app.GetWorld()
                .Messages()
                .PreviousMessages<input::JoystickConnectionMsg>()
                .empty());
    }

    SUBCASE("Records extra gamepad touchpad indices when present") {
      HELIOS_SKIP_IF_NO_SDL_RUNTIME();

      app::App app;
      AddInputPlugins(app);
      app.Initialize();
      test::ScopedShutdown shutdown{app};
      app.Update();

      const auto touchpads = app.GetWorld()
                                 .Messages()
                                 .PreviousMessages<input::GamepadTouchpadMsg>();
      bool found_extra = false;
      for (const auto& msg : touchpads) {
        CHECK_LT(msg.touchpad, input::Gamepad::kMaxTouchpads);
        if (msg.touchpad >= 1) {
          found_extra = true;
        }
      }
      if (!found_extra) {
        MESSAGE("No extra gamepad touchpad; skipping");
        return;
      }
      CHECK(found_extra);
    }
  }
}

TEST_SUITE("helios::sdl3::input::ApplyGamepadOutputs") {
  TEST_CASE("helios::sdl3::input::ApplyGamepadOutputs::operator()") {
    SUBCASE("Clears dirty output flags without an open SDL gamepad") {
      HELIOS_SKIP_IF_NO_SDL_RUNTIME();

      app::App app;
      AddInputPlugins(app);
      app.Initialize();
      test::ScopedShutdown shutdown{app};

      auto& pads = app.GetWorld().WriteResource<input::Gamepads>();
      pads.pads[0].connected = true;
      pads.pads[0].SetRumble(1, 2, 3);
      pads.pads[0].SetLed(4, 5, 6);

      ecs::SystemLocalData local_data = ecs::SystemLocalData::From();
      const ecs::AccessPolicy policy =
          ecs::BuildPolicyFromParams<ecs::Res<const Context>,
                                     ecs::Res<input::Gamepads>,
                                     ecs::Res<GamepadCache>>();
      ApplyGamepadOutputs{}(
          ecs::SystemParamTraits<ecs::Res<const Context>>::Make(
              app.GetWorld(), local_data, policy),
          ecs::SystemParamTraits<ecs::Res<input::Gamepads>>::Make(
              app.GetWorld(), local_data, policy),
          ecs::SystemParamTraits<ecs::Res<GamepadCache>>::Make(
              app.GetWorld(), local_data, policy));

      CHECK_FALSE(app.GetWorld().ReadResource<input::Gamepads>().pads[0].Dirty(
          input::GamepadDirtyFlags::kRumble));
      CHECK_FALSE(app.GetWorld().ReadResource<input::Gamepads>().pads[0].Dirty(
          input::GamepadDirtyFlags::kLed));
    }
  }
}
#endif
