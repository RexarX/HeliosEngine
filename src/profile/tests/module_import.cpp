#ifdef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

import helios.profile;

TEST_SUITE("helios.profile module") {
  TEST_CASE("import helios.profile") {
    SUBCASE("Exported API is reachable") {
      CHECK_GT(helios::profile::kZoneStorageBytes, 0U);
    }
  }
}
#endif
