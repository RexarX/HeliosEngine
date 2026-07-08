#include <doctest/doctest.h>

#include <helios/app/app.hpp>

#include <cstddef>
#include <utility>

using namespace helios::app;

TEST_SUITE("helios::app::FrameCount") {
  TEST_CASE("app::FrameCount::ctor") {
    SUBCASE("Default construction starts at zero") {
      const FrameCount frame_count;
      CHECK_EQ(frame_count.count, 0);
    }
  }
}

TEST_SUITE("helios::app::CountFrame") {
  TEST_CASE("app::CountFrame::operator()") {
    SUBCASE("Increments frame count") {
      App app;
      app.InsertResources(FrameCount{});
      app.AddSystem(kLast, CountFrame{});
      app.Initialize();
      app.Update();

      CHECK_EQ(app.GetWorld().ReadResource<FrameCount>().count, 1);
    }
  }
}

TEST_SUITE("helios::app::FrameCountPlugin") {
  TEST_CASE("app::FrameCountPlugin::Build") {
    SUBCASE("Adds FrameCount resource on initialize") {
      App app;
      app.AddPlugins(FrameCountPlugin{});
      app.Initialize();

      CHECK_EQ(app.GetWorld().HasResource<FrameCount>(), true);
    }

    SUBCASE("Increments FrameCount each frame") {
      App app;
      app.AddPlugins(FrameCountPlugin{});
      app.Initialize();
      app.Update();
      app.Update();

      CHECK_EQ(app.GetWorld().ReadResource<FrameCount>().count, 2);
    }

    SUBCASE("First update systems see frame zero before kLast increments") {
      size_t observed_frame = 99;

      struct ReadFrame {
        size_t* observed_frame = nullptr;

        void operator()(
            helios::ecs::Res<const FrameCount> frame_count) const noexcept {
          *observed_frame = frame_count->count;
        }
      };

      App app;
      app.AddPlugins(FrameCountPlugin{});
      app.AddSystem(kUpdate, ReadFrame{.observed_frame = &observed_frame});
      app.Initialize();
      app.Update();

      CHECK_EQ(observed_frame, 0);
      CHECK_EQ(app.GetWorld().ReadResource<FrameCount>().count, 1);
    }
  }
}
