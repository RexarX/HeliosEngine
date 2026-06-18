#include <doctest/doctest.h>

#include <helios/ecs/resource/param.hpp>
#include <helios/ecs/resource/resource.hpp>

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

}  // namespace

TEST_SUITE("helios::ecs::Res") {
  TEST_CASE("ecs::Res::ctor") {
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

  TEST_CASE("ecs::Res::copy") {
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

  TEST_CASE("ecs::Res::move") {
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

  TEST_CASE("ecs::Res::operator-> (mutable)") {
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

  TEST_CASE("ecs::Res::operator* (mutable)") {
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

  TEST_CASE("ecs::Res::operator-> (const)") {
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

  TEST_CASE("ecs::Res::operator* (const)") {
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
  TEST_CASE("ecs::AsyncRes — alias compiles and works") {
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
