#ifdef HELIOS_ENABLE_CPP_MODULES
#include <doctest/doctest.h>

import helios.ecs;

TEST_SUITE("helios.ecs module") {
  TEST_CASE("import helios.ecs") {
    SUBCASE("Exported API is reachable") {
      CHECK_FALSE(helios::ecs::Entity{}.Valid());
    }
  }
}
#endif
