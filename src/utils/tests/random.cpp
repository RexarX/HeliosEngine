#include <limits>
#include <random>
#include <set>
#include <thread>
#include <type_traits>
#include <vector>

#include <doctest/doctest.h>

#include <helios/utils/random.hpp>

using namespace helios::utils;

TEST_SUITE("helios::utils::Random") {
  TEST_CASE("helios::utils::RandomEngine: concept validation") {
    SUBCASE("DefaultRandomEngine satisfies concept") {
      CHECK(RandomEngine<DefaultRandomEngine>);
    }

    SUBCASE("FastRandomEngine satisfies concept") {
      CHECK(RandomEngine<FastRandomEngine>);
    }

    SUBCASE("SmallRandomEngine satisfies concept") {
      CHECK(RandomEngine<SmallRandomEngine>);
    }

    SUBCASE("LongPeriodRandomEngine satisfies concept") {
      CHECK(RandomEngine<LongPeriodRandomEngine>);
    }

    SUBCASE("Named xoshiro engines satisfy concept") {
      CHECK(RandomEngine<Xoshiro256StarStar>);
      CHECK(RandomEngine<Xoshiro256PlusPlus>);
      CHECK(RandomEngine<Xoshiro256Plus>);
      CHECK(RandomEngine<Xoroshiro128PlusPlus>);
      CHECK(RandomEngine<Xoroshiro128StarStar>);
      CHECK(RandomEngine<Xoroshiro128Plus>);
      CHECK(RandomEngine<Xoshiro128StarStar>);
      CHECK(RandomEngine<Xoshiro128PlusPlus>);
      CHECK(RandomEngine<Xoshiro512StarStar>);
      CHECK(RandomEngine<Xoroshiro1024StarStar>);
    }

    SUBCASE("Standard engines satisfy concept") {
      CHECK(RandomEngine<std::mt19937>);
      CHECK(RandomEngine<std::mt19937_64>);
      CHECK(RandomEngine<std::minstd_rand>);
    }
  }

  TEST_CASE("helios::utils::Random: engine role aliases") {
    SUBCASE("Default is xoshiro256**") {
      CHECK(std::is_same_v<DefaultRandomEngine, Xoshiro256StarStar>);
    }

    SUBCASE("Fast is xoroshiro128++") {
      CHECK(std::is_same_v<FastRandomEngine, Xoroshiro128PlusPlus>);
    }

    SUBCASE("Small is xoshiro128**") {
      CHECK(std::is_same_v<SmallRandomEngine, Xoshiro128StarStar>);
    }

    SUBCASE("Long-period is xoshiro512**") {
      CHECK(std::is_same_v<LongPeriodRandomEngine, Xoshiro512StarStar>);
    }
  }

  TEST_CASE("helios::utils::RandomDeviceSeed: process entropy") {
    SUBCASE("Generates non-zero seed") {
      const uint64_t seed = RandomDeviceSeed();
      CHECK_NE(seed, 0);
    }

    SUBCASE("Returns the same value on successive calls") {
      const uint64_t first = RandomDeviceSeed();
      const uint64_t second = RandomDeviceSeed();
      CHECK_EQ(first, second);
    }
  }

  TEST_CASE("helios::utils::MixThreadSeed: unique mixed seeds") {
    SUBCASE("Generates non-zero seed") {
      CHECK_NE(MixThreadSeed(), 0);
    }

    SUBCASE("Successive calls produce distinct seeds") {
      std::set<uint64_t> seeds;
      for (int i = 0; i < 16; ++i) {
        seeds.insert(MixThreadSeed());
      }
      CHECK_EQ(seeds.size(), 16);
    }

    SUBCASE("Differs from the cached process seed") {
      CHECK_NE(MixThreadSeed(), RandomDeviceSeed());
    }
  }

  TEST_CASE("helios::utils::MakeEngine: engine creation") {
    SUBCASE("Default template argument creates DefaultRandomEngine") {
      auto engine = MakeEngine();
      CHECK(std::is_same_v<decltype(engine), DefaultRandomEngine>);
      const auto value = engine();
      CHECK_GE(value, DefaultRandomEngine::min());
      CHECK_LE(value, DefaultRandomEngine::max());
    }

    SUBCASE("Explicit seed is repeatable") {
      constexpr DefaultRandomEngine::result_type kSeed = 0x123456789abcdef0ULL;
      auto engine1 = MakeEngine(kSeed);
      auto engine2 = MakeEngine(kSeed);
      CHECK_EQ(engine1(), engine2());
      CHECK_EQ(engine1(), engine2());
    }

    SUBCASE("Unseeded engines produce different sequences") {
      auto engine1 = MakeEngine();
      auto engine2 = MakeEngine();
      CHECK_NE(engine1(), engine2());
    }

    SUBCASE("Fast engine template argument") {
      auto engine = MakeEngine<FastRandomEngine>();
      CHECK(std::is_same_v<decltype(engine), FastRandomEngine>);
      const auto value = engine();
      CHECK_GE(value, FastRandomEngine::min());
      CHECK_LE(value, FastRandomEngine::max());
    }

    SUBCASE("Small 32-bit engine") {
      auto engine = MakeEngine<SmallRandomEngine>();
      CHECK(std::is_same_v<decltype(engine)::result_type, uint32_t>);
      const auto value = engine();
      CHECK_GE(value, SmallRandomEngine::min());
      CHECK_LE(value, SmallRandomEngine::max());
    }

    SUBCASE("Long-period engine") {
      auto engine = MakeEngine<LongPeriodRandomEngine>();
      const auto value = engine();
      CHECK_GE(value, LongPeriodRandomEngine::min());
      CHECK_LE(value, LongPeriodRandomEngine::max());
    }
  }

  TEST_CASE("helios::utils::MakeDefaultEngine: engine creation") {
    SUBCASE("Creates valid engine") {
      auto engine = MakeDefaultEngine();
      const auto value = engine();
      CHECK_GE(value, DefaultRandomEngine::min());
      CHECK_LE(value, DefaultRandomEngine::max());
    }

    SUBCASE("Different engines produce different sequences") {
      auto engine1 = MakeDefaultEngine();
      auto engine2 = MakeDefaultEngine();
      CHECK_NE(engine1(), engine2());
    }
  }

  TEST_CASE("helios::utils::MakeFastEngine: engine creation") {
    SUBCASE("Creates valid engine") {
      auto engine = MakeFastEngine();
      const auto value = engine();
      CHECK_GE(value, FastRandomEngine::min());
      CHECK_LE(value, FastRandomEngine::max());
    }
  }

  TEST_CASE("helios::utils::ThreadLocalEngine: per-thread instance") {
    SUBCASE("Returns the same instance on the same thread") {
      auto& engine1 = ThreadLocalEngine<DefaultRandomEngine>();
      auto& engine2 = ThreadLocalEngine<DefaultRandomEngine>();
      CHECK_EQ(&engine1, &engine2);
    }

    SUBCASE("Different engine types have independent instances") {
      auto& def = ThreadLocalEngine<DefaultRandomEngine>();
      auto& fast = ThreadLocalEngine<FastRandomEngine>();
      CHECK_NE(static_cast<void*>(&def), static_cast<void*>(&fast));
    }
  }

  TEST_CASE("helios::utils::DefaultEngine: thread-local engine") {
    SUBCASE("Returns reference to engine") {
      auto& engine1 = DefaultEngine();
      auto& engine2 = DefaultEngine();
      CHECK_EQ(&engine1, &engine2);
    }

    SUBCASE("Engine produces valid output") {
      auto& engine = DefaultEngine();
      const auto value = engine();
      CHECK_GE(value, DefaultRandomEngine::min());
      CHECK_LE(value, DefaultRandomEngine::max());
    }

    SUBCASE("Matches ThreadLocalEngine<DefaultRandomEngine>") {
      CHECK_EQ(&DefaultEngine(), &ThreadLocalEngine<DefaultRandomEngine>());
    }
  }

  TEST_CASE("helios::utils::FastEngine: thread-local fast engine") {
    SUBCASE("Returns reference to engine") {
      auto& engine1 = FastEngine();
      auto& engine2 = FastEngine();
      CHECK_EQ(&engine1, &engine2);
    }

    SUBCASE("Engine produces valid output") {
      auto& engine = FastEngine();
      const auto value = engine();
      CHECK_GE(value, FastRandomEngine::min());
      CHECK_LE(value, FastRandomEngine::max());
    }
  }

  TEST_CASE(
      "helios::utils::MakeEngine: parallel threads get distinct streams") {
    SUBCASE("Two threads produce different first values") {
      DefaultRandomEngine::result_type value_a = 0;
      DefaultRandomEngine::result_type value_b = 0;

      std::thread thread_a{[&] { value_a = MakeDefaultEngine()(); }};
      std::thread thread_b{[&] { value_b = MakeDefaultEngine()(); }};
      thread_a.join();
      thread_b.join();

      CHECK_NE(value_a, value_b);
    }

    SUBCASE("Thread-local engines are distinct objects") {
      DefaultRandomEngine* ptr_a = nullptr;
      DefaultRandomEngine* ptr_b = nullptr;

      std::thread thread_a{[&] { ptr_a = &DefaultEngine(); }};
      std::thread thread_b{[&] { ptr_b = &DefaultEngine(); }};
      thread_a.join();
      thread_b.join();

      CHECK_NE(ptr_a, ptr_b);
    }
  }

  TEST_CASE("helios::utils::RandomGenerator::ctor: construction") {
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

  TEST_CASE(
      "helios::utils::RandomGenerator::Next: distribution-based generation") {
    auto engine = MakeDefaultEngine();
    RandomGenerator<DefaultRandomEngine> generator(engine);

    SUBCASE("Uniform int distribution") {
      std::uniform_int_distribution<int> dist(0, 100);
      const auto value = generator.Next(dist);
      CHECK_GE(value, 0);
      CHECK_LE(value, 100);
    }

    SUBCASE("Uniform real distribution") {
      std::uniform_real_distribution<double> dist(0.0, 1.0);
      const auto value = generator.Next(dist);
      CHECK_GE(value, 0.0);
      CHECK_LT(value, 1.0);
    }
  }

  TEST_CASE("helios::utils::RandomGenerator::Value: type-based generation") {
    auto engine = MakeDefaultEngine();
    RandomGenerator<DefaultRandomEngine> generator(engine);

    SUBCASE("Integer types") {
      const auto int_val = generator.Value<int>();
      CHECK_GE(int_val, std::numeric_limits<int>::min());
      CHECK_LE(int_val, std::numeric_limits<int>::max());

      const auto uint_val = generator.Value<unsigned int>();
      CHECK_GE(uint_val, std::numeric_limits<unsigned int>::min());
      CHECK_LE(uint_val, std::numeric_limits<unsigned int>::max());

      const auto short_val = generator.Value<short>();
      CHECK_GE(short_val, std::numeric_limits<short>::min());
      CHECK_LE(short_val, std::numeric_limits<short>::max());
    }

    SUBCASE("Boolean type") {
      std::set<bool> values;
      for (int i = 0; i < 100; ++i) {
        values.insert(generator.Value<bool>());
      }
      CHECK_EQ(values.size(), 2);
    }

    SUBCASE("Floating point types in [0, 1)") {
      for (int i = 0; i < 100; ++i) {
        const auto float_val = generator.Value<float>();
        CHECK_GE(float_val, 0.0F);
        CHECK_LT(float_val, 1.0F);

        const auto double_val = generator.Value<double>();
        CHECK_GE(double_val, 0.0);
        CHECK_LT(double_val, 1.0);
      }
    }
  }

  TEST_CASE(
      "helios::utils::RandomGenerator::ValueFromRange: range-based "
      "generation") {
    auto engine = MakeDefaultEngine();
    RandomGenerator<DefaultRandomEngine> generator(engine);

    SUBCASE("Integer range") {
      for (int i = 0; i < 100; ++i) {
        const auto value = generator.ValueFromRange(10, 20);
        CHECK_GE(value, 10);
        CHECK_LE(value, 20);
      }
    }

    SUBCASE("Negative integer range") {
      for (int i = 0; i < 100; ++i) {
        const auto value = generator.ValueFromRange(-50, -10);
        CHECK_GE(value, -50);
        CHECK_LE(value, -10);
      }
    }

    SUBCASE("Mixed sign range") {
      for (int i = 0; i < 100; ++i) {
        const auto value = generator.ValueFromRange(-10, 10);
        CHECK_GE(value, -10);
        CHECK_LE(value, 10);
      }
    }

    SUBCASE("Floating point range") {
      for (int i = 0; i < 100; ++i) {
        const auto value = generator.ValueFromRange(5.0, 15.0);
        CHECK_GE(value, 5.0);
        CHECK_LT(value, 15.0);
      }
    }

    SUBCASE("Mixed types (int and float)") {
      const auto value = generator.ValueFromRange(0, 10.5);
      CHECK_GE(value, 0.0);
      CHECK_LT(value, 10.5);
    }

    SUBCASE("Single value range") {
      const auto value = generator.ValueFromRange(42, 42);
      CHECK_EQ(value, 42);
    }
  }

  TEST_CASE("helios::utils::RandomGenerator::EngineRef: engine access") {
    auto engine = MakeDefaultEngine();
    RandomGenerator<DefaultRandomEngine> generator(engine);

    auto& ref = generator.EngineRef();
    CHECK_EQ(&ref, &engine);

    const auto value_before = ref();
    const auto value_after = ref();
    CHECK_NE(value_before, value_after);
  }

  TEST_CASE("helios::utils::RandomDefault: thread-local default generator") {
    SUBCASE("Returns same generator instance") {
      auto& gen1 = RandomDefault();
      auto& gen2 = RandomDefault();
      CHECK_EQ(&gen1, &gen2);
    }

    SUBCASE("Generator works correctly") {
      auto& gen = RandomDefault();
      const auto value = gen.Value<int>();
      CHECK_GE(value, std::numeric_limits<int>::min());
      CHECK_LE(value, std::numeric_limits<int>::max());
    }
  }

  TEST_CASE("helios::utils::RandomFast: thread-local fast generator") {
    SUBCASE("Returns same generator instance") {
      auto& gen1 = RandomFast();
      auto& gen2 = RandomFast();
      CHECK_EQ(&gen1, &gen2);
    }

    SUBCASE("Generator works correctly") {
      auto& gen = RandomFast();
      const auto value = gen.Value<int>();
      CHECK_GE(value, std::numeric_limits<int>::min());
      CHECK_LE(value, std::numeric_limits<int>::max());
    }
  }

  TEST_CASE("helios::utils::RandomValue: convenience function") {
    SUBCASE("Integer type") {
      const auto value = RandomValue<int>();
      CHECK_GE(value, std::numeric_limits<int>::min());
      CHECK_LE(value, std::numeric_limits<int>::max());
    }

    SUBCASE("Float type in [0, 1)") {
      for (int i = 0; i < 100; ++i) {
        const auto value = RandomValue<float>();
        CHECK_GE(value, 0.0F);
        CHECK_LT(value, 1.0F);
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

  TEST_CASE("helios::utils::RandomValueFromRange: convenience function") {
    SUBCASE("Integer range") {
      for (int i = 0; i < 100; ++i) {
        const auto value = RandomValueFromRange(1, 6);
        CHECK_GE(value, 1);
        CHECK_LE(value, 6);
      }
    }

    SUBCASE("Float range") {
      for (int i = 0; i < 100; ++i) {
        const auto value = RandomValueFromRange(0.0, 100.0);
        CHECK_GE(value, 0.0);
        CHECK_LT(value, 100.0);
      }
    }
  }

  TEST_CASE("helios::utils::RandomFastValue: convenience function") {
    SUBCASE("Integer type") {
      const auto value = RandomFastValue<int>();
      CHECK_GE(value, std::numeric_limits<int>::min());
      CHECK_LE(value, std::numeric_limits<int>::max());
    }

    SUBCASE("Float type in [0, 1)") {
      for (int i = 0; i < 100; ++i) {
        const auto value = RandomFastValue<float>();
        CHECK_GE(value, 0.0F);
        CHECK_LT(value, 1.0F);
      }
    }
  }

  TEST_CASE("helios::utils::RandomFastValueFromRange: convenience function") {
    SUBCASE("Integer range") {
      for (int i = 0; i < 100; ++i) {
        const auto value = RandomFastValueFromRange(100, 200);
        CHECK_GE(value, 100);
        CHECK_LE(value, 200);
      }
    }

    SUBCASE("Float range") {
      for (int i = 0; i < 100; ++i) {
        const auto value = RandomFastValueFromRange(-10.0F, 10.0F);
        CHECK_GE(value, -10.0F);
        CHECK_LT(value, 10.0F);
      }
    }
  }

  TEST_CASE("helios::utils::Random: statistical distribution") {
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

      const int tolerance = kSamples / 10;
      CHECK_GT(true_count, kSamples / 2 - tolerance);
      CHECK_LT(true_count, kSamples / 2 + tolerance);
    }
  }

  TEST_CASE("helios::utils::Random: type aliases") {
    SUBCASE("DefaultRandomGenerator is correct alias") {
      CHECK(std::is_same_v<DefaultRandomGenerator,
                           RandomGenerator<DefaultRandomEngine>>);
    }

    SUBCASE("FastRandomGenerator is correct alias") {
      CHECK(std::is_same_v<FastRandomGenerator,
                           RandomGenerator<FastRandomEngine>>);
    }
  }

  TEST_CASE("helios::utils::RandomGenerator: different arithmetic types") {
    auto engine = MakeDefaultEngine();
    RandomGenerator<DefaultRandomEngine> generator(engine);

    SUBCASE("short type") {
      const auto value = generator.Value<short>();
      CHECK_GE(value, std::numeric_limits<short>::min());
      CHECK_LE(value, std::numeric_limits<short>::max());
    }

    SUBCASE("unsigned short type") {
      const auto value = generator.Value<unsigned short>();
      CHECK_GE(value, std::numeric_limits<unsigned short>::min());
      CHECK_LE(value, std::numeric_limits<unsigned short>::max());
    }

    SUBCASE("long type") {
      const auto value = generator.Value<long>();
      CHECK_GE(value, std::numeric_limits<long>::min());
      CHECK_LE(value, std::numeric_limits<long>::max());
    }

    SUBCASE("long long type") {
      const auto value = generator.Value<long long>();
      CHECK_GE(value, std::numeric_limits<long long>::min());
      CHECK_LE(value, std::numeric_limits<long long>::max());
    }

    SUBCASE("long double type") {
      for (int i = 0; i < 100; ++i) {
        const auto value = generator.Value<long double>();
        CHECK_GE(value, 0.0L);
        CHECK_LT(value, 1.0L);
      }
    }
  }

  TEST_CASE("helios::utils::RandomGenerator: additional engines") {
    SUBCASE("Fast engine generator") {
      auto engine = MakeFastEngine();
      RandomGenerator<FastRandomEngine> generator(engine);
      const auto value = generator.ValueFromRange(0, 10);
      CHECK_GE(value, 0);
      CHECK_LE(value, 10);
    }

    SUBCASE("Small engine generator") {
      auto engine = MakeEngine<SmallRandomEngine>();
      RandomGenerator<SmallRandomEngine> generator(engine);
      const auto value = generator.ValueFromRange(1, 6);
      CHECK_GE(value, 1);
      CHECK_LE(value, 6);
    }

    SUBCASE("Long-period engine generator") {
      auto engine = MakeEngine<LongPeriodRandomEngine>();
      RandomGenerator<LongPeriodRandomEngine> generator(engine);
      const auto value = generator.Value<float>();
      CHECK_GE(value, 0.0F);
      CHECK_LT(value, 1.0F);
    }
  }
}  // TEST_SUITE
