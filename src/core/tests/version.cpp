#include <doctest/doctest.h>

#include <helios/version.hpp>

#include <ostream>
#include <string_view>

using namespace helios;

TEST_SUITE("helios::Version") {
  TEST_CASE("HeaderVersion matches the CMake project version") {
    constexpr Version version = HeaderVersion();
    CHECK_EQ(version.major, HELIOS_VERSION_MAJOR);
    CHECK_EQ(version.minor, HELIOS_VERSION_MINOR);
    CHECK_EQ(version.patch, HELIOS_VERSION_PATCH);
  }

  TEST_CASE("LinkedVersion matches the headers in this build") {
    CHECK_EQ(LinkedVersion(), HeaderVersion());
    CHECK_EQ(std::string_view{VersionString()}, HELIOS_VERSION_STRING);
    CHECK(CompatibleWithHeaders());
  }

  TEST_CASE("Compatible ignores patch and rejects an older minor") {
    constexpr Version header{.major = 1, .minor = 2, .patch = 0};
    CHECK(Compatible(header, {.major = 1, .minor = 2, .patch = 9}));
    CHECK(Compatible(header, {.major = 1, .minor = 3, .patch = 0}));
    CHECK_FALSE(Compatible(header, {.major = 1, .minor = 1, .patch = 0}));
    CHECK_FALSE(Compatible(header, {.major = 2, .minor = 2, .patch = 0}));
  }
}
