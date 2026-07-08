#include <doctest/doctest.h>

#include <helios/utils/common_traits.hpp>

#include <chrono>
#include <cstdint>
#include <string>

namespace {

// Test types for PolymorphicConvertible
struct Base {
  virtual ~Base() = default;
  virtual int Value() const { return 1; }
};

struct Derived : Base {
  int Value() const override { return 2; }
};

struct Unrelated {
  virtual ~Unrelated() = default;
};

struct NonPolymorphic {
  int value = 0;
};

struct AnotherNonPolymorphic {
  float data = 0.0f;
};

struct TestFunctor {
  void operator()() const {}
};

struct ValidClock {
  using rep = int64_t;
  using period = std::nano;
  using duration = std::chrono::duration<rep, period>;
  using time_point = std::chrono::time_point<ValidClock>;

  static time_point now() noexcept { return time_point{}; }
};

struct ClockWithoutNow {
  using rep = int64_t;
  using period = std::nano;
  using duration = std::chrono::duration<rep, period>;
  using time_point = std::chrono::time_point<ClockWithoutNow>;
};

struct ClockWithWrongNow {
  using rep = int64_t;
  using period = std::nano;
  using duration = std::chrono::duration<rep, period>;
  using time_point = std::chrono::time_point<ClockWithWrongNow>;

  static duration now() noexcept { return duration{}; }
};

struct DurationLike {
  using rep = int64_t;
  using period = std::milli;
};

}  // namespace

using namespace helios::utils;

TEST_SUITE("helios::utils::ArithmeticTrait") {
  TEST_CASE("utils::ArithmeticTrait: concept validation") {
    SUBCASE("Integral types") {
      CHECK(ArithmeticTrait<int>);
      CHECK(ArithmeticTrait<unsigned long>);
      CHECK(ArithmeticTrait<short>);
      CHECK(ArithmeticTrait<long long>);
    }

    SUBCASE("Floating point types") {
      CHECK(ArithmeticTrait<float>);
      CHECK(ArithmeticTrait<double>);
      CHECK(ArithmeticTrait<long double>);
    }

    SUBCASE("Non-arithmetic types") {
      CHECK_FALSE(ArithmeticTrait<std::string>);
      CHECK_FALSE(ArithmeticTrait<void*>);
    }
  }
}

TEST_SUITE("helios::utils::ClockTrait") {
  TEST_CASE("utils::ClockTrait: concept validation") {
    SUBCASE("std::chrono clocks satisfy ClockTrait") {
      CHECK(ClockTrait<std::chrono::steady_clock>);
      CHECK(ClockTrait<std::chrono::system_clock>);
      CHECK(ClockTrait<ValidClock>);
    }

    SUBCASE("Non-clock types do NOT satisfy ClockTrait") {
      CHECK_FALSE(ClockTrait<int>);
      CHECK_FALSE(ClockTrait<std::chrono::milliseconds>);
      CHECK_FALSE(ClockTrait<ClockWithoutNow>);
      CHECK_FALSE(ClockTrait<ClockWithWrongNow>);
    }
  }
}

TEST_SUITE("helios::utils::DurationTrait") {
  TEST_CASE("utils::DurationTrait: concept validation") {
    SUBCASE("std::chrono durations satisfy DurationTrait") {
      CHECK(DurationTrait<std::chrono::steady_clock::duration>);
      CHECK(DurationTrait<std::chrono::seconds>);
      CHECK(DurationTrait<std::chrono::milliseconds>);
      CHECK(DurationTrait<std::chrono::duration<double, std::milli>>);
    }

    SUBCASE("Non-duration types do NOT satisfy DurationTrait") {
      CHECK_FALSE(DurationTrait<int>);
      CHECK_FALSE(DurationTrait<std::chrono::steady_clock>);
      CHECK_FALSE(DurationTrait<DurationLike>);
    }
  }
}

TEST_SUITE("helios::utils::UniqueTypes") {
  TEST_CASE("utils::UniqueTypes: concept validation") {
    SUBCASE("Basic usage") {
      CHECK(UniqueTypes<int>);
      CHECK(UniqueTypes<int, char>);
    }

    SUBCASE("Const and reference types") {
      CHECK(UniqueTypes<int&, char>);
      CHECK(UniqueTypes<const int, char>);
      CHECK(UniqueTypes<const int&, char>);

      CHECK_FALSE(UniqueTypes<int&, int>);
      CHECK_FALSE(UniqueTypes<const int, int>);
      CHECK_FALSE(UniqueTypes<const int&, int>);
    }
  }
}

TEST_SUITE("helios::utils::PolymorphicConvertible") {
  TEST_CASE("utils::PolymorphicConvertible: concept validation") {
    SUBCASE("Standard convertible types") {
      CHECK(PolymorphicConvertible<int, double>);
      CHECK(PolymorphicConvertible<float, double>);
      CHECK(PolymorphicConvertible<int, long>);
      CHECK(PolymorphicConvertible<short, int>);
    }

    SUBCASE("Same types") {
      CHECK(PolymorphicConvertible<int, int>);
      CHECK(PolymorphicConvertible<double, double>);
      CHECK(PolymorphicConvertible<Base, Base>);
    }

    SUBCASE("Polymorphic: derived to base") {
      CHECK(PolymorphicConvertible<Derived&, Base&>);
      CHECK(PolymorphicConvertible<Derived*, Base*>);
      CHECK(PolymorphicConvertible<Derived, Base>);
    }

    SUBCASE("Polymorphic: base to derived (allowed by concept)") {
      CHECK(PolymorphicConvertible<Base&, Derived&>);
      CHECK(PolymorphicConvertible<Base, Derived>);
    }

    SUBCASE("Unrelated polymorphic types") {
      CHECK_FALSE(PolymorphicConvertible<Base, Unrelated>);
      CHECK_FALSE(PolymorphicConvertible<Derived, Unrelated>);
      CHECK_FALSE(PolymorphicConvertible<Unrelated, Base>);
    }

    SUBCASE("Non-polymorphic types that are not convertible") {
      CHECK_FALSE(PolymorphicConvertible<std::string, int>);
      CHECK_FALSE(
          PolymorphicConvertible<NonPolymorphic, AnotherNonPolymorphic>);
    }

    SUBCASE("Reference types with polymorphism") {
      CHECK(PolymorphicConvertible<Derived&, Base&>);
      CHECK(PolymorphicConvertible<const Derived&, const Base&>);
    }

    SUBCASE("Pointer conversions (standard convertible)") {
      CHECK(PolymorphicConvertible<Derived*, Base*>);
    }

    SUBCASE("Const correctness") {
      CHECK(PolymorphicConvertible<int, const int>);
      CHECK(PolymorphicConvertible<Derived&, const Base&>);
    }

    SUBCASE("Mixed polymorphic and non-polymorphic") {
      CHECK_FALSE(PolymorphicConvertible<Base, NonPolymorphic>);
      CHECK_FALSE(PolymorphicConvertible<NonPolymorphic, Base>);
    }

    SUBCASE("Empty pack / single type edge cases") {
      CHECK(PolymorphicConvertible<void*, void*>);
    }
  }
}

TEST_SUITE("helios::utils::LambdaTrait") {
  TEST_CASE("utils::LambdaTrait: concept validation") {
    SUBCASE("Non-capture lambda satisfies LambdaTrait") {
      constexpr auto lambda = []() {};
      CHECK(LambdaTrait<decltype(lambda)>);
    }

    SUBCASE("Capturing lambda satisfies LambdaTrait") {
      int capture = 42;
      auto lambda = [capture]() { (void)capture; };
      CHECK(LambdaTrait<decltype(lambda)>);
    }

    SUBCASE("Functor struct does NOT satisfy LambdaTrait") {
      CHECK_FALSE(LambdaTrait<TestFunctor>);
    }

    SUBCASE("Primitive type does NOT satisfy LambdaTrait") {
      CHECK_FALSE(LambdaTrait<int>);
      CHECK_FALSE(LambdaTrait<double>);
    }
  }
}

TEST_SUITE("helios::utils::FunctorTrait") {
  TEST_CASE("utils::FunctorTrait: concept validation") {
    SUBCASE("Functor struct satisfies FunctorTrait") {
      CHECK(FunctorTrait<TestFunctor>);
    }

    SUBCASE("Lambda does NOT satisfy FunctorTrait") {
      constexpr auto lambda = []() {};
      CHECK_FALSE(FunctorTrait<decltype(lambda)>);
    }

    SUBCASE("Primitive type does NOT satisfy FunctorTrait") {
      CHECK_FALSE(FunctorTrait<int>);
      CHECK_FALSE(FunctorTrait<float>);
    }

    SUBCASE("LambdaTrait and FunctorTrait are mutually exclusive") {
      constexpr auto lambda = []() {};
      CHECK_NE(LambdaTrait<TestFunctor>, FunctorTrait<TestFunctor>);
      CHECK_NE(LambdaTrait<decltype(lambda)>, FunctorTrait<decltype(lambda)>);
    }
  }
}
