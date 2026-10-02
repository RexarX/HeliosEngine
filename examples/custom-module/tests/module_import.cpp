#ifdef HELIOS_ENABLE_CPP_MODULES
#include <string>

#include <doctest/doctest.h>

import helios.greeting;

TEST_SUITE("helios.greeting module") {
  TEST_CASE("import helios.greeting") {
    SUBCASE("Exported API is reachable") {
      CHECK_EQ(helios::greeting::Format(""), std::string{"Hello, Helios!"});
    }
  }
}
#endif
