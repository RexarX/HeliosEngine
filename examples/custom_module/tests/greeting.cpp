#include <doctest/doctest.h>

#include <helios/greeting/greeting.hpp>

TEST_SUITE("helios::greeting::Format") {
  TEST_CASE("formats a named greeting") {
    CHECK_EQ(helios::greeting::Format("World"), "Hello, World!");
  }

  TEST_CASE("uses default text for an empty name") {
    CHECK_EQ(helios::greeting::Format(""), "Hello, Helios!");
  }
}
