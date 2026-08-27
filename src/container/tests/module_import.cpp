#ifdef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

import helios.container;

TEST_SUITE("helios.container module") {
  TEST_CASE("import helios.container") {
    SUBCASE("Exported API is reachable") {
      helios::container::SparseSet<int> set;
      CHECK(set.Empty());
    }
  }
}
#endif
