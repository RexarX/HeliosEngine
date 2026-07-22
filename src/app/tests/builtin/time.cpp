#include <doctest/doctest.h>

#include <helios/app/app.hpp>

#include <chrono>
#include <thread>
#include <utility>

using namespace helios::app;

TEST_SUITE("helios::app::Time") {
  TEST_CASE("app::Time::ctor") {
    SUBCASE("Default construction starts at zero") {
      const Time time;
      CHECK_EQ(time.delta_time, Time::Duration{});
      CHECK_EQ(time.elapsed, Time::Duration{});
    }
  }

  TEST_CASE("app::Time::Update") {
    SUBCASE("Updates public timing state") {
      Time time;
      time.last_update -= std::chrono::milliseconds{2};
      time.Update();

      CHECK_GT(time.delta_time, Time::Duration{});
      CHECK_EQ(time.elapsed, time.delta_time);
      CHECK_LE(time.last_update, Time::Clock::now());
    }
  }

  TEST_CASE("app::Time::Elapsed") {
    SUBCASE("Returns elapsed as custom arithmetic type and duration") {
      Time time;
      time.elapsed = std::chrono::milliseconds{1500};

      CHECK_EQ(time.Elapsed<int64_t, std::chrono::milliseconds>(), 1500);
      CHECK_EQ(time.Elapsed<double, std::chrono::duration<double>>(),
               doctest::Approx(1.5));
    }
  }

  TEST_CASE("app::Time::ElapsedDuration") {
    SUBCASE("Returns elapsed using custom duration") {
      Time time;
      time.elapsed = std::chrono::milliseconds{3};

      CHECK_EQ(time.ElapsedDuration<std::chrono::microseconds>().count(), 3000);
    }
  }

  TEST_CASE("app::Time::Delta") {
    SUBCASE("Returns delta as custom arithmetic type and duration") {
      Time time;
      time.delta_time = std::chrono::milliseconds{2500};

      CHECK_EQ(time.Delta<int64_t, std::chrono::milliseconds>(), 2500);
      CHECK_EQ(time.Delta<double, std::chrono::duration<double>>(),
               doctest::Approx(2.5));
    }
  }

  TEST_CASE("app::Time::DeltaDuration") {
    SUBCASE("Returns delta using custom duration") {
      Time time;
      time.delta_time = std::chrono::milliseconds{4};

      CHECK_EQ(time.DeltaDuration<std::chrono::microseconds>().count(), 4000);
    }
  }

  TEST_CASE("app::Time::convenience accessors") {
    SUBCASE("Return elapsed and delta in common units") {
      Time time;
      time.elapsed = std::chrono::milliseconds{2};
      time.delta_time = std::chrono::milliseconds{3};

      CHECK_EQ(time.ElapsedSec(), doctest::Approx(0.002));
      CHECK_EQ(time.ElapsedMilliSec(), doctest::Approx(2.0));
      CHECK_EQ(time.ElapsedMicroSec(), 2000);
      CHECK_EQ(time.ElapsedNanoSec(), 2'000'000);
      CHECK_EQ(time.DeltaSec(), doctest::Approx(0.003));
      CHECK_EQ(time.DeltaMilliSec(), doctest::Approx(3.0));
      CHECK_EQ(time.DeltaMicroSec(), 3000);
      CHECK_EQ(time.DeltaNanoSec(), 3'000'000);
    }
  }
}

TEST_SUITE("helios::app::UpdateTime") {
  TEST_CASE("app::UpdateTime::operator()") {
    SUBCASE("Updates delta time and elapsed time") {
      App app;
      app.InsertResources(Time{});
      app.AddSystem(kFirst, UpdateTime{});
      app.Initialize();

      std::this_thread::sleep_for(std::chrono::milliseconds{1});
      app.Update();

      const auto& time = app.GetWorld().ReadResource<Time>();
      CHECK_GT(time.delta_time, Time::Duration{});
      CHECK_EQ(time.elapsed, time.delta_time);
    }
  }
}

TEST_SUITE("helios::app::TimePlugin") {
  TEST_CASE("app::TimePlugin::Build") {
    SUBCASE("Adds Time resource on initialize") {
      App app;
      app.AddPlugins(TimePlugin{});
      app.Initialize();

      CHECK_EQ(app.GetWorld().HasResource<Time>(), true);
    }

    SUBCASE("Updates Time each frame") {
      App app;
      app.AddPlugins(TimePlugin{});
      app.Initialize();

      std::this_thread::sleep_for(std::chrono::milliseconds{1});
      app.Update();
      const auto first_elapsed = app.GetWorld().ReadResource<Time>().elapsed;

      std::this_thread::sleep_for(std::chrono::milliseconds{1});
      app.Update();
      const auto& time = app.GetWorld().ReadResource<Time>();

      CHECK_GT(first_elapsed, Time::Duration{});
      CHECK_GT(time.delta_time, Time::Duration{});
      CHECK_GT(time.elapsed, first_elapsed);
    }
  }
}
