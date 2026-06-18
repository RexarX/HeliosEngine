#include <doctest/doctest.h>

#include <helios/ecs/message/async_reader.hpp>
#include <helios/ecs/message/async_writer.hpp>
#include <helios/ecs/message/message.hpp>
#include <helios/ecs/message/reader.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/query/query.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/ecs/schedule/run_condition.hpp>
#include <helios/ecs/schedule/system_local_data.hpp>
#include <helios/ecs/world_view.hpp>

using namespace helios::ecs;

namespace {

struct AlwaysTrueCondition {
  bool operator()() const { return true; }
};

struct AlwaysFalseCondition {
  bool operator()() const { return false; }
};

struct NamedCondition {
  static constexpr std::string_view kName = "NamedCondition";

  bool operator()() const { return true; }
};

struct Position {
  float x = 0.0F;
};

struct ThreadSafeData {
  static constexpr bool kThreadSafe = true;
  int value = 0;
};

struct TestMessage {
  int value = 0;
};

struct AsyncTestEvent {
  static constexpr bool kAsync = true;
  int value = 0;
};

struct CounterResource {
  int value = 0;
};

struct QueryRunCondition {
  bool operator()(Query<const Position&> query) const {
    size_t count = 0;
    query.ForEach([&count](const Position& /*pos*/) { ++count; });
    return count > 0;
  }
};

struct AlwaysTrueQueryRunCondition {
  bool operator()(Query<const Position&> /*query*/) const { return true; }
};

struct AsyncResRunCondition {
  bool operator()(AsyncRes<const ThreadSafeData> data) const {
    return data->value >= 0;
  }
};

struct WorldViewRunCondition {
  bool operator()(WorldView view) const { return view.EntityCount() >= 0; }
};

struct MessageReaderRunCondition {
  bool operator()(MessageReader<TestMessage> reader) const {
    return reader.Collect().empty() || reader.Collect().front().value >= 0;
  }
};

struct AsyncMessageReaderRunCondition {
  bool operator()(AsyncMessageReader<AsyncTestEvent> reader) const {
    return reader.Empty() || true;
  }
};

struct CombinedRunCondition {
  bool operator()(Query<const Position&> /*query*/,
                  Res<const CounterResource> /*counter*/) const {
    return true;
  }
};

struct QueryAndAsyncResRunCondition {
  bool operator()(Query<const Position&> /*query*/,
                  AsyncRes<const ThreadSafeData> data) const {
    return data->value >= 0;
  }
};

}  // namespace

TEST_SUITE("helios::ecs::RunConditionStorage") {
  TEST_CASE("ecs::RunConditionStorage::ctor") {
    SUBCASE("Constructing from members creates valid storage") {
      auto storage = RunConditionStorage::From(
          RunCondition([](World& /*world*/, SystemLocalData& /*data*/) -> bool {
            return true;
          }));

      CHECK(storage.run_condition);
      CHECK(storage.name.empty());
    }

    SUBCASE("Constructing with explicit name stores the name") {
      auto storage = RunConditionStorage::From(
          RunCondition([](World& /*world*/, SystemLocalData& /*data*/) -> bool {
            return true;
          }),
          {}, {}, std::string("CustomCondition"));

      CHECK_EQ(storage.name, "CustomCondition");
    }
  }

  TEST_CASE("ecs::RunConditionStorage::From") {
    SUBCASE("From with a predicate stores the callable") {
      auto storage = RunConditionStorage::From(
          RunCondition([](World& /*world*/, SystemLocalData& /*data*/) -> bool {
            return true;
          }));

      CHECK(storage.run_condition);
    }

    SUBCASE("From with a predicate that always returns true") {
      auto storage = RunConditionStorage::From(
          RunCondition([](World& /*world*/, SystemLocalData& /*data*/) -> bool {
            return true;
          }));

      World world;
      SystemLocalData local_data = SystemLocalData::From();

      const bool result = storage.run_condition(world, local_data);
      CHECK(result);
    }

    SUBCASE("From with a predicate that always returns false") {
      auto storage = RunConditionStorage::From(
          RunCondition([](World& /*world*/, SystemLocalData& /*data*/) -> bool {
            return false;
          }));

      World world;
      SystemLocalData local_data = SystemLocalData::From();

      const bool result = storage.run_condition(world, local_data);
      CHECK_FALSE(result);
    }
  }

  TEST_CASE("ecs::RunConditionStorage::FromParam") {
    SUBCASE("FromParam with always-true condition") {
      auto storage = RunConditionStorage::FromParam(AlwaysTrueCondition{});

      CHECK(storage.run_condition);

      World world;
      SystemLocalData local_data = SystemLocalData::From();

      const bool result = storage.run_condition(world, local_data);
      CHECK(result);
    }

    SUBCASE("FromParam with always-false condition") {
      auto storage = RunConditionStorage::FromParam(AlwaysFalseCondition{});

      CHECK(storage.run_condition);

      World world;
      SystemLocalData local_data = SystemLocalData::From();

      const bool result = storage.run_condition(world, local_data);
      CHECK_FALSE(result);
    }

    SUBCASE("FromParam with named condition stores callable and name") {
      auto storage = RunConditionStorage::FromParam(NamedCondition{});

      CHECK(storage.run_condition);
      CHECK_EQ(storage.name, "NamedCondition");
    }
  }

  TEST_CASE("ecs::RunConditionStorage::FromParam Query") {
    SUBCASE("FromParam with Query parameter creates valid storage") {
      auto storage =
          RunConditionStorage::FromParam(AlwaysTrueQueryRunCondition{});

      CHECK(storage.run_condition);
    }

    SUBCASE("FromParam with Query parameter evaluates correctly") {
      auto storage = RunConditionStorage::FromParam(QueryRunCondition{});

      World world;
      Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{1.0F});
      SystemLocalData local_data = SystemLocalData::From();

      const bool result = storage.run_condition(world, local_data);
      CHECK(result);
    }
  }

  TEST_CASE("ecs::RunConditionStorage::FromParam AsyncRes") {
    SUBCASE("FromParam with AsyncRes parameter creates valid storage") {
      auto storage = RunConditionStorage::FromParam(AsyncResRunCondition{});

      CHECK(storage.run_condition);
    }

    SUBCASE("FromParam with AsyncRes parameter evaluates correctly") {
      auto storage = RunConditionStorage::FromParam(AsyncResRunCondition{});

      World world;
      world.InsertResources(ThreadSafeData{5});
      SystemLocalData local_data = SystemLocalData::From();

      const bool result = storage.run_condition(world, local_data);
      CHECK(result);
    }
  }

  TEST_CASE("ecs::RunConditionStorage::FromParam WorldView") {
    SUBCASE("FromParam with WorldView parameter creates valid storage") {
      auto storage = RunConditionStorage::FromParam(WorldViewRunCondition{});

      CHECK(storage.run_condition);
    }

    SUBCASE("FromParam with WorldView parameter evaluates correctly") {
      auto storage = RunConditionStorage::FromParam(WorldViewRunCondition{});

      World world;
      [[maybe_unused]] Entity entity = world.CreateEntity();
      SystemLocalData local_data = SystemLocalData::From();

      const bool result = storage.run_condition(world, local_data);
      CHECK(result);
    }
  }

  TEST_CASE("ecs::RunConditionStorage::FromParam message parameters") {
    SUBCASE("FromParam with MessageReader creates valid storage") {
      auto storage =
          RunConditionStorage::FromParam(MessageReaderRunCondition{});

      CHECK(storage.run_condition);
    }

    SUBCASE("FromParam with AsyncMessageReader creates valid storage") {
      auto storage =
          RunConditionStorage::FromParam(AsyncMessageReaderRunCondition{});

      CHECK(storage.run_condition);
    }

    SUBCASE("FromParam with MessageReader evaluates correctly") {
      auto storage =
          RunConditionStorage::FromParam(MessageReaderRunCondition{});

      World world;
      world.AddMessage<TestMessage>();
      SystemLocalData local_data = SystemLocalData::From();

      const bool result = storage.run_condition(world, local_data);
      CHECK(result);
    }

    SUBCASE("FromParam with AsyncMessageReader evaluates correctly") {
      auto storage =
          RunConditionStorage::FromParam(AsyncMessageReaderRunCondition{});

      World world;
      world.AddMessage<AsyncTestEvent>();
      SystemLocalData local_data = SystemLocalData::From();

      const bool result = storage.run_condition(world, local_data);
      CHECK(result);
    }
  }

  TEST_CASE("ecs::RunConditionStorage::FromParam combined parameters") {
    SUBCASE("FromParam with Query and Res creates valid storage") {
      auto storage = RunConditionStorage::FromParam(CombinedRunCondition{});

      CHECK(storage.run_condition);
    }

    SUBCASE("FromParam with Query and Res evaluates correctly") {
      auto storage = RunConditionStorage::FromParam(CombinedRunCondition{});

      World world;
      world.InsertResources(CounterResource{0});
      SystemLocalData local_data = SystemLocalData::From();

      const bool result = storage.run_condition(world, local_data);
      CHECK(result);
    }

    SUBCASE("FromParam with Query and AsyncRes creates valid storage") {
      auto storage =
          RunConditionStorage::FromParam(QueryAndAsyncResRunCondition{});

      CHECK(storage.run_condition);
    }

    SUBCASE("FromParam with Query and AsyncRes evaluates correctly") {
      auto storage =
          RunConditionStorage::FromParam(QueryAndAsyncResRunCondition{});

      World world;
      world.InsertResources(ThreadSafeData{3});
      SystemLocalData local_data = SystemLocalData::From();

      const bool result = storage.run_condition(world, local_data);
      CHECK(result);
    }
  }

  TEST_CASE("ecs::RunConditionStorage::FromParamNamed") {
    SUBCASE("FromParamNamed with lambda produces valid storage and name") {
      auto storage = RunConditionStorage::FromParamNamed(
          "LambdaCondition",
          [](Res<CounterResource> counter) -> bool { return true; });

      CHECK(storage.run_condition);
      CHECK_EQ(storage.name, "LambdaCondition");
    }

    SUBCASE("FromParamNamed lambda evaluates correctly") {
      auto storage = RunConditionStorage::FromParamNamed(
          "AlwaysTrueLambda",
          [](Res<CounterResource> counter) -> bool { return true; });

      World world;
      world.InsertResources(CounterResource{5});
      SystemLocalData local_data = SystemLocalData::From();

      const bool result = storage.run_condition(world, local_data);
      CHECK(result);
    }

    SUBCASE("FromParamNamed lambda returning false evaluates correctly") {
      auto storage = RunConditionStorage::FromParamNamed(
          "AlwaysFalseLambda",
          [](Res<CounterResource> counter) -> bool { return false; });

      World world;
      world.InsertResources(CounterResource{5});
      SystemLocalData local_data = SystemLocalData::From();

      const bool result = storage.run_condition(world, local_data);
      CHECK_FALSE(result);
    }
  }
}
