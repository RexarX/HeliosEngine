#include <doctest/doctest.h>

#include <helios/utils/macro.hpp>

#include <array>
#include <string>

TEST_SUITE("helios::utils::Macro") {
  TEST_CASE("helios::utils::HELIOS_STRINGIFY: stringification macro") {
    SUBCASE("Stringify integer literal") {
      const char* str = HELIOS_STRINGIFY(42);
      CHECK_EQ(std::string(str), "42");
    }

    SUBCASE("Stringify identifier") {
      const char* str = HELIOS_STRINGIFY(hello_world);
      CHECK_EQ(std::string(str), "hello_world");
    }

    SUBCASE("Stringify expression") {
      const char* str = HELIOS_STRINGIFY(1 + 2);
      CHECK_EQ(std::string(str), "1 + 2");
    }

    SUBCASE("Stringify type") {
      const char* str = HELIOS_STRINGIFY(int);
      CHECK_EQ(std::string(str), "int");
    }

    SUBCASE("Stringify template-like syntax") {
      const char* str = HELIOS_STRINGIFY(std::vector<int>);
      CHECK_EQ(std::string(str), "std::vector<int>");
    }

    SUBCASE("Stringify with parentheses") {
      const char* str = HELIOS_STRINGIFY((a, b, c));
      CHECK_EQ(std::string(str), "(a, b, c)");
    }

    SUBCASE("Stringify macro argument") {
#define TEST_VALUE 123
      const char* str = HELIOS_STRINGIFY(TEST_VALUE);
      // HELIOS_STRINGIFY should expand the macro first via
      // HELIOS_STRINGIFY_IMPL
      CHECK_EQ(std::string(str), "123");
#undef TEST_VALUE
    }
  }

  TEST_CASE("helios::utils::HELIOS_CONCAT: concatenation macro") {
    SUBCASE("Concatenate identifiers to form variable name") {
      // HELIOS_CONCAT joins tokens together
      int HELIOS_CONCAT(test_, var) = 42;
      CHECK_EQ(test_var, 42);
    }

    SUBCASE("Concatenate to form function name") {
      auto HELIOS_CONCAT(get_, value) = []() { return 100; };
      CHECK_EQ(get_value(), 100);
    }

    SUBCASE("Concatenate numbers") {
      constexpr int HELIOS_CONCAT(var, 1) = 10;
      constexpr int HELIOS_CONCAT(var, 2) = 20;
      constexpr int HELIOS_CONCAT(var, 3) = 30;

      CHECK_EQ(var1, 10);
      CHECK_EQ(var2, 20);
      CHECK_EQ(var3, 30);
    }

    SUBCASE("Concatenate with underscore") {
      int HELIOS_CONCAT(my, _variable) = 99;
      CHECK_EQ(my_variable, 99);
    }
  }

  TEST_CASE(
      "helios::utils::HELIOS_ANONYMOUS_VAR: anonymous variable generation") {
    SUBCASE("Creates unique variables on different lines") {
      // Each HELIOS_ANONYMOUS_VAR on a different line should create a unique
      // variable
      [[maybe_unused]] int HELIOS_ANONYMOUS_VAR(test_) = 1;
      [[maybe_unused]] int HELIOS_ANONYMOUS_VAR(test_) = 2;
      [[maybe_unused]] int HELIOS_ANONYMOUS_VAR(test_) = 3;

      // If they were the same name, this wouldn't compile
      CHECK(true);
    }

    SUBCASE("Variable is usable") {
      [[maybe_unused]] int HELIOS_ANONYMOUS_VAR(counter_) = 42;
      // We can use the variable by knowing the line number, but typically
      // anonymous variables are meant to be unused after initialization
      CHECK(true);
    }

    SUBCASE("Works with different prefixes") {
      [[maybe_unused]] int HELIOS_ANONYMOUS_VAR(a_) = 1;
      [[maybe_unused]] float HELIOS_ANONYMOUS_VAR(b_) = 2.0f;
      [[maybe_unused]] double HELIOS_ANONYMOUS_VAR(c_) = 3.0;

      CHECK(true);
    }

    SUBCASE("Useful for RAII guards") {
      int counter = 0;

      struct Guard {
        int& ref;
        explicit Guard(int& r) : ref(r) { ++ref; }
        ~Guard() { ++ref; }
      };

      CHECK_EQ(counter, 0);
      {
        [[maybe_unused]] Guard HELIOS_ANONYMOUS_VAR(guard_)(counter);
        CHECK_EQ(counter, 1);
      }
      CHECK_EQ(counter, 2);
    }
  }

  TEST_CASE("helios::utils::Macro combinations") {
    SUBCASE("STRINGIFY and CONCAT together") {
      const char* str = HELIOS_STRINGIFY(HELIOS_CONCAT(hello, _world));
      // The inner CONCAT should be expanded first
      CHECK_EQ(std::string(str), "hello_world");
    }
  }

  TEST_CASE("helios::utils::HELIOS_STRINGIFY_IMPL: direct usage") {
    SUBCASE("Stringify without macro expansion") {
      const char* str = HELIOS_STRINGIFY_IMPL(test);
      CHECK_EQ(std::string(str), "test");
    }
  }

  TEST_CASE("helios::utils::HELIOS_CONCAT_IMPL: direct usage") {
    SUBCASE("Concatenate directly") {
      int HELIOS_CONCAT_IMPL(direct_, concat) = 999;
      CHECK_EQ(direct_concat, 999);
    }
  }

}  // TEST_SUITE
