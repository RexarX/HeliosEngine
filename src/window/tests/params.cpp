#include <doctest/doctest.h>

#include <helios/ecs/component/component.hpp>
#include <helios/ecs/schedule/system_local_data.hpp>
#include <helios/ecs/system/param_policy.hpp>
#include <helios/ecs/system/system_param.hpp>
#include <helios/ecs/world.hpp>
#include <helios/window/window.hpp>

using namespace helios::ecs;
using namespace helios::window;

namespace {

void AddWindowMessages(World& world) {
  world.AddMessages<CreatedMsg, ClosedMsg, CloseRequestedMsg, CreationFailedMsg,
                    ResizedMsg, ClientResizedMsg, ContentScaleChangedMsg,
                    PosChangedMsg, ModeChangedMsg, CursorModeChangedMsg,
                    VisibilityChangedMsg, FocusChangedMsg, MaximizedChangedMsg,
                    IconChangedMsg, ResizableChangedMsg, DecoratedChangedMsg,
                    OpacityChangedMsg, FloatingChangedMsg, HoverChangedMsg,
                    MousePassthroughChangedMsg, ClipboardChangedMsg,
                    DroppedFilesMsg, MonitorsChangedMsg>();
}

}  // namespace

TEST_SUITE("helios::window::Windows") {
  TEST_CASE("helios::window::Windows") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<Windows>);
    }

    SUBCASE("Make queries mutable Window components") {
      World world;
      const auto entity = world.CreateEntity();
      world.AddComponents(entity, Window{});

      auto local_data = SystemLocalData::From();
      const auto policy = BuildPolicyFromParams<Windows>();
      auto windows =
          SystemParamTraits<Windows>::Make(world, local_data, policy);

      CHECK_EQ(windows.query.Count(), 1);
    }

    SUBCASE("RegisterAccess requests write access to Window") {
      const auto policy = BuildPolicyFromParams<Windows>();
      CHECK(policy.HasWriteComponent(ComponentTypeIndex::From<Window>()));
    }
  }
}

TEST_SUITE("helios::window::WindowsView") {
  TEST_CASE("helios::window::WindowsView") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<WindowsView>);
    }

    SUBCASE("Make queries read-only Window components") {
      World world;
      const auto entity = world.CreateEntity();
      world.AddComponents(entity, Window{});

      auto local_data = SystemLocalData::From();
      const auto policy = BuildPolicyFromParams<WindowsView>();
      auto windows =
          SystemParamTraits<WindowsView>::Make(world, local_data, policy);

      CHECK_EQ(windows.query.Count(), 1);
    }

    SUBCASE("RegisterAccess requests read-only access to Window") {
      const auto policy = BuildPolicyFromParams<WindowsView>();
      CHECK(policy.HasReadComponent(ComponentTypeIndex::From<Window>()));
      CHECK_FALSE(policy.HasWriteComponent(ComponentTypeIndex::From<Window>()));
    }
  }
}

TEST_SUITE("helios::window::PrimaryWindows") {
  TEST_CASE("helios::window::PrimaryWindows") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<PrimaryWindows>);
    }

    SUBCASE("Make queries only windows tagged Primary") {
      World world;
      const auto primary = world.CreateEntity();
      const auto auxiliary = world.CreateEntity();
      world.AddBundle(primary, PrimaryWindow{});
      world.AddComponents(auxiliary, Window{});

      auto local_data = SystemLocalData::From();
      const auto policy = BuildPolicyFromParams<PrimaryWindows>();
      auto windows =
          SystemParamTraits<PrimaryWindows>::Make(world, local_data, policy);

      CHECK_EQ(windows.query.Count(), 1);
    }
  }
}

TEST_SUITE("helios::window::PrimaryWindowsView") {
  TEST_CASE("helios::window::PrimaryWindowsView") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<PrimaryWindowsView>);
    }

    SUBCASE("Make queries only windows tagged Primary") {
      World world;
      const auto primary = world.CreateEntity();
      const auto auxiliary = world.CreateEntity();
      world.AddBundle(primary, PrimaryWindow{});
      world.AddComponents(auxiliary, Window{});

      auto local_data = SystemLocalData::From();
      const auto policy = BuildPolicyFromParams<PrimaryWindowsView>();
      auto windows = SystemParamTraits<PrimaryWindowsView>::Make(
          world, local_data, policy);

      CHECK_EQ(windows.query.Count(), 1);
    }

    SUBCASE("RegisterAccess requests read-only access to Window") {
      const auto policy = BuildPolicyFromParams<PrimaryWindowsView>();
      CHECK(policy.HasReadComponent(ComponentTypeIndex::From<Window>()));
      CHECK_FALSE(policy.HasWriteComponent(ComponentTypeIndex::From<Window>()));
    }
  }
}

TEST_SUITE("helios::window::LifecycleMessages") {
  TEST_CASE("helios::window::LifecycleMessages") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<LifecycleMessages>);
    }

    SUBCASE("Make produces empty readers") {
      World world;
      AddWindowMessages(world);

      auto local_data = SystemLocalData::From();
      const auto policy = BuildPolicyFromParams<LifecycleMessages>();
      auto messages =
          SystemParamTraits<LifecycleMessages>::Make(world, local_data, policy);

      CHECK(messages.created.Empty());
      CHECK(messages.closed.Empty());
      CHECK(messages.close_requested.Empty());
      CHECK(messages.failed.Empty());
    }
  }
}

TEST_SUITE("helios::window::GeometryMessages") {
  TEST_CASE("helios::window::GeometryMessages") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<GeometryMessages>);
    }

    SUBCASE("Make produces empty readers") {
      World world;
      AddWindowMessages(world);

      auto local_data = SystemLocalData::From();
      const auto policy = BuildPolicyFromParams<GeometryMessages>();
      auto messages =
          SystemParamTraits<GeometryMessages>::Make(world, local_data, policy);

      CHECK(messages.resized.Empty());
      CHECK(messages.client_resized.Empty());
      CHECK(messages.content_scale.Empty());
      CHECK(messages.pos.Empty());
    }
  }
}

TEST_SUITE("helios::window::AppearanceMessages") {
  TEST_CASE("helios::window::AppearanceMessages") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<AppearanceMessages>);
    }

    SUBCASE("Make produces empty readers") {
      World world;
      AddWindowMessages(world);

      auto local_data = SystemLocalData::From();
      const auto policy = BuildPolicyFromParams<AppearanceMessages>();
      auto messages = SystemParamTraits<AppearanceMessages>::Make(
          world, local_data, policy);

      CHECK(messages.mode.Empty());
      CHECK(messages.cursor_mode.Empty());
      CHECK(messages.visibility.Empty());
      CHECK(messages.focus.Empty());
      CHECK(messages.mouse_passthrough.Empty());
    }
  }
}

TEST_SUITE("helios::window::PlatformMessages") {
  TEST_CASE("helios::window::PlatformMessages") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<PlatformMessages>);
    }

    SUBCASE("Make produces empty readers") {
      World world;
      AddWindowMessages(world);

      auto local_data = SystemLocalData::From();
      const auto policy = BuildPolicyFromParams<PlatformMessages>();
      auto messages =
          SystemParamTraits<PlatformMessages>::Make(world, local_data, policy);

      CHECK(messages.clipboard.Empty());
      CHECK(messages.dropped_files.Empty());
      CHECK(messages.monitors.Empty());
    }
  }
}

TEST_SUITE("helios::window::Messages") {
  TEST_CASE("helios::window::Messages") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<Messages>);
    }

    SUBCASE("Make produces grouped empty readers") {
      World world;
      AddWindowMessages(world);

      auto local_data = SystemLocalData::From();
      const auto policy = BuildPolicyFromParams<Messages>();
      auto messages =
          SystemParamTraits<Messages>::Make(world, local_data, policy);

      CHECK(messages.lifecycle.created.Empty());
      CHECK(messages.geometry.resized.Empty());
      CHECK(messages.appearance.mode.Empty());
      CHECK(messages.platform.monitors.Empty());
    }

    SUBCASE("Make observes messages written on the world") {
      World world;
      AddWindowMessages(world);
      world.WriteMessages<CreatedMsg>().Write({.entity = Entity{1, 1}});

      auto local_data = SystemLocalData::From();
      const auto policy = BuildPolicyFromParams<Messages>();
      auto messages =
          SystemParamTraits<Messages>::Make(world, local_data, policy);

      CHECK_FALSE(messages.lifecycle.created.Empty());
    }
  }
}

TEST_SUITE("helios::window::Writers") {
  TEST_CASE("helios::window::Writers") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<Writers>);
    }

    SUBCASE("Make produces writers that enqueue locally") {
      World world;
      AddWindowMessages(world);

      auto local_data = SystemLocalData::From();
      const auto policy = BuildPolicyFromParams<Writers>();
      auto writers =
          SystemParamTraits<Writers>::Make(world, local_data, policy);

      writers.lifecycle.created.Write({.entity = Entity{1, 1}});
      writers.geometry.resized.Write(
          {.entity = Entity{1, 1}, .width = 64, .height = 32});
      CHECK(SystemParam<Writers>);
    }
  }
}

TEST_SUITE("helios::window::CreationWriters") {
  TEST_CASE("helios::window::CreationWriters") {
    SUBCASE("Satisfies SystemParam") {
      CHECK(SystemParam<CreationWriters>);
    }

    SUBCASE("Make produces writers that enqueue locally") {
      World world;
      AddWindowMessages(world);

      auto local_data = SystemLocalData::From();
      const auto policy = BuildPolicyFromParams<CreationWriters>();
      auto writers =
          SystemParamTraits<CreationWriters>::Make(world, local_data, policy);

      writers.created.Write({.entity = Entity{2, 1}});
      writers.failed.Write({.entity = Entity{2, 1}, .reason = "test"});
      CHECK(SystemParam<CreationWriters>);
    }
  }
}
