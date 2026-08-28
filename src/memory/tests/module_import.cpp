#ifdef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

import helios.memory;

TEST_SUITE("helios.memory module") {
  TEST_CASE("import helios.memory") {
    SUBCASE("Exported API is reachable") {
      CHECK_EQ(helios::mem::kDefaultAlignment, 64);
    }
  }
}
#endif
