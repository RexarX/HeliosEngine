#include <doctest/doctest.h>

#include <helios/utils/common_traits.hpp>

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
