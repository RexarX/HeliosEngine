#ifdef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

import helios.app;

TEST_SUITE("helios.app module") {
  TEST_CASE("import helios.app") {
    SUBCASE("Exported API is reachable") {
      CHECK_FALSE(helios::app::Time::kName.empty());
    }
  }
}
#endif
