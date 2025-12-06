#include <doctest/doctest.h>

#include <helios/core/core.hpp>

#include <string>

TEST_SUITE("helios::Core") {
  TEST_CASE("HELIOS_BIT") {
    CHECK_EQ(HELIOS_BIT(0), 1);
    CHECK_EQ(HELIOS_BIT(1), 2);
    CHECK_EQ(HELIOS_BIT(2), 4);
    CHECK_EQ(HELIOS_BIT(3), 8);
  }

  TEST_CASE("HELIOS_STRINGIFY") {
    CHECK_EQ(std::string(HELIOS_STRINGIFY(hello)), "hello");
  }

  TEST_CASE("HELIOS_CONCAT") {
    constexpr int foo42 = 123;
    CHECK_EQ(HELIOS_CONCAT(foo, 42), 123);
  }

  TEST_CASE("HELIOS_DEBUG_BREAK: Compiles") {
    // This macro is not meant to be executed in tests, but should compile.
    // Uncomment to check compilation only:
    // HELIOS_DEBUG_BREAK();
    CHECK(true);
  }

  TEST_CASE("HELIOS_UNREACHABLE: Compiles") {
    // This macro is not meant to be executed in tests, but should compile.
    // Uncomment to check compilation only:
    // HELIOS_UNREACHABLE();
    CHECK(true);
  }

}  // TEST_SUITE
