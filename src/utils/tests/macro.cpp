#include <doctest/doctest.h>

#include <helios/utils/macro.hpp>

#include <array>
#include <string>

TEST_SUITE("helios::utils::Macro") {
  TEST_CASE("utils::HELIOS_BIT: bit shifting macro") {
    SUBCASE("Bit 0") {
      CHECK_EQ(HELIOS_BIT(0), 1);
    }

    SUBCASE("Bit 1") {
      CHECK_EQ(HELIOS_BIT(1), 2);
    }

    SUBCASE("Bit 2") {
      CHECK_EQ(HELIOS_BIT(2), 4);
    }

    SUBCASE("Bit 3") {
      CHECK_EQ(HELIOS_BIT(3), 8);
    }

    SUBCASE("Bit 4") {
      CHECK_EQ(HELIOS_BIT(4), 16);
    }

    SUBCASE("Bit 7") {
      CHECK_EQ(HELIOS_BIT(7), 128);
    }

    SUBCASE("Bit 8") {
      CHECK_EQ(HELIOS_BIT(8), 256);
    }

    SUBCASE("Bit 15") {
      CHECK_EQ(HELIOS_BIT(15), 32768);
    }

    SUBCASE("Bit 16") {
      CHECK_EQ(HELIOS_BIT(16), 65536);
    }

    SUBCASE("Power of two relationship") {
      for (int i = 0; i < 16; ++i) {
        CHECK_EQ(HELIOS_BIT(i), 1 << i);
      }
    }

    SUBCASE("Usage in bitmask") {
      constexpr int kFlagA = HELIOS_BIT(0);
      constexpr int kFlagB = HELIOS_BIT(1);
      constexpr int kFlagC = HELIOS_BIT(2);

      int flags = kFlagA | kFlagC;

      CHECK((flags & kFlagA) != 0);
      CHECK((flags & kFlagB) == 0);
      CHECK((flags & kFlagC) != 0);
    }
  }

  TEST_CASE("utils::HELIOS_STRINGIFY: stringification macro") {
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

  TEST_CASE("utils::HELIOS_CONCAT: concatenation macro") {
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

  TEST_CASE("utils::HELIOS_ANONYMOUS_VAR: anonymous variable generation") {
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

  TEST_CASE("utils::HELIOS_BIT: constexpr usage") {
    SUBCASE("Can be used in constexpr context") {
      constexpr int bit0 = HELIOS_BIT(0);
      constexpr int bit5 = HELIOS_BIT(5);
      constexpr int bit10 = HELIOS_BIT(10);

      static_assert(bit0 == 1, "Bit 0 should be 1");
      static_assert(bit5 == 32, "Bit 5 should be 32");
      static_assert(bit10 == 1024, "Bit 10 should be 1024");

      CHECK_EQ(bit0, 1);
      CHECK_EQ(bit5, 32);
      CHECK_EQ(bit10, 1024);
    }

    SUBCASE("Can be used in template arguments") {
      std::array<int, HELIOS_BIT(3)> arr;
      CHECK_EQ(arr.size(), 8);
    }

    SUBCASE("Can be used in switch case") {
      int value = 4;
      int result = 0;

      switch (value) {
        case HELIOS_BIT(0):
          result = 1;
          break;
        case HELIOS_BIT(1):
          result = 2;
          break;
        case HELIOS_BIT(2):
          result = 3;
          break;
        default:
          result = 0;
          break;
      }

      CHECK_EQ(result, 3);
    }
  }

  TEST_CASE("utils::Macro combinations") {
    SUBCASE("STRINGIFY and CONCAT together") {
      const char* str = HELIOS_STRINGIFY(HELIOS_CONCAT(hello, _world));
      // The inner CONCAT should be expanded first
      CHECK_EQ(std::string(str), "hello_world");
    }

    SUBCASE("BIT in expressions") {
      constexpr int flags = HELIOS_BIT(0) | HELIOS_BIT(2) | HELIOS_BIT(4);
      CHECK_EQ(flags, 1 + 4 + 16);
      CHECK_EQ(flags, 21);
    }
  }

  TEST_CASE("utils::HELIOS_STRINGIFY_IMPL: direct usage") {
    SUBCASE("Stringify without macro expansion") {
      const char* str = HELIOS_STRINGIFY_IMPL(test);
      CHECK_EQ(std::string(str), "test");
    }
  }

  TEST_CASE("utils::HELIOS_CONCAT_IMPL: direct usage") {
    SUBCASE("Concatenate directly") {
      int HELIOS_CONCAT_IMPL(direct_, concat) = 999;
      CHECK_EQ(direct_concat, 999);
    }
  }

}  // TEST_SUITE
