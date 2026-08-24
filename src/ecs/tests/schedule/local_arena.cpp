#include <doctest/doctest.h>

#include <helios/ecs/resource/resource.hpp>
#include <helios/ecs/schedule/local_arena.hpp>
#include <helios/ecs/schedule/system_local_data.hpp>
#include <helios/ecs/system/access_policy.hpp>
#include <helios/ecs/system/param.hpp>
#include <helios/memory/arena_allocator.hpp>

using namespace helios;
using namespace helios::ecs;

namespace {

template <typename T>
concept HasSystemParamTraits = requires { typename SystemParamTraits<T>; };

template <typename T>
concept HasRegisterAccess = requires(AccessPolicyBuilder& builder) {
  SystemParamTraits<T>::RegisterAccess(builder);
};

}  // namespace

TEST_SUITE("helios::ecs::LocalArena") {
  TEST_CASE("helios::ecs::LocalArena::ctor") {
    SUBCASE("Constructor stores a reference to the given allocator") {
      auto data = SystemLocalData::From();
      const LocalArena local_arena(data.allocator);
      CHECK_EQ(local_arena.GetPtr(), &data.allocator);
    }

    SUBCASE("Copy constructor references the same allocator") {
      auto data = SystemLocalData::From();
      const LocalArena original(data.allocator);
      const LocalArena copy(original);

      CHECK_EQ(copy.GetPtr(), &data.allocator);
    }

    SUBCASE("Move constructor references the same allocator") {
      auto data = SystemLocalData::From();
      LocalArena original(data.allocator);
      const LocalArena moved(std::move(original));

      CHECK_EQ(moved.GetPtr(), &data.allocator);
    }
  }

  TEST_CASE("helios::ecs::LocalArena::operator=") {
    SUBCASE("Copy assignment rebinds to the assigned allocator") {
      auto data_a = SystemLocalData::From();
      auto data_b = SystemLocalData::From();
      const LocalArena arena_a(data_a.allocator);
      LocalArena arena_b(data_b.allocator);

      arena_b = arena_a;

      CHECK_EQ(arena_b.GetPtr(), &data_a.allocator);
    }

    SUBCASE("Move assignment rebinds to the assigned allocator") {
      auto data_a = SystemLocalData::From();
      auto data_b = SystemLocalData::From();
      LocalArena arena_a(data_a.allocator);
      LocalArena arena_b(data_b.allocator);

      arena_b = std::move(arena_a);

      CHECK_EQ(arena_b.GetPtr(), &data_a.allocator);
    }
  }

  TEST_CASE("helios::ecs::LocalArena::operator*") {
    SUBCASE("operator* returns a reference to the underlying allocator") {
      auto data = SystemLocalData::From();
      const LocalArena local_arena(data.allocator);
      mem::ArenaAllocator& arena = *local_arena;

      CHECK_EQ(&arena, &data.allocator);
    }
  }

  TEST_CASE("helios::ecs::LocalArena::operator->") {
    SUBCASE("operator-> returns a pointer to the underlying allocator") {
      auto data = SystemLocalData::From();
      const LocalArena local_arena(data.allocator);

      CHECK_EQ(local_arena.operator->(), &data.allocator);
    }
  }

  TEST_CASE("helios::ecs::LocalArena::GetPtr") {
    SUBCASE("GetPtr returns a pointer to the underlying allocator") {
      auto data = SystemLocalData::From();
      const LocalArena local_arena(data.allocator);

      CHECK_EQ(local_arena.GetPtr(), &data.allocator);
    }
  }

  TEST_CASE("helios::ecs::LocalArena satisfies ResourceTrait") {
    CHECK(ResourceTrait<LocalArena>);
  }
}

TEST_SUITE("helios::ecs::SystemParamTraits") {
  TEST_CASE("helios::ecs::SystemParamTraits: LocalArena") {
    SUBCASE("Local<LocalArena> exists as a system parameter trait") {
      CHECK(HasSystemParamTraits<Local<LocalArena>>);
      CHECK(HasRegisterAccess<Local<LocalArena>>);
    }

    SUBCASE("Local<const LocalArena> exists as a system parameter trait") {
      CHECK(HasSystemParamTraits<Local<const LocalArena>>);
      CHECK(HasRegisterAccess<Local<const LocalArena>>);
    }

    SUBCASE("Local<LocalArena> Make wraps the system's arena allocator") {
      World world;
      SystemLocalData data = SystemLocalData::From();
      AccessPolicy policy;
      CHECK_FALSE(data.resource_manager.Has<LocalArena>());

      const Local<LocalArena> local =
          SystemParamTraits<Local<LocalArena>>::Make(world, data, policy);

      CHECK(data.resource_manager.Has<LocalArena>());
      CHECK_EQ(local->GetPtr(), &data.allocator);
    }

    SUBCASE("Local<const LocalArena> Make wraps the system's arena allocator") {
      World world;
      SystemLocalData data = SystemLocalData::From();
      AccessPolicy policy;
      CHECK_FALSE(data.resource_manager.Has<LocalArena>());

      const Local<const LocalArena> local =
          SystemParamTraits<Local<const LocalArena>>::Make(world, data, policy);

      CHECK(data.resource_manager.Has<LocalArena>());
      CHECK_EQ(local->GetPtr(), &data.allocator);
    }

    SUBCASE(
        "Local<LocalArena> Make reuses the existing system-local LocalArena") {
      World world;
      SystemLocalData data = SystemLocalData::From();
      AccessPolicy policy;

      const Local<LocalArena> first =
          SystemParamTraits<Local<LocalArena>>::Make(world, data, policy);
      const Local<LocalArena> second =
          SystemParamTraits<Local<LocalArena>>::Make(world, data, policy);

      CHECK_EQ(&first.Get(), &second.Get());
    }

    SUBCASE("Local<LocalArena> RegisterAccess produces an empty policy") {
      AccessPolicyBuilder builder;
      SystemParamTraits<Local<LocalArena>>::RegisterAccess(builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }

    SUBCASE("Local<const LocalArena> RegisterAccess produces an empty policy") {
      AccessPolicyBuilder builder;
      SystemParamTraits<Local<const LocalArena>>::RegisterAccess(builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }
  }
}
