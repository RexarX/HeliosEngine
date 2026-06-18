#include <doctest/doctest.h>

#include <helios/ecs/query/details/traits.hpp>

#include <concepts>

using namespace helios::ecs;
using namespace helios::ecs::details;

namespace {

struct Position {
  float x = 0.0F;
  float y = 0.0F;
};

struct Velocity {
  float dx = 0.0F;
  float dy = 0.0F;
};

struct Health {
  int value = 100;
};

struct TagA {};
struct TagB {};

struct MoveOnly {
  MoveOnly() = default;
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly(MoveOnly&&) noexcept = default;
  ~MoveOnly() = default;

  MoveOnly& operator=(const MoveOnly&) = delete;
  MoveOnly& operator=(MoveOnly&&) noexcept = default;
};

}  // namespace

TEST_SUITE("helios::ecs::details::ComponentTypeExtractor") {
  TEST_CASE(
      "ecs::details::ComponentTypeExtractor::type — plain and ref specifiers") {
    SUBCASE("Plain T extracts T") {
      CHECK(std::same_as<ComponentTypeExtractor<Position>::type, Position>);
    }

    SUBCASE("T& extracts T") {
      CHECK(std::same_as<ComponentTypeExtractor<Position&>::type, Position>);
    }

    SUBCASE("const T& extracts T") {
      CHECK(std::same_as<ComponentTypeExtractor<const Position&>::type,
                         Position>);
    }

    SUBCASE("T&& extracts T") {
      CHECK(std::same_as<ComponentTypeExtractor<Position&&>::type, Position>);
    }

    SUBCASE("T* extracts T") {
      CHECK(std::same_as<ComponentTypeExtractor<Position*>::type, Position>);
    }

    SUBCASE("const T* extracts T") {
      CHECK(std::same_as<ComponentTypeExtractor<const Position*>::type,
                         Position>);
    }
  }
}

TEST_SUITE("helios::ecs::details::IsConstAccess") {
  TEST_CASE("ecs::details::IsConstAccess — const access specifiers") {
    SUBCASE("const T& is const access") {
      CHECK(IsConstAccess<const Position&>::value);
    }

    SUBCASE("const T (no ref) is const access") {
      CHECK(IsConstAccess<const Position>::value);
    }

    SUBCASE("Plain T (value copy) is const access — non-mutating") {
      CHECK(IsConstAccess<Position>::value);
    }

    SUBCASE("const T* is const access") {
      CHECK(IsConstAccess<const Position*>::value);
    }
  }

  TEST_CASE("ecs::details::IsConstAccess — mutable access specifiers") {
    SUBCASE("T& is NOT const access") {
      CHECK(!IsConstAccess<Position&>::value);
    }

    SUBCASE("T&& is NOT const access") {
      CHECK(!IsConstAccess<Position&&>::value);
    }

    SUBCASE("T* is NOT const access") {
      CHECK(!IsConstAccess<Position*>::value);
    }
  }

  TEST_CASE("ecs::details::kIsConstAccess variable template") {
    SUBCASE("kIsConstAccess true for const T&") {
      CHECK(kIsConstAccess<const Position&>);
    }

    SUBCASE("kIsConstAccess false for T&") {
      CHECK(!kIsConstAccess<Position&>);
    }
  }
}

TEST_SUITE("helios::ecs::details::kIsMutableAccess") {
  TEST_CASE("ecs::details::kIsMutableAccess — mutable specifiers") {
    SUBCASE("T& is mutable") {
      CHECK(kIsMutableAccess<Position&>);
    }

    SUBCASE("T&& is mutable") {
      CHECK(kIsMutableAccess<Position&&>);
    }

    SUBCASE("T* is mutable") {
      CHECK(kIsMutableAccess<Position*>);
    }
  }

  TEST_CASE("ecs::details::kIsMutableAccess — non-mutable specifiers") {
    SUBCASE("const T& is not mutable") {
      CHECK(!kIsMutableAccess<const Position&>);
    }

    SUBCASE("Plain T (value) is not mutable") {
      CHECK(!kIsMutableAccess<Position>);
    }

    SUBCASE("const T* is not mutable") {
      CHECK(!kIsMutableAccess<const Position*>);
    }
  }
}

TEST_SUITE("helios::ecs::details::IsRvalueAccess") {
  TEST_CASE("ecs::details::IsRvalueAccess::value") {
    SUBCASE("T&& is rvalue access") {
      CHECK(IsRvalueAccess<Position&&>::value);
    }

    SUBCASE("T& is NOT rvalue access") {
      CHECK(!IsRvalueAccess<Position&>::value);
    }

    SUBCASE("const T& is NOT rvalue access") {
      CHECK(!IsRvalueAccess<const Position&>::value);
    }

    SUBCASE("Plain T is NOT rvalue access") {
      CHECK(!IsRvalueAccess<Position>::value);
    }

    SUBCASE("T* is NOT rvalue access") {
      CHECK(!IsRvalueAccess<Position*>::value);
    }

    SUBCASE("const T* is NOT rvalue access") {
      CHECK(!IsRvalueAccess<const Position*>::value);
    }
  }

  TEST_CASE("ecs::details::kIsRvalueAccess variable template") {
    SUBCASE("kIsRvalueAccess true for T&&") {
      CHECK(kIsRvalueAccess<Position&&>);
    }

    SUBCASE("kIsRvalueAccess false for T&") {
      CHECK(!kIsRvalueAccess<Position&>);
    }
  }
}

TEST_SUITE("helios::ecs::details::kAllComponentsConst") {
  TEST_CASE("ecs::details::kAllComponentsConst — all const") {
    SUBCASE("Single const T& is all-const") {
      CHECK(kAllComponentsConst<const Position&>);
    }

    SUBCASE("Multiple const refs are all-const") {
      CHECK(
          kAllComponentsConst<const Position&, const Velocity&, const Health&>);
    }

    SUBCASE("Mix of const T& and plain T (copy) is all-const") {
      CHECK(kAllComponentsConst<const Position&, Health>);
    }

    SUBCASE("const T* counts as const") {
      CHECK(kAllComponentsConst<const Position&, const Velocity*>);
    }
  }

  TEST_CASE("ecs::details::kAllComponentsConst — not all const") {
    SUBCASE("Single T& fails all-const") {
      CHECK(!kAllComponentsConst<Position&>);
    }

    SUBCASE("Mix of const T& and T& fails all-const") {
      CHECK(!kAllComponentsConst<const Position&, Velocity&>);
    }

    SUBCASE("T* fails all-const") {
      CHECK(!kAllComponentsConst<const Position&, Velocity*>);
    }
  }
}

TEST_SUITE("helios::ecs::details::ComponentAccessType") {
  TEST_CASE("ecs::details::ComponentAccessType::type — reference specifiers") {
    SUBCASE("T& yields T&") {
      CHECK(std::same_as<ComponentAccessType<Position&>::type, Position&>);
    }

    SUBCASE("const T& yields const T&") {
      CHECK(std::same_as<ComponentAccessType<const Position&>::type,
                         const Position&>);
    }

    SUBCASE("T&& yields T&&") {
      CHECK(std::same_as<ComponentAccessType<Position&&>::type, Position&&>);
    }
  }

  TEST_CASE("ecs::details::ComponentAccessType::type — value specifiers") {
    SUBCASE("T (plain value) yields T (value copy, cv stripped)") {
      CHECK(std::same_as<ComponentAccessType<Position>::type, Position>);
    }

    SUBCASE("const T yields T (cv stripped for value copy)") {
      CHECK(std::same_as<ComponentAccessType<const Position>::type, Position>);
    }
  }

  TEST_CASE("ecs::details::ComponentAccessType::type — pointer specifiers") {
    SUBCASE("T* yields T* (mutable pointer)") {
      CHECK(std::same_as<ComponentAccessType<Position*>::type, Position*>);
    }

    SUBCASE("const T* yields const T*") {
      CHECK(std::same_as<ComponentAccessType<const Position*>::type,
                         const Position*>);
    }
  }

  TEST_CASE("ecs::details::ComponentAccessType_t alias") {
    SUBCASE("ComponentAccessType_t<T&> is T&") {
      CHECK(std::same_as<ComponentAccessType_t<Position&>, Position&>);
    }

    SUBCASE("ComponentAccessType_t<const T*> is const T*") {
      CHECK(std::same_as<ComponentAccessType_t<const Position*>,
                         const Position*>);
    }
  }
}

TEST_SUITE("helios::ecs::details::kIsConstWorld") {
  TEST_CASE("ecs::details::kIsConstWorld") {
    SUBCASE("Non-const World is not const world") {
      CHECK(!kIsConstWorld<World>);
      CHECK(!kIsConstWorld<World&>);
    }

    SUBCASE("const World is const world") {
      CHECK(kIsConstWorld<const World>);
      CHECK(kIsConstWorld<const World&>);
    }
  }

  TEST_CASE("ecs::details::ValidWorldComponentAccess concept") {
    SUBCASE("Non-const world allows mutable access") {
      CHECK(ValidWorldComponentAccess<World, Position&, const Velocity&>);
    }

    SUBCASE("Non-const world allows all-const access") {
      CHECK(ValidWorldComponentAccess<World, const Position&, const Velocity&>);
    }

    SUBCASE("Const world allows all-const access") {
      CHECK(ValidWorldComponentAccess<const World, const Position&,
                                      const Velocity&>);
    }

    SUBCASE("Const world rejects any mutable access") {
      CHECK(
          !ValidWorldComponentAccess<const World, Position&, const Velocity&>);
    }

    SUBCASE("Const world rejects single mutable T&& access") {
      CHECK(!ValidWorldComponentAccess<const World, Position&&>);
    }
  }
}

TEST_SUITE("helios::ecs::details::ValidComponentAccess") {
  TEST_CASE("ecs::details::ValidComponentAccess") {
    SUBCASE("Plain T satisfies ValidComponentAccess") {
      CHECK(ValidComponentAccess<Position>);
    }

    SUBCASE("T& satisfies ValidComponentAccess") {
      CHECK(ValidComponentAccess<Position&>);
    }

    SUBCASE("const T& satisfies ValidComponentAccess") {
      CHECK(ValidComponentAccess<const Position&>);
    }

    SUBCASE("T* satisfies ValidComponentAccess") {
      CHECK(ValidComponentAccess<Position*>);
    }

    SUBCASE("const T* satisfies ValidComponentAccess") {
      CHECK(ValidComponentAccess<const Position*>);
    }
  }
}

TEST_SUITE("helios::ecs::details::kIsTagAccess") {
  TEST_CASE("ecs::details::kIsTagAccess") {
    SUBCASE("Tag type is tag access") {
      CHECK(kIsTagAccess<TagA>);
      CHECK(kIsTagAccess<const TagA&>);
    }

    SUBCASE("Non-tag type is not tag access") {
      CHECK(!kIsTagAccess<Position>);
      CHECK(!kIsTagAccess<Position&>);
    }

    SUBCASE("Pointer to tag is tag access") {
      CHECK(kIsTagAccess<TagA*>);
    }

    SUBCASE("Pointer to non-tag is not tag access") {
      CHECK(!kIsTagAccess<Position*>);
    }
  }
}

TEST_SUITE("helios::ecs::details::ComponentTypeIndexFromAccess") {
  TEST_CASE("ecs::details::ComponentTypeIndexFromAccess — consistent indices") {
    SUBCASE("T and T& and const T& all yield the same index") {
      constexpr auto idx_plain = ComponentTypeIndexFromAccess<Position>();
      constexpr auto idx_ref = ComponentTypeIndexFromAccess<Position&>();
      constexpr auto idx_const_ref =
          ComponentTypeIndexFromAccess<const Position&>();

      CHECK_EQ(idx_plain, idx_ref);
      CHECK_EQ(idx_plain, idx_const_ref);
    }

    SUBCASE("T* yields same index as T") {
      constexpr auto idx_raw = ComponentTypeIndexFromAccess<Position>();
      constexpr auto idx_opt = ComponentTypeIndexFromAccess<Position*>();

      CHECK_EQ(idx_raw, idx_opt);
    }

    SUBCASE("const T* yields same index as T") {
      constexpr auto idx_raw = ComponentTypeIndexFromAccess<Position>();
      constexpr auto idx_opt = ComponentTypeIndexFromAccess<const Position*>();

      CHECK_EQ(idx_raw, idx_opt);
    }

    SUBCASE("Different component types yield different indices") {
      constexpr auto idx_pos = ComponentTypeIndexFromAccess<Position>();
      constexpr auto idx_vel = ComponentTypeIndexFromAccess<Velocity>();

      CHECK_NE(idx_pos, idx_vel);
    }
  }
}

TEST_SUITE("helios::ecs::details::UniqueComponentAccess") {
  TEST_CASE(
      "ecs::details::UniqueComponentAccess — valid (unique underlying types)") {
    SUBCASE("Single component is unique") {
      CHECK(UniqueComponentAccess<Position>);
    }

    SUBCASE("Different component types are unique") {
      CHECK(UniqueComponentAccess<Position, Velocity, Health>);
    }

    SUBCASE("Mix of pointer and non-pointer of different types is unique") {
      CHECK(UniqueComponentAccess<Position*, const Velocity&, Health>);
    }
  }

  TEST_CASE(
      "ecs::details::UniqueComponentAccess — invalid (duplicate underlying "
      "type)") {
    SUBCASE("Same type with different qualifiers is not unique") {
      CHECK(!UniqueComponentAccess<Position, Position&>);
    }

    SUBCASE("Pointer and raw of same type is not unique") {
      CHECK(!UniqueComponentAccess<Position, Position*>);
    }

    SUBCASE("Two refs of same type is not unique") {
      CHECK(!UniqueComponentAccess<const Position&, Position&>);
    }
  }
}
