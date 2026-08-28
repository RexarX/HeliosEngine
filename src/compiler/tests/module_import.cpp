#ifdef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>
#include <helios/compiler/compiler.hpp>

import helios.compiler;

TEST_SUITE("helios.compiler module") {
  TEST_CASE("import helios.compiler") {
    SUBCASE("Exported API is reachable") {
      CHECK_EQ(HELIOS_EXPECT_TRUE(true), true);
    }
  }
}
#endif
