#include <iterator>
#include <memory_resource>
#include <string>
#include <string_view>

#include <doctest/doctest.h>

#include <helios/utils/format.hpp>

using namespace helios::utils;

TEST_SUITE("helios::utils::FormatTo") {
  TEST_CASE("helios::utils::FormatTo") {
    SUBCASE("Appends formatted text to an empty string") {
      std::string out;
      auto it = FormatTo(out, "{} + {} = {}", 1, 2, 3);

      CHECK_EQ(out, "1 + 2 = 3");
      CHECK_EQ(it, out.end());
    }

    SUBCASE("Appends formatted text to a non-empty string") {
      std::string out = "prefix-";
      FormatTo(out, "{}", 42);
      CHECK_EQ(out, "prefix-42");
    }

    SUBCASE("Appending an empty format string leaves the original unchanged") {
      std::string out = "unchanged";
      FormatTo(out, "");
      CHECK_EQ(out, "unchanged");
    }

    SUBCASE("Returned iterator points at the end of the appended text") {
      std::string out = "abc";
      auto it = FormatTo(out, "{}", 123);

      CHECK_EQ(it, out.end());
      CHECK_EQ(std::string_view{out.begin() + 3, out.end()}, "123");
    }

    SUBCASE("Multiple successive calls keep appending") {
      std::string out;

      FormatTo(out, "{}-", 1);
      FormatTo(out, "{}-", 2);
      FormatTo(out, "{}", 3);

      CHECK_EQ(out, "1-2-3");
    }
  }
}

TEST_SUITE("helios::utils::FormatWith") {
  TEST_CASE("helios::utils::FormatWith") {
    SUBCASE("Formats a simple string with arguments") {
      auto result =
          FormatWith(std::pmr::get_default_resource(), "{} + {} = {}", 1, 2, 3);
      CHECK_EQ(result, "1 + 2 = 3");
    }

    SUBCASE("Formats an empty format string") {
      auto result = FormatWith(std::pmr::get_default_resource(), "");
      CHECK(result.empty());
    }

    SUBCASE("Result uses the provided memory resource") {
      std::pmr::monotonic_buffer_resource resource;
      auto result = FormatWith(&resource, "{}", 7);

      CHECK_EQ(result.get_allocator().resource(), &resource);
      CHECK_EQ(result, "7");
    }
  }
}
