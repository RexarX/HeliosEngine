#ifdef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

import helios.log;

TEST_SUITE("helios.log module") {
  TEST_CASE("import helios.log") {
    SUBCASE("Exported API is reachable") {
      CHECK_EQ(helios::log::Level::kInfo, helios::log::Level::kInfo);
    }
  }
}
#endif
