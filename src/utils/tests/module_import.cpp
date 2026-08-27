#ifdef HELIOS_ENABLE_CPP_MODULES
#include <chrono>

#include <doctest/doctest.h>

import helios.utils;

TEST_SUITE("helios.utils module") {
  TEST_CASE("import helios.utils") {
    SUBCASE("Exported API is reachable") {
      helios::utils::Timer<> timer;
      CHECK_GE(timer.ElapsedNanoSec(), 0);
    }
  }
}
#endif
