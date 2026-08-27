#ifdef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

import helios.window;

TEST_SUITE("helios.window module") {
  TEST_CASE("import helios.window") {
    SUBCASE("Exported API is reachable") {
      CHECK_FALSE(helios::window::SyncFrameLimiterRefreshRate::kName.empty());
    }
  }
}
#endif
