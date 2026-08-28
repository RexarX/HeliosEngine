#include <doctest/doctest.h>

#include <helios/input/axis.hpp>

#include <cstdint>

using namespace helios::input;

namespace {

enum class TestAxis : uint8_t {
  kX,
  kY,
  kTrigger,
  kCount,
};

}  // namespace

TEST_SUITE("helios::input::Axis") {
  TEST_CASE("helios::input::Axis::Set") {
    SUBCASE("Stores the axis value") {
      Axis<TestAxis> axes;
      axes.Set(TestAxis::kX, 0.5F);

      CHECK_EQ(axes.Get(TestAxis::kX), doctest::Approx(0.5F));
      CHECK_EQ(axes.Get(TestAxis::kY), doctest::Approx(0.0F));
    }
  }

  TEST_CASE("helios::input::Axis::SetWithDeadzone") {
    SUBCASE("Zeros values strictly inside the deadzone") {
      Axis<TestAxis> axes;
      axes.SetWithDeadzone(TestAxis::kX, 0.09F, 0.1F);
      CHECK_EQ(axes.Get(TestAxis::kX), doctest::Approx(0.0F));
    }

    SUBCASE("Keeps values on the deadzone boundary") {
      Axis<TestAxis> axes;
      axes.SetWithDeadzone(TestAxis::kX, 0.1F, 0.1F);
      CHECK_EQ(axes.Get(TestAxis::kX), doctest::Approx(0.1F));
    }

    SUBCASE("Keeps values outside the deadzone") {
      Axis<TestAxis> axes;
      axes.SetWithDeadzone(TestAxis::kX, -0.5F, 0.1F);
      CHECK_EQ(axes.Get(TestAxis::kX), doctest::Approx(-0.5F));
    }
  }

  TEST_CASE("helios::input::Axis::Clear") {
    SUBCASE("Zeros every axis") {
      Axis<TestAxis> axes;
      axes.Set(TestAxis::kX, 1.0F);
      axes.Set(TestAxis::kTrigger, 0.8F);
      axes.Clear();

      CHECK_EQ(axes.Get(TestAxis::kX), doctest::Approx(0.0F));
      CHECK_EQ(axes.Get(TestAxis::kTrigger), doctest::Approx(0.0F));
    }
  }

  TEST_CASE("helios::input::Axis::Get") {
    SUBCASE("Returns zero for an unset axis") {
      const Axis<TestAxis> axes;
      CHECK_EQ(axes.Get(TestAxis::kY), doctest::Approx(0.0F));
    }
  }

  TEST_CASE("helios::input::Axis::Values") {
    SUBCASE("Exposes a contiguous span of kSize values") {
      Axis<TestAxis> axes;
      axes.Set(TestAxis::kTrigger, 0.25F);

      const auto values = axes.Values();
      CHECK_EQ(values.size(), Axis<TestAxis>::kSize);
      CHECK_EQ(values[static_cast<size_t>(TestAxis::kTrigger)],
               doctest::Approx(0.25F));
    }
  }
}

TEST_SUITE("helios::input::ApplyLinearDeadzone") {
  TEST_CASE("helios::input::ApplyLinearDeadzone") {
    SUBCASE("Zeros values strictly inside the deadzone") {
      CHECK_EQ(ApplyLinearDeadzone(0.09F, 0.1F, 1.0F, true),
               doctest::Approx(0.0F));
    }

    SUBCASE("Rescales values between deadzone and livezone") {
      CHECK_EQ(ApplyLinearDeadzone(0.55F, 0.1F, 1.0F, true),
               doctest::Approx(0.5F));
    }

    SUBCASE("Saturates at the livezone when rescaling") {
      CHECK_EQ(ApplyLinearDeadzone(0.95F, 0.1F, 0.9F, true),
               doctest::Approx(1.0F));
    }

    SUBCASE("Keeps the raw value when rescale is disabled") {
      CHECK_EQ(ApplyLinearDeadzone(0.5F, 0.1F, 1.0F, false),
               doctest::Approx(0.5F));
    }

    SUBCASE("Treats the whole range as dead when livezone is not greater") {
      CHECK_EQ(ApplyLinearDeadzone(1.0F, 0.5F, 0.5F, true),
               doctest::Approx(0.0F));
    }

    SUBCASE("Uses AxisFilter settings") {
      const AxisFilter filter{
          .deadzone = 0.2F, .livezone = 1.0F, .rescale = true};
      CHECK_EQ(ApplyLinearDeadzone(0.6F, filter), doctest::Approx(0.5F));
    }
  }
}

TEST_SUITE("helios::input::ApplyRadialDeadzone") {
  TEST_CASE("helios::input::ApplyRadialDeadzone") {
    SUBCASE("Zeros vectors inside the circular deadzone") {
      const auto [x, y] = ApplyRadialDeadzone(0.05F, 0.05F, 0.15F, 1.0F, true);
      CHECK_EQ(x, doctest::Approx(0.0F));
      CHECK_EQ(y, doctest::Approx(0.0F));
    }

    SUBCASE("Does not zero a diagonal that an axial deadzone would") {
      const auto [x, y] = ApplyRadialDeadzone(0.09F, 0.09F, 0.1F, 1.0F, true);
      CHECK_GT(x, 0.0F);
      CHECK_GT(y, 0.0F);
      CHECK_EQ(x, doctest::Approx(y));
    }

    SUBCASE("Rescales onto the unit circle at the livezone") {
      const auto [x, y] = ApplyRadialDeadzone(1.0F, 0.0F, 0.15F, 1.0F, true);
      CHECK_EQ(x, doctest::Approx(1.0F));
      CHECK_EQ(y, doctest::Approx(0.0F));
    }

    SUBCASE("Treats the whole range as dead when livezone is not greater") {
      const auto [x, y] = ApplyRadialDeadzone(1.0F, 0.0F, 0.5F, 0.4F, true);
      CHECK_EQ(x, doctest::Approx(0.0F));
      CHECK_EQ(y, doctest::Approx(0.0F));
    }

    SUBCASE("Uses AxisFilter settings") {
      const AxisFilter filter{
          .deadzone = 0.0F, .livezone = 1.0F, .rescale = true};
      const auto [x, y] = ApplyRadialDeadzone(0.5F, 0.0F, filter);
      CHECK_EQ(x, doctest::Approx(0.5F));
      CHECK_EQ(y, doctest::Approx(0.0F));
    }
  }
}

TEST_SUITE("helios::input::RemapTrigger") {
  TEST_CASE("helios::input::RemapTrigger") {
    SUBCASE("Maps GLFW rest to zero") {
      CHECK_EQ(RemapTrigger(-1.0F, -1.0F), doctest::Approx(0.0F));
    }

    SUBCASE("Maps fully pressed to one") {
      CHECK_EQ(RemapTrigger(1.0F, -1.0F), doctest::Approx(1.0F));
    }

    SUBCASE("Uses a calibrated rest center") {
      CHECK_EQ(RemapTrigger(-0.8F, -0.8F), doctest::Approx(0.0F));
      CHECK_EQ(RemapTrigger(1.0F, -0.8F), doctest::Approx(1.0F));
    }

    SUBCASE("Returns zero when center is not below one") {
      CHECK_EQ(RemapTrigger(0.0F, 1.0F), doctest::Approx(0.0F));
    }
  }
}
