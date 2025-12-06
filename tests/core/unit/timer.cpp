#include <doctest/doctest.h>

#include <helios/core/timer.hpp>

#include <chrono>
#include <cstdint>
#include <thread>

TEST_SUITE("helios::Timer") {
  TEST_CASE("Timer::ctor: Default clock") {
    helios::Timer timer;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    const auto elapsed_ms = timer.ElapsedMilliSec();
    CHECK_GE(elapsed_ms, 10.0);

    CHECK_NOTHROW(timer.Reset());
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    CHECK_GE(timer.ElapsedMilliSec(), 5.0);
  }

  TEST_CASE("Timer::ElapsedDuration: Custom types and units") {
    helios::Timer timer;

    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    SUBCASE("Default units") {
      const auto duration = timer.ElapsedDuration<>();
      CHECK_GT(duration.count(), 0);
    }

    SUBCASE("Explicit units") {
      const auto ns = timer.ElapsedDuration<std::chrono::nanoseconds>();
      CHECK_GT(ns.count(), 0);

      const auto us = timer.ElapsedDuration<std::chrono::microseconds>();
      CHECK_GT(us.count(), 0);
    }

    SUBCASE("Integer microseconds") {
      const auto elapsed_us = timer.Elapsed<int64_t, std::chrono::microseconds>();
      CHECK_GT(elapsed_us, 0);
    }

    SUBCASE("Floating seconds") {
      const auto elapsed_sec = timer.Elapsed<double, std::chrono::duration<double>>();
      CHECK_GT(elapsed_sec, 0.0);
    }
  }

  TEST_CASE("Timer::Elapsed: Convenience helpers") {
    helios::Timer timer;

    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    const auto sec = timer.ElapsedSec();
    const auto ms = timer.ElapsedMilliSec();
    const auto us = timer.ElapsedMicroSec();
    const auto ns = timer.ElapsedNanoSec();

    CHECK_GT(sec, 0.0);
    CHECK_GT(ms, 0.0);
    CHECK_GT(us, 0);
    CHECK_GT(ns, 0);

    // Coarse consistency checks: larger units should not be smaller than smaller
    // units when converted back approximately.
    CHECK_GE(ms * 1000.0, static_cast<double>(us) * 0.5);
    CHECK_GE(static_cast<double>(us) * 1000.0, static_cast<double>(ns) * 0.5);
  }

  namespace {

  struct TestClock {
    using rep = int64_t;
    using period = std::nano;
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<TestClock>;

    static inline time_point now_value{time_point(duration(0))};

    static auto now() noexcept -> time_point { return now_value; }
  };

  }  // namespace

  TEST_CASE("Timer::ctor: Custom clock type") {
    using CustomTimer = helios::Timer<TestClock>;

    TestClock::now_value = TestClock::time_point(TestClock::duration(0));
    CustomTimer timer;

    SUBCASE("Initial is zero") {
      const auto elapsed_ns = timer.Elapsed<int64_t, std::chrono::nanoseconds>();
      CHECK_EQ(elapsed_ns, 0);
    }

    SUBCASE("After advancing clock") {
      TestClock::now_value = TestClock::time_point(TestClock::duration(1000));  // 1000 ns
      const auto elapsed_ns = timer.Elapsed<int64_t, std::chrono::nanoseconds>();
      CHECK_EQ(elapsed_ns, 1000);
    }

    SUBCASE("Reset updates start point") {
      TestClock::now_value = TestClock::time_point(TestClock::duration(5000));  // 5000 ns
      timer.Reset();
      TestClock::now_value = TestClock::time_point(TestClock::duration(8000));  // 3000 ns later
      const auto elapsed_ns = timer.Elapsed<int64_t, std::chrono::nanoseconds>();
      CHECK_EQ(elapsed_ns, 3000);
    }

    SUBCASE("Start returns underlying time point") {
      const auto start_before = timer.Start();
      TestClock::now_value = TestClock::time_point(TestClock::duration(100));  // move forward
      const auto start_after = timer.Start();
      CHECK_EQ(start_before, start_after);
    }
  }

}  // TEST_SUITE
