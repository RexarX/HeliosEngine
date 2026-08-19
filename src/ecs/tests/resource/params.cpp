#include <doctest/doctest.h>

#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/resource/resource.hpp>
#include <helios/ecs/system/access_policy.hpp>
#include <helios/ecs/system/param.hpp>

#include <string>

using namespace helios::ecs;

namespace {

struct Counter {
  int value = 0;
};

struct Config {
  std::string name;
  int version = 0;
};

struct DeltaTime {
  float value = 0.0F;

  static constexpr bool kThreadSafe = true;
};

template <typename T>
concept HasInsert =
    requires(const T& local, Counter counter) { local.Insert(counter); };

template <typename T>
concept HasEmplace = requires(const T& local) { local.Emplace(); };

template <typename T>
concept HasResources = requires(const T& local) { local.Resources(); };

template <typename T, typename U>
concept HasResourceAssignment =
    requires(const T& local, U value) { local = value; };

template <typename T>
concept HasSystemParamTraits = requires { typename SystemParamTraits<T>; };

template <typename T>
concept HasRegisterAccess = requires(AccessPolicyBuilder& builder) {
  SystemParamTraits<T>::RegisterAccess(builder);
};

}  // namespace

TEST_SUITE("helios::ecs::Res") {
  TEST_CASE("helios::ecs::Res::ctor") {
    SUBCASE("Constructor stores resource reference") {
      Counter cnt{42};
      const Res<Counter> res(cnt);
      CHECK_EQ(res->value, 42);
    }

    SUBCASE("Constructor accepts const-qualified resource reference") {
      const Counter cnt{7};
      const Res<const Counter> res(cnt);
      CHECK_EQ(res->value, 7);
    }
  }

  TEST_CASE("helios::ecs::Res::copy") {
    SUBCASE("Copy constructor creates a valid copy") {
      Counter cnt{99};
      const Res<Counter> original(cnt);
      const Res<Counter> copy(original);
      CHECK_EQ(copy->value, 99);
    }

    SUBCASE("Copy assignment copies the reference") {
      Counter cnt_a{1};
      Counter cnt_b{2};
      Res<Counter> res_a(cnt_a);
      Res<Counter> res_b(cnt_b);
      res_b = res_a;
      CHECK_EQ(res_b->value, 1);
    }
  }

  TEST_CASE("helios::ecs::Res::move") {
    SUBCASE("Move constructor transfers the reference") {
      Counter cnt{5};
      Res<Counter> original(cnt);
      const Res<Counter> moved(std::move(original));
      CHECK_EQ(moved->value, 5);
    }

    SUBCASE("Move assignment transfers the reference") {
      Counter cnt_a{10};
      Counter cnt_b{20};
      Res<Counter> res_a(cnt_a);
      Res<Counter> res_b(cnt_b);
      res_b = std::move(res_a);
      CHECK_EQ(res_b->value, 10);
    }
  }

  TEST_CASE("helios::ecs::Res::operator-> (mutable)") {
    SUBCASE("operator-> returns pointer to stored resource") {
      Counter cnt{3};
      Res<Counter> res(cnt);
      CHECK_EQ(res->value, 3);
    }

    SUBCASE("Modification through operator-> updates the original resource") {
      Counter cnt{0};
      Res<Counter> res(cnt);
      res->value = 77;
      CHECK_EQ(cnt.value, 77);
    }
  }

  TEST_CASE("helios::ecs::Res::operator* (mutable)") {
    SUBCASE("operator* returns reference to stored resource") {
      Counter cnt{9};
      Res<Counter> res(cnt);
      CHECK_EQ((*res).value, 9);
    }

    SUBCASE("Modification through operator* updates the original resource") {
      Counter cnt{0};
      Res<Counter> res(cnt);
      (*res).value = 42;
      CHECK_EQ(cnt.value, 42);
    }
  }

  TEST_CASE("helios::ecs::Res::operator-> (const)") {
    SUBCASE("operator-> returns const pointer to stored resource") {
      const Counter cnt{3};
      const Res<const Counter> res(cnt);
      CHECK_EQ(res->value, 3);
    }

    SUBCASE("const operator-> returns const pointer") {
      Counter cnt{5};
      const Res<Counter> res(cnt);
      CHECK_EQ(res->value, 5);
    }
  }

  TEST_CASE("helios::ecs::Res::operator* (const)") {
    SUBCASE("operator* returns const reference to stored resource") {
      const Counter cnt{9};
      const Res<const Counter> res(cnt);
      CHECK_EQ((*res).value, 9);
    }

    SUBCASE("const operator* returns const reference") {
      Counter cnt{7};
      const Res<Counter> res(cnt);
      CHECK_EQ((*res).value, 7);
    }
  }
}

TEST_SUITE("helios::ecs::AsyncRes") {
  TEST_CASE("helios::ecs::AsyncRes — alias compiles and works") {
    SUBCASE("AsyncRes wraps an async resource") {
      DeltaTime dt{1.5F};
      const AsyncRes<DeltaTime> res(dt);
      CHECK_EQ(res->value, 1.5F);
    }

    SUBCASE("AsyncRes modification works through pointer") {
      DeltaTime dt{0.0F};
      AsyncRes<DeltaTime> res(dt);
      res->value = 2.0F;
      CHECK_EQ(dt.value, 2.0F);
    }
  }
}

TEST_SUITE("helios::ecs::Local") {
  TEST_CASE("helios::ecs::Local::ctor") {
    SUBCASE("Constructor stores resource reference") {
      Counter counter{42};

      const Local<Counter> local(counter);

      CHECK_EQ(&local.Get(), &counter);
      CHECK_EQ(local.Get().value, 42);
    }

    SUBCASE("Constructor accepts const-qualified local resource type") {
      const Counter counter{7};

      const Local<const Counter> local(counter);

      CHECK_EQ(&local.Get(), &counter);
      CHECK_EQ(local.Get().value, 7);
    }
  }

  TEST_CASE("helios::ecs::Local::copy") {
    SUBCASE("Copy constructor creates a wrapper for the same resource") {
      Counter counter{42};
      const Local<Counter> original(counter);

      const Local<Counter> copy(original);

      CHECK_EQ(&copy.Get(), &counter);
      CHECK_EQ(copy.Get().value, 42);
    }

    SUBCASE("Copy assignment copies the resource reference") {
      Counter counter_a{7};
      Counter counter_b{3};
      const Local<Counter> local_a(counter_a);
      Local<Counter> local_b(counter_b);

      local_b = local_a;
      local_b = Counter{11};

      CHECK_EQ(counter_a.value, 11);
      CHECK_EQ(counter_b.value, 3);
    }
  }

  TEST_CASE("helios::ecs::Local::move") {
    SUBCASE("Move constructor transfers the resource reference") {
      Counter counter{99};
      Local<Counter> original(counter);

      const Local<Counter> moved(std::move(original));

      CHECK_EQ(&moved.Get(), &counter);
      CHECK_EQ(moved.Get().value, 99);
    }

    SUBCASE("Move assignment transfers the resource reference") {
      Counter counter_a{5};
      Counter counter_b{8};
      Local<Counter> local_a(counter_a);
      Local<Counter> local_b(counter_b);

      local_b = std::move(local_a);
      local_b = Counter{13};

      CHECK_EQ(counter_a.value, 13);
      CHECK_EQ(counter_b.value, 8);
    }
  }

  TEST_CASE("helios::ecs::Local::operator=") {
    SUBCASE("Assignment with lvalue replaces the referenced resource value") {
      Counter counter{0};
      Counter replacement{10};
      const Local<Counter> local(counter);

      local = replacement;

      CHECK_EQ(counter.value, 10);
    }

    SUBCASE("Assignment with rvalue replaces the referenced resource value") {
      Counter counter{1};
      const Local<Counter> local(counter);

      local = Counter{22};

      CHECK_EQ(counter.value, 22);
    }

    SUBCASE("Assignment returns the referenced resource") {
      Counter counter{0};
      const Local<Counter> local(counter);

      Counter& assigned = (local = Counter{31});

      CHECK_EQ(&assigned, &counter);
      CHECK_EQ(assigned.value, 31);
    }

    SUBCASE("Const-qualified local resources cannot be assigned through") {
      CHECK(std::is_assignable_v<const Local<Counter>&, Counter>);
      CHECK_FALSE(std::is_assignable_v<const Local<const Counter>&, Counter>);
      CHECK_FALSE((HasResourceAssignment<Local<const Counter>, Counter>));
    }
  }

  TEST_CASE("helios::ecs::Local::operator*") {
    SUBCASE("operator* returns reference to stored resource") {
      Counter counter{5};
      const Local<Counter> local(counter);

      const auto& cnt = *local;

      CHECK_EQ(&cnt, &counter);
      CHECK_EQ(cnt.value, 5);
    }

    SUBCASE(
        "Modification through operator* is reflected in subsequent access") {
      Counter counter{0};
      const Local<Counter> local(counter);

      (*local).value = 42;

      CHECK_EQ(local.Get().value, 42);
      CHECK_EQ(counter.value, 42);
    }
  }

  TEST_CASE("helios::ecs::Local::operator->") {
    SUBCASE("operator-> returns pointer to stored resource") {
      Counter counter{3};
      const Local<Counter> local(counter);

      CHECK_EQ(local.operator->(), &counter);
      CHECK_EQ(local->value, 3);
    }

    SUBCASE(
        "Modification through operator-> is reflected in subsequent access") {
      Counter counter{0};
      const Local<Counter> local(counter);

      local->value = 77;

      CHECK_EQ(local.Get().value, 77);
      CHECK_EQ(counter.value, 77);
    }
  }

  TEST_CASE("helios::ecs::Local::Get") {
    SUBCASE("Get returns reference to stored resource") {
      Counter counter{9};
      const Local<Counter> local(counter);

      CHECK_EQ(&local.Get(), &counter);
      CHECK_EQ(local.Get().value, 9);
    }

    SUBCASE("Get returns const reference for const-qualified local") {
      const Counter counter{4};
      const Local<const Counter> local(counter);

      CHECK_EQ(&local.Get(), &counter);
      CHECK_EQ(local.Get().value, 4);
    }
  }

  TEST_CASE("helios::ecs::Local::removed manager methods") {
    SUBCASE("Insert is not part of the Local API") {
      CHECK_FALSE(HasInsert<Local<Counter>>);
    }

    SUBCASE("Emplace is not part of the Local API") {
      CHECK_FALSE(HasEmplace<Local<Counter>>);
    }

    SUBCASE("Resources is not part of the Local API") {
      CHECK_FALSE(HasResources<Local<Counter>>);
    }
  }
}

TEST_SUITE("helios::ecs::SystemParamTraits") {
  TEST_CASE("helios::ecs::SystemParamTraits: Res") {
    SUBCASE("Res<const T> exists as a system parameter trait") {
      CHECK(HasSystemParamTraits<Res<const Config>>);
      CHECK(HasRegisterAccess<Res<const Config>>);
    }

    SUBCASE("Res<T> exists as a system parameter trait") {
      CHECK(HasSystemParamTraits<Res<Config>>);
      CHECK(HasRegisterAccess<Res<Config>>);
    }

    SUBCASE("OptRes<const T> exists as a system parameter trait") {
      CHECK(HasSystemParamTraits<OptRes<const Config>>);
      CHECK(HasRegisterAccess<OptRes<const Config>>);
    }

    SUBCASE("OptRes<T> exists as a system parameter trait") {
      CHECK(HasSystemParamTraits<OptRes<Config>>);
      CHECK(HasRegisterAccess<OptRes<Config>>);
    }

    SUBCASE("Res<const T> RegisterAccess adds read resource") {
      AccessPolicyBuilder builder;
      SystemParamTraits<Res<const Config>>::RegisterAccess(builder);

      const auto policy = builder.Build();
      CHECK(policy.HasReadResource(ResourceTypeIndex::From<Config>()));
      CHECK_FALSE(policy.HasWriteResource(ResourceTypeIndex::From<Config>()));
    }

    SUBCASE("Res<T> RegisterAccess adds write resource") {
      AccessPolicyBuilder builder;
      SystemParamTraits<Res<Config>>::RegisterAccess(builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasReadResource(ResourceTypeIndex::From<Config>()));
      CHECK(policy.HasWriteResource(ResourceTypeIndex::From<Config>()));
    }

    SUBCASE("OptRes<const T> RegisterAccess adds read resource") {
      AccessPolicyBuilder builder;
      SystemParamTraits<OptRes<const Config>>::RegisterAccess(builder);

      const auto policy = builder.Build();
      CHECK(policy.HasReadResource(ResourceTypeIndex::From<Config>()));
    }

    SUBCASE("OptRes<T> RegisterAccess adds write resource") {
      AccessPolicyBuilder builder;
      SystemParamTraits<OptRes<Config>>::RegisterAccess(builder);

      const auto policy = builder.Build();
      CHECK(policy.HasWriteResource(ResourceTypeIndex::From<Config>()));
    }

    SUBCASE("Async resource is not registered by RegisterAccess") {
      AccessPolicyBuilder builder;
      SystemParamTraits<Res<DeltaTime>>::RegisterAccess(builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasResources());
    }

    SUBCASE("Async resource in optional is not registered") {
      AccessPolicyBuilder builder;
      SystemParamTraits<OptRes<DeltaTime>>::RegisterAccess(builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasResources());
    }
  }

  TEST_CASE("helios::ecs::SystemParamTraits::Local") {
    SUBCASE("Local<const T> exists as a system parameter trait") {
      CHECK(HasSystemParamTraits<Local<const Config>>);
      CHECK(HasRegisterAccess<Local<const Config>>);
    }

    SUBCASE("Local<T> exists as a system parameter trait") {
      CHECK(HasSystemParamTraits<Local<Config>>);
      CHECK(HasRegisterAccess<Local<Config>>);
    }

    SUBCASE("optional<Local<T>> is not a system parameter") {
      CHECK_FALSE(SystemParam<std::optional<Local<const Config>>>);
      CHECK_FALSE(SystemParam<std::optional<Local<Config>>>);
      CHECK_FALSE(HasRegisterAccess<std::optional<Local<const Config>>>);
      CHECK_FALSE(HasRegisterAccess<std::optional<Local<Config>>>);
    }

    SUBCASE("Local<T> Make default-creates system-local resources") {
      World world;
      auto data = SystemLocalData::From();
      AccessPolicy policy;
      CHECK_FALSE(data.resource_manager.Has<Config>());

      const auto local =
          SystemParamTraits<Local<Config>>::Make(world, data, policy);
      local = Config{.name = "Test", .version = 1};

      auto& config = data.resource_manager.Get<Config>();
      CHECK_EQ(config.name, "Test");
      CHECK_EQ(config.version, 1);
    }

    SUBCASE("Local<const T> Make default-creates system-local resources") {
      World world;
      SystemLocalData data = SystemLocalData::From();
      AccessPolicy policy;
      CHECK_FALSE(data.resource_manager.Has<Config>());

      const auto local =
          SystemParamTraits<Local<const Config>>::Make(world, data, policy);

      CHECK_EQ(local->version, 0);
      CHECK(data.resource_manager.Has<Config>());
    }

    SUBCASE("Local<const T> Make reads existing system-local resources") {
      World world;
      SystemLocalData data = SystemLocalData::From();
      AccessPolicy policy;
      data.resource_manager.Insert(Config{.name = "Test2", .version = 2});

      const auto local =
          SystemParamTraits<Local<const Config>>::Make(world, data, policy);

      CHECK_EQ(local->name, "Test2");
      CHECK_EQ(local->version, 2);
    }

    SUBCASE("Local<const T> RegisterAccess produces empty policy") {
      AccessPolicyBuilder builder;
      SystemParamTraits<Local<const Config>>::RegisterAccess(builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }

    SUBCASE("Local<T> RegisterAccess produces empty policy") {
      AccessPolicyBuilder builder;
      SystemParamTraits<Local<Config>>::RegisterAccess(builder);

      const auto policy = builder.Build();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }
  }
}
