#include <helios/core_pch.hpp>

#include <helios/core/assert.hpp>

#include <helios/core/core.hpp>

#ifdef HELIOS_ENABLE_STACKTRACE
#include <boost/stacktrace.hpp>
#endif

#include <cstdio>
#include <cstdlib>
#include <source_location>
#include <string>
#include <string_view>

namespace helios {

void AbortWithStacktrace(std::string_view message) noexcept {
#if defined(__cpp_lib_print) && (__cpp_lib_print >= 202302L)
  // Use std::println when available (C++23)
  std::println("\n=== FATAL ERROR ===");
  std::println("Message: {}", message);

#ifdef HELIOS_ENABLE_STACKTRACE
  try {
    const boost::stacktrace::stacktrace stack_trace;
    if (stack_trace.size() > 1) {
      std::println("\nStack trace:");
      for (size_t i = 1; i < stack_trace.size() && i < 32; ++i) {
        std::string entry_str = boost::stacktrace::to_string(stack_trace[i]);
        std::println("  {}: {}", i, entry_str);
      }
    } else {
      std::println("\nStack trace: <empty>");
    }
  } catch (...) {
    std::println("\nStack trace: <error during capture>");
  }
#else
  std::println("\nStack trace: <not available - build with HELIOS_ENABLE_STACKTRACE>");
#endif

  std::println("===================\n");
  std::fflush(stdout);
#else
  // Fallback to fprintf to stderr when std::println is not available
  std::fprintf(stderr, "\n=== FATAL ERROR ===\n");
  std::fprintf(stderr, "Message: %.*s\n", static_cast<int>(message.size()), message.data());

#ifdef HELIOS_ENABLE_STACKTRACE
  try {
    const boost::stacktrace::stacktrace stack_trace;
    if (stack_trace.size() > 1) {
      std::fprintf(stderr, "\nStack trace:\n");
      for (size_t i = 1; i < stack_trace.size() && i < 32; ++i) {
        std::string entry_str = boost::stacktrace::to_string(stack_trace[i]);
        std::fprintf(stderr, "  %zu: %s\n", i, entry_str.c_str());
      }
    } else {
      std::fprintf(stderr, "\nStack trace: <empty>\n");
    }
  } catch (...) {
    std::fprintf(stderr, "\nStack trace: <error during capture>\n");
  }
#else
  std::fprintf(stderr, "\nStack trace: <not available - build with HELIOS_ENABLE_STACKTRACE>\n");
#endif

  std::fprintf(stderr, "===================\n\n");
  std::fflush(stderr);
#endif

  HELIOS_DEBUG_BREAK();
  std::abort();
}

namespace details {

// Weak symbol stub - will be overridden by logger.hpp inline version if included
// On MSVC, the definition is provided by logger.cpp, so we skip this fallback
// On GCC/Clang, we use weak attribute so the inline version from logger.hpp takes precedence
#if !defined(_MSC_VER)
#if defined(__GNUC__) || defined(__clang__)
__attribute__((weak))
#endif
void LogAssertionFailureViaLogger([[maybe_unused]] std::string_view condition,
                                  [[maybe_unused]] const std::source_location& loc,
                                  [[maybe_unused]] std::string_view message) noexcept {
  // Default implementation does nothing.
  // The real implementation is provided by logger.hpp when included.
}
#endif

}  // namespace details

}  // namespace helios
