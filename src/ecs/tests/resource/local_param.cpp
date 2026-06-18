#include <doctest/doctest.h>

#include <helios/ecs/resource/local_param.hpp>
#include <helios/ecs/resource/manager.hpp>

#include <string>
#include <type_traits>

using namespace helios::ecs;

namespace {

struct Counter {
  int value = 0;
};

struct Config {
  std::string name;
  int version = 0;
};

struct Vec3 {
  float x = 0.0F;
  float y = 0.0F;
  float z = 0.0F;
};

}  // namespace

TEST_SUITE("helios::ecs::Local") {
  TEST_CASE("ecs::Local::ctor") {
    SUBCASE("Constructor stores resource manager reference") {
      ResourceManager resources;
      const Local<Counter> local(resources);
      CHECK_EQ(local.Resources().Count(), 0);
    }

    SUBCASE("Constructor accepts const-qualified local resource type") {
      ResourceManager resources;
      const Local<const Counter> local(resources);
      CHECK_EQ(local.Resources().Count(), 0);
    }
  }

  TEST_CASE("ecs::Local::copy") {
    SUBCASE("Copy constructor creates a valid copy") {
      ResourceManager resources;
      resources.Emplace<Counter>(42);
      const Local<Counter> original(resources);
      const Local<Counter> copy(original);
      CHECK_EQ(copy.Get().value, 42);
    }

    SUBCASE("Copy assignment copies the reference") {
      ResourceManager resources;
      resources.Emplace<Counter>(7);
      const Local<Counter> local_a(resources);
      Local<Counter> local_b(resources);
      local_b = local_a;
      CHECK_EQ(local_b.Get().value, 7);
    }
  }

  TEST_CASE("ecs::Local::move") {
    SUBCASE("Move constructor transfers the reference") {
      ResourceManager resources;
      resources.Emplace<Counter>(99);
      Local<Counter> original(resources);
      const Local<Counter> moved(std::move(original));
      CHECK_EQ(moved.Get().value, 99);
    }

    SUBCASE("Move assignment transfers the reference") {
      ResourceManager resources;
      resources.Emplace<Counter>(5);
      Local<Counter> local_a(resources);
      Local<Counter> local_b(resources);
      local_b = std::move(local_a);
      CHECK_EQ(local_b.Get().value, 5);
    }
  }

  TEST_CASE("ecs::Local::Insert") {
    SUBCASE("Insert stores the resource and makes it retrievable") {
      ResourceManager resources;
      const Local<Counter> local(resources);
      local.Insert(Counter{42});
      CHECK_EQ(local.Get().value, 42);
    }

    SUBCASE("Insert with lvalue copies the resource") {
      ResourceManager resources;
      const Local<Counter> local(resources);
      Counter cnt{10};
      local.Insert(cnt);
      CHECK_EQ(local.Get().value, 10);
    }

    SUBCASE("Insert replaces an existing resource of the same type") {
      ResourceManager resources;
      const Local<Counter> local(resources);
      local.Insert(Counter{1});
      local.Insert(Counter{2});
      CHECK_EQ(local.Get().value, 2);
    }
  }

  TEST_CASE("ecs::Local::Emplace") {
    SUBCASE("Emplace default-constructs a resource in-place") {
      ResourceManager resources;
      const Local<Counter> local(resources);
      local.Emplace();
      CHECK_EQ(local.Get().value, 0);
    }

    SUBCASE("Emplace forwards constructor arguments") {
      ResourceManager resources;
      const Local<Vec3> local(resources);
      local.Emplace(1.0F, 2.0F, 3.0F);
      CHECK_EQ(local.Get().x, 1.0F);
      CHECK_EQ(local.Get().y, 2.0F);
      CHECK_EQ(local.Get().z, 3.0F);
    }

    SUBCASE("Emplace replaces an existing resource") {
      ResourceManager resources;
      const Local<Counter> local(resources);
      local.Emplace(1);
      local.Emplace(2);
      CHECK_EQ(local.Get().value, 2);
    }

    SUBCASE("Emplace with std::string resource stores the value correctly") {
      ResourceManager resources;
      const Local<Config> local(resources);
      local.Emplace("engine", 3);
      CHECK_EQ(local.Get().name, "engine");
      CHECK_EQ(local.Get().version, 3);
    }
  }

  TEST_CASE("ecs::Local::operator*") {
    SUBCASE("operator* returns reference to stored resource") {
      ResourceManager resources;
      resources.Emplace<Counter>(5);
      const Local<Counter> local(resources);
      const auto& cnt = *local;
      CHECK_EQ(cnt.value, 5);
    }

    SUBCASE(
        "Modification through operator* is reflected in subsequent access") {
      ResourceManager resources;
      resources.Emplace<Counter>(0);
      const Local<Counter> local(resources);
      (*local).value = 42;
      CHECK_EQ(local.Get().value, 42);
    }
  }

  TEST_CASE("ecs::Local::operator->") {
    SUBCASE("operator-> returns pointer to stored resource") {
      ResourceManager resources;
      resources.Emplace<Counter>(3);
      const Local<Counter> local(resources);
      CHECK_EQ(local->value, 3);
    }

    SUBCASE(
        "Modification through operator-> is reflected in subsequent access") {
      ResourceManager resources;
      resources.Emplace<Counter>(0);
      const Local<Counter> local(resources);
      local->value = 77;
      CHECK_EQ(local.Get().value, 77);
    }
  }

  TEST_CASE("ecs::Local::Get") {
    SUBCASE("Get returns reference to stored resource") {
      ResourceManager resources;
      resources.Emplace<Counter>(9);
      const Local<Counter> local(resources);
      CHECK_EQ(local.Get().value, 9);
    }

    SUBCASE("Get returns const reference for const-qualified local") {
      ResourceManager resources;
      resources.Emplace<Counter>(4);
      const Local<const Counter> local(resources);
      CHECK_EQ(local.Get().value, 4);
    }
  }

  TEST_CASE("ecs::Local::Resources") {
    SUBCASE("Resources returns reference to the resource manager") {
      ResourceManager resources;
      const Local<Counter> local(resources);
      CHECK_EQ(local.Resources().Count(), 0);
    }

    SUBCASE("Resources reflects changes made through the local") {
      ResourceManager resources;
      const Local<Counter> local(resources);
      local.Emplace(1);
      CHECK(local.Resources().Has<Counter>());
    }
  }
}
