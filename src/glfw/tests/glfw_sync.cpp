#include <doctest/doctest.h>

#include <helios/ecs/world.hpp>
#include <helios/glfw/details/glfw_state.hpp>
#include <helios/glfw/details/glfw_sync.hpp>
#include <helios/window/components.hpp>
#include <helios/window/properties.hpp>
#include <helios/window/resources.hpp>

#include "available.hpp"

#include <GLFW/glfw3.h>

using namespace helios::ecs;
using namespace helios::glfw;
using namespace helios::window;

TEST_SUITE("helios::glfw::MarkMonitorDependentDirty") {
  TEST_CASE("helios::glfw::MarkMonitorDependentDirty") {
    SUBCASE("Marks monitor dirty when a window pins a monitor index") {
      World world;
      NativeWindows native;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Window::FromProperties(Properties{
                                      .monitor_index = 0,
                                      .mode = Mode::kWindowed,
                                  }));
      native.Insert(entity, {});

      MarkMonitorDependentDirty(world, native);

      const auto& window = world.ReadComponent<Window>(entity);
      CHECK(window.Dirty(DirtyFlag::kMonitor));
      CHECK_FALSE(window.Dirty(DirtyFlag::kMode));
    }

    SUBCASE("Marks mode dirty for fullscreen presentation") {
      World world;
      NativeWindows native;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Window::FromProperties(Properties{
                                      .mode = Mode::kFullscreen,
                                  }));
      native.Insert(entity, {});

      MarkMonitorDependentDirty(world, native);

      const auto& window = world.ReadComponent<Window>(entity);
      CHECK(window.Dirty(DirtyFlag::kMode));
      CHECK_FALSE(window.Dirty(DirtyFlag::kMonitor));
    }

    SUBCASE("Skips entities without a Window component") {
      World world;
      NativeWindows native;
      const Entity entity = world.CreateEntity();
      native.Insert(entity, {});

      MarkMonitorDependentDirty(world, native);
      CHECK_FALSE(world.HasComponent<Window>(entity));
    }
  }
}

TEST_SUITE("helios::glfw::MonitorAtIndex") {
  TEST_CASE("helios::glfw::MonitorAtIndex") {
    SUBCASE("Returns the primary monitor at index 0") {
      HELIOS_SKIP_IF_NO_GLFW();
      REQUIRE_EQ(glfwInit(), GLFW_TRUE);
      if (glfwGetPrimaryMonitor() == nullptr) {
        glfwTerminate();
        MESSAGE("No GLFW monitor available (headless CI); skipping");
        return;
      }

      CHECK_NE(MonitorAtIndex(0), nullptr);
      CHECK_EQ(MonitorAtIndex(-1), nullptr);
      CHECK_EQ(MonitorAtIndex(10'000), nullptr);

      glfwTerminate();
    }
  }
}

TEST_SUITE("helios::glfw::RefreshMonitors") {
  TEST_CASE("helios::glfw::RefreshMonitors") {
    SUBCASE("Populates the monitors resource from GLFW") {
      HELIOS_SKIP_IF_NO_GLFW();
      REQUIRE_EQ(glfwInit(), GLFW_TRUE);
      if (glfwGetPrimaryMonitor() == nullptr) {
        glfwTerminate();
        MESSAGE("No GLFW monitor available (headless CI); skipping");
        return;
      }

      Monitors monitors;
      RefreshMonitors(monitors);

      CHECK_FALSE(monitors.monitors.empty());
      CHECK_GE(monitors.monitors[0].width, 1U);
      CHECK_GE(monitors.monitors[0].height, 1U);
      CHECK_FALSE(monitors.monitors[0].modes.empty());

      glfwTerminate();
    }
  }
}

TEST_SUITE("helios::glfw::ResolveCreationSize") {
  TEST_CASE("helios::glfw::ResolveCreationSize") {
    SUBCASE("Leaves an explicit size unchanged") {
      Window window = Window::FromProperties(Properties{
          .width = 64,
          .height = 48,
      });
      ResolveCreationSize(window);
      CHECK_EQ(*window.properties.width, 64U);
      CHECK_EQ(*window.properties.height, 48U);
    }

    SUBCASE("Fills missing dimensions from the primary monitor") {
      HELIOS_SKIP_IF_NO_GLFW();
      REQUIRE_EQ(glfwInit(), GLFW_TRUE);
      if (glfwGetPrimaryMonitor() == nullptr) {
        glfwTerminate();
        MESSAGE("No GLFW monitor available (headless CI); skipping");
        return;
      }

      Window window = Window::FromProperties(Properties{});
      ResolveCreationSize(window);

      CHECK(window.properties.width.has_value());
      CHECK(window.properties.height.has_value());
      CHECK_GE(*window.properties.width, 1U);
      CHECK_GE(*window.properties.height, 1U);

      glfwTerminate();
    }
  }
}
