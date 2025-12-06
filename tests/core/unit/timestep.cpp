#include <doctest/doctest.h>

#include <helios/core/timestep.hpp>

TEST_SUITE("helios::Timestep") {
  TEST_CASE("Timestep::ctor: Construction and conversion") {
    const helios::Timestep timestep(0.016F);
    CHECK_EQ(timestep.Sec(), doctest::Approx(0.016F));
    CHECK_EQ(timestep.MilliSec(), doctest::Approx(16.0F));
    CHECK_EQ(timestep.Framerate(), doctest::Approx(62.5F));
    const float value = static_cast<float>(timestep);
    CHECK_EQ(value, doctest::Approx(0.016F));
  }

}  // TEST_SUITE
