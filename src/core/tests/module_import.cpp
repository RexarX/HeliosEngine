#ifdef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

import helios.core;

TEST_SUITE("helios.core module") {
  TEST_CASE("import helios.core") {
    SUBCASE("Exported API is reachable") {
      CHECK_EQ(helios::GetAssertionHandler(), helios::kDefaultAssertionHandler);
    }
  }
}
#endif
