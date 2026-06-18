#include <doctest/doctest.h>

#include <helios/ecs/resource/resource.hpp>

#include <concepts>
#include <string_view>
#include <type_traits>

using namespace helios::ecs;

namespace {

struct PlainResource {
  int value = 0;
};

struct MoveOnlyResource {
  MoveOnlyResource() = default;
  MoveOnlyResource(MoveOnlyResource&&) = default;
  MoveOnlyResource(const MoveOnlyResource&) = delete;
  ~MoveOnlyResource() = default;

  MoveOnlyResource& operator=(const MoveOnlyResource&) = delete;
  MoveOnlyResource& operator=(MoveOnlyResource&&) = default;
};

struct CopyOnlyResource {
  CopyOnlyResource() = default;
  CopyOnlyResource(const CopyOnlyResource&) = default;
  CopyOnlyResource(CopyOnlyResource&&) = default;
  ~CopyOnlyResource() = default;

  CopyOnlyResource& operator=(const CopyOnlyResource&) = default;
  CopyOnlyResource& operator=(CopyOnlyResource&&) = default;
};

struct NamedResource {
  static constexpr std::string_view kName = "MyNamedResource";
};

struct ThreadSafeResource {
  static constexpr bool kThreadSafe = true;
};

struct ThreadUnsafeResource {
  static constexpr bool kThreadSafe = false;
};

// Tracks whether callbacks were called and which world was passed.
struct CallbackTracker {
  inline static bool insert_called = false;
  inline static bool remove_called = false;
  inline static World* insert_world_ptr = nullptr;
  inline static World* remove_world_ptr = nullptr;

  static void Reset() {
    insert_called = false;
    remove_called = false;
    insert_world_ptr = nullptr;
    remove_world_ptr = nullptr;
  }

  void OnInsert(World& world) {
    insert_called = true;
    insert_world_ptr = &world;
  }

  void OnRemove(World& world) {
    remove_called = true;
    remove_world_ptr = &world;
  }
};

struct InsertOnlyResource {
  inline static bool insert_called = false;
  void OnInsert(World& /*world*/) { insert_called = true; }
};

struct RemoveOnlyResource {
  inline static bool remove_called = false;
  void OnRemove(World& /*world*/) { remove_called = true; }
};

struct NonDestructible {
  ~NonDestructible() = delete;
};

struct NeitherMovableNorCopyable {
  NeitherMovableNorCopyable(const NeitherMovableNorCopyable&) = delete;
  NeitherMovableNorCopyable(NeitherMovableNorCopyable&&) = delete;
};

struct PolymorphicResource {
  virtual ~PolymorphicResource() = default;
};

[[nodiscard]] World& DeclWorld() noexcept {
  return *reinterpret_cast<World*>(0x42);
}

}  // namespace

TEST_SUITE("helios::ecs::ResourceTrait") {
  TEST_CASE("ecs::ResourceTrait — satisfied by valid resource types") {
    SUBCASE("plain struct with default members satisfies ResourceTrait") {
      CHECK(ResourceTrait<PlainResource>);
    }

    SUBCASE("move-only type satisfies ResourceTrait") {
      CHECK(ResourceTrait<MoveOnlyResource>);
    }

    SUBCASE("copy-only type satisfies ResourceTrait") {
      CHECK(ResourceTrait<CopyOnlyResource>);
    }

    SUBCASE("cv-ref-qualified T still satisfies ResourceTrait") {
      CHECK(ResourceTrait<const PlainResource&>);
    }
  }

  TEST_CASE("ecs::ResourceTrait — not satisfied by invalid types") {
    SUBCASE("non-destructible type does not satisfy ResourceTrait") {
      CHECK(!ResourceTrait<NonDestructible>);
    }

    SUBCASE(
        "type that is neither move- nor copy-constructible does not satisfy "
        "ResourceTrait") {
      CHECK(!ResourceTrait<NeitherMovableNorCopyable>);
    }

    SUBCASE("polymorphic type does not satisfy ResourceTrait") {
      CHECK(!ResourceTrait<PolymorphicResource>);
    }
  }
}

TEST_SUITE("helios::ecs::ResourceWithNameTrait") {
  TEST_CASE("ecs::ResourceWithNameTrait — satisfied when kName is present") {
    SUBCASE(
        "type with static constexpr kName satisfies ResourceWithNameTrait") {
      CHECK(ResourceWithNameTrait<NamedResource>);
    }
  }

  TEST_CASE("ecs::ResourceWithNameTrait — not satisfied when kName is absent") {
    SUBCASE(
        "plain resource without kName does not satisfy ResourceWithNameTrait") {
      CHECK(!ResourceWithNameTrait<PlainResource>);
    }
  }
}

TEST_SUITE("helios::ecs::ResourceWithThreadSafetyTrait") {
  TEST_CASE(
      "ecs::ResourceWithThreadSafetyTrait — satisfied when kThreadSafe is "
      "present") {
    SUBCASE(
        "type with kThreadSafe = true satisfies "
        "ResourceWithThreadSafetyTrait") {
      CHECK(ResourceWithThreadSafetyTrait<ThreadSafeResource>);
    }

    SUBCASE(
        "type with kThreadSafe = false satisfies "
        "ResourceWithThreadSafetyTrait") {
      CHECK(ResourceWithThreadSafetyTrait<ThreadUnsafeResource>);
    }
  }

  TEST_CASE(
      "ecs::ResourceWithThreadSafetyTrait — not satisfied when kThreadSafe is "
      "absent") {
    SUBCASE(
        "plain resource without kThreadSafe does not satisfy "
        "ResourceWithThreadSafetyTrait") {
      CHECK(!ResourceWithThreadSafetyTrait<PlainResource>);
    }
  }
}

TEST_SUITE("helios::ecs::ResourceWithInsertionCallbackTrait") {
  TEST_CASE(
      "ecs::ResourceWithInsertionCallbackTrait — satisfied when OnInsert is "
      "present") {
    SUBCASE(
        "type providing static OnInsert(World&) satisfies "
        "ResourceWithInsertionCallbackTrait") {
      CHECK(ResourceWithInsertionCallbackTrait<InsertOnlyResource>);
    }

    SUBCASE(
        "type providing both OnInsert and OnRemove satisfies "
        "ResourceWithInsertionCallbackTrait") {
      CHECK(ResourceWithInsertionCallbackTrait<CallbackTracker>);
    }
  }

  TEST_CASE(
      "ecs::ResourceWithInsertionCallbackTrait — not satisfied when OnInsert "
      "is absent") {
    SUBCASE(
        "type without OnInsert does not satisfy "
        "ResourceWithInsertionCallbackTrait") {
      CHECK(!ResourceWithInsertionCallbackTrait<PlainResource>);
    }

    SUBCASE(
        "type with only OnRemove does not satisfy "
        "ResourceWithInsertionCallbackTrait") {
      CHECK(!ResourceWithInsertionCallbackTrait<RemoveOnlyResource>);
    }
  }
}

TEST_SUITE("helios::ecs::ResourceWithRemovalCallbackTrait") {
  TEST_CASE(
      "ecs::ResourceWithRemovalCallbackTrait — satisfied when OnRemove is "
      "present") {
    SUBCASE(
        "type providing static OnRemove(World&) satisfies "
        "ResourceWithRemovalCallbackTrait") {
      CHECK(ResourceWithRemovalCallbackTrait<RemoveOnlyResource>);
    }

    SUBCASE(
        "type providing both OnInsert and OnRemove satisfies "
        "ResourceWithRemovalCallbackTrait") {
      CHECK(ResourceWithRemovalCallbackTrait<CallbackTracker>);
    }
  }

  TEST_CASE(
      "ecs::ResourceWithRemovalCallbackTrait — not satisfied when OnRemove is "
      "absent") {
    SUBCASE(
        "type without OnRemove does not satisfy "
        "ResourceWithRemovalCallbackTrait") {
      CHECK(!ResourceWithRemovalCallbackTrait<PlainResource>);
    }

    SUBCASE(
        "type with only OnInsert does not satisfy "
        "ResourceWithRemovalCallbackTrait") {
      CHECK(!ResourceWithRemovalCallbackTrait<InsertOnlyResource>);
    }
  }
}

TEST_SUITE("helios::ecs::ResourceNameOf") {
  TEST_CASE("ecs::ResourceNameOf<T>() — type template overload") {
    SUBCASE("returns kName when ResourceWithNameTrait is satisfied") {
      CHECK(ResourceNameOf<NamedResource>() == "MyNamedResource");
    }

    SUBCASE("returns type name via TypeNameOf when kName is absent") {
      // We only check that the result is non-empty; the exact string is
      // implementation-defined by utils::TypeNameOf.
      CHECK(!ResourceNameOf<PlainResource>().empty());
    }
  }

  TEST_CASE("ecs::ResourceNameOf(instance) — instance overload") {
    SUBCASE("returns the same value as the type overload for named resources") {
      NamedResource instance{};
      CHECK(ResourceNameOf(instance) == ResourceNameOf<NamedResource>());
    }

    SUBCASE(
        "returns the same value as the type overload for unnamed resources") {
      PlainResource instance{};
      CHECK(ResourceNameOf(instance) == ResourceNameOf<PlainResource>());
    }
  }
}

TEST_SUITE("helios::ecs::IsResourceThreadSafe") {
  TEST_CASE("ecs::IsResourceThreadSafe<T>() — type template overload") {
    SUBCASE("returns true when kThreadSafe = true") {
      CHECK(IsResourceThreadSafe<ThreadSafeResource>());
    }

    SUBCASE("returns false when kThreadSafe = false") {
      CHECK(!IsResourceThreadSafe<ThreadUnsafeResource>());
    }

    SUBCASE("returns false when kThreadSafe is absent") {
      CHECK(!IsResourceThreadSafe<PlainResource>());
    }
  }

  TEST_CASE("ecs::IsResourceThreadSafe(instance) — instance overload") {
    SUBCASE("returns same value as type overload for thread-safe resource") {
      ThreadSafeResource instance{};
      CHECK(IsResourceThreadSafe(instance) ==
            IsResourceThreadSafe<ThreadSafeResource>());
    }

    SUBCASE("returns same value as type overload for thread-unsafe resource") {
      ThreadUnsafeResource instance{};
      CHECK(IsResourceThreadSafe(instance) ==
            IsResourceThreadSafe<ThreadUnsafeResource>());
    }

    SUBCASE("returns false for resource without kThreadSafe") {
      PlainResource instance{};
      CHECK(!IsResourceThreadSafe(instance));
    }
  }
}

TEST_SUITE("helios::ecs::ResourceCallOnInsert") {
  TEST_CASE("ecs::ResourceCallOnInsert(instance, world) — instance overload") {
    SUBCASE("delegates to the type overload") {
      CallbackTracker::Reset();
      World& world = DeclWorld();
      CallbackTracker instance{};
      ResourceCallOnInsert(instance, world);
      CHECK(CallbackTracker::insert_called);
    }

    SUBCASE("does nothing for resource without OnInsert") {
      World& world = DeclWorld();
      PlainResource instance{};
      CHECK_NOTHROW(ResourceCallOnInsert(instance, world));
    }
  }
}

TEST_SUITE("helios::ecs::ResourceCallOnRemove") {
  TEST_CASE("ecs::ResourceCallOnRemove(instance, world) — instance overload") {
    SUBCASE("delegates to the type overload") {
      CallbackTracker::Reset();
      World& world = DeclWorld();
      CallbackTracker instance{};
      ResourceCallOnRemove(instance, world);
      CHECK(CallbackTracker::remove_called);
    }

    SUBCASE("does nothing for resource without OnRemove") {
      World& world = DeclWorld();
      PlainResource instance{};
      CHECK_NOTHROW(ResourceCallOnRemove(instance, world));
    }
  }
}
