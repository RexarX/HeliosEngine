#include <doctest/doctest.h>

#include <helios/async/executor.hpp>
#include <helios/ecs/command/commands.hpp>
#include <helios/ecs/message/reader.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/schedule/executor/main_thread.hpp>
#include <helios/ecs/schedule/scheduler.hpp>

using namespace helios::ecs;

namespace {

struct MyFirstLabel {};
struct MySecondLabel {};
struct MyThirdLabel {};
struct UpdateLabel {};

struct TestStage {
  static constexpr std::string_view kName = "TestStage";
};

struct OtherStage {
  static constexpr std::string_view kName = "OtherStage";
};

struct OtherStageSchedule {};
struct TestStageSchedule {};

struct StageScheduleALabel {};
struct StageScheduleBLabel {};

struct NamedLabel {
  static constexpr std::string_view kName = "NamedLabel";
};

struct SchedALabel {};
struct SchedBLabel {};
struct SchedCLabel {};

struct CounterResource {
  int value = 0;
};

struct IncrementSystem {
  void operator()(Res<CounterResource> counter) const { ++counter->value; }
};

struct SchedulerMsg {
  int value = 0;
};

struct WriteMsgSystem {
  int msg_value = 0;

  void operator()(MessageWriter<SchedulerMsg> writer) const {
    writer.Write(SchedulerMsg{msg_value});
  }
};

struct ReadMsgSystem {
  void operator()(MessageReader<SchedulerMsg> reader,
                  Res<CounterResource> counter) const {
    for (const auto msg : reader) {
      counter->value += msg->value;
    }
  }
};

struct CreateEntityCmd {
  static void Execute(World& world) {
    [[maybe_unused]] const auto entity = world.CreateEntity();
  }
};

struct EnqueueCommandSystem {
  void operator()(Commands commands) const {
    commands.Enqueue(CreateEntityCmd{});
  }
};

struct EntityCountSystem {
  void operator()(Res<CounterResource> counter, WorldView view) const {
    counter->value = static_cast<int>(view.EntityCount());
  }
};

struct ReserveEntitySystem {
  void operator()(Commands commands) const { commands.Spawn(); }
};

struct CheckEntitySystem {
  void operator()(WorldView view, Res<CounterResource> counter) const {
    counter->value = static_cast<int>(view.EntityCount());
  }
};

}  // namespace

TEST_SUITE("helios::ecs::ScheduleOrdering") {
  TEST_CASE("ecs::ScheduleOrdering::ctor") {
    SUBCASE("Constructing with scheduler and hash is valid") {
      Scheduler scheduler;
      Schedule schedule;
      auto ordering = scheduler.Add(MyFirstLabel{}, std::move(schedule));
      CHECK(scheduler.IsDirty());
    }

    SUBCASE("Constructing with scheduler and index is valid") {
      Scheduler scheduler;
      Schedule schedule;
      auto ordering = scheduler.Add(ScheduleTypeId::From(MyFirstLabel{}),
                                    std::move(schedule));

      CHECK(scheduler.IsDirty());
    }
  }

  TEST_CASE("ecs::ScheduleOrdering::After") {
    SUBCASE("After marks schedule to run after the given label") {
      Scheduler scheduler;
      Schedule schedule;
      auto ordering = scheduler.Add(MyFirstLabel{}, std::move(schedule));

      ordering.After(MySecondLabel{});

      CHECK(scheduler.IsDirty());
    }

    SUBCASE("After with same label twice adds it only once") {
      Scheduler scheduler;
      Schedule schedule;
      auto ordering = scheduler.Add(MyFirstLabel{}, std::move(schedule));

      ordering.After(MySecondLabel{});
      ordering.After(MySecondLabel{});

      CHECK(scheduler.IsDirty());
    }
  }

  TEST_CASE("ecs::ScheduleOrdering::Before") {
    SUBCASE("Before marks schedule to run before the given label") {
      Scheduler scheduler;
      Schedule schedule;
      auto ordering = scheduler.Add(MyFirstLabel{}, std::move(schedule));

      ordering.Before(MySecondLabel{});

      CHECK(scheduler.IsDirty());
    }

    SUBCASE("Before with same label twice adds it only once") {
      Scheduler scheduler;
      Schedule schedule;
      auto ordering = scheduler.Add(MyFirstLabel{}, std::move(schedule));

      ordering.Before(MySecondLabel{});
      ordering.Before(MySecondLabel{});

      CHECK(scheduler.IsDirty());
    }
  }

  TEST_CASE("ecs::ScheduleOrdering::Done") {
    SUBCASE("Done returns a reference to the parent scheduler") {
      Scheduler scheduler;
      Schedule schedule;
      auto ordering = scheduler.Add(MyFirstLabel{}, std::move(schedule));

      Scheduler& result = ordering.Done();

      CHECK_EQ(&result, &scheduler);
    }
  }
}

TEST_SUITE("helios::ecs::Scheduler") {
  TEST_CASE("ecs::Scheduler::ctor") {
    SUBCASE("Default-constructed scheduler is dirty") {
      const Scheduler scheduler;
      CHECK(scheduler.IsDirty());
    }
  }

  TEST_CASE("ecs::Scheduler::operator=") {
    SUBCASE("Move assignment transfers state") {
      Scheduler source;
      source.Add(MyFirstLabel{}, Schedule{});
      source.Build();

      Scheduler target;
      target = std::move(source);

      CHECK_FALSE(target.IsDirty());
    }
  }

  TEST_CASE("ecs::Scheduler::Build") {
    SUBCASE("Building an empty scheduler succeeds") {
      Scheduler scheduler;
      scheduler.Build();
      CHECK_FALSE(scheduler.IsDirty());
    }

    SUBCASE("Building a scheduler with one schedule succeeds") {
      Scheduler scheduler;
      scheduler.Add(MyFirstLabel{}, Schedule{});

      scheduler.Build();

      CHECK_FALSE(scheduler.IsDirty());
    }

    SUBCASE("Building with multiple schedules succeeds") {
      Scheduler scheduler;
      scheduler.Add(MyFirstLabel{}, Schedule{});
      scheduler.Add(MySecondLabel{}, Schedule{});

      scheduler.Build();

      CHECK_FALSE(scheduler.IsDirty());
    }

    SUBCASE("Building with ordering constraints succeeds") {
      Scheduler scheduler;
      auto first = scheduler.Add(MyFirstLabel{}, Schedule{});
      scheduler.Add(MySecondLabel{}, Schedule{});
      first.After(MySecondLabel{});

      scheduler.Build();

      CHECK_FALSE(scheduler.IsDirty());
    }
  }

  TEST_CASE("ecs::Scheduler::Run") {
    SUBCASE("Run with explicit executor executes schedules") {
      Scheduler scheduler;
      scheduler.Add(UpdateLabel{}, Schedule{});
      scheduler.In(UpdateLabel{}).Add(IncrementSystem{});
      scheduler.Build();

      World world;
      world.InsertResources(CounterResource{0});
      MainThreadExecutor executor;
      scheduler.Run(world, executor);

      CHECK_EQ(world.ReadResource<CounterResource>().value, 1);
    }

    SUBCASE("Run without executor executes schedules") {
      Scheduler scheduler;
      scheduler.Add(UpdateLabel{}, Schedule{});
      scheduler.In(UpdateLabel{}).Add(IncrementSystem{});
      scheduler.Build();

      World world;
      world.InsertResources(CounterResource{0});
      scheduler.Run(world);

      CHECK_EQ(world.ReadResource<CounterResource>().value, 1);
    }
  }

  TEST_CASE("RunStage") {
    SUBCASE("RunStage executes only schedules in the given stage") {
      std::array<int, 2> order{};
      int call_index = 0;

      struct OrderSystem {
        int* order_slot = nullptr;
        int* call_index = nullptr;

        void operator()(Res<CounterResource> /*counter*/) const {
          *order_slot = (*call_index)++;
        }
      };

      Scheduler scheduler;
      scheduler.AddStage(TestStage{});
      scheduler.Add(StageScheduleALabel{}, Schedule{}).InStage(TestStage{});
      scheduler.Add(StageScheduleBLabel{}, Schedule{})
          .InStage(TestStage{})
          .After(StageScheduleALabel{});
      scheduler.Add(MyThirdLabel{}, Schedule{})
          .InStage(TestStage{})
          .After(StageScheduleBLabel{});

      scheduler.In(StageScheduleALabel{})
          .Add(OrderSystem{.order_slot = order.data(),
                           .call_index = &call_index});
      scheduler.In(StageScheduleBLabel{})
          .Add(OrderSystem{.order_slot = &order[1], .call_index = &call_index});
      scheduler.In(MyThirdLabel{}).Add(IncrementSystem{});

      scheduler.Build();

      World world;
      world.InsertResources(CounterResource{});
      scheduler.RunStage(TestStage{}, world);

      CHECK_LT(order[0], order[1]);
      CHECK_EQ(world.ReadResource<CounterResource>().value, 1);
    }

    SUBCASE("Stage-level ordering is respected during Run") {
      std::array<int, 2> stage_order{};
      int call_index = 0;

      struct StageMarkerSystem {
        int* order_slot = nullptr;
        int* call_index = nullptr;

        void operator()(Res<CounterResource> /*counter*/) const {
          *order_slot = (*call_index)++;
        }
      };

      Scheduler scheduler;
      scheduler.AddStage(OtherStage{});
      scheduler.AddStage(TestStage{});
      scheduler.OrderStage(TestStage{}).After(OtherStage{});
      scheduler.Add(OtherStageSchedule{}, Schedule{}).InStage(OtherStage{});
      scheduler.Add(TestStageSchedule{}, Schedule{}).InStage(TestStage{});
      scheduler.In(OtherStageSchedule{})
          .Add(StageMarkerSystem{.order_slot = stage_order.data(),
                                 .call_index = &call_index});
      scheduler.In(TestStageSchedule{})
          .Add(StageMarkerSystem{.order_slot = &stage_order[1],
                                 .call_index = &call_index});

      scheduler.Build();

      World world;
      world.InsertResources(CounterResource{});
      scheduler.Run(world);

      CHECK_LT(stage_order[0], stage_order[1]);
    }
  }

  TEST_CASE("Clear") {
    SUBCASE("Clear removes all schedules and marks scheduler dirty") {
      Scheduler scheduler;
      scheduler.Add(MyFirstLabel{}, Schedule{});
      scheduler.Build();

      scheduler.Clear();

      CHECK(scheduler.IsDirty());
      CHECK_EQ(scheduler.TryGetSchedule(MyFirstLabel{}), nullptr);
    }
  }

  TEST_CASE("ecs::Scheduler::In") {
    SUBCASE("In with label hash returns the schedule") {
      Scheduler scheduler;
      scheduler.Add(MyFirstLabel{}, Schedule{});

      const auto index = ScheduleTypeIndex::From(MyFirstLabel{});
      Schedule& schedule = scheduler.In(index);

      CHECK_EQ(&schedule, scheduler.TryGetSchedule(index));
    }

    SUBCASE("In with template label type returns the schedule") {
      Scheduler scheduler;
      scheduler.Add(MyFirstLabel{}, Schedule{});

      Schedule& schedule = scheduler.In(MyFirstLabel{});

      CHECK_EQ(&schedule, scheduler.TryGetSchedule(MyFirstLabel{}));
    }
  }

  TEST_CASE("ecs::Scheduler::Add") {
    SUBCASE("Adding a schedule with a label returns an ordering handle") {
      Scheduler scheduler;
      Schedule schedule;

      auto ordering = scheduler.Add(MyFirstLabel{}, std::move(schedule));

      CHECK(scheduler.IsDirty());
    }

    SUBCASE("Adding a schedule with a named label works") {
      Scheduler scheduler;
      Schedule schedule;

      auto ordering = scheduler.Add(NamedLabel{}, std::move(schedule));

      CHECK_NE(scheduler.TryGetSchedule(NamedLabel{}), nullptr);
    }

    SUBCASE("Adding with ScheduleTypeId overload works") {
      Scheduler scheduler;
      Schedule schedule;

      auto ordering = scheduler.Add(ScheduleTypeId::From(MyFirstLabel{}),
                                    std::move(schedule));

      CHECK(scheduler.IsDirty());
    }
  }

  TEST_CASE("ecs::Scheduler::Remove") {
    SUBCASE("Remove returns true for an existing schedule") {
      Scheduler scheduler;
      scheduler.Add(MyFirstLabel{}, Schedule{});

      const bool removed = scheduler.Remove(MyFirstLabel{});

      CHECK(removed);
      CHECK(scheduler.IsDirty());
    }

    SUBCASE("Remove returns false for a nonexistent schedule") {
      Scheduler scheduler;

      const bool removed = scheduler.Remove(MyFirstLabel{});

      CHECK_FALSE(removed);
    }

    SUBCASE("Remove only affects the targeted schedule") {
      Scheduler scheduler;
      scheduler.Add(MyFirstLabel{}, Schedule{});
      scheduler.Add(MySecondLabel{}, Schedule{});

      scheduler.Remove(MyFirstLabel{});

      CHECK_FALSE(scheduler.TryGetSchedule(MyFirstLabel{}));
      CHECK_NE(scheduler.TryGetSchedule(MySecondLabel{}), nullptr);
    }
  }

  TEST_CASE("ecs::Scheduler::TryGetSchedule") {
    SUBCASE("Mutable TryGetSchedule returns pointer for existing schedule") {
      Scheduler scheduler;
      scheduler.Add(MyFirstLabel{}, Schedule{});

      const auto index = ScheduleTypeIndex::From(MyFirstLabel{});
      Schedule* schedule = scheduler.TryGetSchedule(index);

      CHECK_NE(schedule, nullptr);
    }

    SUBCASE("Mutable TryGetSchedule returns nullptr for nonexistent schedule") {
      Scheduler scheduler;

      const auto index = ScheduleTypeIndex::From(MyFirstLabel{});
      Schedule* schedule = scheduler.TryGetSchedule(index);

      CHECK_EQ(schedule, nullptr);
    }

    SUBCASE(
        "Const TryGetSchedule returns const pointer for existing schedule") {
      Scheduler scheduler;
      scheduler.Add(MyFirstLabel{}, Schedule{});

      const Scheduler& const_scheduler = scheduler;
      const auto index = ScheduleTypeIndex::From(MyFirstLabel{});
      const Schedule* schedule = const_scheduler.TryGetSchedule(index);

      CHECK_NE(schedule, nullptr);
    }

    SUBCASE("Const TryGetSchedule returns nullptr for nonexistent schedule") {
      const Scheduler scheduler;

      const auto index = ScheduleTypeIndex::From(MyFirstLabel{});
      const Schedule* schedule = scheduler.TryGetSchedule(index);

      CHECK_EQ(schedule, nullptr);
    }

    SUBCASE("Template TryGetSchedule returns pointer for existing schedule") {
      Scheduler scheduler;
      scheduler.Add(MyFirstLabel{}, Schedule{});

      Schedule* schedule = scheduler.TryGetSchedule(MyFirstLabel{});

      CHECK_NE(schedule, nullptr);
    }

    SUBCASE("Const template TryGetSchedule returns const pointer") {
      Scheduler scheduler;
      scheduler.Add(MyFirstLabel{}, Schedule{});

      const Scheduler& const_scheduler = scheduler;
      const Schedule* schedule = const_scheduler.TryGetSchedule(MyFirstLabel{});

      CHECK_NE(schedule, nullptr);
    }
  }

  TEST_CASE("ecs::Scheduler::IsDirty") {
    SUBCASE("New scheduler is dirty") {
      const Scheduler scheduler;
      CHECK(scheduler.IsDirty());
    }

    SUBCASE("After adding a schedule, scheduler is dirty") {
      Scheduler scheduler;
      scheduler.Add(MyFirstLabel{}, Schedule{});

      CHECK(scheduler.IsDirty());
    }

    SUBCASE("After Build, scheduler is clean") {
      Scheduler scheduler;
      scheduler.Add(MyFirstLabel{}, Schedule{});

      scheduler.Build();

      CHECK_FALSE(scheduler.IsDirty());
    }

    SUBCASE("After removing a schedule, scheduler is dirty") {
      Scheduler scheduler;
      scheduler.Add(MyFirstLabel{}, Schedule{});
      scheduler.Build();

      scheduler.Remove(MyFirstLabel{});

      CHECK(scheduler.IsDirty());
    }
  }

  TEST_CASE("ecs::Scheduler: schedule ordering execution") {
    SUBCASE("Ordering between schedules is respected during Run") {
      std::array<int, 3> order{};
      int call_index = 0;

      struct OrderSystem {
        int* order_slot = nullptr;
        int* call_index = nullptr;

        void operator()(Res<CounterResource> /*counter*/) const {
          *order_slot = (*call_index)++;
        }
      };

      Scheduler scheduler;
      auto first_ordering = scheduler.Add(MyFirstLabel{}, Schedule{});
      auto second_ordering = scheduler.Add(MySecondLabel{}, Schedule{});
      auto third_ordering = scheduler.Add(MyThirdLabel{}, Schedule{});

      scheduler.In(MyFirstLabel{})
          .Add(OrderSystem{.order_slot = order.data(),
                           .call_index = &call_index});
      scheduler.In(MySecondLabel{})
          .Add(OrderSystem{.order_slot = &order[1], .call_index = &call_index});
      scheduler.In(MyThirdLabel{})
          .Add(OrderSystem{.order_slot = &order[2], .call_index = &call_index});

      first_ordering.Before(MySecondLabel{});
      second_ordering.Before(MyThirdLabel{});

      scheduler.Build();

      World world;
      world.InsertResources(CounterResource{});
      scheduler.Run(world);

      CHECK_LT(order[0], order[1]);
      CHECK_LT(order[1], order[2]);
    }
  }

  TEST_CASE("ecs::Scheduler: commands across schedules") {
    SUBCASE("Commands enqueued in schedule A take effect before schedule B") {
      Scheduler scheduler;
      scheduler.Add(SchedALabel{}, Schedule{});
      scheduler.Add(SchedBLabel{}, Schedule{}).After(SchedALabel{});

      scheduler.In(SchedALabel{}).Add(EnqueueCommandSystem{});
      scheduler.In(SchedBLabel{}).Add(EntityCountSystem{});

      scheduler.Build();

      World world;
      world.InsertResources(CounterResource{0});
      scheduler.Run(world);

      CHECK_EQ(world.ReadResource<CounterResource>().value, 1);
    }

    SUBCASE(
        "Commands executed by world.Flush between schedules, visible to next "
        "schedule") {
      Scheduler scheduler;
      scheduler.Add(SchedALabel{}, Schedule{});
      scheduler.Add(SchedBLabel{}, Schedule{}).After(SchedALabel{});
      scheduler.Add(SchedCLabel{}, Schedule{}).After(SchedBLabel{});

      scheduler.In(SchedALabel{})
          .Add("EnqueueCreateEntityA",
               [](Commands commands) { commands.Enqueue(CreateEntityCmd{}); });
      scheduler.In(SchedBLabel{})
          .Add("EnqueueCreateEntityB",
               [](Commands commands) { commands.Enqueue(CreateEntityCmd{}); });
      scheduler.In(SchedCLabel{})
          .Add("CountEntities",
               [](WorldView view, Res<CounterResource> counter) {
                 counter->value = static_cast<int>(view.EntityCount());
               });

      scheduler.Build();

      World world;
      world.InsertResources(CounterResource{0});
      scheduler.Run(world);

      CHECK_EQ(world.ReadResource<CounterResource>().value, 2);
    }
  }

  TEST_CASE("ecs::Scheduler: messages across schedules") {
    SUBCASE("Message written in schedule A is readable in schedule B") {
      Scheduler scheduler;
      scheduler.Add(SchedALabel{}, Schedule{});
      scheduler.Add(SchedBLabel{}, Schedule{}).After(SchedALabel{});

      scheduler.In(SchedALabel{}).Add(WriteMsgSystem{.msg_value = 42});
      scheduler.In(SchedBLabel{}).Add(ReadMsgSystem{});

      scheduler.Build();

      World world;
      world.InsertResources(CounterResource{0});
      world.AddMessage<SchedulerMsg>();
      scheduler.Run(world);
      world.Update();

      CHECK_EQ(world.ReadResource<CounterResource>().value, 42);
    }

    SUBCASE("Message written in frame N survives to frame N+1") {
      Scheduler write_scheduler;
      write_scheduler.Add(UpdateLabel{}, Schedule{});

      write_scheduler.In(UpdateLabel{}).Add(WriteMsgSystem{.msg_value = 7});

      write_scheduler.Build();

      World world;
      world.InsertResources(CounterResource{0});
      world.AddMessage<SchedulerMsg>();
      write_scheduler.Run(world);
      world.Update();

      Scheduler read_scheduler;
      read_scheduler.Add(UpdateLabel{}, Schedule{});

      read_scheduler.In(UpdateLabel{}).Add(ReadMsgSystem{});

      read_scheduler.Build();

      read_scheduler.Run(world);

      CHECK_EQ(world.ReadResource<CounterResource>().value, 7);
    }
  }

  TEST_CASE("ecs::Scheduler: stages are not schedules") {
    SUBCASE("Registered stages are not returned by TryGetSchedule") {
      Scheduler scheduler;
      scheduler.AddStage(TestStage{});
      scheduler.Add(StageScheduleALabel{}, Schedule{}).InStage(TestStage{});

      CHECK_FALSE(scheduler.TryGetSchedule(TestStage{}));
      CHECK_NE(scheduler.TryGetSchedule(StageScheduleALabel{}), nullptr);
      CHECK(scheduler.HasStage(TestStage{}));
    }

    SUBCASE("Add overwrite resets schedule ordering metadata") {
      Scheduler scheduler;
      scheduler.Add(MyFirstLabel{}, Schedule{}).After(MySecondLabel{});
      scheduler.Add(MySecondLabel{}, Schedule{});
      scheduler.Add(MyFirstLabel{}, Schedule{});

      scheduler.Build();
      CHECK_FALSE(scheduler.IsDirty());
    }
  }

  TEST_CASE("ecs::Scheduler: entity reservations across schedules") {
    SUBCASE("Reserved entities are alive in next schedule after Flush") {
      Scheduler scheduler;
      scheduler.Add(SchedALabel{}, Schedule{});
      scheduler.Add(SchedBLabel{}, Schedule{}).After(SchedALabel{});

      scheduler.In(SchedALabel{}).Add("SpawnEntity", [](Commands commands) {
        commands.Spawn();
      });
      scheduler.In(SchedBLabel{}).Add(CheckEntitySystem{});

      scheduler.Build();

      World world;
      world.InsertResources(CounterResource{0});
      scheduler.Run(world);

      CHECK_EQ(world.ReadResource<CounterResource>().value, 1);
    }
  }
}
