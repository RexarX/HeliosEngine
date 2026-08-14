#include <doctest/doctest.h>

#include <helios/app/app.hpp>
#include <helios/app/application.hpp>
#include <helios/glfw/details/glfw_state.hpp>
#include <helios/glfw/glfw.hpp>
#include <helios/glfw/systems/apply.hpp>
#include <helios/glfw/systems/destroy.hpp>
#include <helios/window/window.hpp>

#include "available.hpp"

#ifdef HELIOS_MODULE_INPUT_AVAILABLE
#include <helios/glfw/systems/input.hpp>
#include <helios/input/input.hpp>
#endif

#include <GLFW/glfw3.h>

#include <string>
#include <variant>

using namespace helios::app;
using namespace helios::ecs;
using namespace helios::glfw;
using namespace helios::window;

namespace {

[[nodiscard]] Properties HiddenTestProperties(
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

  void operator()(Windows windows) const {
    for (auto&& [window] : windows.query) {
      if (window.close_requested && dirty_cleared != nullptr) {
        *dirty_cleared = window.dirty_flags == DirtyFlag::kNone;
      }
    }
  }
};

}  // namespace

TEST_SUITE("helios::glfw::CreateNativeWindows") {
  TEST_CASE("helios::glfw::CreateNativeWindows::operator()") {
    SUBCASE("Creates a hidden window and emits CreatedMsg") {
      HELIOS_SKIP_IF_NO_GLFW();

      App app;
      app.AddPluginGroups(WindowPlugin{});
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};

      auto& world = app.GetWorld();
      const Entity entity = world.CreateEntity();
      world.AddBundle(
          entity, PrimaryWindow{
                      .window = Window::FromProperties(HiddenTestProperties()),
                  });
      app.Update();

      CHECK(world.HasComponent<NativeHandleComponent>(entity));
      CHECK_FALSE(world.HasComponent<CreationFailed>(entity));
      CHECK(world.ReadResource<NativeWindows>().Contains(entity));
      CHECK(world.ReadResource<Context>().initialized);
      CHECK_FALSE(world.ReadComponent<Window>(entity).Dirty(DirtyFlag::kTitle));

      const auto created = world.Messages().PreviousMessages<CreatedMsg>();
      REQUIRE_EQ(created.size(), 1U);
      CHECK_EQ(created[0].entity, entity);
      CHECK_EQ(world.ReadComponent<Window>(entity).properties.title,
               "HeliosTest");

#if defined(HELIOS_PLATFORM_WINDOWS)
      CHECK(std::holds_alternative<Win32Handle>(
          world.ReadComponent<NativeHandleComponent>(entity).handle));
#endif
    }
  }
}

TEST_SUITE("helios::glfw::ApplyChanges") {
  TEST_CASE("helios::glfw::ApplyChanges::operator()") {
    SUBCASE("Applies title, size, and visibility then clears dirty flags") {
      HELIOS_SKIP_IF_NO_GLFW();

      App app;
      app.AddPluginGroups(WindowPlugin{});
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};

      auto& world = app.GetWorld();
      const Entity entity = world.CreateEntity();
      world.AddComponents(
          entity, Window::FromProperties(HiddenTestProperties("Apply")));
      app.Update();

      auto& window = world.WriteComponent<Window>(entity);
      window.SetTitle("Applied");
      window.SetSize(80, 48);
      window.SetVisible(false);
      window.SetOpacity(0.75F);
      window.SetMousePassthrough(true);
      app.Update();

      const auto& updated = world.ReadComponent<Window>(entity);
      CHECK_EQ(updated.properties.title, "Applied");
      CHECK_EQ(updated.dirty_flags, DirtyFlag::kNone);
      CHECK_FALSE(updated.properties.visible);
      CHECK(updated.properties.mouse_passthrough);

      const auto titles =
          world.Messages().PreviousMessages<VisibilityChangedMsg>();
      CHECK_FALSE(titles.empty());
    }

    SUBCASE("Clears dirty flags on close-requested windows") {
      HELIOS_SKIP_IF_NO_GLFW();

      bool dirty_cleared = false;
      App app;
      app.AddPluginGroups(WindowPlugin{});
      RegisterEventsSchedule(app.GetMainSubApp().GetScheduler());
      app.AddSystem(kEvents, CaptureCloseRequestedDirty{&dirty_cleared})
          .After<ApplyChanges>()
          .Before<DestroyClosedWindows>();
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};

      auto& world = app.GetWorld();
      const Entity entity = world.CreateEntity();
      world.AddComponents(
          entity, Window::FromProperties(HiddenTestProperties("Closing")));
      app.Update();

      auto& window = world.WriteComponent<Window>(entity);
      window.SetTitle("Closing");
      window.RequestClose();
      app.Update();

      CHECK(dirty_cleared);
    }

    SUBCASE("Applies exclusive fullscreen from the desktop video mode") {
      HELIOS_SKIP_IF_NO_GLFW();

      App app;
      app.AddPluginGroups(WindowPlugin{});
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};
      HELIOS_SKIP_IF_NO_MONITOR();

      auto& world = app.GetWorld();
      const Entity entity = world.CreateEntity();
      world.AddComponents(
          entity, Window::FromProperties(HiddenTestProperties("Exclusive")));
      app.Update();

      world.WriteComponent<Window>(entity).SetMode(Mode::kFullscreen);
      app.Update();

      const auto& updated = world.ReadComponent<Window>(entity);
      CHECK_EQ(updated.dirty_flags, DirtyFlag::kNone);
      CHECK_EQ(updated.properties.mode, Mode::kFullscreen);

      const NativeWindows::Entry* entry =
          world.ReadResource<NativeWindows>().TryGet(entity);
      REQUIRE_NE(entry, nullptr);
      REQUIRE_NE(entry->native.window, nullptr);
      GLFWmonitor* monitor = glfwGetWindowMonitor(entry->native.window);
      if (monitor == nullptr) {
        MESSAGE(
            "Exclusive fullscreen was not applied (virtual/CI display); "
            "skipping Hz change");
        world.WriteComponent<Window>(entity).SetMode(Mode::kWindowed);
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
        world.WriteComponent<Window>(entity).SetRefreshRate(other_hz);
        app.Update();

        entry = world.ReadResource<NativeWindows>().TryGet(entity);
        REQUIRE_NE(entry, nullptr);
        monitor = glfwGetWindowMonitor(entry->native.window);
        REQUIRE_NE(monitor, nullptr);
        current = glfwGetVideoMode(monitor);
        REQUIRE_NE(current, nullptr);
        CHECK_EQ(static_cast<uint32_t>(current->refreshRate), other_hz);
      }

      world.WriteComponent<Window>(entity).SetMode(Mode::kWindowed);
      app.Update();
    }
  }
}

TEST_SUITE("helios::glfw::DestroyClosedWindows") {
  TEST_CASE("helios::glfw::DestroyClosedWindows::operator()") {
    SUBCASE("Destroys a closed primary window and requests AppExit") {
      HELIOS_SKIP_IF_NO_GLFW();

      App app;
      app.AddPluginGroups(WindowPlugin{});
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};

      auto& world = app.GetWorld();
      const Entity entity = world.CreateEntity();
      world.AddBundle(
          entity, PrimaryWindow{
                      .window = Window::FromProperties(HiddenTestProperties()),
                  });
      app.Update();
      world.WriteComponent<Window>(entity).RequestClose();
      app.Update();

      CHECK_FALSE(world.Exists(entity));
      CHECK(world.ReadResource<NativeWindows>().Empty());

      const auto closed = world.Messages().PreviousMessages<ClosedMsg>();
      REQUIRE_EQ(closed.size(), 1U);
      CHECK_EQ(closed[0].entity, entity);

      const auto exits = world.Messages().PreviousMessages<AppExit>();
      CHECK_FALSE(exits.empty());
    }

    SUBCASE("Does not request exit when triggers are none") {
      HELIOS_SKIP_IF_NO_GLFW();

      App app;
      app.AddPluginGroups(WindowPlugin{}.Configure(
          helios::window::Plugin{{.exit_triggers = kExitTriggersNone}}));
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};

      auto& world = app.GetWorld();
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity,
                          Window::FromProperties(HiddenTestProperties()));
      app.Update();
      world.WriteComponent<Window>(entity).RequestClose();
      app.Update();

      CHECK_FALSE(world.Exists(entity));
      CHECK(world.Messages().PreviousMessages<AppExit>().empty());
    }
  }
}

TEST_SUITE("helios::glfw::PollEvents") {
  TEST_CASE("helios::glfw::PollEvents::operator()") {
    SUBCASE("Writes pending clipboard text") {
      HELIOS_SKIP_IF_NO_GLFW();

      App app;
      app.AddPluginGroups(WindowPlugin{});
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};

      auto& world = app.GetWorld();
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity,
                          Window::FromProperties(HiddenTestProperties()));
      app.Update();

      auto& clipboard = world.WriteResource<Clipboard>();
      clipboard.text = "helios-clipboard";
      clipboard.pending_write = true;
      app.Update();

      CHECK_FALSE(world.ReadResource<Clipboard>().pending_write);
      CHECK_EQ(world.ReadResource<Clipboard>().text, "helios-clipboard");
    }

    SUBCASE("WaitTimeout returns and writes pending clipboard text") {
      HELIOS_SKIP_IF_NO_GLFW();

      App app;
      app.AddPluginGroups(WindowPlugin{}.Configure(helios::window::Plugin{{
          .event_mode = EventMode::kWaitTimeout,
          .event_wait_timeout = 0.001,
      }}));
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};

      auto& world = app.GetWorld();
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity,
                          Window::FromProperties(HiddenTestProperties()));
      app.Update();

      auto& clipboard = world.WriteResource<Clipboard>();
      clipboard.text = "helios-wait";
      clipboard.pending_write = true;
      app.Update();

      CHECK_FALSE(world.ReadResource<Clipboard>().pending_write);
      CHECK_EQ(world.ReadResource<Clipboard>().text, "helios-wait");
    }
  }
}

TEST_SUITE("helios::glfw::Init") {
  TEST_CASE("helios::glfw::Init::operator()") {
    SUBCASE("Marks context initialized and snapshots monitors") {
      HELIOS_SKIP_IF_NO_GLFW();

      App app;
      app.AddPluginGroups(WindowPlugin{});
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};

      CHECK(app.GetWorld().ReadResource<Context>().initialized);
      CHECK_NE(app.GetWorld().ReadResource<Context>().world, nullptr);
      if (glfwGetPrimaryMonitor() == nullptr) {
        CHECK(app.GetWorld().ReadResource<Monitors>().monitors.empty());
        return;
      }
      CHECK_FALSE(app.GetWorld().ReadResource<Monitors>().monitors.empty());
    }
  }
}

TEST_SUITE("helios::glfw::Shutdown") {
  TEST_CASE("helios::glfw::Shutdown::operator()") {
    SUBCASE("Terminates GLFW and clears native windows") {
      HELIOS_SKIP_IF_NO_GLFW();

      App app;
      app.AddPluginGroups(WindowPlugin{});
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};

      auto& world = app.GetWorld();
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity,
                          Window::FromProperties(HiddenTestProperties()));
      app.Update();
      CHECK(world.ReadResource<NativeWindows>().Contains(entity));

      helios::glfw::Plugin{}.Destroy(app);

      CHECK_FALSE(world.ReadResource<Context>().initialized);
      CHECK(world.ReadResource<NativeWindows>().Empty());
    }
  }
}

#ifdef HELIOS_MODULE_INPUT_AVAILABLE
TEST_SUITE("helios::glfw::ApplyCursors") {
  TEST_CASE("helios::glfw::ApplyCursors::operator()") {
    SUBCASE("Applies a standard cursor icon and clears dirty") {
      HELIOS_SKIP_IF_NO_GLFW();

      App app;
      app.AddPluginGroups(WindowInputPlugin{});
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};

      auto& world = app.GetWorld();
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity,
                          Window::FromProperties(HiddenTestProperties()),
                          helios::input::Cursor{});
      app.Update();

      world.WriteComponent<helios::input::Cursor>(entity).SetIcon(
          helios::input::CursorIcon::kIBeam);
      app.Update();

      CHECK_FALSE(world.ReadComponent<helios::input::Cursor>(entity).dirty);
      CHECK_EQ(world.ReadComponent<helios::input::Cursor>(entity).icon,
               helios::input::CursorIcon::kIBeam);
    }
  }
}

TEST_SUITE("helios::glfw::ApplyRawMouseMotion") {
  TEST_CASE("helios::glfw::ApplyRawMouseMotion::operator()") {
    SUBCASE("Enables and disables raw motion for a disabled cursor") {
      HELIOS_SKIP_IF_NO_GLFW();

      App app;
      app.AddPluginGroups(WindowInputPlugin{});
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};

      if (glfwRawMouseMotionSupported() != GLFW_TRUE) {
        MESSAGE("Raw mouse motion is unsupported; skipping");
        return;
      }

      auto& world = app.GetWorld();
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity,
                          Window::FromProperties(HiddenTestProperties()));
      app.Update();

      world.WriteComponent<Window>(entity).SetCursorMode(CursorMode::kDisabled);
      world.WriteResource<helios::input::Settings>().raw_mouse_motion = true;
      app.Update();

      const NativeWindows::Entry* entry =
          world.ReadResource<NativeWindows>().TryGet(entity);
      REQUIRE_NE(entry, nullptr);
      REQUIRE_NE(entry->native.window, nullptr);
      CHECK_EQ(glfwGetInputMode(entry->native.window, GLFW_RAW_MOUSE_MOTION),
               GLFW_TRUE);

      world.WriteResource<helios::input::Settings>().raw_mouse_motion = false;
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

      App app;
      app.AddPluginGroups(WindowInputPlugin{});
      app.Initialize();
      test::ScopedGlfwShutdown shutdown{app};
      app.Update();

      CHECK(app.GetWorld().HasResource<GamepadCache>());
      CHECK(app.GetWorld()
                .Messages()
                .PreviousMessages<helios::input::GamepadConnectionMsg>()
                .empty());
    }
  }
}
#endif
