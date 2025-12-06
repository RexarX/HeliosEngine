#include <doctest/doctest.h>

#include <helios/core/ecs/component.hpp>

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

using namespace helios::ecs;

namespace {

// Test component types
struct Position {
  float x = 0.0F;
  float y = 0.0F;
  float z = 0.0F;
};

struct Velocity {
  float dx = 0.0F;
  float dy = 0.0F;
  float dz = 0.0F;
};

struct Name {
  std::string value;
};

struct TagComponent {};

struct LargeComponent {
  std::array<uint8_t, 512> data = {};
};

struct NonTrivialComponent {
  std::unique_ptr<int> ptr;

  NonTrivialComponent() : ptr(std::make_unique<int>(42)) {}
  NonTrivialComponent(const NonTrivialComponent& other) : ptr(std::make_unique<int>(*other.ptr)) {}
  NonTrivialComponent(NonTrivialComponent&&) = default;
  ~NonTrivialComponent() = default;

  NonTrivialComponent& operator=(const NonTrivialComponent&) = delete;
  NonTrivialComponent& operator=(NonTrivialComponent&&) = default;
};

// Invalid component types for testing
struct ConstComponent {
  const int value = 42;
};

class RefComponent {
public:
  int& ref;

  constexpr RefComponent(int& r) noexcept : ref(r) {}
};

}  // namespace

TEST_SUITE("ecs::Component") {
  TEST_CASE("ComponentTrait: valid component types") {
    // Valid component types
    CHECK(ComponentTrait<Position>);
    CHECK(ComponentTrait<Velocity>);
    CHECK(ComponentTrait<Name>);
    CHECK(ComponentTrait<TagComponent>);
    CHECK(ComponentTrait<LargeComponent>);
    CHECK(ComponentTrait<NonTrivialComponent>);
    CHECK(ComponentTrait<ConstComponent>);  // Contains const member
    CHECK(ComponentTrait<int>);
    CHECK(ComponentTrait<std::string>);
    CHECK(ComponentTrait<std::vector<int>>);
    CHECK(ComponentTrait<const Position>);
    CHECK(ComponentTrait<volatile Position>);
    CHECK(ComponentTrait<Position&>);
    CHECK(ComponentTrait<const Position&>);
    CHECK(ComponentTrait<Position*>);
    CHECK(ComponentTrait<const Position*>);

    CHECK_FALSE(ComponentTrait<void>);
  }

  TEST_CASE("TagComponentTrait: empty components") {
    // Tag components (empty)
    CHECK(TagComponentTrait<TagComponent>);

    // Not tag components
    CHECK_FALSE(TagComponentTrait<Position>);
    CHECK_FALSE(TagComponentTrait<Name>);
    CHECK_FALSE(TagComponentTrait<int>);
    CHECK_FALSE(TagComponentTrait<LargeComponent>);
  }

  TEST_CASE("TrivialComponentTrait: trivially copyable components") {
    // Trivial components
    CHECK(TrivialComponentTrait<Position>);
    CHECK(TrivialComponentTrait<Velocity>);
    CHECK(TrivialComponentTrait<TagComponent>);
    CHECK(TrivialComponentTrait<int>);
    CHECK(TrivialComponentTrait<float>);

    // Non-trivial components
    CHECK_FALSE(TrivialComponentTrait<Name>);
    CHECK_FALSE(TrivialComponentTrait<NonTrivialComponent>);
    CHECK_FALSE(TrivialComponentTrait<std::string>);
    CHECK_FALSE(TrivialComponentTrait<std::vector<int>>);
  }

  TEST_CASE("ComponentTraits: size-based traits") {
    // Tiny components (≤ 16 bytes)
    CHECK(TinyComponentTrait<TagComponent>);
    CHECK(TinyComponentTrait<int>);
    CHECK(TinyComponentTrait<float>);
    CHECK(TinyComponentTrait<Position>);  // 3 floats = 12 bytes

    // Small components (≤ 64 bytes)
    CHECK(SmallComponentTrait<TagComponent>);
    CHECK(SmallComponentTrait<Position>);
    CHECK(SmallComponentTrait<Velocity>);

    // Medium components (64 < size ≤ 256 bytes)
    CHECK_FALSE(MediumComponentTrait<Position>);
    CHECK_FALSE(MediumComponentTrait<TagComponent>);

    // Large components (> 256 bytes)
    CHECK(LargeComponentTrait<LargeComponent>);  // 512 bytes
    CHECK_FALSE(LargeComponentTrait<Position>);
    CHECK_FALSE(LargeComponentTrait<TagComponent>);
  }

  TEST_CASE("ComponentTraits: structure properties") {
    // Test Position traits
    constexpr auto pos_traits = ComponentTraits<Position>{};
    CHECK_EQ(pos_traits.kSize, sizeof(Position));
    CHECK_EQ(pos_traits.kAlignment, alignof(Position));
    CHECK(pos_traits.kIsTrivial);
    CHECK(pos_traits.kIsTiny);
    CHECK(pos_traits.kIsSmall);
    CHECK_FALSE(pos_traits.kIsMedium);
    CHECK_FALSE(pos_traits.kIsLarge);

    // Test TagComponent traits
    constexpr auto tag_traits = ComponentTraits<TagComponent>{};
    CHECK_EQ(tag_traits.kSize, sizeof(TagComponent));
    CHECK_EQ(tag_traits.kAlignment, alignof(TagComponent));
    CHECK(tag_traits.kIsTrivial);
    CHECK(tag_traits.kIsTiny);
    CHECK(tag_traits.kIsSmall);

    // Test LargeComponent traits
    constexpr auto large_traits = ComponentTraits<LargeComponent>{};
    CHECK_EQ(large_traits.kSize, sizeof(LargeComponent));
    CHECK_EQ(large_traits.kAlignment, alignof(LargeComponent));
    CHECK(large_traits.kIsTrivial);
    CHECK_FALSE(large_traits.kIsTiny);
    CHECK_FALSE(large_traits.kIsSmall);
    CHECK_FALSE(large_traits.kIsMedium);
    CHECK(large_traits.kIsLarge);

    // Test NonTrivialComponent traits
    constexpr auto nontrivial_traits = ComponentTraits<NonTrivialComponent>{};
    CHECK_EQ(nontrivial_traits.kSize, sizeof(NonTrivialComponent));
    CHECK_EQ(nontrivial_traits.kAlignment, alignof(NonTrivialComponent));
    CHECK_FALSE(nontrivial_traits.kIsTrivial);
  }

  TEST_CASE("ComponentTypeIdOf: returns unique type IDs") {
    // Type IDs should be consistent
    constexpr auto pos_id1 = ComponentTypeIdOf<Position>();
    constexpr auto pos_id2 = ComponentTypeIdOf<Position>();
    CHECK_EQ(pos_id1, pos_id2);

    // Different types should have different IDs
    constexpr auto vel_id = ComponentTypeIdOf<Velocity>();
    constexpr auto name_id = ComponentTypeIdOf<Name>();
    CHECK_NE(pos_id1, vel_id);
    CHECK_NE(pos_id1, name_id);
    CHECK_NE(vel_id, name_id);

    // IDs should be non-zero
    CHECK_NE(pos_id1, 0);
    CHECK_NE(vel_id, 0);
    CHECK_NE(name_id, 0);
  }

  TEST_CASE("ComponentTypeInfo::Create: creation") {
    constexpr auto pos_info = ComponentTypeInfo::Create<Position>();
    constexpr auto vel_info = ComponentTypeInfo::Create<Velocity>();
    constexpr auto name_info = ComponentTypeInfo::Create<Name>();

    // Check type IDs
    CHECK_EQ(pos_info.TypeId(), ComponentTypeIdOf<Position>());
    CHECK_EQ(vel_info.TypeId(), ComponentTypeIdOf<Velocity>());
    CHECK_EQ(name_info.TypeId(), ComponentTypeIdOf<Name>());

    // Check sizes
    CHECK_EQ(pos_info.Size(), sizeof(Position));
    CHECK_EQ(vel_info.Size(), sizeof(Velocity));
    CHECK_EQ(name_info.Size(), sizeof(Name));

    // Check alignments
    CHECK_EQ(pos_info.Alignment(), alignof(Position));
    CHECK_EQ(vel_info.Alignment(), alignof(Velocity));
    CHECK_EQ(name_info.Alignment(), alignof(Name));

    // Check trivial flags
    CHECK(pos_info.IsTrivial());
    CHECK(vel_info.IsTrivial());
    CHECK_FALSE(name_info.IsTrivial());
  }

  TEST_CASE("ComponentTypeInfo::operator==: equality") {
    constexpr auto pos_info1 = ComponentTypeInfo::Create<Position>();
    constexpr auto pos_info2 = ComponentTypeInfo::Create<Position>();
    constexpr auto vel_info = ComponentTypeInfo::Create<Velocity>();

    // Same types should be equal
    CHECK_EQ(pos_info1, pos_info2);
    CHECK_FALSE(pos_info1 != pos_info2);

    // Different types should not be equal
    CHECK_FALSE(pos_info1 == vel_info);
    CHECK_NE(pos_info1, vel_info);
  }

  TEST_CASE("ComponentTypeInfo::operator<: comparison") {
    auto pos_info = ComponentTypeInfo::Create<Position>();
    auto vel_info = ComponentTypeInfo::Create<Velocity>();

    // Comparison should be based on type ID
    if (pos_info.TypeId() < vel_info.TypeId()) {
      CHECK_LT(pos_info, vel_info);
      CHECK_FALSE(vel_info < pos_info);
    } else {
      CHECK_LT(vel_info, pos_info);
      CHECK_FALSE(pos_info < vel_info);
    }

    // Same type should not be less than itself
    auto pos_info2 = ComponentTypeInfo::Create<Position>();
    CHECK_FALSE(pos_info < pos_info2);
    CHECK_FALSE(pos_info2 < pos_info);
  }

  TEST_CASE("ComponentTypeInfo: copy semantics") {
    constexpr auto original = ComponentTypeInfo::Create<Position>();

    // Copy constructor
    auto copy = original;
    CHECK_EQ(copy, original);
    CHECK_EQ(copy.TypeId(), original.TypeId());
    CHECK_EQ(copy.Size(), original.Size());
    CHECK_EQ(copy.Alignment(), original.Alignment());
    CHECK_EQ(copy.IsTrivial(), original.IsTrivial());

    // Copy assignment
    auto assigned = ComponentTypeInfo::Create<Velocity>();
    assigned = original;
    CHECK_EQ(assigned, original);
    CHECK_EQ(assigned.TypeId(), original.TypeId());
  }

  TEST_CASE("ComponentTypeInfo: move semantics") {
    auto original = ComponentTypeInfo::Create<Position>();
    auto original_type_id = original.TypeId();
    auto original_size = original.Size();

    // Move constructor
    auto moved = std::move(original);
    CHECK_EQ(moved.TypeId(), original_type_id);
    CHECK_EQ(moved.Size(), original_size);

    // Move assignment
    auto assigned = ComponentTypeInfo::Create<Velocity>();
    auto source = ComponentTypeInfo::Create<Name>();
    auto source_type_id = source.TypeId();

    assigned = std::move(source);
    CHECK_EQ(assigned.TypeId(), source_type_id);
  }

  TEST_CASE("std::hash<ComponentTypeInfo>: hashing") {
    auto pos_info = ComponentTypeInfo::Create<Position>();
    auto vel_info = ComponentTypeInfo::Create<Velocity>();

    std::hash<ComponentTypeInfo> hasher;

    // Hash should be based on type ID
    CHECK_EQ(hasher(pos_info), pos_info.TypeId());
    CHECK_EQ(hasher(vel_info), vel_info.TypeId());

    // Same types should have same hash
    auto pos_info2 = ComponentTypeInfo::Create<Position>();
    CHECK_EQ(hasher(pos_info), hasher(pos_info2));

    // Different types should have different hashes
    CHECK_NE(hasher(pos_info), hasher(vel_info));
  }

  TEST_CASE("ComponentTypeInfo: use in containers") {
    std::unordered_set<ComponentTypeInfo> info_set;
    std::unordered_map<ComponentTypeInfo, std::string> info_map;

    auto pos_info = ComponentTypeInfo::Create<Position>();
    auto vel_info = ComponentTypeInfo::Create<Velocity>();
    auto name_info = ComponentTypeInfo::Create<Name>();

    // Test unordered_set
    info_set.insert(pos_info);
    info_set.insert(vel_info);
    info_set.insert(name_info);
    info_set.insert(pos_info);  // Duplicate

    CHECK_EQ(info_set.size(), 3);
    CHECK(info_set.contains(pos_info));
    CHECK(info_set.contains(vel_info));
    CHECK(info_set.contains(name_info));

    // Test unordered_map
    info_map[pos_info] = "Position";
    info_map[vel_info] = "Velocity";
    info_map[name_info] = "Name";

    CHECK_EQ(info_map.size(), 3);
    CHECK_EQ(info_map[pos_info], "Position");
    CHECK_EQ(info_map[vel_info], "Velocity");
    CHECK_EQ(info_map[name_info], "Name");
  }

  TEST_CASE("ComponentTypeInfo: constexpr operations") {
    // Test that ComponentTypeInfo operations can be used in constexpr context
    constexpr auto pos_info = ComponentTypeInfo::Create<Position>();
    constexpr auto type_id = pos_info.TypeId();
    constexpr auto size = pos_info.Size();
    constexpr auto alignment = pos_info.Alignment();
    constexpr auto is_trivial = pos_info.IsTrivial();

    CHECK_NE(type_id, 0);
    CHECK_EQ(size, sizeof(Position));
    CHECK_EQ(alignment, alignof(Position));
    CHECK(is_trivial);

    // Test equality in constexpr context
    constexpr auto pos_info2 = ComponentTypeInfo::Create<Position>();
    constexpr bool are_equal = pos_info == pos_info2;
    CHECK(are_equal);
  }

  TEST_CASE("ComponentTrait: edge cases and special types") {
    // Test with fundamental types
    CHECK(ComponentTrait<char>);
    CHECK(ComponentTrait<bool>);
    CHECK(ComponentTrait<double>);
    CHECK(TinyComponentTrait<char>);
    CHECK(TinyComponentTrait<bool>);
    CHECK(TinyComponentTrait<double>);

    // Test with arrays
    CHECK(ComponentTrait<int[10]>);
    CHECK_FALSE(TinyComponentTrait<int[100]>);  // 400 bytes

    // Test with enums
    enum class TestEnum { A, B, C };
    CHECK(ComponentTrait<TestEnum>);
    CHECK(TrivialComponentTrait<TestEnum>);
    CHECK(TinyComponentTrait<TestEnum>);

    // Test type IDs are consistent across calls
    auto id1 = ComponentTypeIdOf<TestEnum>();
    auto id2 = ComponentTypeIdOf<TestEnum>();
    CHECK_EQ(id1, id2);
  }

}  // TEST_SUITE
