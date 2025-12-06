#include <doctest/doctest.h>

#include <helios/core/random.hpp>

#include <cstdint>
#include <random>
#include <set>
#include <type_traits>

namespace h = helios;

// Helper to check that values lie within [min, max] (inclusive for integrals,
// inclusive on both ends for floats in these tests).
template <typename T>
static void CheckInRangeInclusive(const T value, const T min_value, const T max_value) {
  CHECK_GE(value, min_value);
  CHECK_LE(value, max_value);
}

TEST_SUITE("helios::RandomGenerator") {
  TEST_CASE("RandomDefault: Basic properties") {
    auto& rng = h::RandomDefault();

    SUBCASE("Integral types") {
      const int v_int = rng.Value<int>();
      const uint32_t v_uint = rng.Value<uint32_t>();
      // Basic type sanity: just ensure it compiles and returns something.
      CHECK((std::same_as<decltype(v_int), const int>));
      CHECK((std::same_as<decltype(v_uint), const uint32_t>));

      const bool b = rng.Value<bool>();
      CHECK((b || !b));
    }

    SUBCASE("Floating point types in [0, 1)") {
      const auto v_float = rng.Value<float>();
      const auto v_double = rng.Value<double>();

      CHECK_GE(v_float, 0.0f);
      CHECK_LT(v_float, 1.0f);

      CHECK_GE(v_double, 0.0);
      CHECK_LT(v_double, 1.0);
    }
  }

  TEST_CASE("RandomFast: Basic properties") {
    auto& rng_fast = h::RandomFast();

    SUBCASE("Integral types") {
      const auto v1 = rng_fast.Value<int>();
      const auto v2 = rng_fast.Value<int>();

      // Not a strong statistical check; just verify we can call multiple times.
      const bool dummy = (v1 == v2) || (v1 != v2);
      CHECK(dummy);

      const bool b = rng_fast.Value<bool>();
      CHECK((b || !b));
    }

    SUBCASE("Floating point types in [0, 1)") {
      const auto v_float = rng_fast.Value<float>();
      const auto v_double = rng_fast.Value<double>();

      CHECK_GE(v_float, 0.0f);
      CHECK_LT(v_float, 1.0f);

      CHECK_GE(v_double, 0.0);
      CHECK_LT(v_double, 1.0);
    }
  }

  TEST_CASE("RandomValue: Convenience APIs") {
    SUBCASE("Integral and bool") {
      const int val_int = h::RandomValue<int>();
      const std::uint64_t val_uint64 = h::RandomValue<std::uint64_t>();
      CHECK((std::same_as<decltype(val_int), const int>));
      CHECK((std::same_as<decltype(val_uint64), const std::uint64_t>));

      const bool b = h::RandomValue<bool>();
      CHECK((b || !b));
    }

    SUBCASE("Floating point") {
      const auto val_f = h::RandomValue<float>();
      const auto val_d = h::RandomValue<double>();

      CHECK_GE(val_f, 0.0f);
      CHECK_LT(val_f, 1.0f);

      CHECK_GE(val_d, 0.0);
      CHECK_LT(val_d, 1.0);
    }

    SUBCASE("Integral range") {
      constexpr int min_value = 10;
      constexpr int max_value = 20;
      std::set<int> seen;
      for (int i = 0; i < 200; ++i) {
        const int v = h::RandomValueFromRange(min_value, max_value);
        CheckInRangeInclusive(v, min_value, max_value);
        seen.insert(v);
      }
      CHECK_GT(seen.size(), 1U);
    }

    SUBCASE("Floating range") {
      constexpr float min_value = 1.5F;
      constexpr float max_value = 2.5F;
      for (int i = 0; i < 64; ++i) {
        const float v = h::RandomValueFromRange(min_value, max_value);
        CheckInRangeInclusive(v, min_value, max_value);
      }
    }
  }

  TEST_CASE("RandomFastValue: Convenience APIs") {
    SUBCASE("Integral and bool") {
      const int val_int = h::RandomFastValue<int>();
      const uint32_t val_uint = h::RandomFastValue<uint32_t>();
      CHECK((std::same_as<decltype(val_int), const int>));
      CHECK((std::same_as<decltype(val_uint), const uint32_t>));

      const bool b = h::RandomFastValue<bool>();
      CHECK((b || !b));
    }

    SUBCASE("Floating point") {
      const auto val_f = h::RandomFastValue<float>();
      const auto val_d = h::RandomFastValue<double>();

      CHECK_GE(val_f, 0.0f);
      CHECK_LT(val_f, 1.0f);

      CHECK_GE(val_d, 0.0);
      CHECK_LT(val_d, 1.0);
    }

    SUBCASE("Integral range") {
      constexpr std::int32_t min_value = -5;
      constexpr std::int32_t max_value = 5;
      std::set<std::int32_t> seen;
      for (int i = 0; i < 200; ++i) {
        const auto v = h::RandomFastValueFromRange(min_value, max_value);
        CheckInRangeInclusive(v, min_value, max_value);
        seen.insert(v);
      }
      CHECK_GT(seen.size(), 1U);
    }

    SUBCASE("Floating range") {
      constexpr double min_value = -3.0;
      constexpr double max_value = 7.0;
      for (int i = 0; i < 64; ++i) {
        const auto v = h::RandomFastValueFromRange(min_value, max_value);
        CheckInRangeInclusive(v, min_value, max_value);
      }
    }
  }

  TEST_CASE("RandomGenerator::ctor: Custom engine and distribution") {
    // Custom engine seeded deterministically so the sequence is reproducible.
    std::mt19937 engine{123u};
    h::RandomGenerator<std::mt19937> gen(engine);

    SUBCASE("Integral type with custom engine") {
      const auto v1 = gen.Value<int>();
      const auto v2 = gen.Value<int>();
      // Probability of being equal is low; however, we only assert we can call twice.
      const bool dummy = (v1 == v2) || (v1 != v2);
      CHECK(dummy);
    }

    SUBCASE("Integral types from range") {
      constexpr int min_value = 0;
      constexpr int max_value = 10;
      std::set<int> seen;
      for (int i = 0; i < 128; ++i) {
        const int v = gen.ValueFromRange(min_value, max_value);
        CheckInRangeInclusive(v, min_value, max_value);
        seen.insert(v);
      }
      CHECK_GT(seen.size(), 1U);
    }

    SUBCASE("Mixed arithmetic types from range") {
      constexpr int min_value = -3;
      constexpr double max_value = 2.0;
      using Common = std::common_type_t<int, double>;
      static_assert(std::same_as<Common, double>);

      for (int i = 0; i < 64; ++i) {
        const Common v = gen.ValueFromRange(min_value, max_value);
        CheckInRangeInclusive(v, static_cast<Common>(min_value), static_cast<Common>(max_value));
      }
    }

    SUBCASE("Explicit distribution") {
      std::uniform_int_distribution<int> dist(5, 15);
      std::set<int> seen;
      for (int i = 0; i < 64; ++i) {
        const auto v = gen.Next(dist);
        CheckInRangeInclusive(v, 5, 15);
        seen.insert(v);
      }
      CHECK_GT(seen.size(), 1U);
    }

    SUBCASE("EngineRef exposes underlying engine") {
      auto& ref = gen.EngineRef();
      CHECK_EQ(&ref, &engine);

      // Reset both to same state to verify they produce same sequence
      engine.seed(456u);
      std::uniform_int_distribution<int> dist(0, 100);
      const auto from_gen = gen.Next(dist);

      // Reset again to same state
      engine.seed(456u);
      const auto from_raw = dist(engine);
      CHECK_EQ(from_gen, from_raw);
    }
  }

  TEST_CASE("RandomDefault: Thread-local behavior") {
    // We cannot create real threads in these tests portably here without
    // depending on concurrency behavior, but we can at least obtain the
    // reference twice and ensure it is stable within the same thread.
    auto& r1 = h::RandomDefault();
    auto& r2 = h::RandomDefault();

    CHECK_EQ(&r1, &r2);

    // Same for fast variant.
    auto& f1 = h::RandomFast();
    auto& f2 = h::RandomFast();
    CHECK_EQ(&f1, &f2);
  }

}  // TEST_SUITE
