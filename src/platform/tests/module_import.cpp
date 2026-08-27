#ifdef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>
#include <helios/platform/platform.hpp>

import helios.platform;

TEST_SUITE("helios.platform module") {
  TEST_CASE("import helios.platform") {
    SUBCASE("Exported API is reachable") {
      CHECK_EQ(1, 1);
    }
  }
}
#endif
