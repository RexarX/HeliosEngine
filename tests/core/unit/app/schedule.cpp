#include <doctest/doctest.h>

#include <helios/core/app/schedule.hpp>
#include <helios/core/app/schedules.hpp>

#include <array>
#include <cstddef>
#include <string_view>
#include <type_traits>

using namespace helios::app;

// Test schedule types
namespace {

struct EmptySchedule {};

struct NamedSchedule {
  static constexpr std::string_view GetName() noexcept { return "NamedSchedule"; }
};

struct ScheduleWithBefore {
  static constexpr auto Before() noexcept { return std::array<ScheduleId, 0>{}; }
  static constexpr auto After() noexcept { return std::array<ScheduleId, 0>{}; }
};

struct ScheduleWithAfter {
  static constexpr auto Before() noexcept { return std::array<ScheduleId, 0>{}; }
  static constexpr auto After() noexcept { return std::array{ScheduleIdOf<EmptySchedule>()}; }
};

struct NonEmptySchedule {
  int value = 0;  // Not empty!
};

}  // namespace

TEST_SUITE("app::Schedule") {
  TEST_CASE("ScheduleTrait: empty struct satisfies") {
    CHECK(ScheduleTrait<EmptySchedule>);
    CHECK(ScheduleTrait<NamedSchedule>);
    CHECK(ScheduleTrait<ScheduleWithBefore>);
  }

  TEST_CASE("ScheduleTrait: non-empty struct does not satisfy") {
    CHECK_FALSE(ScheduleTrait<NonEmptySchedule>);
    CHECK_FALSE(ScheduleTrait<int>);
    CHECK_FALSE(ScheduleTrait<std::string>);
  }

  TEST_CASE("ScheduleWithNameTrait: with GetName satisfies") {
    CHECK(ScheduleWithNameTrait<NamedSchedule>);
  }

  TEST_CASE("ScheduleWithNameTrait: without GetName does not satisfy") {
    CHECK_FALSE(ScheduleWithNameTrait<EmptySchedule>);
    CHECK_FALSE(ScheduleWithNameTrait<ScheduleWithBefore>);
  }

  TEST_CASE("ScheduleIdOf: same type returns same ID") {
    constexpr ScheduleId id1 = ScheduleIdOf<EmptySchedule>();
    constexpr ScheduleId id2 = ScheduleIdOf<EmptySchedule>();
    CHECK_EQ(id1, id2);
  }

  TEST_CASE("ScheduleIdOf: different types return different IDs") {
    constexpr ScheduleId id1 = ScheduleIdOf<EmptySchedule>();
    constexpr ScheduleId id2 = ScheduleIdOf<NamedSchedule>();
    constexpr ScheduleId id3 = ScheduleIdOf<ScheduleWithBefore>();

    CHECK_NE(id1, id2);
    CHECK_NE(id1, id3);
    CHECK_NE(id2, id3);
  }

  TEST_CASE("ScheduleIdOf: constexpr") {
    constexpr ScheduleId id = ScheduleIdOf<EmptySchedule>();
    CHECK_EQ(id, ScheduleIdOf<EmptySchedule>());
  }

  TEST_CASE("ScheduleNameOf: with custom name returns custom name") {
    constexpr std::string_view name = ScheduleNameOf<NamedSchedule>();
    CHECK_EQ(name, "NamedSchedule");
  }

  TEST_CASE("ScheduleNameOf: without custom name returns CTTI name") {
    constexpr std::string_view name = ScheduleNameOf<EmptySchedule>();
    CHECK_FALSE(name.empty());
    // CTTI name will contain "EmptySchedule" somewhere
    CHECK(name.find("EmptySchedule") != std::string_view::npos);
  }

  TEST_CASE("ScheduleNameOf: constexpr") {
    constexpr std::string_view name = ScheduleNameOf<NamedSchedule>();
    CHECK_EQ(name, "NamedSchedule");
  }

  TEST_CASE("ScheduleWithBeforeTrait: detects Before method") {
    CHECK(ScheduleWithBeforeTrait<ScheduleWithBefore>);
    CHECK(ScheduleWithBeforeTrait<ScheduleWithAfter>);
  }

  TEST_CASE("ScheduleWithBeforeTrait: false without Before method") {
    CHECK_FALSE(ScheduleWithBeforeTrait<EmptySchedule>);
    CHECK_FALSE(ScheduleWithBeforeTrait<NamedSchedule>);
  }

  TEST_CASE("ScheduleWithAfterTrait: detects After method") {
    CHECK(ScheduleWithAfterTrait<ScheduleWithBefore>);
    CHECK(ScheduleWithAfterTrait<ScheduleWithAfter>);
  }

  TEST_CASE("ScheduleWithAfterTrait: false without After method") {
    CHECK_FALSE(ScheduleWithAfterTrait<EmptySchedule>);
    CHECK_FALSE(ScheduleWithAfterTrait<NamedSchedule>);
  }

  TEST_CASE("ScheduleBeforeOf: returns Before array") {
    constexpr auto before = ScheduleBeforeOf<ScheduleWithBefore>();
    CHECK_EQ(before.size(), 0);
  }

  TEST_CASE("ScheduleBeforeOf: returns empty array for schedules without Before") {
    constexpr auto before = ScheduleBeforeOf<EmptySchedule>();
    CHECK_EQ(before.size(), 0);
  }

  TEST_CASE("ScheduleAfterOf: returns After array") {
    constexpr auto after = ScheduleAfterOf<ScheduleWithAfter>();
    CHECK_EQ(after.size(), 1);
    CHECK_EQ(after[0], ScheduleIdOf<EmptySchedule>());
  }

  TEST_CASE("ScheduleAfterOf: returns empty array for schedules without After") {
    constexpr auto after = ScheduleAfterOf<EmptySchedule>();
    CHECK_EQ(after.size(), 0);
  }
}

TEST_SUITE("app::DefaultSchedules") {
  TEST_CASE("PreStartup: is valid schedule") {
    CHECK(ScheduleTrait<PreStartup>);
    CHECK(ScheduleWithNameTrait<PreStartup>);
    CHECK(ScheduleWithBeforeTrait<PreStartup>);
    CHECK(ScheduleWithAfterTrait<PreStartup>);
  }

  TEST_CASE("PreStartup: properties") {
    CHECK_EQ(ScheduleNameOf<PreStartup>(), "PreStartup");
    CHECK_EQ(ScheduleBeforeOf<PreStartup>().size(), 1);
    CHECK_EQ(ScheduleBeforeOf<PreStartup>()[0], ScheduleIdOf<Startup>());
    CHECK_EQ(ScheduleAfterOf<PreStartup>().size(), 0);
    CHECK_EQ(ScheduleStageOf<PreStartup>(), ScheduleIdOf<StartUpStage>());
  }

  TEST_CASE("Startup: is valid schedule") {
    CHECK(ScheduleTrait<Startup>);
    CHECK(ScheduleWithNameTrait<Startup>);
  }

  TEST_CASE("Startup: properties") {
    CHECK_EQ(ScheduleNameOf<Startup>(), "Startup");
    CHECK_EQ(ScheduleBeforeOf<Startup>().size(), 1);
    CHECK_EQ(ScheduleBeforeOf<Startup>()[0], ScheduleIdOf<PostStartup>());
    CHECK_EQ(ScheduleAfterOf<Startup>().size(), 1);
    CHECK_EQ(ScheduleAfterOf<Startup>()[0], ScheduleIdOf<PreStartup>());
    CHECK_EQ(ScheduleStageOf<Startup>(), ScheduleIdOf<StartUpStage>());
  }

  TEST_CASE("PostStartup: is valid schedule") {
    CHECK(ScheduleTrait<PostStartup>);
    CHECK(ScheduleWithNameTrait<PostStartup>);
  }

  TEST_CASE("PostStartup: properties") {
    CHECK_EQ(ScheduleNameOf<PostStartup>(), "PostStartup");
    CHECK_EQ(ScheduleBeforeOf<PostStartup>().size(), 0);
    CHECK_EQ(ScheduleAfterOf<PostStartup>().size(), 1);
    CHECK_EQ(ScheduleAfterOf<PostStartup>()[0], ScheduleIdOf<Startup>());
    CHECK_EQ(ScheduleStageOf<PostStartup>(), ScheduleIdOf<StartUpStage>());
  }

  TEST_CASE("Main: is valid schedule") {
    CHECK(ScheduleTrait<Main>);
    CHECK(ScheduleWithNameTrait<Main>);
  }

  TEST_CASE("Main: properties") {
    CHECK_EQ(ScheduleNameOf<Main>(), "Main");
    CHECK_EQ(ScheduleBeforeOf<Main>().size(), 0);
    CHECK_EQ(ScheduleAfterOf<Main>().size(), 0);
    CHECK_EQ(ScheduleStageOf<Main>(), ScheduleIdOf<MainStage>());
  }

  TEST_CASE("First: is valid schedule") {
    CHECK(ScheduleTrait<First>);
    CHECK(ScheduleWithNameTrait<First>);
  }

  TEST_CASE("First: properties") {
    CHECK_EQ(ScheduleNameOf<First>(), "First");
    CHECK_EQ(ScheduleBeforeOf<First>().size(), 1);
    CHECK_EQ(ScheduleBeforeOf<First>()[0], ScheduleIdOf<PreUpdate>());
    CHECK_EQ(ScheduleAfterOf<First>().size(), 0);
    CHECK_EQ(ScheduleStageOf<First>(), ScheduleIdOf<UpdateStage>());
  }

  TEST_CASE("PreUpdate: is valid schedule") {
    CHECK(ScheduleTrait<PreUpdate>);
    CHECK(ScheduleWithNameTrait<PreUpdate>);
  }

  TEST_CASE("PreUpdate: properties") {
    CHECK_EQ(ScheduleNameOf<PreUpdate>(), "PreUpdate");
    CHECK_EQ(ScheduleBeforeOf<PreUpdate>().size(), 1);
    CHECK_EQ(ScheduleBeforeOf<PreUpdate>()[0], ScheduleIdOf<Update>());
    CHECK_EQ(ScheduleAfterOf<PreUpdate>().size(), 1);
    CHECK_EQ(ScheduleAfterOf<PreUpdate>()[0], ScheduleIdOf<First>());
    CHECK_EQ(ScheduleStageOf<PreUpdate>(), ScheduleIdOf<UpdateStage>());
  }

  TEST_CASE("Update: is valid schedule") {
    CHECK(ScheduleTrait<Update>);
    CHECK(ScheduleWithNameTrait<Update>);
  }

  TEST_CASE("Update: properties") {
    CHECK_EQ(ScheduleNameOf<Update>(), "Update");
    CHECK_EQ(ScheduleBeforeOf<Update>().size(), 1);
    CHECK_EQ(ScheduleBeforeOf<Update>()[0], ScheduleIdOf<PostUpdate>());
    CHECK_EQ(ScheduleAfterOf<Update>().size(), 1);
    CHECK_EQ(ScheduleAfterOf<Update>()[0], ScheduleIdOf<PreUpdate>());
    CHECK_EQ(ScheduleStageOf<Update>(), ScheduleIdOf<UpdateStage>());
  }

  TEST_CASE("PostUpdate: is valid schedule") {
    CHECK(ScheduleTrait<PostUpdate>);
    CHECK(ScheduleWithNameTrait<PostUpdate>);
  }

  TEST_CASE("PostUpdate: properties") {
    CHECK_EQ(ScheduleNameOf<PostUpdate>(), "PostUpdate");
    CHECK_EQ(ScheduleBeforeOf<PostUpdate>().size(), 1);
    CHECK_EQ(ScheduleBeforeOf<PostUpdate>()[0], ScheduleIdOf<Last>());
    CHECK_EQ(ScheduleAfterOf<PostUpdate>().size(), 1);
    CHECK_EQ(ScheduleAfterOf<PostUpdate>()[0], ScheduleIdOf<Update>());
    CHECK_EQ(ScheduleStageOf<PostUpdate>(), ScheduleIdOf<UpdateStage>());
  }

  TEST_CASE("Last: is valid schedule") {
    CHECK(ScheduleTrait<Last>);
    CHECK(ScheduleWithNameTrait<Last>);
  }

  TEST_CASE("Last: properties") {
    CHECK_EQ(ScheduleNameOf<Last>(), "Last");
    CHECK_EQ(ScheduleBeforeOf<Last>().size(), 0);
    CHECK_EQ(ScheduleAfterOf<Last>().size(), 1);
    CHECK_EQ(ScheduleAfterOf<Last>()[0], ScheduleIdOf<PostUpdate>());
    CHECK_EQ(ScheduleStageOf<Last>(), ScheduleIdOf<UpdateStage>());
  }

  TEST_CASE("PreCleanUp: is valid schedule") {
    CHECK(ScheduleTrait<PreCleanUp>);
    CHECK(ScheduleWithNameTrait<PreCleanUp>);
  }

  TEST_CASE("PreCleanUp: properties") {
    CHECK_EQ(ScheduleNameOf<PreCleanUp>(), "PreCleanUp");
    CHECK_EQ(ScheduleBeforeOf<PreCleanUp>().size(), 1);
    CHECK_EQ(ScheduleBeforeOf<PreCleanUp>()[0], ScheduleIdOf<CleanUp>());
    CHECK_EQ(ScheduleAfterOf<PreCleanUp>().size(), 0);
    CHECK_EQ(ScheduleStageOf<PreCleanUp>(), ScheduleIdOf<CleanUpStage>());
  }

  TEST_CASE("CleanUp: is valid schedule") {
    CHECK(ScheduleTrait<CleanUp>);
    CHECK(ScheduleWithNameTrait<CleanUp>);
  }

  TEST_CASE("CleanUp: properties") {
    CHECK_EQ(ScheduleNameOf<CleanUp>(), "CleanUp");
    CHECK_EQ(ScheduleBeforeOf<CleanUp>().size(), 0);
    CHECK_EQ(ScheduleAfterOf<CleanUp>().size(), 0);
    CHECK_EQ(ScheduleStageOf<CleanUp>(), ScheduleIdOf<CleanUpStage>());
  }

  TEST_CASE("PostCleanUp: is valid schedule") {
    CHECK(ScheduleTrait<PostCleanUp>);
    CHECK(ScheduleWithNameTrait<PostCleanUp>);
  }

  TEST_CASE("PostCleanUp: properties") {
    CHECK_EQ(ScheduleNameOf<PostCleanUp>(), "PostCleanUp");
    CHECK_EQ(ScheduleBeforeOf<PostCleanUp>().size(), 0);
    CHECK_EQ(ScheduleAfterOf<PostCleanUp>().size(), 1);
    CHECK_EQ(ScheduleAfterOf<PostCleanUp>()[0], ScheduleIdOf<CleanUp>());
    CHECK_EQ(ScheduleStageOf<PostCleanUp>(), ScheduleIdOf<CleanUpStage>());
  }

  TEST_CASE("DefaultSchedules: all have unique IDs") {
    CHECK_NE(ScheduleIdOf<PreStartup>(), ScheduleIdOf<Startup>());
    CHECK_NE(ScheduleIdOf<PreStartup>(), ScheduleIdOf<PostStartup>());
    CHECK_NE(ScheduleIdOf<PreStartup>(), ScheduleIdOf<Main>());
    CHECK_NE(ScheduleIdOf<PreStartup>(), ScheduleIdOf<First>());
    CHECK_NE(ScheduleIdOf<PreStartup>(), ScheduleIdOf<PreUpdate>());
    CHECK_NE(ScheduleIdOf<PreStartup>(), ScheduleIdOf<Update>());
    CHECK_NE(ScheduleIdOf<PreStartup>(), ScheduleIdOf<PostUpdate>());
    CHECK_NE(ScheduleIdOf<PreStartup>(), ScheduleIdOf<Last>());

    CHECK_NE(ScheduleIdOf<PreStartup>(), ScheduleIdOf<PreCleanUp>());
    CHECK_NE(ScheduleIdOf<PreStartup>(), ScheduleIdOf<CleanUp>());
    CHECK_NE(ScheduleIdOf<PreStartup>(), ScheduleIdOf<PostCleanUp>());

    CHECK_NE(ScheduleIdOf<Startup>(), ScheduleIdOf<PostStartup>());
    CHECK_NE(ScheduleIdOf<Startup>(), ScheduleIdOf<Main>());
    CHECK_NE(ScheduleIdOf<Main>(), ScheduleIdOf<First>());
    CHECK_NE(ScheduleIdOf<First>(), ScheduleIdOf<PreUpdate>());
    CHECK_NE(ScheduleIdOf<PreUpdate>(), ScheduleIdOf<Update>());
    CHECK_NE(ScheduleIdOf<Update>(), ScheduleIdOf<PostUpdate>());
    CHECK_NE(ScheduleIdOf<PostUpdate>(), ScheduleIdOf<Last>());
  }

  TEST_CASE("DefaultSchedules: all have unique names") {
    CHECK_NE(ScheduleNameOf<PreStartup>(), ScheduleNameOf<Startup>());
    CHECK_NE(ScheduleNameOf<Startup>(), ScheduleNameOf<PostStartup>());
    CHECK_NE(ScheduleNameOf<Main>(), ScheduleNameOf<First>());
    CHECK_NE(ScheduleNameOf<First>(), ScheduleNameOf<PreUpdate>());
    CHECK_NE(ScheduleNameOf<PreUpdate>(), ScheduleNameOf<Update>());
    CHECK_NE(ScheduleNameOf<Update>(), ScheduleNameOf<PostUpdate>());
    CHECK_NE(ScheduleNameOf<PostUpdate>(), ScheduleNameOf<Last>());
    CHECK_NE(ScheduleNameOf<Last>(), ScheduleNameOf<PreCleanUp>());
    CHECK_NE(ScheduleNameOf<PreCleanUp>(), ScheduleNameOf<CleanUp>());
    CHECK_NE(ScheduleNameOf<CleanUp>(), ScheduleNameOf<PostCleanUp>());
  }

  TEST_CASE("DefaultSchedules: all names are non-empty") {
    CHECK_FALSE(ScheduleNameOf<PreStartup>().empty());
    CHECK_FALSE(ScheduleNameOf<Startup>().empty());
    CHECK_FALSE(ScheduleNameOf<PostStartup>().empty());
    CHECK_FALSE(ScheduleNameOf<Main>().empty());
    CHECK_FALSE(ScheduleNameOf<First>().empty());
    CHECK_FALSE(ScheduleNameOf<PreUpdate>().empty());
    CHECK_FALSE(ScheduleNameOf<Update>().empty());
    CHECK_FALSE(ScheduleNameOf<PostUpdate>().empty());
    CHECK_FALSE(ScheduleNameOf<Last>().empty());

    CHECK_FALSE(ScheduleNameOf<PreCleanUp>().empty());
    CHECK_FALSE(ScheduleNameOf<CleanUp>().empty());
    CHECK_FALSE(ScheduleNameOf<PostCleanUp>().empty());
  }

  TEST_CASE("DefaultSchedules: ordering constraints form valid chain") {
    // PreStartup runs before Startup
    const auto pre_startup_before = ScheduleBeforeOf<PreStartup>();
    CHECK_EQ(pre_startup_before.size(), 1);
    CHECK_EQ(pre_startup_before[0], ScheduleIdOf<Startup>());

    // Startup runs after PreStartup and before PostStartup
    const auto startup_after = ScheduleAfterOf<Startup>();
    CHECK_EQ(startup_after.size(), 1);
    CHECK_EQ(startup_after[0], ScheduleIdOf<PreStartup>());

    const auto startup_before = ScheduleBeforeOf<Startup>();
    CHECK_EQ(startup_before.size(), 1);
    CHECK_EQ(startup_before[0], ScheduleIdOf<PostStartup>());

    // PostStartup runs after Startup
    const auto post_startup_after = ScheduleAfterOf<PostStartup>();
    CHECK_EQ(post_startup_after.size(), 1);
    CHECK_EQ(post_startup_after[0], ScheduleIdOf<Startup>());

    // First runs before PreUpdate
    const auto first_before = ScheduleBeforeOf<First>();
    CHECK_EQ(first_before.size(), 1);
    CHECK_EQ(first_before[0], ScheduleIdOf<PreUpdate>());

    // PreUpdate runs after First and before Update
    const auto pre_update_after = ScheduleAfterOf<PreUpdate>();
    CHECK_EQ(pre_update_after.size(), 1);
    CHECK_EQ(pre_update_after[0], ScheduleIdOf<First>());

    const auto pre_update_before = ScheduleBeforeOf<PreUpdate>();
    CHECK_EQ(pre_update_before.size(), 1);
    CHECK_EQ(pre_update_before[0], ScheduleIdOf<Update>());

    // Update runs after PreUpdate and before PostUpdate
    const auto update_after = ScheduleAfterOf<Update>();
    CHECK_EQ(update_after.size(), 1);
    CHECK_EQ(update_after[0], ScheduleIdOf<PreUpdate>());

    const auto update_before = ScheduleBeforeOf<Update>();
    CHECK_EQ(update_before.size(), 1);
    CHECK_EQ(update_before[0], ScheduleIdOf<PostUpdate>());

    // PostUpdate runs after Update and before Last
    const auto post_update_after = ScheduleAfterOf<PostUpdate>();
    CHECK_EQ(post_update_after.size(), 1);
    CHECK_EQ(post_update_after[0], ScheduleIdOf<Update>());

    const auto post_update_before = ScheduleBeforeOf<PostUpdate>();
    CHECK_EQ(post_update_before.size(), 1);
    CHECK_EQ(post_update_before[0], ScheduleIdOf<Last>());

    // Last runs after PostUpdate
    const auto last_after = ScheduleAfterOf<Last>();
    CHECK_EQ(last_after.size(), 1);
    CHECK_EQ(last_after[0], ScheduleIdOf<PostUpdate>());

    // PreCleanUp runs before CleanUp
    const auto pre_cleanup_before = ScheduleBeforeOf<PreCleanUp>();
    CHECK_EQ(pre_cleanup_before.size(), 1);
    CHECK_EQ(pre_cleanup_before[0], ScheduleIdOf<CleanUp>());

    // CleanUp is a stage schedule, so it has no ordering constraints
    const auto cleanup_after = ScheduleAfterOf<CleanUp>();
    CHECK_EQ(cleanup_after.size(), 0);

    // PostCleanUp runs after CleanUp
    const auto post_cleanup_after = ScheduleAfterOf<PostCleanUp>();
    CHECK_EQ(post_cleanup_after.size(), 1);
    CHECK_EQ(post_cleanup_after[0], ScheduleIdOf<CleanUp>());
  }

  TEST_CASE("IsStage: stage schedules return true") {
    // Stage schedules should return true
    CHECK(IsStage<StartUpStage>());
    CHECK(IsStage<MainStage>());
    CHECK(IsStage<UpdateStage>());
    CHECK(IsStage<CleanUpStage>());

    // Regular schedules should return false
    CHECK_FALSE(IsStage<PreStartup>());
    CHECK_FALSE(IsStage<Startup>());
    CHECK_FALSE(IsStage<PostStartup>());
    CHECK_FALSE(IsStage<Main>());
    CHECK_FALSE(IsStage<First>());
    CHECK_FALSE(IsStage<PreUpdate>());
    CHECK_FALSE(IsStage<Update>());
    CHECK_FALSE(IsStage<PostUpdate>());
    CHECK_FALSE(IsStage<Last>());
    CHECK_FALSE(IsStage<PreCleanUp>());
    CHECK_FALSE(IsStage<CleanUp>());
    CHECK_FALSE(IsStage<PostCleanUp>());
  }

  TEST_CASE("ScheduleStageOf: returns correct stage") {
    // StartUpStage schedules
    CHECK_EQ(ScheduleStageOf<PreStartup>(), ScheduleIdOf<StartUpStage>());
    CHECK_EQ(ScheduleStageOf<Startup>(), ScheduleIdOf<StartUpStage>());
    CHECK_EQ(ScheduleStageOf<PostStartup>(), ScheduleIdOf<StartUpStage>());

    // MainStage schedules
    CHECK_EQ(ScheduleStageOf<Main>(), ScheduleIdOf<MainStage>());

    // UpdateStage schedules
    CHECK_EQ(ScheduleStageOf<First>(), ScheduleIdOf<UpdateStage>());
    CHECK_EQ(ScheduleStageOf<PreUpdate>(), ScheduleIdOf<UpdateStage>());
    CHECK_EQ(ScheduleStageOf<Update>(), ScheduleIdOf<UpdateStage>());
    CHECK_EQ(ScheduleStageOf<PostUpdate>(), ScheduleIdOf<UpdateStage>());
    CHECK_EQ(ScheduleStageOf<Last>(), ScheduleIdOf<UpdateStage>());

    // CleanUpStage schedules
    CHECK_EQ(ScheduleStageOf<PreCleanUp>(), ScheduleIdOf<CleanUpStage>());
    CHECK_EQ(ScheduleStageOf<CleanUp>(), ScheduleIdOf<CleanUpStage>());
    CHECK_EQ(ScheduleStageOf<PostCleanUp>(), ScheduleIdOf<CleanUpStage>());
  }

  TEST_CASE("ScheduleNameOf: stage schedules have names") {
    CHECK_EQ(ScheduleNameOf<StartUpStage>(), "StartUpStage");
    CHECK_EQ(ScheduleNameOf<MainStage>(), "MainStage");
    CHECK_EQ(ScheduleNameOf<UpdateStage>(), "UpdateStage");
    CHECK_EQ(ScheduleNameOf<CleanUpStage>(), "CleanUpStage");
  }

  TEST_CASE("ScheduleWithBeforeTrait: stage schedules do not have Before/After") {
    // Stages do not require Before/After methods
    CHECK_FALSE(ScheduleWithBeforeTrait<StartUpStage>);
    CHECK_FALSE(ScheduleWithAfterTrait<StartUpStage>);
    CHECK_FALSE(ScheduleWithBeforeTrait<MainStage>);
    CHECK_FALSE(ScheduleWithAfterTrait<MainStage>);
    CHECK_FALSE(ScheduleWithBeforeTrait<UpdateStage>);
    CHECK_FALSE(ScheduleWithAfterTrait<UpdateStage>);
    CHECK_FALSE(ScheduleWithBeforeTrait<CleanUpStage>);
    CHECK_FALSE(ScheduleWithAfterTrait<CleanUpStage>);
  }
}
