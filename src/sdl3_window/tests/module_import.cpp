#ifdef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

import helios.sdl3.window;

TEST_SUITE("helios.sdl3.window module") {
  TEST_CASE("import helios.sdl3.window") {
    SUBCASE("Exported API is reachable") {
      CHECK_FALSE(helios::sdl3::window::StartupSet::kName.empty());
    }
  }
}
#endif
