#include <doctest/doctest.h>

#include <helios/utils/random.hpp>

#include <algorithm>
#include <cmath>
#include <set>
#include <vector>

using namespace helios::utils;

TEST_SUITE("helios::utils::Random") {
  TEST_CASE("utils::RandomEngine: concept validation") {
    SUBCASE("DefaultRandomEngine satisfies concept") {
      CHECK(RandomEngine<DefaultRandomEngine>);
    }

    SUBCASE("FastRandomEngine satisfies concept") {
      CHECK(RandomEngine<FastRandomEngine>);
    }

    SUBCASE("Standard engines satisfy concept") {
      CHECK(RandomEngine<std::mt19937>);
      CHECK(RandomEngine<std::mt19937_64>);
      CHECK(RandomEngine<std::minstd_rand>);
    }
  }

  TEST_CASE("utils::RandomDeviceSeed: seed generation") {
    SUBCASE("Generates non-zero seed") {
      uint64_t seed = RandomDeviceSeed();
      // While technically 0 is possible, it's astronomically unlikely
      CHECK_NE(seed, 0);
    }

    SUBCASE("Generates different seeds on successive calls") {
      std::set<uint64_t> seeds;
      for (int i = 0; i < 10; ++i) {
        seeds.insert(RandomDeviceSeed());
      }
      // Should have multiple unique seeds
      CHECK_GT(seeds.size(), 1);
    }
  }

  TEST_CASE("utils::MakeDefaultEngine: engine creation") {
    SUBCASE("Creates valid engine") {
      auto engine = MakeDefaultEngine();
      // Engine should produce valid output
      auto value = engine();
      // Just verify it doesn't crash and produces a value
      CHECK_GE(value, DefaultRandomEngine::min());
      CHECK_LE(value, DefaultRandomEngine::max());
    }

    SUBCASE("Different engines produce different sequences") {
      auto engine1 = MakeDefaultEngine();
      auto engine2 = MakeDefaultEngine();

      // Engines seeded differently should produce different values
      auto value1 = engine1();
      auto value2 = engine2();
      // This could theoretically be equal, but astronomically unlikely
      CHECK_NE(value1, value2);
    }
  }

  TEST_CASE("utils::MakeFastEngine: engine creation") {
    SUBCASE("Creates valid engine") {
      auto engine = MakeFastEngine();
      auto value = engine();
      CHECK_GE(value, FastRandomEngine::min());
      CHECK_LE(value, FastRandomEngine::max());
    }
  }

  TEST_CASE("utils::DefaultEngine: thread-local engine") {
    SUBCASE("Returns reference to engine") {
      auto& engine1 = DefaultEngine();
      auto& engine2 = DefaultEngine();
      CHECK_EQ(&engine1, &engine2);
    }

    SUBCASE("Engine produces valid output") {
      auto& engine = DefaultEngine();
      auto value = engine();
      CHECK_GE(value, DefaultRandomEngine::min());
      CHECK_LE(value, DefaultRandomEngine::max());
    }
  }

  TEST_CASE("utils::FastEngineInstance: thread-local fast engine") {
    SUBCASE("Returns reference to engine") {
      auto& engine1 = FastEngineInstance();
      auto& engine2 = FastEngineInstance();
      CHECK_EQ(&engine1, &engine2);
    }

    SUBCASE("Engine produces valid output") {
      auto& engine = FastEngineInstance();
      auto value = engine();
      CHECK_GE(value, FastRandomEngine::min());
      CHECK_LE(value, FastRandomEngine::max());
    }
  }

  TEST_CASE("utils::RandomGenerator::ctor: construction") {
    SUBCASE("Construction from engine reference") {
      auto engine = MakeDefaultEngine();
      RandomGenerator<DefaultRandomEngine> generator(engine);
      CHECK_EQ(&generator.EngineRef(), &engine);
    }

    SUBCASE("Copy construction") {
      auto engine = MakeDefaultEngine();
      RandomGenerator<DefaultRandomEngine> original(engine);
      auto copy = original;
      CHECK_EQ(&copy.EngineRef(), &engine);
    }

    SUBCASE("Move construction") {
      auto engine = MakeDefaultEngine();
      RandomGenerator<DefaultRandomEngine> original(engine);
      auto moved = std::move(original);
      CHECK_EQ(&moved.EngineRef(), &engine);
    }
  }

  TEST_CASE("utils::RandomGenerator::Next: distribution-based generation") {
    auto engine = MakeDefaultEngine();
    RandomGenerator<DefaultRandomEngine> generator(engine);

    SUBCASE("Uniform int distribution") {
      std::uniform_int_distribution<int> dist(0, 100);
      auto value = generator.Next(dist);
      CHECK_GE(value, 0);
      CHECK_LE(value, 100);
    }

    SUBCASE("Uniform real distribution") {
      std::uniform_real_distribution<double> dist(0.0, 1.0);
      auto value = generator.Next(dist);
      CHECK_GE(value, 0.0);
      CHECK_LT(value, 1.0);
    }
  }

  TEST_CASE("utils::RandomGenerator::Value: type-based generation") {
    auto engine = MakeDefaultEngine();
    RandomGenerator<DefaultRandomEngine> generator(engine);

    SUBCASE("Integer types") {
      auto int_val = generator.Value<int>();
      CHECK_GE(int_val, std::numeric_limits<int>::min());
      CHECK_LE(int_val, std::numeric_limits<int>::max());

      auto uint_val = generator.Value<unsigned int>();
      CHECK_GE(uint_val, std::numeric_limits<unsigned int>::min());
      CHECK_LE(uint_val, std::numeric_limits<unsigned int>::max());

      auto short_val = generator.Value<short>();
      CHECK_GE(short_val, std::numeric_limits<short>::min());
      CHECK_LE(short_val, std::numeric_limits<short>::max());
    }

    SUBCASE("Boolean type") {
      std::set<bool> values;
      for (int i = 0; i < 100; ++i) {
        values.insert(generator.Value<bool>());
      }
      // Should have generated both true and false
      CHECK_EQ(values.size(), 2);
    }

    SUBCASE("Floating point types in [0, 1)") {
      for (int i = 0; i < 100; ++i) {
        auto float_val = generator.Value<float>();
        CHECK_GE(float_val, 0.0f);
        CHECK_LT(float_val, 1.0f);

        auto double_val = generator.Value<double>();
        CHECK_GE(double_val, 0.0);
        CHECK_LT(double_val, 1.0);
      }
    }
  }

  TEST_CASE("utils::RandomGenerator::ValueFromRange: range-based generation") {
    auto engine = MakeDefaultEngine();
    RandomGenerator<DefaultRandomEngine> generator(engine);

    SUBCASE("Integer range") {
      for (int i = 0; i < 100; ++i) {
        auto value = generator.ValueFromRange(10, 20);
        CHECK_GE(value, 10);
        CHECK_LE(value, 20);
      }
    }

    SUBCASE("Negative integer range") {
      for (int i = 0; i < 100; ++i) {
        auto value = generator.ValueFromRange(-50, -10);
        CHECK_GE(value, -50);
        CHECK_LE(value, -10);
      }
    }

    SUBCASE("Mixed sign range") {
      for (int i = 0; i < 100; ++i) {
        auto value = generator.ValueFromRange(-10, 10);
        CHECK_GE(value, -10);
        CHECK_LE(value, 10);
      }
    }

    SUBCASE("Floating point range") {
      for (int i = 0; i < 100; ++i) {
        auto value = generator.ValueFromRange(5.0, 15.0);
        CHECK_GE(value, 5.0);
        CHECK_LT(value, 15.0);
      }
    }

    SUBCASE("Mixed types (int and float)") {
      auto value = generator.ValueFromRange(0, 10.5);
      CHECK_GE(value, 0.0);
      CHECK_LT(value, 10.5);
    }

    SUBCASE("Single value range") {
      auto value = generator.ValueFromRange(42, 42);
      CHECK_EQ(value, 42);
    }
  }

  TEST_CASE("utils::RandomGenerator::EngineRef: engine access") {
    auto engine = MakeDefaultEngine();
    RandomGenerator<DefaultRandomEngine> generator(engine);

    auto& ref = generator.EngineRef();
    CHECK_EQ(&ref, &engine);

    // Modify through reference
    auto value_before = ref();
    auto value_after = ref();
    // Sequential calls should give different values
    CHECK_NE(value_before, value_after);
  }

  TEST_CASE("utils::RandomDefault: thread-local default generator") {
    SUBCASE("Returns same generator instance") {
      auto& gen1 = RandomDefault();
      auto& gen2 = RandomDefault();
      CHECK_EQ(&gen1, &gen2);
    }

    SUBCASE("Generator works correctly") {
      auto& gen = RandomDefault();
      auto value = gen.Value<int>();
      // Just verify it produces a value
      CHECK_GE(value, std::numeric_limits<int>::min());
      CHECK_LE(value, std::numeric_limits<int>::max());
    }
  }

  TEST_CASE("utils::RandomFast: thread-local fast generator") {
    SUBCASE("Returns same generator instance") {
      auto& gen1 = RandomFast();
      auto& gen2 = RandomFast();
      CHECK_EQ(&gen1, &gen2);
    }

    SUBCASE("Generator works correctly") {
      auto& gen = RandomFast();
      auto value = gen.Value<int>();
      CHECK_GE(value, std::numeric_limits<int>::min());
      CHECK_LE(value, std::numeric_limits<int>::max());
    }
  }

  TEST_CASE("utils::RandomValue: convenience function") {
    SUBCASE("Integer type") {
      auto value = RandomValue<int>();
      CHECK_GE(value, std::numeric_limits<int>::min());
      CHECK_LE(value, std::numeric_limits<int>::max());
    }

    SUBCASE("Float type in [0, 1)") {
      for (int i = 0; i < 100; ++i) {
        auto value = RandomValue<float>();
        CHECK_GE(value, 0.0f);
        CHECK_LT(value, 1.0f);
      }
    }

    SUBCASE("Bool type") {
      std::set<bool> values;
      for (int i = 0; i < 100; ++i) {
        values.insert(RandomValue<bool>());
      }
      CHECK_EQ(values.size(), 2);
    }
  }

  TEST_CASE("utils::RandomValueFromRange: convenience function") {
    SUBCASE("Integer range") {
      for (int i = 0; i < 100; ++i) {
        auto value = RandomValueFromRange(1, 6);
        CHECK_GE(value, 1);
        CHECK_LE(value, 6);
      }
    }

    SUBCASE("Float range") {
      for (int i = 0; i < 100; ++i) {
        auto value = RandomValueFromRange(0.0, 100.0);
        CHECK_GE(value, 0.0);
        CHECK_LT(value, 100.0);
      }
    }
  }

  TEST_CASE("utils::RandomFastValue: convenience function") {
    SUBCASE("Integer type") {
      auto value = RandomFastValue<int>();
      CHECK_GE(value, std::numeric_limits<int>::min());
      CHECK_LE(value, std::numeric_limits<int>::max());
    }

    SUBCASE("Float type in [0, 1)") {
      for (int i = 0; i < 100; ++i) {
        auto value = RandomFastValue<float>();
        CHECK_GE(value, 0.0f);
        CHECK_LT(value, 1.0f);
      }
    }
  }

  TEST_CASE("utils::RandomFastValueFromRange: convenience function") {
    SUBCASE("Integer range") {
      for (int i = 0; i < 100; ++i) {
        auto value = RandomFastValueFromRange(100, 200);
        CHECK_GE(value, 100);
        CHECK_LE(value, 200);
      }
    }

    SUBCASE("Float range") {
      for (int i = 0; i < 100; ++i) {
        auto value = RandomFastValueFromRange(-10.0f, 10.0f);
        CHECK_GE(value, -10.0f);
        CHECK_LT(value, 10.0f);
      }
    }
  }

  TEST_CASE("utils::Random: statistical distribution") {
    // Very basic statistical test - values should be roughly uniformly
    // distributed
    SUBCASE("Integer range distribution is reasonable") {
      constexpr int kMin = 0;
      constexpr int kMax = 9;
      constexpr int kSamples = 10000;
      constexpr int kBuckets = kMax - kMin + 1;

      std::vector<int> counts(kBuckets, 0);
      for (size_t i = 0; i < static_cast<size_t>(kSamples); ++i) {
        const int value = RandomValueFromRange(kMin, kMax);
        ++counts[static_cast<size_t>(value - kMin)];
      }

      // Each bucket should have roughly kSamples / kBuckets counts
      // Allow for reasonable variance (within 50% of expected)
      const int expected = kSamples / kBuckets;
      const int tolerance = expected / 2;

      for (size_t i = 0; i < static_cast<size_t>(kBuckets); ++i) {
        CHECK_GT(counts[i], expected - tolerance);
        CHECK_LT(counts[i], expected + tolerance);
      }
    }

    SUBCASE("Boolean distribution is roughly 50/50") {
      constexpr int kSamples = 10000;
      int true_count = 0;

      for (int i = 0; i < kSamples; ++i) {
        if (RandomValue<bool>()) {
          ++true_count;
        }
      }

      // Should be roughly 50% (allow 10% variance)
      const int tolerance = kSamples / 10;
      CHECK_GT(true_count, kSamples / 2 - tolerance);
      CHECK_LT(true_count, kSamples / 2 + tolerance);
    }
  }

  TEST_CASE("utils::Random: type aliases") {
    SUBCASE("DefaultRandomGenerator is correct alias") {
      CHECK(std::is_same_v<DefaultRandomGenerator,
                           RandomGenerator<DefaultRandomEngine>>);
    }

    SUBCASE("FastRandomGeneratorType is correct alias") {
      CHECK(std::is_same_v<FastRandomGeneratorType,
                           RandomGenerator<FastRandomEngine>>);
    }
  }

  TEST_CASE("utils::RandomGenerator: different arithmetic types") {
    auto engine = MakeDefaultEngine();
    RandomGenerator<DefaultRandomEngine> generator(engine);

    SUBCASE("short type") {
      auto value = generator.Value<short>();
      CHECK_GE(value, std::numeric_limits<short>::min());
      CHECK_LE(value, std::numeric_limits<short>::max());
    }

    SUBCASE("unsigned short type") {
      auto value = generator.Value<unsigned short>();
      CHECK_GE(value, std::numeric_limits<unsigned short>::min());
      CHECK_LE(value, std::numeric_limits<unsigned short>::max());
    }

    SUBCASE("long type") {
      auto value = generator.Value<long>();
      CHECK_GE(value, std::numeric_limits<long>::min());
      CHECK_LE(value, std::numeric_limits<long>::max());
    }

    SUBCASE("long long type") {
      auto value = generator.Value<long long>();
      CHECK_GE(value, std::numeric_limits<long long>::min());
      CHECK_LE(value, std::numeric_limits<long long>::max());
    }

    SUBCASE("long double type") {
      for (int i = 0; i < 100; ++i) {
        auto value = generator.Value<long double>();
        CHECK_GE(value, 0.0L);
        CHECK_LT(value, 1.0L);
      }
    }
  }

}  // TEST_SUITE
