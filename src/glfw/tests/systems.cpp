#ifndef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

#include <helios/app/app.hpp>
#include <helios/app/application.hpp>
#include <helios/glfw/glfw.hpp>
#include <helios/glfw/state.hpp>
#include <helios/glfw/systems/apply.hpp>
#include <helios/glfw/systems/destroy.hpp>
#include <helios/window/window.hpp>

#include "available.hpp"

#ifdef HELIOS_MODULE_INPUT_AVAILABLE
#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/schedule/system_local_data.hpp>
#include <helios/ecs/system/param.hpp>
#include <helios/ecs/system/param_policy.hpp>
#include <helios/glfw/sync.hpp>
#include <helios/glfw/systems/input.hpp>
#include <helios/input/input.hpp>
#endif

#include <GLFW/glfw3.h>

#include <cstdint>
#include <string>

using namespace helios;
using namespace helios::glfw;

namespace {

[[nodiscard]] window::Properties HiddenTestProperties(
    std::string title = "HeliosTest") {
  return {
      .title = std::move(title),
      .width = 64,
      .height = 64,
      .visible = false,
      .focused = false,
  };
}

struct CaptureCloseRequestedDirty {
  bool* dirty_cleared = nullptr;

  void operator()(window::Windows windows) const {
    for (auto&& [window] : windows.query) {
      if (window.close_requested && dirty_cleared != nullptr) {
        *dirty_cleared = window.dirty_flags == window::DirtyFlag::kNone;
      }
    }
  }
};

}  // namespace

TEST_SUITE("helios::glfw::CreateNativeWindows") {
  TEST_CASE("helios::glfw::CreateNativeWindows::operator()") {
    SUBCASE("Creates a hidden window and emits CreatedMsg") {
      HELIOS_SKIP_IF_NO_GLFW();

      app::App app;
      app.AddPluginGroups(WindowPlugin{});
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};

      auto& world = app.GetWorld();
      const ecs::Entity entity = world.CreateEntity();
      world.AddBundle(
          entity,
          window::PrimaryWindow{
              .window = window::Window::FromProperties(HiddenTestProperties()),
          });
      app.Update();

      CHECK(world.HasComponent<window::NativeHandleComponent>(entity));
      CHECK_FALSE(world.HasComponent<window::CreationFailed>(entity));
      CHECK(world.ReadResource<NativeWindows>().Contains(entity));
      CHECK(world.ReadResource<Context>().initialized);
      CHECK_FALSE(world.ReadComponent<window::Window>(entity).Dirty(
          window::DirtyFlag::kTitle));

      const auto created =
          world.Messages().PreviousMessages<window::CreatedMsg>();
      REQUIRE_EQ(created.size(), 1U);
      CHECK_EQ(created[0].entity, entity);
      CHECK_EQ(created[0].properties.title, "HeliosTest");
      CHECK_EQ(world.ReadComponent<window::Window>(entity).properties.title,
               "HeliosTest");

#if defined(HELIOS_PLATFORM_WINDOWS)
      CHECK(std::holds_alternative<window::Win32Handle>(
          world.ReadComponent<window::NativeHandleComponent>(entity).handle));
#endif
    }
  }
}

TEST_SUITE("helios::glfw::window::ApplyChanges") {
  TEST_CASE("helios::glfw::window::ApplyChanges::operator()") {
    SUBCASE("Applies title, size, and visibility then clears dirty flags") {
      HELIOS_SKIP_IF_NO_GLFW();

      app::App app;
      app.AddPluginGroups(WindowPlugin{});
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};

      auto& world = app.GetWorld();
      const ecs::Entity entity = world.CreateEntity();
      world.AddComponents(entity, window::Window::FromProperties(
                                      HiddenTestProperties("Apply")));
      app.Update();

      auto& window = world.WriteComponent<window::Window>(entity);
      window.SetTitle("Applied");
      window.SetSize(80, 48);
      window.SetVisible(false);
      window.SetOpacity(0.75F);
      window.SetMousePassthrough(true);
      app.Update();

      const auto& updated = world.ReadComponent<window::Window>(entity);
      CHECK_EQ(updated.properties.title, "Applied");
      CHECK_EQ(updated.dirty_flags, window::DirtyFlag::kNone);
      CHECK_FALSE(updated.properties.visible);
      CHECK(updated.properties.mouse_passthrough);

      const auto titles =
          world.Messages().PreviousMessages<window::VisibilityChangedMsg>();
      CHECK_FALSE(titles.empty());
    }

    SUBCASE("Clears dirty flags on close-requested windows") {
      HELIOS_SKIP_IF_NO_GLFW();

      bool dirty_cleared = false;
      app::App app;
      app.AddPluginGroups(WindowPlugin{});
      window::RegisterEventsSchedule(app.GetMainSubApp().GetScheduler());
      app.AddSystem(window::kEvents, CaptureCloseRequestedDirty{&dirty_cleared})
          .After<ApplyChanges>()
          .Before<DestroyClosedWindows>();
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};

      auto& world = app.GetWorld();
      const ecs::Entity entity = world.CreateEntity();
      world.AddComponents(entity, window::Window::FromProperties(
                                      HiddenTestProperties("Closing")));
      app.Update();

      auto& window = world.WriteComponent<window::Window>(entity);
      window.SetTitle("Closing");
      window.RequestClose();
      app.Update();

      CHECK(dirty_cleared);
    }

    SUBCASE("Applies exclusive fullscreen from the desktop video mode") {
      HELIOS_SKIP_IF_NO_GLFW();

      app::App app;
      app.AddPluginGroups(WindowPlugin{});
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};
      HELIOS_SKIP_IF_NO_MONITOR();

      auto& world = app.GetWorld();
      const ecs::Entity entity = world.CreateEntity();
      world.AddComponents(entity, window::Window::FromProperties(
                                      HiddenTestProperties("Exclusive")));
      app.Update();

      world.WriteComponent<window::Window>(entity).SetMode(
          window::Mode::kFullscreen);
      app.Update();

      const auto& updated = world.ReadComponent<window::Window>(entity);
      CHECK_EQ(updated.dirty_flags, window::DirtyFlag::kNone);
      CHECK_EQ(updated.properties.mode, window::Mode::kFullscreen);

      const NativeWindows::Entry* entry =
          world.ReadResource<NativeWindows>().TryGet(entity);
      REQUIRE_NE(entry, nullptr);
      REQUIRE_NE(entry->native.window, nullptr);
      GLFWmonitor* monitor = glfwGetWindowMonitor(entry->native.window);
      if (monitor == nullptr) {
        MESSAGE(
            "Exclusive fullscreen was not applied (virtual/CI display); "
            "skipping Hz change");
        world.WriteComponent<window::Window>(entity).SetMode(
            window::Mode::kWindowed);
        app.Update();
        return;
      }

      int mode_count = 0;
      const GLFWvidmode* modes = glfwGetVideoModes(monitor, &mode_count);
      const GLFWvidmode* current = glfwGetVideoMode(monitor);
      REQUIRE_NE(current, nullptr);

      uint32_t other_hz = 0;
      for (int index = 0; index < mode_count; ++index) {
        if (modes[index].width == current->width &&
            modes[index].height == current->height &&
            modes[index].refreshRate != current->refreshRate) {
          other_hz = static_cast<uint32_t>(modes[index].refreshRate);
          break;
        }
      }

      if (other_hz == 0U) {
        MESSAGE("Monitor has no alternate refresh rate; skipping Hz change");
      } else {
        world.WriteComponent<window::Window>(entity).SetRefreshRate(other_hz);
        app.Update();

        entry = world.ReadResource<NativeWindows>().TryGet(entity);
        REQUIRE_NE(entry, nullptr);
        monitor = glfwGetWindowMonitor(entry->native.window);
        REQUIRE_NE(monitor, nullptr);
        current = glfwGetVideoMode(monitor);
        REQUIRE_NE(current, nullptr);
        CHECK_EQ(static_cast<uint32_t>(current->refreshRate), other_hz);
      }

      world.WriteComponent<window::Window>(entity).SetMode(
          window::Mode::kWindowed);
      app.Update();
    }
  }
}

TEST_SUITE("helios::glfw::window::DestroyClosedWindows") {
  TEST_CASE("helios::glfw::window::DestroyClosedWindows::operator()") {
    SUBCASE("Destroys a closed primary window and requests AppExit") {
      HELIOS_SKIP_IF_NO_GLFW();

      app::App app;
      app.AddPluginGroups(WindowPlugin{});
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};

      auto& world = app.GetWorld();
      const ecs::Entity entity = world.CreateEntity();
      world.AddBundle(
          entity,
          window::PrimaryWindow{
              .window = window::Window::FromProperties(HiddenTestProperties()),
          });
      app.Update();
      world.WriteComponent<window::Window>(entity).RequestClose();
      app.Update();

      CHECK_FALSE(world.Exists(entity));
      CHECK(world.ReadResource<NativeWindows>().Empty());

      const auto closed =
          world.Messages().PreviousMessages<window::ClosedMsg>();
      REQUIRE_EQ(closed.size(), 1U);
      CHECK_EQ(closed[0].entity, entity);

      const auto exits = world.Messages().PreviousMessages<app::AppExit>();
      CHECK_FALSE(exits.empty());
    }

    SUBCASE("Does not request exit when triggers are none") {
      HELIOS_SKIP_IF_NO_GLFW();

      app::App app;
      app.AddPluginGroups(WindowPlugin{}.Configure(
          window::Plugin{{.exit_triggers = window::kExitTriggersNone}}));
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};

      auto& world = app.GetWorld();
      const ecs::Entity entity = world.CreateEntity();
      world.AddComponents(
          entity, window::Window::FromProperties(HiddenTestProperties()));
      app.Update();
      world.WriteComponent<window::Window>(entity).RequestClose();
      app.Update();

      CHECK_FALSE(world.Exists(entity));
      CHECK(world.Messages().PreviousMessages<app::AppExit>().empty());
    }
  }
}

TEST_SUITE("helios::glfw::PollEvents") {
  TEST_CASE("helios::glfw::PollEvents::operator()") {
    SUBCASE("Writes pending clipboard text") {
      HELIOS_SKIP_IF_NO_GLFW();

      app::App app;
      app.AddPluginGroups(WindowPlugin{});
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};

      auto& world = app.GetWorld();
      const ecs::Entity entity = world.CreateEntity();
      world.AddComponents(
          entity, window::Window::FromProperties(HiddenTestProperties()));
      app.Update();

      auto& clipboard = world.WriteResource<window::Clipboard>();
      clipboard.text = "helios-clipboard";
      clipboard.pending_write = true;
      app.Update();

      CHECK_FALSE(world.ReadResource<window::Clipboard>().pending_write);
      CHECK_EQ(world.ReadResource<window::Clipboard>().text,
               "helios-clipboard");
    }

    SUBCASE("WaitTimeout returns and writes pending clipboard text") {
      HELIOS_SKIP_IF_NO_GLFW();

      app::App app;
      app.AddPluginGroups(WindowPlugin{}.Configure(window::Plugin{{
          .event_wait_timeout = 0.001,
          .event_mode = window::EventMode::kWaitTimeout,
      }}));
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};

      auto& world = app.GetWorld();
      const ecs::Entity entity = world.CreateEntity();
      world.AddComponents(
          entity, window::Window::FromProperties(HiddenTestProperties()));
      app.Update();

      auto& clipboard = world.WriteResource<window::Clipboard>();
      clipboard.text = "helios-wait";
      clipboard.pending_write = true;
      app.Update();

      CHECK_FALSE(world.ReadResource<window::Clipboard>().pending_write);
      CHECK_EQ(world.ReadResource<window::Clipboard>().text, "helios-wait");
    }
  }
}

TEST_SUITE("helios::glfw::Init") {
  TEST_CASE("helios::glfw::Init::operator()") {
    SUBCASE("Marks context initialized and snapshots monitors") {
      HELIOS_SKIP_IF_NO_GLFW();

      app::App app;
      app.AddPluginGroups(WindowPlugin{});
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};

      CHECK(app.GetWorld().ReadResource<Context>().initialized);
      CHECK_NE(app.GetWorld().ReadResource<Context>().world, nullptr);
      if (glfwGetPrimaryMonitor() == nullptr) {
        CHECK(app.GetWorld().ReadResource<window::Monitors>().monitors.empty());
        return;
      }
      CHECK_FALSE(
          app.GetWorld().ReadResource<window::Monitors>().monitors.empty());
    }
  }
}

TEST_SUITE("helios::glfw::Shutdown") {
  TEST_CASE("helios::glfw::Shutdown::operator()") {
    SUBCASE("Terminates GLFW and clears native windows") {
      HELIOS_SKIP_IF_NO_GLFW();

      app::App app;
      app.AddPluginGroups(WindowPlugin{});
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};

      auto& world = app.GetWorld();
      const ecs::Entity entity = world.CreateEntity();
      world.AddComponents(
          entity, window::Window::FromProperties(HiddenTestProperties()));
      app.Update();
      CHECK(world.ReadResource<NativeWindows>().Contains(entity));

      Plugin{}.Destroy(app);

      CHECK_FALSE(world.ReadResource<Context>().initialized);
      CHECK(world.ReadResource<NativeWindows>().Empty());
    }
  }
}

#ifdef HELIOS_MODULE_INPUT_AVAILABLE
TEST_SUITE("helios::glfw::ApplyCursorMode") {
  TEST_CASE("helios::glfw::ApplyCursorMode") {
    SUBCASE("Sets captured cursor mode on a native window") {
      HELIOS_SKIP_IF_NO_GLFW();

      app::App app;
      app.AddPluginGroups(WindowInputPlugin{});
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};

      auto& world = app.GetWorld();
      const ecs::Entity entity = world.CreateEntity();
      world.AddComponents(
          entity, window::Window::FromProperties(HiddenTestProperties()));
      app.Update();

      NativeWindows::Entry* entry =
          world.WriteResource<NativeWindows>().TryGet(entity);
      REQUIRE_NE(entry, nullptr);
      REQUIRE_NE(entry->native.window, nullptr);

      ApplyCursorMode(*entry->native.window, window::CursorMode::kCaptured);
      CHECK_EQ(glfwGetInputMode(entry->native.window, GLFW_CURSOR),
               GLFW_CURSOR_CAPTURED);
    }
  }
}

TEST_SUITE("helios::glfw::ApplyCursors") {
  TEST_CASE("helios::glfw::ApplyCursors::operator()") {
    SUBCASE("Applies a standard cursor icon and clears dirty") {
      HELIOS_SKIP_IF_NO_GLFW();

      app::App app;
      app.AddPluginGroups(WindowInputPlugin{});
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};

      auto& world = app.GetWorld();
      const ecs::Entity entity = world.CreateEntity();
      world.AddComponents(
          entity, window::Window::FromProperties(HiddenTestProperties()),
          input::Cursor{});
      app.Update();

      world.WriteComponent<input::Cursor>(entity).SetIcon(
          input::CursorIcon::kIBeam);
      app.Update();

      CHECK_FALSE(world.ReadComponent<input::Cursor>(entity).dirty);
      CHECK_EQ(world.ReadComponent<input::Cursor>(entity).icon,
               input::CursorIcon::kIBeam);
    }
  }
}

TEST_SUITE("helios::glfw::ApplyRawMouseMotion") {
  TEST_CASE("helios::glfw::ApplyRawMouseMotion::operator()") {
    SUBCASE("Enables and disables raw motion for a disabled cursor") {
      HELIOS_SKIP_IF_NO_GLFW();

      app::App app;
      app.AddPluginGroups(WindowInputPlugin{});
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};

      if (glfwRawMouseMotionSupported() != GLFW_TRUE) {
        MESSAGE("Raw mouse motion is unsupported; skipping");
        return;
      }

      auto& world = app.GetWorld();
      const ecs::Entity entity = world.CreateEntity();
      world.AddComponents(
          entity, window::Window::FromProperties(HiddenTestProperties()));
      app.Update();

      world.WriteComponent<window::Window>(entity).SetCursorMode(
          window::CursorMode::kDisabled);
      world.WriteResource<input::Settings>().raw_mouse_motion = true;
      app.Update();

      const NativeWindows::Entry* entry =
          world.ReadResource<NativeWindows>().TryGet(entity);
      REQUIRE_NE(entry, nullptr);
      REQUIRE_NE(entry->native.window, nullptr);
      CHECK_EQ(glfwGetInputMode(entry->native.window, GLFW_RAW_MOUSE_MOTION),
               GLFW_TRUE);

      world.WriteResource<input::Settings>().raw_mouse_motion = false;
      app.Update();

      entry = world.ReadResource<NativeWindows>().TryGet(entity);
      REQUIRE_NE(entry, nullptr);
      REQUIRE_NE(entry->native.window, nullptr);
      CHECK_EQ(glfwGetInputMode(entry->native.window, GLFW_RAW_MOUSE_MOTION),
               GLFW_FALSE);
    }
  }
}

TEST_SUITE("helios::glfw::PollGamepads") {
  TEST_CASE("helios::glfw::PollGamepads::operator()") {
    SUBCASE("Runs without connected gamepads") {
      HELIOS_SKIP_IF_NO_GLFW();

      app::App app;
      app.AddPluginGroups(WindowInputPlugin{});
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};
      app.Update();

      CHECK(app.GetWorld().HasResource<GamepadCache>());
      CHECK(app.GetWorld()
                .Messages()
                .PreviousMessages<input::JoystickConnectionMsg>()
                .empty());
    }
  }
}

TEST_SUITE("helios::glfw::ApplyGamepadMappings") {
  TEST_CASE("helios::glfw::ApplyGamepadMappings::operator()") {
    SUBCASE("Consumes queued mapping lines") {
      HELIOS_SKIP_IF_NO_GLFW();

      app::App app;
      app.AddPluginGroups(WindowInputPlugin{});
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};

      auto& mappings = app.GetWorld().WriteResource<input::GamepadMappings>();
      mappings.Add(
          "03000000000000000000000000000000,Test "
          "Pad,a:b0,b:b1,back:b6,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0."
          "1,guide:b8,leftshoulder:b4,leftstick:b9,lefttrigger:a2,leftx:a0,"
          "lefty:a1,rightshoulder:b5,rightstick:b10,righttrigger:a5,rightx:a3,"
          "righty:a4,start:b7,x:b2,y:b3,platform:window::Windows,");

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

TEST_SUITE("helios::glfw::ApplyGamepadOutputs") {
  TEST_CASE("helios::glfw::ApplyGamepadOutputs::operator()") {
    SUBCASE("Clears dirty output flags") {
      HELIOS_SKIP_IF_NO_GLFW();

      app::App app;
      app.AddPluginGroups(WindowInputPlugin{});
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};

      auto& pads = app.GetWorld().WriteResource<input::Gamepads>();
      pads.pads[0].connected = true;
      pads.pads[0].SetRumble(1, 2, 3);
      pads.pads[0].SetLed(4, 5, 6);

      ecs::SystemLocalData local_data = ecs::SystemLocalData::From();
      const ecs::AccessPolicy policy =
          ecs::BuildPolicyFromParams<ecs::Res<const Context>,
                                     ecs::Res<input::Gamepads>>();
      ApplyGamepadOutputs{}(
          ecs::SystemParamTraits<ecs::Res<const Context>>::Make(
              app.GetWorld(), local_data, policy),
          ecs::SystemParamTraits<ecs::Res<input::Gamepads>>::Make(
              app.GetWorld(), local_data, policy));

      CHECK_FALSE(app.GetWorld().ReadResource<input::Gamepads>().pads[0].Dirty(
          input::GamepadDirtyFlags::kRumble));
      CHECK_FALSE(app.GetWorld().ReadResource<input::Gamepads>().pads[0].Dirty(
          input::GamepadDirtyFlags::kLed));
    }
  }
}
#endif
#endif
