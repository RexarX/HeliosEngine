#include <doctest/doctest.h>

#include <helios/core/app/app.hpp>
#include <helios/core/app/runners.hpp>
#include <helios/core/app/schedules.hpp>
#include <helios/core/app/system_context.hpp>
#include <helios/core/app/time.hpp>
#include <helios/core/ecs/events/builtin_events.hpp>
#include <helios/core/ecs/system.hpp>

#include <chrono>
#include <thread>

using namespace helios::app;
using namespace helios::ecs;

TEST_SUITE("Time Resource") {
  TEST_CASE("Time default construction") {
    Time time;

    CHECK_EQ(time.DeltaSeconds(), doctest::Approx(0.0F));
    CHECK_EQ(time.ElapsedSeconds(), doctest::Approx(0.0F));
    CHECK_EQ(time.FrameCount(), 0);
    CHECK(time.IsFirstFrame());
  }

  TEST_CASE("Time tick updates delta") {
    Time time;

    // Small sleep to ensure measurable delta
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    time.Tick();

    CHECK_GT(time.DeltaSeconds(), 0.0F);
    CHECK_GT(time.DeltaMilliseconds(), 0.0F);
    CHECK_GT(time.ElapsedSeconds(), 0.0F);
    CHECK_EQ(time.FrameCount(), 1);
    CHECK_FALSE(time.IsFirstFrame());
  }

  TEST_CASE("Time tick increments frame count") {
    Time time;

    for (int i = 0; i < 5; ++i) {
      time.Tick();
    }

    CHECK_EQ(time.FrameCount(), 5);
  }

  TEST_CASE("Time reset clears state") {
    Time time;

    // Tick a few times
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    time.Tick();
    time.Tick();
    time.Tick();

    CHECK_GT(time.FrameCount(), 0);

    time.Reset();

    CHECK_EQ(time.DeltaSeconds(), doctest::Approx(0.0F));
    CHECK_EQ(time.ElapsedSeconds(), doctest::Approx(0.0F));
    CHECK_EQ(time.FrameCount(), 0);
    CHECK(time.IsFirstFrame());
  }

  TEST_CASE("Time FPS calculation") {
    Time time;

    // Simulate a frame at roughly 100 FPS (10ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    time.Tick();

    const float fps = time.Fps();
    CHECK_GT(fps, 0.0F);
    CHECK_LT(fps, 1000.0F);  // Reasonable upper bound
  }

  TEST_CASE("Time delta duration") {
    Time time;

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    time.Tick();

    const auto delta = time.Delta();
    CHECK_GT(delta.count(), 0);
  }

  TEST_CASE("Time elapsed accumulates") {
    Time time;

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    time.Tick();
    const float elapsed1 = time.ElapsedSeconds();

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    time.Tick();
    const float elapsed2 = time.ElapsedSeconds();

    CHECK_GT(elapsed2, elapsed1);
  }

  TEST_CASE("Time resource satisfies ResourceTrait") {
    static_assert(ResourceTrait<Time>, "Time must satisfy ResourceTrait");
  }
}
