#ifdef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

import helios.sdl3.input;

TEST_SUITE("helios.sdl3.input module") {
  TEST_CASE("import helios.sdl3.input") {
    SUBCASE("Exported API is reachable") {
      CHECK_FALSE(helios::sdl3::input::StartupSet::kName.empty());
    }
  }
}
#endif
