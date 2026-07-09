#include <doctest/doctest.h>

#include <helios/ecs/command/commands.hpp>
#include <helios/ecs/component/component.hpp>
#include <helios/ecs/message/async_reader.hpp>
#include <helios/ecs/message/async_writer.hpp>
#include <helios/ecs/message/message.hpp>
#include <helios/ecs/message/reader.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/query/query.hpp>
#include <helios/ecs/resource/local_param.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/ecs/resource/resource.hpp>
#include <helios/ecs/schedule/executor/main_thread.hpp>
#include <helios/ecs/schedule/schedule.hpp>
#include <helios/ecs/system/param_traits.hpp>
#include <helios/ecs/system/system.hpp>
#include <helios/ecs/world.hpp>
#include <helios/ecs/world_view.hpp>
#include <helios/utils/hash.hpp>

#include <algorithm>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

using namespace helios;
using namespace helios::ecs;

namespace {

// Test component types
struct Position {
  float x = 0.0F;
  float y = 0.0F;
};

struct Velocity {
  float dx = 0.0F;
  float dy = 0.0F;
};

struct Health {
  int value = 0;
};

struct Disabled {};

// Test resource types
struct Camera {
  float fov = 90.0F;
};

struct RenderSettings {
  bool vsync = false;
};

struct AsyncCounter {
  static constexpr bool kThreadSafe = true;

  int value = 0;
};

// Test message types
struct DamageMsg {
  int amount = 0;
};

struct ConsumableDamageMsg {
  static constexpr bool kConsumable = true;

  int amount = 0;
};

struct AsyncSignalMsg {
  static constexpr bool kAsync = true;

  int value = 0;
};

// --- Basic system types (empty operator()) ---

struct SimpleSystem {
  void operator()() {}
};

struct NamedSystem {
  static constexpr std::string_view kName = "NamedSystem";

  void operator()() {}
};

struct ReadOnlySystem {
  void operator()() {}
};

struct ComplexSystem {
  static constexpr std::string_view kName = "ComplexSystem";

  void operator()() {}

  int data = 0;
};

struct LargeSystem {
  void operator()() {}

  double data[100] = {};
};

struct AlignedSystem {
  void operator()() {}

  alignas(16) float data[4] = {};
};

// --- Systems with ECS parameter types ---

struct QueryOnlySystem {
  void operator()(Query<const Position&, Velocity&> /*query*/) {}
};

struct ResourceOnlySystem {
  void operator()(Res<const Camera> /*camera*/,
                  Res<RenderSettings> /*settings*/) {}
};

struct LocalOnlySystem {
  void operator()(Local<Health> /*health*/) {}
};

struct CommandsOnlySystem {
  void operator()(Commands /*commands*/) {}
};

struct WorldViewOnlySystem {
  void operator()(WorldView /*view*/) {}
};

struct MessageReaderOnlySystem {
  void operator()(MessageReader<DamageMsg> /*reader*/) {}
};

struct ConsumableMessageReaderOnlySystem {
  void operator()(ConsumableMessageReader<ConsumableDamageMsg> /*reader*/) {}
};

struct MessageWriterOnlySystem {
  void operator()(MessageWriter<DamageMsg> /*writer*/) {}
};

struct AsyncMessageReaderOnlySystem {
  void operator()(AsyncMessageReader<AsyncSignalMsg> /*reader*/) {}
};

struct AsyncMessageWriterOnlySystem {
  void operator()(AsyncMessageWriter<AsyncSignalMsg> /*writer*/) {}
};

struct OptionalResourceSystem {
  void operator()(std::optional<Res<const Camera>> /*camera*/,
                  std::optional<Res<RenderSettings>> /*settings*/) {}
};

struct OptionalLocalSystem {
  void operator()(std::optional<Local<const Camera>> /*camera*/,
                  std::optional<Local<RenderSettings>> /*settings*/) {}
};

struct QueryResourceSystem {
  void operator()(Query<const Position&, Velocity&> /*query*/,
                  Res<const Camera> /*camera*/,
                  Res<RenderSettings> /*settings*/) {}
};

struct QueryLocalSystem {
  void operator()(Query<const Position&> /*query*/, Local<Camera> /*camera*/) {}
};

struct QueryCommandsSystem {
  void operator()(Query<const Position&> /*query*/, Commands /*commands*/) {}
};

struct QueryWorldViewSystem {
  void operator()(Query<const Position&> /*query*/, WorldView /*view*/) {}
};

struct QueryMessageSystem {
  void operator()(Query<const Position&> /*query*/,
                  MessageReader<DamageMsg> /*reader*/,
                  MessageWriter<DamageMsg> /*writer*/) {}
};

struct QueryResourceLocalSystem {
  void operator()(Query<const Position&> /*query*/,
                  Res<const Camera> /*camera*/, Local<Camera> /*local*/) {}
};

struct AllParamsSystem {
  void operator()(
      Query<const Position&, Velocity&> /*query*/, Res<const Camera> /*camera*/,
      Res<RenderSettings> /*settings*/, Local<Health> /*health*/,
      Commands /*commands*/, WorldView /*view*/,
      MessageReader<DamageMsg> /*reader*/,
      ConsumableMessageReader<ConsumableDamageMsg> /*consumable_reader*/,
      MessageWriter<DamageMsg> /*writer*/,
      AsyncMessageReader<AsyncSignalMsg> /*async_reader*/,
      AsyncMessageWriter<AsyncSignalMsg> /*async_writer*/) {}
};

struct ConstOperatorSystem {
  void operator()(Query<const Position&> /*query*/) const {}
};

struct ConstOperatorResourceSystem {
  void operator()(Res<const Camera> /*camera*/) const {}
};

struct ConstOperatorAllParamsSystem {
  void operator()(Query<const Position&> /*query*/,
                  Res<const Camera> /*camera*/, Local<Camera> /*local*/,
                  Commands /*commands*/, WorldView /*view*/) const {}
};

struct NoexceptSystem {
  void operator()(Query<const Position&> /*query*/) noexcept {}
};

struct ConstNoexceptSystem {
  void operator()(Res<const Camera> /*camera*/) const noexcept {}
};

struct StaticOperatorSystem {
  static void operator()(Query<const Position&> /*query*/) {}
};

class PolymorphicSystem {
public:
  virtual ~PolymorphicSystem() = default;

  void operator()() {}
};

struct NonCopyableNonMovableSystem {
  NonCopyableNonMovableSystem() = default;
  NonCopyableNonMovableSystem(const NonCopyableNonMovableSystem&) = delete;
  NonCopyableNonMovableSystem(NonCopyableNonMovableSystem&&) = delete;

  NonCopyableNonMovableSystem& operator=(const NonCopyableNonMovableSystem&) =
      delete;
  NonCopyableNonMovableSystem& operator=(NonCopyableNonMovableSystem&&) =
      delete;

  void operator()() {}
};

struct EmptyCallSystem {};

// A type where operator() takes a parameter that lacks SystemParamTraits
struct InvalidParamSystem {
  void operator()(int /*bad*/, float /*also_bad*/) {}
};

// A type where operator() mixes valid and invalid params
struct MixedValidInvalidParamSystem {
  void operator()(Query<const Position&> /*query*/, int /*bad*/) {}
};

// Free function for testing
constexpr void FreeFunctionNoParams() {}

// Free function with ECS params
constexpr void FreeFunctionWithQuery(Query<const Position&> /*query*/) {}

// Free function with mixed ECS params
constexpr void FreeFunctionWithMixed(Query<const Position&> /*query*/,
                                     Res<const Camera> /*camera*/) {}

// Noexcept free function with ECS params
constexpr void FreeFunctionWithQueryNoexcept(
    Query<const Position&> /*query*/) noexcept {}

// Another free function for testing (invalid param)
constexpr void AnotherFreeFunction(int /*value*/) {}

}  // namespace

namespace helios::ecs::free_function_system_test {

struct FreeCounter {
  int value = 0;
};

struct FreePosition {
  float x = 0.0F;
};

struct FreeSet {};

inline int no_param_calls = 0;

void CountNoParamCall() {
  ++no_param_calls;
}

void IncrementCounter(Res<FreeCounter> counter) {
  ++counter->value;
}

void IncrementCounterNoexcept(Res<FreeCounter> counter) noexcept {
  ++counter->value;
}

void AccumulatePositions(Query<const FreePosition&> query,
                         Res<FreeCounter> counter) {
  query.ForEach(
      [&counter](const FreePosition& /*position*/) { ++counter->value; });
}

bool CounterIsZero(Res<const FreeCounter> counter) {
  return counter->value == 0;
}

bool CounterIsPositive(Res<const FreeCounter> counter) {
  return counter->value > 0;
}

}  // namespace helios::ecs::free_function_system_test

TEST_SUITE("helios::ecs::SystemTrait") {
  TEST_CASE("ecs::SystemTrait::concept") {
    SUBCASE("Functors with empty operator satisfy SystemTrait") {
      CHECK(SystemTrait<SimpleSystem>);
      CHECK(SystemTrait<NamedSystem>);
      CHECK(SystemTrait<ComplexSystem>);
    }

    SUBCASE("Free functions satisfy SystemTrait") {
      CHECK(SystemTrait<decltype(&FreeFunctionNoParams)>);
      CHECK_FALSE(SystemTrait<decltype(&AnotherFreeFunction)>);
    }

    SUBCASE("Lambdas satisfy SystemTrait") {
      const auto lambda = []() {};
      CHECK(SystemTrait<decltype(lambda)>);
    }

    SUBCASE("Systems with Query params satisfy SystemTrait") {
      CHECK(SystemTrait<QueryOnlySystem>);
      CHECK(SystemTrait<QueryResourceLocalSystem>);
    }

    SUBCASE("Systems with Resource params satisfy SystemTrait") {
      CHECK(SystemTrait<ResourceOnlySystem>);
    }

    SUBCASE("Systems with Local params satisfy SystemTrait") {
      CHECK(SystemTrait<LocalOnlySystem>);
    }

    SUBCASE("Systems with Commands param satisfy SystemTrait") {
      CHECK(SystemTrait<CommandsOnlySystem>);
    }

    SUBCASE("Systems with WorldView param satisfy SystemTrait") {
      CHECK(SystemTrait<WorldViewOnlySystem>);
    }

    SUBCASE("Systems with MessageReader param satisfy SystemTrait") {
      CHECK(SystemTrait<MessageReaderOnlySystem>);
    }

    SUBCASE("Systems with ConsumableMessageReader param satisfy SystemTrait") {
      CHECK(SystemTrait<ConsumableMessageReaderOnlySystem>);
    }

    SUBCASE("Systems with MessageWriter param satisfy SystemTrait") {
      CHECK(SystemTrait<MessageWriterOnlySystem>);
    }

    SUBCASE("Systems with AsyncMessageReader param satisfy SystemTrait") {
      CHECK(SystemTrait<AsyncMessageReaderOnlySystem>);
    }

    SUBCASE("Systems with AsyncMessageWriter param satisfy SystemTrait") {
      CHECK(SystemTrait<AsyncMessageWriterOnlySystem>);
    }

    SUBCASE("Systems with optional Resource params satisfy SystemTrait") {
      CHECK(SystemTrait<OptionalResourceSystem>);
    }

    SUBCASE("Systems with optional Local params satisfy SystemTrait") {
      CHECK(SystemTrait<OptionalLocalSystem>);
    }
  }

  TEST_CASE("ecs::SystemTrait::combinations") {
    SUBCASE("Query + Resource combination satisfies SystemTrait") {
      CHECK(SystemTrait<QueryResourceSystem>);
    }

    SUBCASE("Query + Local combination satisfies SystemTrait") {
      CHECK(SystemTrait<QueryLocalSystem>);
    }

    SUBCASE("Query + Commands combination satisfies SystemTrait") {
      CHECK(SystemTrait<QueryCommandsSystem>);
    }

    SUBCASE("Query + WorldView combination satisfies SystemTrait") {
      CHECK(SystemTrait<QueryWorldViewSystem>);
    }

    SUBCASE("Query + MessageReader + MessageWriter satisfies SystemTrait") {
      CHECK(SystemTrait<QueryMessageSystem>);
    }

    SUBCASE("Query + Resource + Local combination satisfies SystemTrait") {
      CHECK(SystemTrait<QueryResourceLocalSystem>);
    }

    SUBCASE("All param types combined satisfy SystemTrait") {
      CHECK(SystemTrait<AllParamsSystem>);
    }
  }

  TEST_CASE("ecs::SystemTrait::const_operator") {
    SUBCASE("Const operator() with Query param satisfies SystemTrait") {
      CHECK(SystemTrait<ConstOperatorSystem>);
    }

    SUBCASE("Const operator() with Resource param satisfies SystemTrait") {
      CHECK(SystemTrait<ConstOperatorResourceSystem>);
    }

    SUBCASE("Const operator() with all param types satisfies SystemTrait") {
      CHECK(SystemTrait<ConstOperatorAllParamsSystem>);
    }
  }

  TEST_CASE("ecs::SystemTrait::noexcept_operator") {
    SUBCASE("Noexcept operator() satisfies SystemTrait") {
      CHECK(SystemTrait<NoexceptSystem>);
    }

    SUBCASE("Const noexcept operator() satisfies SystemTrait") {
      CHECK(SystemTrait<ConstNoexceptSystem>);
    }

    SUBCASE("Noexcept free function satisfies SystemTrait") {
      CHECK(SystemTrait<decltype(&FreeFunctionWithQueryNoexcept)>);
    }
  }

  TEST_CASE("ecs::SystemTrait::negative") {
    SUBCASE("Polymorphic type does not satisfy SystemTrait") {
      CHECK_FALSE(SystemTrait<PolymorphicSystem>);
    }

    SUBCASE("Non-copyable and non-movable type does not satisfy SystemTrait") {
      CHECK_FALSE(SystemTrait<NonCopyableNonMovableSystem>);
    }

    SUBCASE("Type without operator() does not satisfy SystemTrait") {
      CHECK_FALSE(SystemTrait<EmptyCallSystem>);
    }

    SUBCASE("Type with invalid param types does not satisfy SystemTrait") {
      CHECK_FALSE(SystemTrait<InvalidParamSystem>);
    }

    SUBCASE(
        "Type with mixed valid and invalid params does not satisfy "
        "SystemTrait") {
      CHECK_FALSE(SystemTrait<MixedValidInvalidParamSystem>);
    }

    SUBCASE("Free function with invalid param does not satisfy SystemTrait") {
      CHECK_FALSE(SystemTrait<decltype(&AnotherFreeFunction)>);
    }
  }

  TEST_CASE("ecs::SystemTrait::free_functions") {
    SUBCASE("Free function with no params satisfies SystemTrait") {
      CHECK(SystemTrait<decltype(&FreeFunctionNoParams)>);
    }

    SUBCASE("Free function with Query param satisfies SystemTrait") {
      CHECK(SystemTrait<decltype(&FreeFunctionWithQuery)>);
    }

    SUBCASE("Free function with mixed ECS params satisfies SystemTrait") {
      CHECK(SystemTrait<decltype(&FreeFunctionWithMixed)>);
    }
  }

  TEST_CASE("ecs::SystemTrait::lambda") {
    SUBCASE("Lambda with Query param satisfies SystemTrait") {
      const auto lambda = [](Query<const Position&> /*query*/) {};
      CHECK(SystemTrait<decltype(lambda)>);
    }

    SUBCASE("Lambda with Resource param satisfies SystemTrait") {
      const auto lambda = [](Res<const Camera> /*camera*/) {};
      CHECK(SystemTrait<decltype(lambda)>);
    }

    SUBCASE("Lambda with mixed params satisfies SystemTrait") {
      const auto lambda = [](Query<const Position&> /*query*/,
                             Res<const Camera> /*camera*/,
                             Commands /*commands*/) {};
      CHECK(SystemTrait<decltype(lambda)>);
    }
  }
}

TEST_SUITE("helios::ecs::SystemWithNameTrait") {
  TEST_CASE("ecs::SystemWithNameTrait::concept") {
    SUBCASE("Systems with kName satisfy SystemWithNameTrait") {
      CHECK(SystemWithNameTrait<NamedSystem>);
      CHECK(SystemWithNameTrait<ComplexSystem>);
    }

    SUBCASE("Systems without kName do not satisfy SystemWithNameTrait") {
      CHECK_FALSE(SystemWithNameTrait<SimpleSystem>);
      CHECK_FALSE(SystemWithNameTrait<ReadOnlySystem>);
      CHECK_FALSE(SystemWithNameTrait<LargeSystem>);
    }
  }
}

TEST_SUITE("helios::ecs::SystemNameOf") {
  TEST_CASE("ecs::SystemNameOf::basic") {
    SUBCASE("System with custom name returns custom name") {
      constexpr auto name = SystemNameOf<NamedSystem>();
      CHECK_EQ(name, "NamedSystem");
    }

    SUBCASE("System with custom name (ComplexSystem)") {
      constexpr auto name = SystemNameOf<ComplexSystem>();
      CHECK_EQ(name, "ComplexSystem");
    }

    SUBCASE("System without custom name returns type name") {
      constexpr auto name = SystemNameOf<SimpleSystem>();
      CHECK_FALSE(name.empty());
    }

    SUBCASE("System name with instance parameter") {
      NamedSystem sys{};

      constexpr auto name_from_type = SystemNameOf<NamedSystem>();
      constexpr auto name_from_instance = SystemNameOf(sys);

      CHECK_EQ(name_from_type, name_from_instance);
    }

    SUBCASE("Read-only system without custom name returns type name") {
      constexpr auto name = SystemNameOf<ReadOnlySystem>();
      CHECK_FALSE(name.empty());
    }

    SUBCASE("Large system without custom name returns type name") {
      constexpr auto name = SystemNameOf<LargeSystem>();
      CHECK_FALSE(name.empty());
    }

    SUBCASE("Aligned system without custom name returns type name") {
      constexpr auto name = SystemNameOf<AlignedSystem>();
      CHECK_FALSE(name.empty());
    }
  }

  TEST_CASE("ecs::System: edge cases") {
    SUBCASE("System with all traits specified") {
      CHECK(SystemTrait<ComplexSystem>);
      CHECK(SystemWithNameTrait<ComplexSystem>);
      CHECK_EQ(SystemNameOf<ComplexSystem>(), "ComplexSystem");
    }

    SUBCASE("Large system satisfies SystemTrait") {
      CHECK(SystemTrait<LargeSystem>);
    }

    SUBCASE("Aligned system satisfies SystemTrait") {
      CHECK(SystemTrait<AlignedSystem>);
    }

    SUBCASE("Multiple systems have unique type indices") {
      const auto indices = {
          SystemTypeIndex::From<SimpleSystem>(),
          SystemTypeIndex::From<NamedSystem>(),
          SystemTypeIndex::From<ReadOnlySystem>(),
          SystemTypeIndex::From<ComplexSystem>(),
          SystemTypeIndex::From<LargeSystem>(),
          SystemTypeIndex::From<AlignedSystem>(),
      };

      std::vector<SystemTypeIndex> index_vec(indices);
      std::ranges::sort(index_vec);
      const auto last = std::ranges::unique(index_vec);

      CHECK_EQ(last.begin(), index_vec.end());
    }

    SUBCASE("Named system also satisfies base SystemTrait") {
      CHECK(SystemTrait<NamedSystem>);
      CHECK(SystemWithNameTrait<NamedSystem>);
    }

    SUBCASE("System name is constexpr") {
      constexpr auto name = SystemNameOf<NamedSystem>();
      CHECK_EQ(name, "NamedSystem");
    }

    SUBCASE("System type index is constexpr") {
      constexpr auto index = SystemTypeIndex::From<SimpleSystem>();
      CHECK_NE(index.Hash(), 0);
    }
  }
}

TEST_SUITE("helios::ecs::FreeFunctionSystem") {
  namespace ff = free_function_system_test;

  using FreeIncrementSystem =
      std::remove_cvref_t<decltype(kSystem<ff::IncrementCounter>)>;

  TEST_CASE("ecs::FreeFunctionSystem::concept") {
    SUBCASE("Wrapped free function satisfies functor system traits") {
      CHECK(SystemTrait<FreeIncrementSystem>);
      CHECK(FunctorSystemTrait<FreeIncrementSystem>);
      CHECK(SystemWithNameTrait<FreeIncrementSystem>);
    }

    SUBCASE("Raw free function pointer is not an unnamed functor system") {
      CHECK(SystemTrait<decltype(&ff::IncrementCounter)>);
      CHECK_FALSE(FunctorSystemTrait<decltype(&ff::IncrementCounter)>);
    }

    SUBCASE("Invalid free function params keep wrapper out of SystemTrait") {
      CHECK_FALSE(SystemTrait<decltype(kSystem<AnotherFreeFunction>)>);
    }
  }

  TEST_CASE("ecs::FreeFunctionSystem::kName") {
    SUBCASE("Name contains namespace-qualified free function") {
      constexpr std::string_view name = SystemNameOf<FreeIncrementSystem>();

      CHECK_EQ(name,
               "helios::ecs::free_function_system_test::"
               "IncrementCounter");
    }
  }

  TEST_CASE("ecs::FreeFunctionSystem::operator()") {
    SUBCASE("Wrapper directly invokes no-param free function") {
      ff::no_param_calls = 0;

      kSystem<ff::CountNoParamCall>();

      CHECK_EQ(ff::no_param_calls, 1);
    }
  }

  TEST_CASE("ecs::FreeFunctionSystem through Schedule::Add") {
    SUBCASE("Runs a wrapped free function system with resource params") {
      Schedule schedule;
      schedule.Add(kSystem<ff::IncrementCounter>);
      REQUIRE(schedule.Build().has_value());

      World world;
      world.InsertResources(ff::FreeCounter{});

      MainThreadExecutor executor;
      schedule.RunAndWait(world, executor);

      CHECK_EQ(world.ReadResource<ff::FreeCounter>().value, 1);
    }

    SUBCASE("Deduces query and resource access from wrapped free function") {
      Schedule schedule;
      schedule.Add(kSystem<ff::AccumulatePositions>);
      REQUIRE(schedule.Build().has_value());

      World world;
      world.InsertResources(ff::FreeCounter{});
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, ff::FreePosition{});

      MainThreadExecutor executor;
      schedule.RunAndWait(world, executor);

      CHECK_EQ(world.ReadResource<ff::FreeCounter>().value, 1);
    }

    SUBCASE("Supports explicit-name raw free function pointer registration") {
      Schedule schedule;
      schedule.Add("RawFreeFunctionSystem", &ff::IncrementCounter);
      REQUIRE(schedule.Build().has_value());

      World world;
      world.InsertResources(ff::FreeCounter{});

      MainThreadExecutor executor;
      schedule.RunAndWait(world, executor);

      CHECK_EQ(world.ReadResource<ff::FreeCounter>().value, 1);
    }
  }

  TEST_CASE("ecs::FreeFunctionSystem through grouped Schedule::Add") {
    SUBCASE("Runs multiple wrapped free function systems as a group") {
      Schedule schedule;
      schedule.Add(kSystem<ff::IncrementCounter>,
                   kSystem<ff::IncrementCounterNoexcept>);
      REQUIRE(schedule.Build().has_value());

      World world;
      world.InsertResources(ff::FreeCounter{});

      MainThreadExecutor executor;
      schedule.RunAndWait(world, executor);

      CHECK_EQ(world.ReadResource<ff::FreeCounter>().value, 2);
    }
  }

  TEST_CASE("ecs::FreeFunctionSystem through RunIf") {
    SUBCASE("Wrapped free function run condition can enable a system") {
      Schedule schedule;
      schedule.Add(kSystem<ff::IncrementCounter>)
          .RunIf(kSystem<ff::CounterIsZero>);
      REQUIRE(schedule.Build().has_value());

      World world;
      world.InsertResources(ff::FreeCounter{});

      MainThreadExecutor executor;
      schedule.RunAndWait(world, executor);

      CHECK_EQ(world.ReadResource<ff::FreeCounter>().value, 1);
    }

    SUBCASE("Wrapped free function set condition can skip a system") {
      Schedule schedule;
      schedule.Set<ff::FreeSet>().RunIf(kSystem<ff::CounterIsPositive>);
      schedule.Add(kSystem<ff::IncrementCounter>).InSet<ff::FreeSet>();
      REQUIRE(schedule.Build().has_value());

      World world;
      world.InsertResources(ff::FreeCounter{});

      MainThreadExecutor executor;
      schedule.RunAndWait(world, executor);

      CHECK_EQ(world.ReadResource<ff::FreeCounter>().value, 0);
    }
  }
}

TEST_SUITE("helios::ecs::SystemId") {
  TEST_CASE("ecs::SystemId::From(name)") {
    SUBCASE("Same name produces same id") {
      constexpr auto id1 = SystemId::From("MovementSystem");
      constexpr auto id2 = SystemId::From("MovementSystem");
      CHECK_EQ(id1, id2);
    }

    SUBCASE("Different names produce different ids") {
      constexpr auto id1 = SystemId::From("SystemA");
      constexpr auto id2 = SystemId::From("SystemB");
      CHECK_NE(id1, id2);
    }

    SUBCASE("Id is non-zero for non-empty name") {
      constexpr auto id = SystemId::From("TestSystem");
      CHECK_NE(id, SystemId{.id = 0});
    }

    SUBCASE("Empty name produces hash id") {
      constexpr auto id = SystemId::From("");
      CHECK_EQ(id.id, utils::Fnv1aHash(""));
    }

    SUBCASE("Id matches Fnv1aHash of name") {
      constexpr std::string_view name = "PhysicsSystem";
      constexpr auto id = SystemId::From(name);
      constexpr auto expected_hash = utils::Fnv1aHash(name);
      CHECK_EQ(id.id, expected_hash);
    }

    SUBCASE("Id is deterministic across calls") {
      constexpr std::string_view name = "DeterministicSystem";
      constexpr auto id1 = SystemId::From(name);
      constexpr auto id2 = SystemId::From(name);
      CHECK_EQ(id1, id2);
    }
  }

  TEST_CASE("ecs::SystemId::From(SystemTypeIndex)") {
    SUBCASE("From type index produces non-zero id") {
      constexpr auto index = SystemTypeIndex::From<SimpleSystem>();
      constexpr auto id = SystemId::From(index);
      CHECK_NE(id, SystemId{.id = 0});
    }

    SUBCASE("Same type index produces same id") {
      constexpr auto index = SystemTypeIndex::From<SimpleSystem>();
      constexpr auto id1 = SystemId::From(index);
      constexpr auto id2 = SystemId::From(index);
      CHECK_EQ(id1, id2);
    }
  }

  TEST_CASE("ecs::SystemId::From(SystemTypeId)") {
    SUBCASE("From type id produces same as from type index") {
      constexpr auto type_id = SystemTypeId::From<SimpleSystem>();
      constexpr auto id1 = SystemId::From(type_id);
      constexpr auto id2 =
          SystemId::From(SystemTypeIndex::From<SimpleSystem>());
      CHECK_EQ(id1, id2);
    }
  }

  TEST_CASE("ecs::SystemId::From<T>") {
    SUBCASE("Same type produces same id") {
      constexpr auto id1 = SystemId::From<SimpleSystem>();
      constexpr auto id2 = SystemId::From<SimpleSystem>();
      CHECK_EQ(id1, id2);
    }

    SUBCASE("Different types produce different ids") {
      constexpr auto id1 = SystemId::From<SimpleSystem>();
      constexpr auto id2 = SystemId::From<NamedSystem>();
      CHECK_NE(id1, id2);
    }

    SUBCASE("Id is non-zero for valid type") {
      constexpr auto id = SystemId::From<SimpleSystem>();
      CHECK_NE(id, SystemId{.id = 0});
    }

    SUBCASE("Id is deterministic across calls") {
      constexpr auto id1 = SystemId::From<SimpleSystem>();
      constexpr auto id2 = SystemId::From<SimpleSystem>();
      CHECK_EQ(id1, id2);
    }

    SUBCASE("From<T>(instance) produces same as From<T>()") {
      SimpleSystem sys{};
      constexpr auto id1 = SystemId::From<SimpleSystem>();
      constexpr auto id2 = SystemId::From(sys);
      CHECK_EQ(id1, id2);
    }

    SUBCASE("System with ECS params also produces valid id") {
      constexpr auto id = SystemId::From<QueryOnlySystem>();
      CHECK_NE(id, SystemId{.id = 0});

      constexpr auto id2 = SystemId::From<AllParamsSystem>();
      CHECK_NE(id, id2);
    }
  }

  TEST_CASE("ecs::SystemId::comparison") {
    SUBCASE("Default id is zero") {
      const SystemId default_id;
      CHECK_EQ(default_id.id, 0);
    }

    SUBCASE("Equal ids compare equal") {
      constexpr auto id1 = SystemId::From<SimpleSystem>();
      constexpr auto id2 = SystemId::From<SimpleSystem>();
      CHECK_EQ(id1, id2);
    }

    SUBCASE("Different ids compare different") {
      constexpr auto id1 = SystemId::From<SimpleSystem>();
      constexpr auto id2 = SystemId::From<NamedSystem>();
      CHECK_NE(id1, id2);
    }
  }

  TEST_CASE("ecs::SystemId::hash") {
    SUBCASE("Hash of default id is zero") {
      const SystemId id;
      const auto hash = std::hash<SystemId>{}(id);
      CHECK_EQ(hash, 0);
    }

    SUBCASE("Hash equals id") {
      constexpr auto id = SystemId::From("TestHash");
      const auto hash = std::hash<SystemId>{}(id);
      CHECK_EQ(hash, id.id);
    }
  }
}
