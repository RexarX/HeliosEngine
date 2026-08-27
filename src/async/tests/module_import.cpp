#ifdef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

import helios.async;

TEST_SUITE("helios.async module") {
  TEST_CASE("import helios.async") {
    SUBCASE("Exported API is reachable") {
      CHECK_EQ(sizeof(helios::async::Executor*), sizeof(void*));
    }
  }
}
#endif
