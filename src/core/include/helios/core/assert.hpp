#pragma once

#include <helios/core/core.hpp>

#include <cstdio>
#include <cstdlib>
#include <format>
#include <source_location>
#include <string>
#include <string_view>

#if defined(__cpp_lib_print) && (__cpp_lib_print >= 202302L)
#include <print>
#endif

namespace helios {

namespace details {

/**
 * @brief Bridge to logger-provided assertion logging.
 * @details The logger implementation provides an inline function named
 * `LogAssertionFailureViaLogger` in `helios::details`.
 * We forward-declare it here so `assert.hpp` can use logger integration when available.
 *
 * If the logger is present in the build, that inline function will be used.
 * If not, calls to `LogAssertionFailure` below will safely fall back to
 * printing to stderr (via std::println) and aborting.
 *
 * @param condition The failed condition as a string
 * @param loc Source location of the assertion
 * @param message Additional message to log
 */
void LogAssertionFailureViaLogger(std::string_view condition, const std::source_location& loc,
                                  std::string_view message) noexcept;

/**
 * @brief Unified assertion logging function used by macros below.
 * @details Tries to forward the assertion to the logger integration function above.
 *
 * If the logger isn't available (or calling it throws), this function
 * simply returns so the caller may perform a fallback print-and-abort.
 *
 * @param condition The failed condition as a string
 * @param loc Source location of the assertion
 * @param message Additional message to log
 */
inline void LogAssertionFailure(std::string_view condition, const std::source_location& loc,
                                std::string_view message) noexcept {
  try {
    // Attempt to forward to the logger integration.
    LogAssertionFailureViaLogger(condition, loc, message);
  } catch (...) {
    // Ignore any exception and allow caller to fallback to printing.
  }
}

/**
 * @brief Fallback assertion handler when logger is not available.
 * @param condition The failed condition as a string
 * @param loc Source location of the assertion
 * @param message Additional message to log
 *
 * This function will first attempt to use the logger via `LogAssertionFailure`.
 * If that doesn't produce visible output, it will print a formatted message using `std::println` and abort.
 *
 * @param condition The failed condition as a string
 * @param loc Source location of the assertion
 * @param message Additional message to log
 */
inline void AssertionFailed(std::string_view condition, const std::source_location& loc,
                            std::string_view message) noexcept {
  // Try to use logger if available (best-effort)
  try {
    LogAssertionFailure(condition, loc, message);
  } catch (...) {
    // Swallow exceptions from logger attempts
  }

  // Fallback printing using std::println (C++23)
#if defined(__cpp_lib_print) && (__cpp_lib_print >= 202302L)
  if (!message.empty()) {
    std::println("Assertion failed: {} | {} [{}:{}]", condition, message, loc.file_name(), loc.line());
  } else {
    std::println("Assertion failed: {}\nFile: {}\nLine: {}\nFunction: {}", condition, loc.file_name(), loc.line(),
                 loc.function_name());
  }
#else
  if (!message.empty()) {
    std::fprintf(stderr, "Assertion failed: %.*s | %.*s [%s:%u]\n", static_cast<int>(condition.size()),
                 condition.data(), static_cast<int>(message.size()), message.data(), loc.file_name(), loc.line());
  } else {
    std::fprintf(stderr, "Assertion failed: %.*s\nFile: %s\nLine: %u\nFunction: %s\n",
                 static_cast<int>(condition.size()), condition.data(), loc.file_name(), loc.line(),
                 loc.function_name());
  }
#endif
}

#ifdef HELIOS_ENABLE_ASSERTS
inline constexpr bool kEnableAssert = true;
#else
inline constexpr bool kEnableAssert = false;
#endif

}  // namespace details

/**
 * @brief Prints a message with stack trace and aborts the program execution.
 * @details Useful for placing in dead code branches or unreachable states.
 * @param message The message to print before aborting
 */
void AbortWithStacktrace(std::string_view message) noexcept;

}  // namespace helios

// Assert macros that work independently of logger but use it by default

/**
 * @brief Assertion macro that aborts execution in debug builds.
 * @details Does nothing in release builds.
 * Attempts to use logger if available, falls back to printing via std::println if not.
 * Supports format strings and arguments.
 * @param condition The condition to check
 * @param ... Optional message (can be format string with arguments)
 * @hideinitializer
 */
#ifdef HELIOS_ENABLE_ASSERTS
#define HELIOS_ASSERT(condition, ...)                                                             \
  do {                                                                                            \
    if constexpr (::helios::details::kEnableAssert) {                                             \
      if (HELIOS_EXPECT_FALSE(!(condition))) [[unlikely]] {                                       \
        constexpr auto loc = std::source_location::current();                                     \
        if constexpr (sizeof(#__VA_ARGS__) > 1) {                                                 \
          try {                                                                                   \
            const std::string msg = std::format("" __VA_ARGS__);                                  \
            ::helios::details::AssertionFailed(#condition, loc, msg);                             \
          } catch (...) {                                                                         \
            ::helios::details::AssertionFailed(#condition, loc, "Formatting error in assertion"); \
          }                                                                                       \
        } else {                                                                                  \
          ::helios::details::AssertionFailed(#condition, loc, "");                                \
        }                                                                                         \
        HELIOS_DEBUG_BREAK();                                                                     \
      }                                                                                           \
    }                                                                                             \
  } while (false)
#else
#define HELIOS_ASSERT(condition, ...) [[maybe_unused]] static constexpr auto HELIOS_ANONYMOUS_VAR(unused_assert) = 0
#endif

/**
 * @brief Invariant check that asserts in debug builds and logs error in release.
 * @details Provides runtime safety checks that are enforced even in release builds.
 * In debug builds, triggers assertion. In release builds, logs error and continues.
 * @param condition The condition to check
 * @param ... Optional message (can be format string with arguments)
 * @hideinitializer
 */
#ifdef HELIOS_ENABLE_ASSERTS
#define HELIOS_INVARIANT(condition, ...)                                                        \
  do {                                                                                          \
    if (HELIOS_EXPECT_FALSE(!(condition))) [[unlikely]] {                                       \
      constexpr auto loc = std::source_location::current();                                     \
      if constexpr (sizeof(#__VA_ARGS__) > 1) {                                                 \
        try {                                                                                   \
          const std::string msg = std::format("" __VA_ARGS__);                                  \
          ::helios::details::AssertionFailed(#condition, loc, msg);                             \
        } catch (...) {                                                                         \
          ::helios::details::AssertionFailed(#condition, loc, "Formatting error in invariant"); \
        }                                                                                       \
      } else {                                                                                  \
        ::helios::details::AssertionFailed(#condition, loc, "");                                \
      }                                                                                         \
      HELIOS_DEBUG_BREAK();                                                                     \
    }                                                                                           \
  } while (false)
#else
#define HELIOS_INVARIANT(condition, ...)                                \
  do {                                                                  \
    if (HELIOS_EXPECT_FALSE(!(condition))) [[unlikely]] {               \
      constexpr auto loc = std::source_location::current();             \
      if constexpr (sizeof(#__VA_ARGS__) > 1) {                         \
        try {                                                           \
          const std::string msg = std::format("" __VA_ARGS__);          \
          ::helios::details::LogAssertionFailure(#condition, loc, msg); \
        } catch (...) {                                                 \
          ::helios::details::LogAssertionFailure(#condition, loc, "");  \
        }                                                               \
      } else {                                                          \
        ::helios::details::LogAssertionFailure(#condition, loc, "");    \
      }                                                                 \
    }                                                                   \
  } while (false)
#endif

/**
 * @brief Verify macro that always checks the condition.
 * @details Similar to assert but runs in both debug and release builds.
 * Useful for validating external input or critical invariants.
 * @param condition The condition to check
 * @param ... Optional message (can be format string with arguments)
 * @hideinitializer
 */
#define HELIOS_VERIFY(condition, ...)                                                        \
  do {                                                                                       \
    if (HELIOS_EXPECT_FALSE(!(condition))) [[unlikely]] {                                    \
      constexpr auto loc = std::source_location::current();                                  \
      if constexpr (sizeof(#__VA_ARGS__) > 1) {                                              \
        try {                                                                                \
          const std::string msg = std::format("" __VA_ARGS__);                               \
          ::helios::details::AssertionFailed(#condition, loc, msg);                          \
        } catch (...) {                                                                      \
          ::helios::details::AssertionFailed(#condition, loc, "Formatting error in verify"); \
        }                                                                                    \
      } else {                                                                               \
        ::helios::details::AssertionFailed(#condition, loc, "");                             \
      }                                                                                      \
      HELIOS_DEBUG_BREAK();                                                                  \
    }                                                                                        \
  } while (false)

/**
 * @brief Verify macro with logger name that always checks the condition.
 * @details Similar to HELIOS_VERIFY but allows specifying a custom logger.
 * @note The custom-logger variant delegates to the same assertion handling;
 * the logger selection is performed by the logger integration function.
 * @param logger_name The name of the logger to use
 * @param condition The condition to check
 * @param ... Optional message (can be format string with arguments)
 * @hideinitializer
 */
#define HELIOS_VERIFY_LOGGER(logger_name, condition, ...)                                    \
  do {                                                                                       \
    if (HELIOS_EXPECT_FALSE(!(condition))) [[unlikely]] {                                    \
      constexpr auto loc = std::source_location::current();                                  \
      if constexpr (sizeof(#__VA_ARGS__) > 1) {                                              \
        try {                                                                                \
          const std::string msg = std::format("" __VA_ARGS__);                               \
          ::helios::details::AssertionFailed(#condition, loc, msg);                          \
        } catch (...) {                                                                      \
          ::helios::details::AssertionFailed(#condition, loc, "Formatting error in verify"); \
        }                                                                                    \
      } else {                                                                               \
        ::helios::details::AssertionFailed(#condition, loc, "");                             \
      }                                                                                      \
      HELIOS_DEBUG_BREAK();                                                                  \
    }                                                                                        \
  } while (false)
