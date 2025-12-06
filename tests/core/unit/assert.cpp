#include <doctest/doctest.h>

#include <helios/core/assert.hpp>

TEST_SUITE("helios::Assert") {
  TEST_CASE("HELIOS_ASSERT: True condition with message") {
    // Should not trigger assertion
    CHECK_NOTHROW(HELIOS_ASSERT(true, "This should not abort"));
    CHECK_NOTHROW(HELIOS_ASSERT(1 == 1, "Math works"));
    CHECK_NOTHROW(HELIOS_ASSERT(42 > 0, "Positive number"));
  }

  TEST_CASE("HELIOS_ASSERT: True condition without message") {
    // Should not trigger assertion
    CHECK_NOTHROW(HELIOS_ASSERT(true));
    CHECK_NOTHROW(HELIOS_ASSERT(1 == 1));
    CHECK_NOTHROW(HELIOS_ASSERT(42 > 0));
  }

  TEST_CASE("HELIOS_INVARIANT: True condition") {
    // Should not trigger
    CHECK_NOTHROW(HELIOS_INVARIANT(true, "Invariant holds"));
    CHECK_NOTHROW(HELIOS_INVARIANT(1 == 1, "Math invariant"));
    CHECK_NOTHROW(HELIOS_INVARIANT(42 > 0, "Positive invariant"));
  }

  TEST_CASE("HELIOS_VERIFY: True condition") {
    // Should not trigger
    CHECK_NOTHROW(HELIOS_VERIFY(true, "Verification passed"));
    CHECK_NOTHROW(HELIOS_VERIFY(1 == 1, "Math verification"));
    CHECK_NOTHROW(HELIOS_VERIFY(42 > 0, "Positive verification"));
  }

  TEST_CASE("HELIOS_VERIFY_LOGGER: True condition") {
    // Should not trigger
    CHECK_NOTHROW(HELIOS_VERIFY_LOGGER("test_logger", true, "Verification passed"));
    CHECK_NOTHROW(HELIOS_VERIFY_LOGGER("test_logger", 1 == 1, "Math verification"));
    CHECK_NOTHROW(HELIOS_VERIFY_LOGGER("test_logger", 42 > 0, "Positive verification"));
  }

  TEST_CASE("kEnableAssert: Flag") {
#ifdef HELIOS_ENABLE_ASSERTS
    CHECK(helios::details::kEnableAssert);
#else
    CHECK_FALSE(helios::details::kEnableAssert);
#endif
  }

  TEST_CASE("Assert: Macros compile in debug and release") {
    // This test mainly checks that macros compile correctly
    // and don't produce warnings in release mode

    bool condition = true;

    HELIOS_ASSERT(condition);
    HELIOS_ASSERT(condition, "message");
    HELIOS_INVARIANT(condition, "invariant");
    HELIOS_VERIFY(condition, "verify");
    HELIOS_VERIFY_LOGGER("logger", condition, "verify with logger");

    // In release builds, these should compile to minimal/no-op code
    // In debug builds, they should perform the actual checks
    CHECK(true);  // Just to make doctest happy
  }

  // Note: We cannot test actual assertion failures in unit tests
  // as they would abort the test process. Assertion failures should
  // be tested manually or in separate integration tests with
  // subprocess management.

  TEST_CASE("AbortWithStacktrace: Function exists") {
    // We can't actually call it (would abort), but we can verify it exists
    // by taking its address
    auto func_ptr = &helios::AbortWithStacktrace;
    CHECK_NE(func_ptr, nullptr);
  }

  TEST_CASE("Assert: Works independently of logger") {
    // Assertions should work even before logger is initialized
    // or if logger fails. This is verified by the fact that
    // assert.hpp doesn't directly include logger.hpp

    bool test_condition = true;
    HELIOS_ASSERT(test_condition);
    HELIOS_ASSERT(test_condition, "Works without logger dependency");

    CHECK(true);  // Test passes if we get here
  }

}  // TEST_SUITE
