#ifdef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

import helios.input;

TEST_SUITE("helios.input module") {
  TEST_CASE("import helios.input") {
    SUBCASE("Exported API is reachable") {
      CHECK_EQ(helios::input::Key::kA, helios::input::Key::kA);
    }
  }
}
#endif
