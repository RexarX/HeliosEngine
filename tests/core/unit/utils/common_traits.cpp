#include <doctest/doctest.h>

#include <helios/core/utils/common_traits.hpp>

#include <string>

TEST_SUITE("utils::CommonTraits") {
  TEST_CASE("ArithmeticTrait: concept validation") {
    SUBCASE("Integral types") {
      CHECK(helios::utils::ArithmeticTrait<int>);
      CHECK(helios::utils::ArithmeticTrait<unsigned long>);
      CHECK(helios::utils::ArithmeticTrait<short>);
      CHECK(helios::utils::ArithmeticTrait<long long>);
    }

    SUBCASE("Floating point types") {
      CHECK(helios::utils::ArithmeticTrait<float>);
      CHECK(helios::utils::ArithmeticTrait<double>);
      CHECK(helios::utils::ArithmeticTrait<long double>);
    }

    SUBCASE("Non-arithmetic types") {
      CHECK_FALSE(helios::utils::ArithmeticTrait<std::string>);
      CHECK_FALSE(helios::utils::ArithmeticTrait<void*>);
      CHECK_FALSE(helios::utils::ArithmeticTrait<std::string>);
    }
  }

  TEST_CASE("UniqueTypes: concept validation") {
    SUBCASE("Basic usage") {
      CHECK(helios::utils::UniqueTypes<int>);
      CHECK(helios::utils::UniqueTypes<int, char>);
    }

    SUBCASE("Const and reference types") {
      CHECK(helios::utils::UniqueTypes<int&, char>);
      CHECK(helios::utils::UniqueTypes<const int, char>);
      CHECK(helios::utils::UniqueTypes<const int&, char>);

      CHECK_FALSE(helios::utils::UniqueTypes<int&, int>);
      CHECK_FALSE(helios::utils::UniqueTypes<const int, int>);
      CHECK_FALSE(helios::utils::UniqueTypes<const int&, int>);
    }
  }

}  // TEST_SUITE
