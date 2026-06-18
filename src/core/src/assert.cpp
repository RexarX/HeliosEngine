#include <pch.hpp>

#include <helios/assert.hpp>
#include <helios/stacktrace.hpp>
#include <helios/utils/filesystem.hpp>

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

namespace {

constexpr size_t kDefaultAssertionStacktraceFrames = 10;

[[nodiscard]] constexpr StacktraceConfig BuildAssertionStacktraceConfig(
    const std::source_location& loc) noexcept {
  auto config = StacktraceConfig::FromSourceLocation(loc);
  config.start_frame = 1;
  config.max_frames = kDefaultAssertionStacktraceFrames;
  config.stop_before = "__libc_start_main";
  return config;
}

}  // namespace

namespace details {

std::string FormatAssertionMessage(std::string_view condition,
                                   const std::source_location& loc,
                                   std::string_view message) {
  std::string result;
  result.reserve(256);

  if (!message.empty()) {
    std::format_to(std::back_inserter(result), "Assertion failed: {} | {}",
                   condition, message);
  } else {
    std::format_to(std::back_inserter(result), "Assertion failed: {}",
                   condition);
  }

  const std::string_view filename = utils::GetFileName(loc.file_name());
  std::format_to(std::back_inserter(result), " [{}:{}]", filename, loc.line());

#ifdef HELIOS_ENABLE_STACKTRACE
  try {
    const auto stacktrace =
        Stacktrace::Capture(BuildAssertionStacktraceConfig(loc));
    std::format_to(std::back_inserter(result), "\n{}", stacktrace.ToString());
  } catch (...) {
    result.append("\nStack trace: <error>");
  }
#endif

  return result;
}

}  // namespace details

void AbortWithStacktrace(std::string_view message) noexcept {
#if defined(__cpp_lib_print) && (__cpp_lib_print >= 202302L)
  std::println(stderr, "\n=== FATAL ERROR ===");
  std::println(stderr, "Message: {}", message);

#ifdef HELIOS_ENABLE_STACKTRACE
  const auto stacktrace = Stacktrace::Capture(
      BuildAssertionStacktraceConfig(std::source_location::current()));
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
  const auto stacktrace = Stacktrace::Capture(
      BuildAssertionStacktraceConfig(std::source_location::current()));
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
    [[maybe_unused]] const std::source_location& loc,
    [[maybe_unused]] std::string_view message) noexcept {
}

#endif  // !_MSC_VER

}  // namespace details

}  // namespace helios
