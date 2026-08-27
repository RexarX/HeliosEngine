#include <doctest/doctest.h>

#include <helios/ecs/world.hpp>
#include <helios/window/native_handle.hpp>

#include <variant>

using namespace helios::ecs;
using namespace helios::window;

TEST_SUITE("helios::window::NativeHandle") {
  TEST_CASE("helios::window::NativeHandle") {
    SUBCASE("Default handle is monostate") {
      const NativeHandle handle;
      CHECK(std::holds_alternative<std::monostate>(handle));
    }

#if defined(HELIOS_PLATFORM_WINDOWS)
    SUBCASE("Win32Handle is a valid alternative") {
      const NativeHandle handle = Win32Handle{
          .hwnd = reinterpret_cast<void*>(1),
          .hinstance = reinterpret_cast<void*>(2),
      };
      CHECK(std::holds_alternative<Win32Handle>(handle));
      const auto& win32 = std::get<Win32Handle>(handle);
      CHECK_NE(win32.hwnd, nullptr);
      CHECK_NE(win32.hinstance, nullptr);
    }
#endif

#if defined(HELIOS_PLATFORM_MACOS)
    SUBCASE("CocoaHandle is a valid alternative") {
      const NativeHandle handle = CocoaHandle{
          .ns_window = reinterpret_cast<void*>(1),
          .ns_view = reinterpret_cast<void*>(2),
      };
      CHECK(std::holds_alternative<CocoaHandle>(handle));
    }
#endif

#if defined(HELIOS_PLATFORM_LINUX_X11)
    SUBCASE("XlibHandle is a valid alternative") {
      const NativeHandle handle =
          XlibHandle{.display = reinterpret_cast<void*>(1), .window = 42};
      CHECK(std::holds_alternative<XlibHandle>(handle));
      CHECK_EQ(std::get<XlibHandle>(handle).window, 42UL);
    }
#endif

#if defined(HELIOS_PLATFORM_LINUX_WAYLAND)
    SUBCASE("WaylandHandle is a valid alternative") {
      const NativeHandle handle = WaylandHandle{
          .display = reinterpret_cast<void*>(1),
          .surface = reinterpret_cast<void*>(2),
      };
      CHECK(std::holds_alternative<WaylandHandle>(handle));
    }
#endif
  }
}

TEST_SUITE("helios::window::NativeHandleComponent") {
  TEST_CASE("helios::window::NativeHandleComponent") {
    SUBCASE("Attaches as an archetype component") {
      World world;
      const auto entity = world.CreateEntity();
      world.AddComponents(entity, NativeHandleComponent{});
      CHECK(world.HasComponent<NativeHandleComponent>(entity));
    }
  }
}
