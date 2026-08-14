#include <doctest/doctest.h>

#include <helios/app/frame_order.hpp>
#include <helios/app/schedules.hpp>
#include <helios/ecs/schedule/stage.hpp>
#include <helios/ecs/world.hpp>

using namespace helios::app;
using namespace helios::ecs;

namespace {

struct CustomStage {
  static constexpr std::string_view kName = "CustomStage";
};

struct OtherStage {
  static constexpr std::string_view kName = "OtherStage";
};

}  // namespace

TEST_SUITE("helios::app::FrameOrder") {
  TEST_CASE("helios::app::FrameOrder::TryPushBack") {
    SUBCASE("Appends unique stages") {
      FrameOrder order;
      CHECK(order.TryPushBack(kUpdateStage));
      CHECK(order.TryPushBack(kExtractStage));
      CHECK_FALSE(order.TryPushBack(kUpdateStage));
      CHECK_EQ(order.Labels().size(), 2);
      CHECK(order.Contains(kUpdateStage));
      CHECK(order.Contains(kExtractStage));
    }
  }

  TEST_CASE("helios::app::FrameOrder::InsertBefore") {
    SUBCASE("Inserts relative to existing label") {
      FrameOrder order;
      order.TryPushBack(kUpdateStage);
      order.TryPushBack(kExtractStage);
      order.InsertBefore(kUpdateStage, CustomStage{});

      CHECK_EQ(order.Labels().size(), 3);
      CHECK_EQ(order.Labels()[0].Hash(),
               StageTypeIndex::From(CustomStage{}).Hash());
      CHECK_EQ(order.Labels()[1].Hash(),
               StageTypeIndex::From(kUpdateStage).Hash());
      CHECK_EQ(order.Labels()[2].Hash(),
               StageTypeIndex::From(kExtractStage).Hash());
    }
  }

  TEST_CASE("helios::app::FrameOrder::InsertAfter") {
    SUBCASE("Inserts relative to existing label") {
      FrameOrder order;
      order.TryPushBack(kUpdateStage);
      order.TryPushBack(kExtractStage);
      order.InsertAfter(kUpdateStage, OtherStage{});

      CHECK_EQ(order.Labels().size(), 3);
      CHECK_EQ(order.Labels()[0].Hash(),
               StageTypeIndex::From(kUpdateStage).Hash());
      CHECK_EQ(order.Labels()[1].Hash(),
               StageTypeIndex::From(OtherStage{}).Hash());
      CHECK_EQ(order.Labels()[2].Hash(),
               StageTypeIndex::From(kExtractStage).Hash());
    }
  }

  TEST_CASE("helios::app::FrameOrder::Contains") {
    SUBCASE("Reports membership by stage type") {
      FrameOrder order;
      order.TryPushBack(kUpdateStage);
      CHECK(order.Contains(kUpdateStage));
      CHECK_FALSE(order.Contains(kExtractStage));
    }
  }
}

TEST_SUITE("helios::app::RegisterBuiltinFrameOrders") {
  TEST_CASE("helios::app::RegisterBuiltinFrameOrders") {
    SUBCASE("Inserts default main and pump orders") {
      World world;
      RegisterBuiltinFrameOrders(world);

      const auto& main = world.ReadResource<MainFrameOrder>();
      const auto& pump = world.ReadResource<FramePumpOrder>();

      CHECK_EQ(main.Labels().size(), 2);
      CHECK(main.Contains(kUpdateStage));
      CHECK(main.Contains(kExtractStage));
      CHECK_EQ(pump.Labels().size(), 1);
      CHECK(pump.Contains(kUpdateStage));
      CHECK_FALSE(pump.Contains(kExtractStage));
    }

    SUBCASE("Is idempotent") {
      World world;
      RegisterBuiltinFrameOrders(world);
      world.WriteResource<MainFrameOrder>().InsertBefore(kUpdateStage,
                                                         CustomStage{});
      RegisterBuiltinFrameOrders(world);

      CHECK(world.ReadResource<MainFrameOrder>().Contains(CustomStage{}));
    }
  }
}
