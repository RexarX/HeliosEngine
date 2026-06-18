#include <doctest/doctest.h>

#include <helios/stacktrace.hpp>

#include <algorithm>
#include <source_location>
#include <string>

using namespace helios;

namespace {

[[nodiscard]] auto CaptureForTest(
    std::source_location loc = std::source_location::current()) -> Stacktrace {
  auto config = StacktraceConfig::FromSourceLocation(loc);
  config.start_frame = 1;
  config.max_frames = 8;
  config.stop_before = "__libc_start_main";
  config.exclude_frames_containing = "doctest";
  return Stacktrace::Capture(config);
}

}  // namespace

TEST_SUITE("helios::Stacktrace") {
  TEST_CASE("Stacktrace::Capture: captures trace") {
    const Stacktrace stacktrace = CaptureForTest();
    const std::string trace_text = stacktrace.ToString();

    INFO("Captured stacktrace:\n" << trace_text);
    CHECK_FALSE(trace_text.empty());
    CHECK_NE(trace_text.find("Stack trace:"), std::string::npos);
  }

  TEST_CASE("Stacktrace::Capture: contains source details when available") {
    const Stacktrace stacktrace = CaptureForTest();
    const std::string trace_text = stacktrace.ToString();

    INFO("Captured stacktrace:\n" << trace_text);

#ifdef HELIOS_USE_STL_STACKTRACE
    CHECK_FALSE(trace_text.empty());
#else
    const bool has_source_location =
        trace_text.find(" at ") != std::string::npos;
    const bool has_named_symbol = trace_text.find("??") == std::string::npos ||
                                  trace_text.find(" in /") != std::string::npos;
    const bool has_any_symbol_info = has_source_location || has_named_symbol;
    CHECK(has_any_symbol_info);
#endif
  }

  TEST_CASE("Stacktrace::Capture: max_frames respected") {
    auto config =
        StacktraceConfig::FromSourceLocation(std::source_location::current());
    config.max_frames = 2;
    config.start_frame = 1;
    config.stop_before = "__libc_start_main";

    const auto stacktrace = Stacktrace::Capture(config);
    CHECK_LE(stacktrace.Size(), 2);
  }

  TEST_CASE("Stacktrace::Capture: include/exclude filtering") {
    auto base_config =
        StacktraceConfig::FromSourceLocation(std::source_location::current());
    base_config.max_frames = 16;
    base_config.start_frame = 1;
    base_config.stop_before = "__libc_start_main";

    auto include_config = base_config;
    include_config.include_frames_containing = "Capture";
    const auto included = Stacktrace::Capture(include_config);

    for (const auto& frame : included.Frames()) {
      CHECK_NE(frame.find("Capture"), std::string::npos);
    }

    auto exclude_config = base_config;
    exclude_config.exclude_frames_containing = "doctest";
    const auto excluded = Stacktrace::Capture(exclude_config);

    const bool contains_doctest = std::ranges::any_of(
        excluded.Frames(),
        [](std::string_view frame) { return frame.contains("doctest"); });
    CHECK_FALSE(contains_doctest);
  }

  TEST_CASE("Stacktrace::ToString: header control") {
    const auto stacktrace = CaptureForTest();
    const std::string with_header = stacktrace.ToString(true);
    const std::string without_header = stacktrace.ToString(false);

    CHECK_NE(with_header.find("Stack trace:"), std::string::npos);
    CHECK_EQ(without_header.find("Stack trace:"), std::string::npos);
  }
}
