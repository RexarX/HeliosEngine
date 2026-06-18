#pragma once

#include <helios/compiler/compiler.hpp>
#include <helios/platform/platform.hpp>
#include <helios/utils/macro.hpp>

#include <cstdio>
#include <cstdlib>
#include <format>
#include <functional>
#include <source_location>
#include <string>
#include <string_view>

#if defined(__cpp_lib_print) && (__cpp_lib_print >= 202302L)
#include <print>
#endif

namespace helios {

/**
 * @brief Function signature for custom assertion handlers.
 * @param condition The failed condition as a string
 * @param loc Source location of the assertion
 * @param message Additional message (may be empty)
 */
using AssertionHandler = void (*)(std::string_view condition,
                                  const std::source_location& loc,
                                  std::string_view message) noexcept;

/// @brief Default assertion handler (`nullptr` means use built-in default
/// behavior).
inline constexpr AssertionHandler kDefaultAssertionHandler = nullptr;

namespace details {

#ifdef HELIOS_ENABLE_ASSERTS
inline constexpr bool kEnableAssert = true;
#else
inline constexpr bool kEnableAssert = false;
#endif

/**
 * @brief Formats a complete assertion failure message with source location and
 * stack trace.
 * @param condition The failed condition as a string
 * @param loc Source location of the assertion
 * @param message Additional message (may be empty)
 * @return Formatted assertion message
 */
[[nodiscard]] std::string FormatAssertionMessage(
    std::string_view condition, const std::source_location& loc,
    std::string_view message);

/**
 * @brief Storage for the custom assertion handler.
 * @details When set, this handler is called instead of the default behavior.
 * The handler can be set by calling `SetAssertionHandler()`.
 */
inline AssertionHandler g_custom_assertion_handler = nullptr;

/**
 * @brief Default assertion handler that prints to stderr.
 * @param condition The failed condition as a string
 * @param loc Source location of the assertion
 * @param message Additional message
 */
[[noreturn]] inline void DefaultAssertionHandler(
    std::string_view condition, const std::source_location& loc,
    std::string_view message) noexcept {
  const std::string formatted = FormatAssertionMessage(condition, loc, message);

#if defined(__cpp_lib_print) && (__cpp_lib_print >= 202302L)
  std::println(stderr, "{}", formatted);
#else
  std::fprintf(stderr, "%s\n", formatted.c_str());
#endif
  std::fflush(stderr);
  std::abort();
}

/**
 * @brief Log plugin assertion handler (weak symbol).
 * @details This is defined as a weak symbol that defaults to `nullptr`.
 * When the log plugin is linked, it will provide the real implementation.
 * The implementation should log via the logger system with critical level.
 * @param condition The failed condition as a string
 * @param loc Source location of the assertion
 * @param message Additional message
 */
#ifdef _MSC_VER
// MSVC doesn't support weak symbols, so we use a different approach.
// The log plugin will set this via `SetLogPluginHandler` at initialization.
inline AssertionHandler g_log_plugin_handler = nullptr;

inline void LogPluginAssertionHandler(std::string_view condition,
                                      const std::source_location& loc,
                                      std::string_view message) noexcept {
  if (g_log_plugin_handler) {
    g_log_plugin_handler(condition, loc, message);
  }
}

inline bool HasLogPluginHandler() noexcept {
  return g_log_plugin_handler != nullptr;
}

/**
 * @brief Sets the log plugin handler (called by log plugin at initialization).
 * @param handler The handler function from the log plugin
 */
inline void SetLogPluginHandler(AssertionHandler handler) noexcept {
  g_log_plugin_handler = handler;
}

#else
// GCC/Clang support weak symbols

/**
 * @brief Weak symbol for log plugin handler check.
 * @details The log plugin provides the real implementation.
 * @return true if the log plugin handler is available
 */
[[gnu::weak]] bool HasLogPluginHandler() noexcept;

/**
 * @brief Weak symbol for log plugin assertion handler.
 * @details The log plugin provides the real implementation.
 */
[[gnu::weak]] void LogPluginAssertionHandler(std::string_view condition,
                                             const std::source_location& loc,
                                             std::string_view message) noexcept;

#endif

/**
 * @brief Unified assertion handling function.
 * @details Priority order:
 *   1. Custom handler (if set by user via SetAssertionHandler)
 *   2. Log plugin handler (if log module/plugin is available and linked)
 *   3. Default handler (prints to stderr)
 *
 * @param condition The failed condition as a string
 * @param loc Source location of the assertion
 * @param message Additional message
 */
inline void HandleAssertion(std::string_view condition,
                            const std::source_location& loc,
                            std::string_view message) noexcept {
  // Priority 1: Custom user handler
  if (g_custom_assertion_handler != nullptr) {
    g_custom_assertion_handler(condition, loc, message);
    return;
  }

  // Priority 2: Log plugin handler (if available)
#ifdef _MSC_VER
  if (HasLogPluginHandler()) {
    LogPluginAssertionHandler(condition, loc, message);
    return;
  }
#else
  if (HasLogPluginHandler != nullptr && LogPluginAssertionHandler != nullptr &&
      HasLogPluginHandler()) {
    LogPluginAssertionHandler(condition, loc, message);
    return;
  }
#endif

  // Priority 3: Default handler (printf/println to stderr)
  DefaultAssertionHandler(condition, loc, message);
}

}  // namespace details

/**
 * @brief Sets a custom assertion handler.
 * @details When set, this handler is called for all assertion failures instead
 * of the default behavior. Set to `nullptr` to restore default behavior.
 *
 * Priority order for assertion handling:
 *   1. Custom handler (if set via this function)
 *   2. Log plugin handler (if log plugin is linked)
 *   3. Default handler (prints to stderr)
 *
 * @param handler The custom handler function, or `nullptr` to use default
 *
 * @code
 * // Set custom handler
 * helios::SetAssertionHandler(
 *     [](std::string_view condition, const std::source_location& loc,
 *        std::string_view message) noexcept {
 *   // Your custom logging here
 *   helios::log::Critical("Assert: {} | {} [{}:{}]", condition, message,
 *                         loc.file_name(), loc.line());
 * });
 *
 * // Reset to default behavior
 * helios::SetAssertionHandler(helios::kDefaultAssertionHandler);
 * @endcode
 */
inline void SetAssertionHandler(AssertionHandler handler) noexcept {
  details::g_custom_assertion_handler = handler;
}

/**
 * @brief Gets the current custom assertion handler.
 * @return The current custom handler, or `nullptr` if using default behavior
 */
[[nodiscard]] inline AssertionHandler GetAssertionHandler() noexcept {
  return details::g_custom_assertion_handler;
}

/**
 * @brief Prints a message with stack trace and aborts the program execution.
 * @details Useful for placing in dead code branches or unreachable states.
 * @param message The message to print before aborting
 */
void AbortWithStacktrace(std::string_view message) noexcept;

}  // namespace helios

// NOLINTBEGIN(cppcoreguidelines-avoid-do-while)
// NOLINTBEGIN(cppcoreguidelines-macro-usage)

/**
 * @brief Assertion macro that aborts execution in debug builds.
 * @details Does nothing in release builds.
 * Uses the configured assertion handler (custom -> log plugin -> default).
 * Supports format strings and arguments.
 * @param condition The condition to check
 * @param ... Optional message (can be format string with arguments)
 * @hideinitializer
 */
#ifdef HELIOS_ENABLE_ASSERTS
#define HELIOS_ASSERT(condition, ...)                                \
  do {                                                               \
    if constexpr (::helios::details::kEnableAssert) {                \
      if (HELIOS_EXPECT_FALSE(!(condition))) [[unlikely]] {          \
        if constexpr (sizeof(#__VA_ARGS__) > 1) {                    \
          try {                                                      \
            const std::string msg = std::format("" __VA_ARGS__);     \
            ::helios::details::HandleAssertion(                      \
                #condition, ::std::source_location::current(), msg); \
          } catch (...) {                                            \
            ::helios::details::HandleAssertion(                      \
                #condition, ::std::source_location::current(),       \
                "Formatting error in assertion");                    \
          }                                                          \
        } else {                                                     \
          ::helios::details::HandleAssertion(                        \
              #condition, ::std::source_location::current(), "");    \
        }                                                            \
        HELIOS_DEBUG_BREAK();                                        \
      }                                                              \
    }                                                                \
  } while (false)
#else
#define HELIOS_ASSERT(condition, ...) \
  [[maybe_unused]] static constexpr auto HELIOS_ANONYMOUS_VAR(unused_assert) = 0
#endif

/**
 * @brief Invariant check that asserts in debug builds and logs error in
 * release.
 * @details Provides runtime safety checks that are enforced even in release
 * builds. In debug builds, triggers assertion. In release builds, logs error
 * and continues.
 * @param condition The condition to check
 * @param ... Optional message (can be format string with arguments)
 * @hideinitializer
 */
#ifdef HELIOS_ENABLE_ASSERTS
#define HELIOS_INVARIANT(condition, ...)                           \
  do {                                                             \
    if (HELIOS_EXPECT_FALSE(!(condition))) [[unlikely]] {          \
      if constexpr (sizeof(#__VA_ARGS__) > 1) {                    \
        try {                                                      \
          const std::string msg = std::format("" __VA_ARGS__);     \
          ::helios::details::HandleAssertion(                      \
              #condition, ::std::source_location::current(), msg); \
        } catch (...) {                                            \
          ::helios::details::HandleAssertion(                      \
              #condition, ::std::source_location::current(),       \
              "Formatting error in invariant");                    \
        }                                                          \
      } else {                                                     \
        ::helios::details::HandleAssertion(                        \
            #condition, ::std::source_location::current(), "");    \
      }                                                            \
      HELIOS_DEBUG_BREAK();                                        \
    }                                                              \
  } while (false)
#else
#define HELIOS_INVARIANT(condition, ...)                           \
  do {                                                             \
    if (HELIOS_EXPECT_FALSE(!(condition))) [[unlikely]] {          \
      if constexpr (sizeof(#__VA_ARGS__) > 1) {                    \
        try {                                                      \
          const std::string msg = std::format("" __VA_ARGS__);     \
          ::helios::details::HandleAssertion(                      \
              #condition, ::std::source_location::current(), msg); \
        } catch (...) {                                            \
          ::helios::details::HandleAssertion(                      \
              #condition, ::std::source_location::current(), "");  \
        }                                                          \
      } else {                                                     \
        ::helios::details::HandleAssertion(                        \
            #condition, ::std::source_location::current(), "");    \
      }                                                            \
    }                                                              \
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
#define HELIOS_VERIFY(condition, ...)                              \
  do {                                                             \
    if (HELIOS_EXPECT_FALSE(!(condition))) [[unlikely]] {          \
      if constexpr (sizeof(#__VA_ARGS__) > 1) {                    \
        try {                                                      \
          const std::string msg = std::format("" __VA_ARGS__);     \
          ::helios::details::HandleAssertion(                      \
              #condition, ::std::source_location::current(), msg); \
        } catch (...) {                                            \
          ::helios::details::HandleAssertion(                      \
              #condition, ::std::source_location::current(),       \
              "Formatting error in verify");                       \
        }                                                          \
      } else {                                                     \
        ::helios::details::HandleAssertion(                        \
            #condition, ::std::source_location::current(), "");    \
      }                                                            \
      HELIOS_DEBUG_BREAK();                                        \
    }                                                              \
  } while (false)

// NOLINTEND(cppcoreguidelines-macro-usage)
// NOLINTEND(cppcoreguidelines-avoid-do-while)
