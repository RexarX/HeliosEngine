#include <doctest/doctest.h>

#include <helios/ecs/resource/local_param.hpp>

#include <concepts>
#include <type_traits>
#include <utility>

using namespace helios::ecs;

namespace {

struct Counter {
  int value = 0;
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

}  // namespace

TEST_SUITE("helios::ecs::Local") {
  TEST_CASE("ecs::Local::ctor") {
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

  TEST_CASE("ecs::Local::copy") {
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

  TEST_CASE("ecs::Local::move") {
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

  TEST_CASE("ecs::Local::operator=") {
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

  TEST_CASE("ecs::Local::operator*") {
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

  TEST_CASE("ecs::Local::operator->") {
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

  TEST_CASE("ecs::Local::Get") {
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

  TEST_CASE("ecs::Local::removed manager methods") {
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
