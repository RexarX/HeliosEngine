#include <doctest/doctest.h>

#include <helios/core/ecs/resource.hpp>

#include <atomic>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using namespace helios::ecs;

namespace {

// Test resource types
struct SimpleResource {
  int value = 0;
};

struct ResourceWithName {
  int data = 0;

  static constexpr std::string_view GetName() noexcept { return "CustomResourceName"; }
};

struct ResourceWithThreadSafety {
  int data = 0;

  static constexpr bool ThreadSafe() noexcept { return true; }
};

struct ResourceWithBoth {
  int data = 0;

  static constexpr std::string_view GetName() noexcept { return "ThreadSafeResource"; }
  static constexpr bool ThreadSafe() noexcept { return false; }
};

struct ComplexResource {
  std::string message;
  std::vector<int> data;
  int counter = 0;
};

struct EmptyResource {};

struct GameState {
  int level = 1;
  float score = 0.0F;
  bool paused = false;

  static constexpr std::string_view GetName() noexcept { return "GameState"; }
};

// Atomic resource types
struct AtomicCounter {
  std::atomic<int> value{0};

  AtomicCounter() = default;
  AtomicCounter(int v) : value(v) {}
};

// Invalid resource types
struct NonMovable {
  int value = 0;

  NonMovable() = default;
  NonMovable(const NonMovable&) = delete;
  NonMovable(NonMovable&&) = delete;
  NonMovable& operator=(const NonMovable&) = delete;
  NonMovable& operator=(NonMovable&&) = delete;
};

struct NonDestructible {
  int value = 0;
  ~NonDestructible() = delete;
};

// Template resource for testing
template <typename T>
struct TemplateResource {
  T value{};
};

}  // namespace

TEST_SUITE("ecs::Resource") {
  TEST_CASE("ResourceTrait: Valid Resource Types") {
    SUBCASE("Simple POD resource") {
      CHECK(ResourceTrait<SimpleResource>);
    }

    SUBCASE("Resource with name trait") {
      CHECK(ResourceTrait<ResourceWithName>);
    }

    SUBCASE("Resource with thread-safety trait") {
      CHECK(ResourceTrait<ResourceWithThreadSafety>);
    }

    SUBCASE("Resource with both traits") {
      CHECK(ResourceTrait<ResourceWithBoth>);
    }

    SUBCASE("Complex resource with STL types") {
      CHECK(ResourceTrait<ComplexResource>);
    }

    SUBCASE("Empty resource") {
      CHECK(ResourceTrait<EmptyResource>);
    }

    SUBCASE("Game state resource") {
      CHECK(ResourceTrait<GameState>);
    }

    SUBCASE("Fundamental types") {
      CHECK(ResourceTrait<int>);
      CHECK(ResourceTrait<float>);
      CHECK(ResourceTrait<double>);
      CHECK(ResourceTrait<bool>);
    }

    SUBCASE("STL types") {
      CHECK(ResourceTrait<std::string>);
      CHECK(ResourceTrait<std::vector<int>>);
      CHECK(ResourceTrait<std::unordered_map<int, std::string>>);
    }
  }

  TEST_CASE("ResourceTrait: Invalid Resource Types") {
    SUBCASE("Void type") {
      CHECK_FALSE(ResourceTrait<void>);
    }

    SUBCASE("Function types") {
      CHECK_FALSE(ResourceTrait<void()>);
      CHECK_FALSE(ResourceTrait<int(int)>);
    }
  }

  TEST_CASE("ResourceTrait: CV-qualified and Reference Types") {
    SUBCASE("Const types are valid (cvref removed)") {
      CHECK(ResourceTrait<const SimpleResource>);
      CHECK(ResourceTrait<const int>);
    }

    SUBCASE("Volatile types are valid (cvref removed)") {
      CHECK(ResourceTrait<volatile SimpleResource>);
    }

    SUBCASE("Const volatile types are valid (cvref removed)") {
      CHECK(ResourceTrait<const volatile SimpleResource>);
    }

    SUBCASE("Reference types are valid (cvref removed)") {
      CHECK(ResourceTrait<int&>);
      CHECK(ResourceTrait<const int&>);
      CHECK(ResourceTrait<SimpleResource&>);
      CHECK(ResourceTrait<const SimpleResource&>);
    }

    SUBCASE("Pointer types are valid") {
      CHECK(ResourceTrait<int*>);
      CHECK(ResourceTrait<const int*>);
      CHECK(ResourceTrait<SimpleResource*>);
    }
  }

  TEST_CASE("ResourceWithNameTrait: Valid Types") {
    SUBCASE("Resource with GetName method") {
      CHECK(ResourceWithNameTrait<ResourceWithName>);
    }

    SUBCASE("Resource with both name and thread-safety") {
      CHECK(ResourceWithNameTrait<ResourceWithBoth>);
    }

    SUBCASE("Game state resource") {
      CHECK(ResourceWithNameTrait<GameState>);
    }
  }

  TEST_CASE("ResourceWithNameTrait: Invalid Types") {
    SUBCASE("Resource without GetName method") {
      CHECK_FALSE(ResourceWithNameTrait<SimpleResource>);
      CHECK_FALSE(ResourceWithNameTrait<ComplexResource>);
      CHECK_FALSE(ResourceWithNameTrait<ResourceWithThreadSafety>);
    }

    SUBCASE("Fundamental types") {
      CHECK_FALSE(ResourceWithNameTrait<int>);
      CHECK_FALSE(ResourceWithNameTrait<std::string>);
    }

    SUBCASE("Non-resource types") {
      CHECK_FALSE(ResourceWithNameTrait<void>);
    }
  }

  TEST_CASE("ResourceWithThreadSafetyTrait: Valid Types") {
    SUBCASE("Resource with ThreadSafe method") {
      CHECK(ResourceWithThreadSafetyTrait<ResourceWithThreadSafety>);
    }

    SUBCASE("Resource with both traits") {
      CHECK(ResourceWithThreadSafetyTrait<ResourceWithBoth>);
    }
  }

  TEST_CASE("ResourceWithThreadSafetyTrait: Invalid Types") {
    SUBCASE("Resource without ThreadSafe method") {
      CHECK_FALSE(ResourceWithThreadSafetyTrait<SimpleResource>);
      CHECK_FALSE(ResourceWithThreadSafetyTrait<ComplexResource>);
      CHECK_FALSE(ResourceWithThreadSafetyTrait<ResourceWithName>);
    }

    SUBCASE("Fundamental types") {
      CHECK_FALSE(ResourceWithThreadSafetyTrait<int>);
      CHECK_FALSE(ResourceWithThreadSafetyTrait<bool>);
    }
  }

  TEST_CASE("AtomicResourceTrait: Valid Types") {
    SUBCASE("Fundamental atomic types") {
      CHECK(AtomicResourceTrait<int>);
      CHECK(AtomicResourceTrait<bool>);
      CHECK(AtomicResourceTrait<float>);
      CHECK(AtomicResourceTrait<double>);
    }

    SUBCASE("Pointer types") {
      CHECK(AtomicResourceTrait<int*>);
      CHECK(AtomicResourceTrait<void*>);
    }
  }

  TEST_CASE("AtomicResourceTrait: Invalid Types") {
    SUBCASE("Non-atomic types") {
      // These types cannot be used with std::atomic, so they don't satisfy AtomicResourceTrait
      // We don't test them directly as it would cause compilation errors
      // The concept itself will prevent usage at compile time
    }
  }

  TEST_CASE("ResourceTypeIdOf: Unique Type IDs") {
    SUBCASE("Different resources have different type IDs") {
      constexpr ResourceTypeId simple_id = ResourceTypeIdOf<SimpleResource>();
      constexpr ResourceTypeId named_id = ResourceTypeIdOf<ResourceWithName>();
      constexpr ResourceTypeId complex_id = ResourceTypeIdOf<ComplexResource>();
      constexpr ResourceTypeId empty_id = ResourceTypeIdOf<EmptyResource>();
      constexpr ResourceTypeId game_state_id = ResourceTypeIdOf<GameState>();

      CHECK_NE(simple_id, named_id);
      CHECK_NE(simple_id, complex_id);
      CHECK_NE(simple_id, empty_id);
      CHECK_NE(simple_id, game_state_id);
      CHECK_NE(named_id, complex_id);
      CHECK_NE(named_id, empty_id);
      CHECK_NE(named_id, game_state_id);
      CHECK_NE(complex_id, empty_id);
      CHECK_NE(complex_id, game_state_id);
      CHECK_NE(empty_id, game_state_id);
    }

    SUBCASE("Same resource type has same type ID") {
      constexpr ResourceTypeId id1 = ResourceTypeIdOf<SimpleResource>();
      constexpr ResourceTypeId id2 = ResourceTypeIdOf<SimpleResource>();

      CHECK_EQ(id1, id2);
    }

    SUBCASE("Type ID is compile-time constant") {
      constexpr ResourceTypeId id = ResourceTypeIdOf<SimpleResource>();
      static_assert(id == ResourceTypeIdOf<SimpleResource>(), "Type ID should be constexpr");
    }

    SUBCASE("Type IDs are non-zero") {
      constexpr ResourceTypeId simple_id = ResourceTypeIdOf<SimpleResource>();
      constexpr ResourceTypeId complex_id = ResourceTypeIdOf<ComplexResource>();

      CHECK_NE(simple_id, 0);
      CHECK_NE(complex_id, 0);
    }
  }

  TEST_CASE("ResourceNameOf: Resource Name Resolution") {
    SUBCASE("Resource with custom name") {
      constexpr std::string_view name = ResourceNameOf<ResourceWithName>();
      CHECK_EQ(name, "CustomResourceName");
    }

    SUBCASE("Game state resource with custom name") {
      constexpr std::string_view name = ResourceNameOf<GameState>();
      CHECK_EQ(name, "GameState");
    }

    SUBCASE("Resource with both traits uses custom name") {
      constexpr std::string_view name = ResourceNameOf<ResourceWithBoth>();
      CHECK_EQ(name, "ThreadSafeResource");
    }

    SUBCASE("Resource without custom name uses type name") {
      constexpr std::string_view name = ResourceNameOf<SimpleResource>();
      // Should contain type information (exact format depends on CTTI)
      CHECK_FALSE(name.empty());
    }

    SUBCASE("Different resources have different names") {
      constexpr std::string_view name1 = ResourceNameOf<SimpleResource>();
      constexpr std::string_view name2 = ResourceNameOf<ComplexResource>();

      CHECK_NE(name1, name2);
    }

    SUBCASE("Name is compile-time constant") {
      constexpr std::string_view name = ResourceNameOf<ResourceWithName>();
      static_assert(!name.empty(), "Name should be constexpr");
    }
  }

  TEST_CASE("ResourceNameOf: Name Consistency") {
    SUBCASE("Multiple calls return same name") {
      constexpr std::string_view name1 = ResourceNameOf<ResourceWithName>();
      constexpr std::string_view name2 = ResourceNameOf<ResourceWithName>();

      CHECK_EQ(name1, name2);
    }

    SUBCASE("Custom name is preferred over type name") {
      constexpr std::string_view name = ResourceNameOf<GameState>();

      // Should use custom name, not type-generated name
      CHECK_EQ(name, "GameState");
    }

    SUBCASE("Empty resource has valid name") {
      constexpr std::string_view name = ResourceNameOf<EmptyResource>();
      CHECK_FALSE(name.empty());
    }
  }

  TEST_CASE("Resource: Type Properties") {
    SUBCASE("SimpleResource properties") {
      CHECK(std::is_move_constructible_v<SimpleResource>);
      CHECK(std::is_move_assignable_v<SimpleResource>);
      CHECK(std::is_copy_constructible_v<SimpleResource>);
      CHECK(std::is_copy_assignable_v<SimpleResource>);
      CHECK(std::is_destructible_v<SimpleResource>);
    }

    SUBCASE("ComplexResource properties") {
      CHECK(std::is_move_constructible_v<ComplexResource>);
      CHECK(std::is_move_assignable_v<ComplexResource>);
      CHECK(std::is_destructible_v<ComplexResource>);
    }

    SUBCASE("EmptyResource properties") {
      CHECK(std::is_move_constructible_v<EmptyResource>);
      CHECK(std::is_move_assignable_v<EmptyResource>);
      CHECK(std::is_empty_v<EmptyResource>);
    }

    SUBCASE("GameState properties") {
      CHECK(std::is_move_constructible_v<GameState>);
      CHECK(std::is_move_assignable_v<GameState>);
      CHECK(std::is_destructible_v<GameState>);
    }
  }

  TEST_CASE("Resource: Practical Usage") {
    SUBCASE("Create and move simple resource") {
      SimpleResource resource1{42};
      SimpleResource resource2 = std::move(resource1);

      CHECK_EQ(resource2.value, 42);
    }

    SUBCASE("Create complex resource") {
      ComplexResource resource{"Test message", {1, 2, 3, 4, 5}, 100};

      CHECK_EQ(resource.message, "Test message");
      CHECK_EQ(resource.data.size(), 5);
      CHECK_EQ(resource.counter, 100);
    }

    SUBCASE("Move complex resource") {
      ComplexResource resource1{"Original", {10, 20}, 50};
      ComplexResource resource2 = std::move(resource1);

      CHECK_EQ(resource2.message, "Original");
      CHECK_EQ(resource2.data.size(), 2);
      CHECK_EQ(resource2.counter, 50);
    }

    SUBCASE("Empty resource is valid") {
      EmptyResource resource1;
      EmptyResource resource2 = std::move(resource1);

      // No data to check, but should compile and work
      static_cast<void>(resource2);
    }

    SUBCASE("Game state resource usage") {
      GameState state{5, 1234.5F, true};

      CHECK_EQ(state.level, 5);
      CHECK_EQ(state.score, 1234.5F);
      CHECK(state.paused);
      CHECK_EQ(GameState::GetName(), "GameState");
    }
  }

  TEST_CASE("Resource: Type ID Stability") {
    SUBCASE("Type ID remains constant across multiple queries") {
      std::vector<ResourceTypeId> ids;

      for (int i = 0; i < 10; ++i) {
        ids.push_back(ResourceTypeIdOf<SimpleResource>());
      }

      for (size_t i = 1; i < ids.size(); ++i) {
        CHECK_EQ(ids[i], ids[0]);
      }
    }

    SUBCASE("Type ID is usable as map key") {
      std::unordered_map<ResourceTypeId, std::string> resource_names;

      resource_names[ResourceTypeIdOf<SimpleResource>()] = "Simple";
      resource_names[ResourceTypeIdOf<ComplexResource>()] = "Complex";
      resource_names[ResourceTypeIdOf<GameState>()] = "GameState";

      CHECK_EQ(resource_names.size(), 3);
      CHECK_EQ(resource_names[ResourceTypeIdOf<SimpleResource>()], "Simple");
      CHECK_EQ(resource_names[ResourceTypeIdOf<ComplexResource>()], "Complex");
      CHECK_EQ(resource_names[ResourceTypeIdOf<GameState>()], "GameState");
    }
  }

  TEST_CASE("Resource: Custom Traits") {
    SUBCASE("ThreadSafe trait returns correct value") {
      CHECK(ResourceWithThreadSafety::ThreadSafe());
      CHECK_FALSE(ResourceWithBoth::ThreadSafe());
    }

    SUBCASE("GetName returns correct value") {
      CHECK_EQ(ResourceWithName::GetName(), "CustomResourceName");
      CHECK_EQ(GameState::GetName(), "GameState");
      CHECK_EQ(ResourceWithBoth::GetName(), "ThreadSafeResource");
    }
  }

  TEST_CASE("Resource: Edge Cases") {
    SUBCASE("Large resource type") {
      struct LargeResource {
        std::array<uint8_t, 1024> data = {};
      };

      CHECK(ResourceTrait<LargeResource>);
      constexpr ResourceTypeId id = ResourceTypeIdOf<LargeResource>();
      CHECK_NE(id, 0);
    }

    SUBCASE("Nested resource types") {
      struct Inner {
        int value = 0;
      };

      struct Outer {
        Inner inner;
        std::vector<Inner> inners;
      };

      CHECK(ResourceTrait<Inner>);
      CHECK(ResourceTrait<Outer>);
      CHECK_NE(ResourceTypeIdOf<Inner>(), ResourceTypeIdOf<Outer>());
    }

    SUBCASE("Template resource types") {
      CHECK(ResourceTrait<TemplateResource<int>>);
      CHECK(ResourceTrait<TemplateResource<float>>);
      CHECK_NE(ResourceTypeIdOf<TemplateResource<int>>(), ResourceTypeIdOf<TemplateResource<float>>());
    }

    SUBCASE("Array types") {
      CHECK(ResourceTrait<int[10]>);
      CHECK(ResourceTrait<std::array<int, 10>>);

      constexpr auto id1 = ResourceTypeIdOf<int[10]>();
      constexpr auto id2 = ResourceTypeIdOf<std::array<int, 10>>();
      CHECK_NE(id1, id2);
    }
  }

  TEST_CASE("Resource: Constexpr Usage") {
    SUBCASE("Type IDs are compile-time constants") {
      constexpr ResourceTypeId id1 = ResourceTypeIdOf<SimpleResource>();
      constexpr ResourceTypeId id2 = ResourceTypeIdOf<ComplexResource>();

      static_assert(id1 != id2, "Different resources should have different IDs");
    }

    SUBCASE("Names are compile-time constants") {
      constexpr std::string_view name1 = ResourceNameOf<ResourceWithName>();
      constexpr std::string_view name2 = ResourceNameOf<GameState>();

      static_assert(name1 == "CustomResourceName", "Custom name should be compile-time");
      static_assert(name2 == "GameState", "Custom name should be compile-time");
    }

    SUBCASE("Concept checks are compile-time") {
      static_assert(ResourceTrait<SimpleResource>, "Should be valid resource");
      static_assert(ResourceWithNameTrait<ResourceWithName>, "Should have name trait");
      static_assert(ResourceWithThreadSafetyTrait<ResourceWithThreadSafety>, "Should have thread-safety trait");
    }
  }

}  // TEST_SUITE
