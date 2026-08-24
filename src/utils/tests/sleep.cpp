#include <doctest/doctest.h>

#include <helios/utils/sleep.hpp>

#include <chrono>

using namespace helios::utils;
using namespace std::chrono_literals;

TEST_SUITE("helios::utils::SleepFor") {
  TEST_CASE("helios::utils::SleepFor") {
    SUBCASE("Returns immediately for a non-positive duration") {
      const auto start = std::chrono::steady_clock::now();
      SleepFor(0ms);
      SleepFor(-1ms);
      CHECK_LT(std::chrono::steady_clock::now() - start, 5ms);
    }

    SUBCASE("Waits at least a short duration") {
      const auto start = std::chrono::steady_clock::now();
      SleepFor(8ms);
      CHECK_GE(std::chrono::steady_clock::now() - start, 4ms);
    }
  }
}

TEST_SUITE("helios::utils::SleepUntil") {
  TEST_CASE("helios::utils::SleepUntil") {
    SUBCASE("Returns immediately when the deadline is in the past") {
      const auto start = std::chrono::steady_clock::now();
      SleepUntil(start - 1ms);
      CHECK_LT(std::chrono::steady_clock::now() - start, 5ms);
    }
  }
}

TEST_SUITE("helios::utils::PreciseSleep") {
  TEST_CASE("helios::utils::PreciseSleep") {
    SUBCASE("Returns immediately for a non-positive duration") {
      const auto start = std::chrono::steady_clock::now();
      PreciseSleep(0ns);
      CHECK_LT(std::chrono::steady_clock::now() - start, 5ms);
    }

    SUBCASE("Returns immediately for a non-positive duration (unpinned)") {
      const auto start = std::chrono::steady_clock::now();
      PreciseSleep(0ns, /*pinned_thread=*/false);
      CHECK_LT(std::chrono::steady_clock::now() - start, 5ms);
    }

    SUBCASE("Reaches a short future duration") {
      const auto start = std::chrono::steady_clock::now();
      PreciseSleep(8ms);
      CHECK_GE(std::chrono::steady_clock::now() - start, 8ms);
    }

    SUBCASE("Reaches a short future duration when explicitly pinned") {
      const auto start = std::chrono::steady_clock::now();
      PreciseSleep(8ms, /*pinned_thread=*/true);
      CHECK_GE(std::chrono::steady_clock::now() - start, 8ms);
    }

    SUBCASE("Reaches a short future duration when unpinned") {
      const auto start = std::chrono::steady_clock::now();
      PreciseSleep(8ms, /*pinned_thread=*/false);
      CHECK_GE(std::chrono::steady_clock::now() - start, 8ms);
    }
  }
}

TEST_SUITE("helios::utils::PreciseSleepUntil") {
  TEST_CASE("helios::utils::PreciseSleepUntil") {
    SUBCASE("Returns immediately when the deadline is in the past") {
      const auto start = std::chrono::steady_clock::now();
      PreciseSleepUntil(start - 1ms);
      CHECK_LT(std::chrono::steady_clock::now() - start, 5ms);
    }

    SUBCASE("Returns immediately when the deadline is in the past (unpinned)") {
      const auto start = std::chrono::steady_clock::now();
      PreciseSleepUntil(start - 1ms,
                        /*pinned_thread=*/false);
      CHECK_LT(std::chrono::steady_clock::now() - start, 5ms);
    }

    SUBCASE("Waits until a short future deadline") {
      const auto deadline = std::chrono::steady_clock::now() + 8ms;
      PreciseSleepUntil(deadline);
      CHECK_GE(std::chrono::steady_clock::now(), deadline);
    }

    SUBCASE("Waits until a short future deadline when explicitly pinned") {
      const auto deadline = std::chrono::steady_clock::now() + 8ms;
      PreciseSleepUntil(deadline, /*pinned_thread=*/true);
      CHECK_GE(std::chrono::steady_clock::now(), deadline);
    }

    SUBCASE("Waits until a short future deadline when unpinned") {
      const auto deadline = std::chrono::steady_clock::now() + 8ms;
      PreciseSleepUntil(deadline, /*pinned_thread=*/false);
      CHECK_GE(std::chrono::steady_clock::now(), deadline);
    }
  }
}
