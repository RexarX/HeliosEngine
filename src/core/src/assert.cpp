#include <pch.hpp>

#include <helios/stacktrace.hpp>
#include <helios/utils/filesystem.hpp>
#include <helios/utils/format.hpp>

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <format>
#include <source_location>
#include <string>
#include <string_view>

#if defined(__cpp_lib_print) && (__cpp_lib_print >= 202302L)
#include <print>
#endif

#include <helios/assert.hpp>

namespace helios {

namespace {

constexpr size_t kDefaultAssertionStacktraceFrames = 10;

[[nodiscard]] constexpr StacktraceConfig BuildAssertionStacktraceConfig(
    const std::source_location& loc =
        std::source_location::current()) noexcept {
  auto config = StacktraceConfig::FromSourceLocation(loc);
  config.start_frame = 1;
  config.max_frames = kDefaultAssertionStacktraceFrames;
  config.stop_before = "__libc_start_main";
  return config;
}

}  // namespace

namespace details {

std::string FormatAssertionMessage(std::string_view condition,
                                   std::string_view message,
                                   const std::source_location& loc) {
  std::string result;
  result.reserve(256);

  if (!message.empty()) {
    utils::FormatTo(result, "Assertion failed: {} | {}", condition, message);
  } else {
    utils::FormatTo(result, "Assertion failed: {}", condition);
  }

  const std::string_view filename = utils::GetFileName(loc.file_name());
  utils::FormatTo(result, " [{}:{}]", filename, loc.line());

#ifdef HELIOS_ENABLE_STACKTRACE
  try {
    const auto stacktrace =
        Stacktrace::Capture(BuildAssertionStacktraceConfig(loc));
    utils::FormatTo(result, "\n{}", stacktrace.ToString());
  } catch (...) {
    result.append("\nStack trace: <error>");
  }
#endif

  return result;
}

void DefaultAssertionHandler(std::string_view condition,
                             std::string_view message,
                             const std::source_location& loc) noexcept {
  const auto formatted = FormatAssertionMessage(condition, message, loc);

#if defined(__cpp_lib_print) && (__cpp_lib_print >= 202302L)
  std::println(stderr, "{}", formatted);
#else
  std::fprintf(stderr, "%s\n", formatted.c_str());
#endif
  std::fflush(stderr);
  std::abort();
}

}  // namespace details

void AbortWithStacktrace(std::string_view message,
                         const std::source_location& loc) noexcept {
#if defined(__cpp_lib_print) && (__cpp_lib_print >= 202302L)
  std::println(stderr, "\n=== FATAL ERROR ===");
  std::println(stderr, "Message: {}", message);

#ifdef HELIOS_ENABLE_STACKTRACE
  const auto stacktrace =
      Stacktrace::Capture(BuildAssertionStacktraceConfig(loc));
  std::println(stderr, "\n{}", stacktrace.ToString());
#else
  std::println(
      stderr,
      "\nStack trace: <not available - build with HELIOS_ENABLE_STACKTRACE>");
#endif

  std::println(stderr, "===================\n");
#else
  std::fprintf(stderr, "\n=== FATAL ERROR ===\n");
  std::fprintf(stderr, "Message: %.*s\n", static_cast<int>(message.size()),
               message.data());

#ifdef HELIOS_ENABLE_STACKTRACE
  const auto stacktrace =
      Stacktrace::Capture(BuildAssertionStacktraceConfig(loc));
  const std::string text = stacktrace.ToString();
  std::fprintf(stderr, "\n%s\n", text.c_str());
#else
  std::fprintf(
      stderr,
      "\nStack trace: <not available - build with HELIOS_ENABLE_STACKTRACE>\n");
#endif

  std::fprintf(stderr, "===================\n\n");
#endif
  std::fflush(stderr);

  HELIOS_DEBUG_BREAK();
  std::abort();
}

namespace details {

#ifndef _MSC_VER

#if defined(__GNUC__) || defined(__clang__)
[[gnu::weak]]
#endif
bool HasLogPluginHandler() noexcept {
  return false;
}

#if defined(__GNUC__) || defined(__clang__)
[[gnu::weak]]
#endif
void LogPluginAssertionHandler(
    [[maybe_unused]] std::string_view condition,
    [[maybe_unused]] std::string_view message,
    [[maybe_unused]] const std::source_location& loc) noexcept {
}

#endif  // !_MSC_VER

}  // namespace details

void HandleAssertion(std::string_view condition, std::string_view message,
                     const std::source_location& loc) noexcept {
  // Priority 1: Custom user handler
  if (details::g_custom_assertion_handler != nullptr) {
    details::g_custom_assertion_handler(condition, message, loc);
    return;
  }

  // Priority 2: Log plugin handler (if available)
#ifdef _MSC_VER
  if (details::HasLogPluginHandler()) {
    details::LogPluginAssertionHandler(condition, message, loc);
    return;
  }
#else
  if (details::HasLogPluginHandler != nullptr &&
      details::LogPluginAssertionHandler != nullptr &&
      details::HasLogPluginHandler()) {
    details::LogPluginAssertionHandler(condition, message, loc);
    return;
  }
#endif

  // Priority 3: Default handler (printf/println to stderr)
  details::DefaultAssertionHandler(condition, message, loc);
}

}  // namespace helios
