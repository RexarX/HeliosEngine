#include <doctest/doctest.h>

#include <helios/assert.hpp>

#include <string>
#include <string_view>

using namespace helios;

namespace {

// Track assertion handler calls
struct AssertionTracker {
  bool called = false;
  std::string condition;
  std::string message;
  std::source_location location;

  void Reset() {
    called = false;
    condition.clear();
    message.clear();
  }
};

thread_local AssertionTracker g_tracker;

void TestAssertionHandler(std::string_view condition,
                          const std::source_location& loc,
                          std::string_view message) noexcept {
  g_tracker.called = true;
  g_tracker.condition = std::string(condition);
  g_tracker.message = std::string(message);
  g_tracker.location = loc;
}

}  // namespace

TEST_SUITE("helios::Assert") {
  TEST_CASE("SetAssertionHandler: custom handler registration") {
    SUBCASE("Set custom handler") {
      const auto previous = GetAssertionHandler();
      SetAssertionHandler(TestAssertionHandler);
      CHECK_EQ(GetAssertionHandler(), TestAssertionHandler);

      // Restore
      SetAssertionHandler(previous);
    }

    SUBCASE("Get default handler returns nullptr initially") {
      const auto previous = GetAssertionHandler();
      SetAssertionHandler(nullptr);
      CHECK_EQ(GetAssertionHandler(), nullptr);

      // Restore
      SetAssertionHandler(previous);
    }

    SUBCASE("Reset handler to nullptr") {
      const auto previous = GetAssertionHandler();
      SetAssertionHandler(TestAssertionHandler);
      CHECK_NE(GetAssertionHandler(), nullptr);
      SetAssertionHandler(nullptr);
      CHECK_EQ(GetAssertionHandler(), nullptr);

      // Restore
      SetAssertionHandler(previous);
    }
  }

  TEST_CASE("HandleAssertion: handler invocation") {
    const auto previous = GetAssertionHandler();
    SetAssertionHandler(TestAssertionHandler);
    g_tracker.Reset();

    SUBCASE("Custom handler receives correct condition") {
      constexpr auto loc = std::source_location::current();
      details::HandleAssertion("test_condition", loc, "");

      CHECK(g_tracker.called);
      CHECK_EQ(g_tracker.condition, "test_condition");
      CHECK(g_tracker.message.empty());
    }

    SUBCASE("Custom handler receives correct message") {
      constexpr auto loc = std::source_location::current();
      details::HandleAssertion("another_condition", loc, "Test message");

      CHECK(g_tracker.called);
      CHECK_EQ(g_tracker.condition, "another_condition");
      CHECK_EQ(g_tracker.message, "Test message");
    }

    SUBCASE("Custom handler receives source location") {
      constexpr auto loc = std::source_location::current();
      const auto expected_line = loc.line();
      const std::string expected_file = loc.file_name();

      details::HandleAssertion("loc_test", loc, "");

      CHECK(g_tracker.called);
      CHECK_EQ(g_tracker.location.line(), expected_line);
      CHECK_EQ(std::string(g_tracker.location.file_name()), expected_file);
    }

    // Restore
    SetAssertionHandler(previous);
    g_tracker.Reset();
  }

  TEST_CASE("FormatAssertionMessage: message formatting") {
    SUBCASE("Includes condition and source location") {
      constexpr auto loc = std::source_location::current();
      const std::string formatted =
          details::FormatAssertionMessage("test_condition", loc, "");

      CHECK(formatted.starts_with("Assertion failed: test_condition"));
      CHECK(formatted.contains("["));
      CHECK(formatted.contains(":"));
      CHECK(formatted.contains(std::to_string(loc.line())));
    }

    SUBCASE("Includes custom message") {
      constexpr auto loc = std::source_location::current();
      const std::string formatted = details::FormatAssertionMessage(
          "another_condition", loc, "Test message");

      CHECK(formatted.contains(
          "Assertion failed: another_condition | Test message"));
    }

#ifdef HELIOS_ENABLE_STACKTRACE
    SUBCASE("Includes stack trace header") {
      constexpr auto loc = std::source_location::current();
      const std::string formatted =
          details::FormatAssertionMessage("stack_test", loc, "");

      CHECK(formatted.contains("Stack trace:"));
    }
#endif
  }

  // Note: We cannot test HELIOS_ASSERT, HELIOS_INVARIANT, HELIOS_VERIFY macros
  // with failing conditions because they trigger HELIOS_DEBUG_BREAK() which
  // would stop the test process. We can only test that passing conditions work.
  TEST_CASE("HELIOS_ASSERT: passing conditions") {
    SUBCASE("True condition doesn't trigger") {
      const auto previous = GetAssertionHandler();
      SetAssertionHandler(TestAssertionHandler);
      g_tracker.Reset();

      HELIOS_ASSERT(true);
      CHECK_FALSE(g_tracker.called);

      HELIOS_ASSERT(1 == 1);
      CHECK_FALSE(g_tracker.called);

      HELIOS_ASSERT(1 < 2);
      CHECK_FALSE(g_tracker.called);

      SetAssertionHandler(previous);
    }

    SUBCASE("True condition with message doesn't trigger") {
      const auto previous = GetAssertionHandler();
      SetAssertionHandler(TestAssertionHandler);
      g_tracker.Reset();

      HELIOS_ASSERT(true, "Should not trigger");
      CHECK_FALSE(g_tracker.called);

      HELIOS_ASSERT(1 == 1, "Values are equal");
      CHECK_FALSE(g_tracker.called);

      SetAssertionHandler(previous);
    }

    SUBCASE("True condition with formatted message doesn't trigger") {
      const auto previous = GetAssertionHandler();
      SetAssertionHandler(TestAssertionHandler);
      g_tracker.Reset();

      HELIOS_ASSERT(true, "Value is {}", 42);
      CHECK_FALSE(g_tracker.called);

      HELIOS_ASSERT(5 > 3, "Expected {} > {}", 5, 3);
      CHECK_FALSE(g_tracker.called);

      SetAssertionHandler(previous);
    }
  }

  TEST_CASE("HELIOS_VERIFY: passing conditions") {
    SUBCASE("True condition doesn't trigger") {
      const auto previous = GetAssertionHandler();
      SetAssertionHandler(TestAssertionHandler);
      g_tracker.Reset();

      HELIOS_VERIFY(true);
      CHECK_FALSE(g_tracker.called);

      HELIOS_VERIFY(1 == 1);
      CHECK_FALSE(g_tracker.called);

      SetAssertionHandler(previous);
    }

    SUBCASE("True condition with message doesn't trigger") {
      const auto previous = GetAssertionHandler();
      SetAssertionHandler(TestAssertionHandler);
      g_tracker.Reset();

      HELIOS_VERIFY(true, "Should not trigger");
      CHECK_FALSE(g_tracker.called);

      SetAssertionHandler(previous);
    }

    SUBCASE("True condition with formatted message doesn't trigger") {
      const auto previous = GetAssertionHandler();
      SetAssertionHandler(TestAssertionHandler);
      g_tracker.Reset();

      HELIOS_VERIFY(true, "Value is {}", 42);
      CHECK_FALSE(g_tracker.called);

      SetAssertionHandler(previous);
    }
  }

  TEST_CASE("HELIOS_INVARIANT: passing conditions") {
    SUBCASE("True condition doesn't trigger") {
      const auto previous = GetAssertionHandler();
      SetAssertionHandler(TestAssertionHandler);
      g_tracker.Reset();

      HELIOS_INVARIANT(true);
      CHECK_FALSE(g_tracker.called);

      HELIOS_INVARIANT(1 == 1);
      CHECK_FALSE(g_tracker.called);

      SetAssertionHandler(previous);
    }

    SUBCASE("True condition with message doesn't trigger") {
      const auto previous = GetAssertionHandler();
      SetAssertionHandler(TestAssertionHandler);
      g_tracker.Reset();

      HELIOS_INVARIANT(true, "Should not trigger");
      CHECK_FALSE(g_tracker.called);

      SetAssertionHandler(previous);
    }

    SUBCASE("True condition with formatted message doesn't trigger") {
      const auto previous = GetAssertionHandler();
      SetAssertionHandler(TestAssertionHandler);
      g_tracker.Reset();

      HELIOS_INVARIANT(true, "Value is {}", 42);
      CHECK_FALSE(g_tracker.called);

      SetAssertionHandler(previous);
    }
  }

  TEST_CASE("AssertionHandler: type validation") {
    SUBCASE("AssertionHandler is a function pointer type") {
      CHECK(std::is_pointer_v<AssertionHandler>);
      CHECK(std::is_function_v<std::remove_pointer_t<AssertionHandler>>);
    }

    SUBCASE("Handler can be stored and retrieved") {
      const auto previous = GetAssertionHandler();

      AssertionHandler handler = TestAssertionHandler;
      SetAssertionHandler(handler);
      CHECK_EQ(GetAssertionHandler(), handler);

      SetAssertionHandler(previous);
    }
  }

#ifdef HELIOS_ENABLE_ASSERTS
  TEST_CASE("HELIOS_ASSERT: compile-time constant validation") {
    // When asserts are enabled, kEnableAssert should be true
    CHECK(details::kEnableAssert);
  }
#else
  TEST_CASE("HELIOS_ASSERT: disabled mode") {
    // When asserts are disabled, kEnableAssert should be false
    CHECK_FALSE(details::kEnableAssert);
  }
#endif
}  // TEST_SUITE
