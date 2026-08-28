#ifdef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

import helios.glfw;

TEST_SUITE("helios.glfw module") {
  TEST_CASE("import helios.glfw") {
    SUBCASE("Exported API is reachable") {
      CHECK_FALSE(helios::glfw::Plugin::kName.empty());
    }
  }
}
#endif
