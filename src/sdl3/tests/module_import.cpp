#ifdef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

import helios.sdl3;

TEST_SUITE("helios.sdl3 module") {
  TEST_CASE("import helios.sdl3") {
    SUBCASE("Exported API is reachable") {
      CHECK_FALSE(helios::sdl3::StartupSet::kName.empty());
    }
  }
}
#endif
