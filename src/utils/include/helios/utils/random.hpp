#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.utils;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <xoshiro.h>

#include <atomic>
#include <chrono>
#include <concepts>
#include <cstdint>
#include <functional>
#include <limits>
#include <random>
#include <thread>
#include <type_traits>
#endif
#include <helios/utils/common_traits.hpp>

namespace helios::utils::details {

/// @brief MurmurHash3 finalizer; avalanches all bits of a 64-bit word.
[[nodiscard]] constexpr uint64_t MurmurScramble64(uint64_t num) noexcept {
  num ^= num >> 33;
  num *= 0xff51afd7ed558ccdULL;
  num ^= num >> 33;
  num *= 0xc4ceb9fe1a85ec53ULL;
  num ^= num >> 33;
  return num;
}

/// @brief Reads 64 bits from `std::random_device`, combining calls if needed.
[[nodiscard]] inline uint64_t ReadRandomDevice64() {
  std::random_device rd{};
  using Result = std::random_device::result_type;
  constexpr auto result_bits = sizeof(Result) * 8U;
  constexpr auto target_bits = sizeof(uint64_t) * 8U;

  if constexpr (result_bits >= target_bits) {
    return static_cast<uint64_t>(rd());
  } else {
    uint64_t value = 0;
    auto shift = 0U;
    while (shift < target_bits) {
      value |= static_cast<uint64_t>(rd()) << shift;
      shift += result_bits;
    }
    return value;
  }
}

/**
 * @brief Process-wide entropy captured once on first use.
 * @details Function-local static (not a namespace-scope global) so
 * initialization is thread-safe and avoids the static initialization order
 * fiasco. Mixes `std::random_device` with a scrambled high-resolution clock
 * because some platforms provide a weak `random_device`.
 */
[[nodiscard]] inline uint64_t CachedProcessEntropy() {
  static const uint64_t entropy = [] {
    uint64_t value = ReadRandomDevice64();
    using Clock = std::chrono::high_resolution_clock;
    const auto ticks =
        static_cast<uint64_t>(Clock::now().time_since_epoch().count());
    value ^= MurmurScramble64(ticks);
    return value == 0 ? 0x9e3779b97f4a7c15ULL : value;
  }();
  return entropy;
}

/// @brief Monotonic counter so recycled OS thread ids still get unique seeds.
[[nodiscard]] inline uint64_t NextSeedSequence() noexcept {
  static std::atomic<uint64_t> sequence{1};
  return sequence.fetch_add(1, std::memory_order_relaxed);
}

}  // namespace helios::utils::details

HELIOS_MODULE_EXPORT
namespace helios::utils {

/**
 * @brief Concept for random number engines compatible with std distributions.
 * @details Requires presence of `result_type`, `operator()`, and
 * `min()`/`max()` members.
 */
template <typename T>
concept RandomEngine = requires(T&& engine) {
  typename std::remove_cvref_t<T>::result_type;
  { engine() } -> std::convertible_to<typename T::result_type>;
  {
    std::remove_cvref_t<T>::min()
  } -> std::convertible_to<typename std::remove_cvref_t<T>::result_type>;
  {
    std::remove_cvref_t<T>::max()
  } -> std::convertible_to<typename std::remove_cvref_t<T>::result_type>;
};

/**
 * @brief Concept for standard-like distributions.
 * @details Requires presence of `result_type` and callable operator(engine).
 */
template <typename T, typename Engine>
concept Distribution = requires(T&& dist, Engine engine) {
  typename std::remove_cvref_t<T>::result_type;
  {
    dist(engine)
  } -> std::convertible_to<typename std::remove_cvref_t<T>::result_type>;
};

/// @brief xoshiro256** — all-purpose 64-bit generator (256 bits of state).
using Xoshiro256StarStar = xso::xoshiro_4x64_star_star;

/// @brief xoshiro256++ — slightly faster all-purpose 64-bit alternative.
using Xoshiro256PlusPlus = xso::xoshiro_4x64_plus_plus;

/// @brief xoshiro256+ — fastest 256-bit engine; weaker low bits.
using Xoshiro256Plus = xso::xoshiro_4x64_plus;

/// @brief xoroshiro128++ — fast 64-bit generator (128 bits of state).
using Xoroshiro128PlusPlus = xso::xoroshiro_2x64_plus_plus;

/// @brief xoroshiro128** — 128-bit-state alternative with a `**` scrambler.
using Xoroshiro128StarStar = xso::xoroshiro_2x64_star_star;

/// @brief xoroshiro128+ — smallest/fastest 64-bit engine; weaker low bits.
using Xoroshiro128Plus = xso::xoroshiro_2x64_plus;

/// @brief xoshiro128** — 32-bit output, 128 bits of state.
using Xoshiro128StarStar = xso::xoshiro_4x32_star_star;

/// @brief xoshiro128++ — 32-bit output alternative.
using Xoshiro128PlusPlus = xso::xoshiro_4x32_plus_plus;

/// @brief xoshiro512** — 512 bits of state for extra period / more streams.
using Xoshiro512StarStar = xso::xoshiro_8x64_star_star;

/// @brief xoroshiro1024** — 1024 bits of state for huge parallel workloads.
using Xoroshiro1024StarStar = xso::xoroshiro_16x64_star_star;

/**
 * @brief Default engine type used by random utilities.
 * @details xoshiro256** (`xso::rng`): 64-bit output, 256 bits of state,
 * period 2^256-1. Best general-purpose choice in the xoshiro family, faster
 * than `std::mt19937_64` with comparable statistical quality.
 */
using DefaultRandomEngine = Xoshiro256StarStar;

/**
 * @brief Fast engine type used by random utilities.
 * @details xoroshiro128++: 64-bit output, 128 bits of state. Smaller and
 * faster than the default; still suitable for gameplay / sampling. Not for
 * cryptography.
 */
using FastRandomEngine = Xoroshiro128PlusPlus;

/**
 * @brief Compact 32-bit engine.
 * @details xoshiro128**: 32-bit output, 128 bits of state. Prefer this when
 * `result_type` must be 32-bit or state size matters more than output width.
 */
using SmallRandomEngine = Xoshiro128StarStar;

/**
 * @brief Long-period engine for large parallel jobs.
 * @details xoshiro512**: 64-bit output, 512 bits of state, period 2^512-1.
 */
using LongPeriodRandomEngine = Xoshiro512StarStar;

/**
 * @brief Returns the process-wide 64-bit entropy word.
 * @details Captured once on first call from `std::random_device` mixed with
 * a high-resolution clock. Subsequent calls are free and return the same
 * value for the lifetime of the process. Prefer `MixThreadSeed()` when
 * constructing engines so threads do not share a stream.
 * @return Stable per-process 64-bit seed
 */
[[nodiscard]] inline uint64_t RandomDeviceSeed() {
  return details::CachedProcessEntropy();
}

/**
 * @brief Mixes process entropy with the calling thread and a unique sequence.
 * @details Combines `RandomDeviceSeed()`, `std::this_thread::get_id()`, and
 * an atomic counter. The counter matters because OS thread ids can be reused
 * after a thread exits. Each call returns a different 64-bit word, suitable
 * as a `seed(word)` argument for xoshiro/xoroshiro (they expand it with
 * SplitMix64 into the full state).
 * @return Unique 64-bit seed for this call
 */
[[nodiscard]] inline uint64_t MixThreadSeed() {
  const uint64_t thread_hash = static_cast<uint64_t>(
      std::hash<std::thread::id>{}(std::this_thread::get_id()));
  uint64_t state = details::CachedProcessEntropy();
  state ^= details::MurmurScramble64(thread_hash);
  state += details::NextSeedSequence() * 0x9e3779b97f4a7c15ULL;
  const uint64_t mixed = details::MurmurScramble64(state);
  return mixed == 0 ? thread_hash | 1U : mixed;
}

/**
 * @brief Creates an engine seeded from process entropy, thread id, and
 * sequence.
 * @tparam Engine RandomEngine type to construct (defaults to
 * `DefaultRandomEngine`)
 * @return Engine instance with a unique mixed seed
 */
template <RandomEngine Engine = DefaultRandomEngine>
[[nodiscard]] Engine MakeEngine() {
  using Word = typename Engine::result_type;
  const uint64_t seed = MixThreadSeed();
  if constexpr (sizeof(Word) >= sizeof(uint64_t)) {
    return Engine{static_cast<Word>(seed)};
  } else {
    constexpr auto word_bits = sizeof(Word) * 8U;
    return Engine{static_cast<Word>(seed ^ (seed >> word_bits))};
  }
}

/**
 * @brief Creates an engine from an explicit seed for repeatable streams.
 * @tparam Engine RandomEngine type to construct (defaults to
 * `DefaultRandomEngine`)
 * @param seed Seed word expanded into the engine state
 * @return Engine instance seeded from `seed`
 */
template <RandomEngine Engine = DefaultRandomEngine>
[[nodiscard]] Engine MakeEngine(typename Engine::result_type seed) {
  return Engine{seed};
}

/**
 * @brief Creates a default-quality random engine with a unique mixed seed.
 * @return `DefaultRandomEngine` instance
 */
[[nodiscard]] inline DefaultRandomEngine MakeDefaultEngine() {
  return MakeEngine<DefaultRandomEngine>();
}

/**
 * @brief Creates a fast random engine with a unique mixed seed.
 * @return `FastRandomEngine` instance
 */
[[nodiscard]] inline FastRandomEngine MakeFastEngine() {
  return MakeEngine<FastRandomEngine>();
}

/**
 * @brief Thread-local engine of type `Engine`, seeded once per thread.
 * @tparam Engine RandomEngine type stored in this thread
 * @return Reference to the thread-local engine
 */
template <RandomEngine Engine>
[[nodiscard]] Engine& ThreadLocalEngine() {
  thread_local Engine engine = MakeEngine<Engine>();
  return engine;
}

/**
 * @brief Thread-local default-quality engine.
 * @return Reference to thread-local `DefaultRandomEngine`
 */
[[nodiscard]] inline DefaultRandomEngine& DefaultEngine() {
  return ThreadLocalEngine<DefaultRandomEngine>();
}

/**
 * @brief Thread-local fast engine.
 * @return Reference to thread-local `FastRandomEngine`
 */
[[nodiscard]] inline FastRandomEngine& FastEngine() {
  return ThreadLocalEngine<FastRandomEngine>();
}

/**
 * @brief Random number utilities with a user-provided engine.
 * @details This wrapper delegates all random generation to an underlying engine
 * instance supplied by the user. It never owns the engine and does not perform
 * any static initialization of engines itself.
 * @tparam Engine RandomEngine type used for generation
 */
template <RandomEngine Engine>
class RandomGenerator {
public:
  /**
   * @brief Constructs a `RandomGenerator` from an existing engine reference.
   * @details The engine is not owned and must outlive this object.
   * @param engine Reference to engine used for random generation
   */
  explicit RandomGenerator(Engine& engine) noexcept : engine_(engine) {}
  RandomGenerator(const RandomGenerator&) noexcept = default;
  RandomGenerator(RandomGenerator&&) noexcept = default;
  ~RandomGenerator() noexcept = default;

  RandomGenerator& operator=(const RandomGenerator&) noexcept = default;
  RandomGenerator& operator=(RandomGenerator&&) noexcept = default;

  /**
   * @brief Generates a value using the provided distribution.
   * @details This is a low-level interface that accepts an arbitrary
   * distribution object. Intended for cases where caller needs full control
   * over distribution parameters.
   * @tparam Dist Distribution type compatible with `Engine`
   * @param dist Distribution instance used for generation
   * @return Generated random value of type `Dist::result_type`
   */
  template <typename Dist>
    requires Distribution<Dist, Engine>
  [[nodiscard]] auto Next(Dist& dist) noexcept(
      std::is_nothrow_invocable_v<Dist, Engine&>) ->
      typename Dist::result_type {
    return dist(engine_.get());
  }

  /**
   * @brief Generates a random arithmetic value using a reasonable default
   * distribution.
   * @details For integral types, uses `std::uniform_int_distribution` over the
   * full representable range, except for `bool` which uses a uniform {false,
   * true}. For floating point types, uses `std::uniform_real_distribution` in
   * the [0, 1) range to avoid dependence on `std::numeric_limits<>::min()`.
   * @tparam T Arithmetic type to generate
   * @return Randomly generated value of type `T`
   */
  template <utils::ArithmeticTrait T>
  [[nodiscard]] T Value();

  /**
   * @brief Generates a random arithmetic value within the specified range.
   * @details For integral types, uses `std::uniform_int_distribution` with
   * closed interval [min, max]. For floating point types, uses
   * std::uniform_real_distribution with interval [min, max).
   * @tparam T Arithmetic type
   * @tparam U Arithmetic type
   * @param min Lower bound of the range
   * @param max Upper bound of the range
   * @return Random value of `std::common_type_t<T, U>` within [min, max] or
   * [min, max)
   */
  template <utils::ArithmeticTrait T, utils::ArithmeticTrait U>
  [[nodiscard]] auto ValueFromRange(T min, U max) -> std::common_type_t<T, U>;

  /**
   * @brief Provides access to the underlying engine.
   * @return Reference to the engine used by this generator
   */
  [[nodiscard]] Engine& EngineRef() const noexcept { return engine_.get(); }

private:
  std::reference_wrapper<Engine> engine_;
};

template <RandomEngine Engine>
template <utils::ArithmeticTrait T>
inline T RandomGenerator<Engine>::Value() {
  Engine& engine = engine_.get();
  if constexpr (std::integral<T>) {
    if constexpr (std::same_as<T, bool>) {
      std::uniform_int_distribution<int> dist(0, 1);
      return dist(engine) == 1;
    } else {
      std::uniform_int_distribution<T> dist(std::numeric_limits<T>::min(),
                                            std::numeric_limits<T>::max());
      return dist(engine);
    }
  } else {
    std::uniform_real_distribution<T> dist(static_cast<T>(0),
                                           static_cast<T>(1));
    return dist(engine);
  }
}

template <RandomEngine Engine>
template <utils::ArithmeticTrait T, utils::ArithmeticTrait U>
inline auto RandomGenerator<Engine>::ValueFromRange(T min, U max)
    -> std::common_type_t<T, U> {
  using Common = std::common_type_t<T, U>;
  const auto cmin = static_cast<Common>(min);
  const auto cmax = static_cast<Common>(max);

  Engine& engine = engine_.get();
  if constexpr (std::integral<Common>) {
    using DistType =
        std::conditional_t<std::signed_integral<Common>, int64_t, uint64_t>;
    std::uniform_int_distribution<DistType> dist(static_cast<DistType>(cmin),
                                                 static_cast<DistType>(cmax));
    return static_cast<Common>(dist(engine));
  } else {
    std::uniform_real_distribution<Common> dist(cmin, cmax);
    return dist(engine);
  }
}

/**
 * @brief Convenience alias for a generator using the default-quality engine.
 * @details Uses thread-local `DefaultEngine()` as the underlying engine.
 */
using DefaultRandomGenerator = RandomGenerator<DefaultRandomEngine>;

/**
 * @brief Convenience alias for a generator using the fast engine.
 * @details Uses thread-local `FastEngine()` as the underlying engine.
 */
using FastRandomGenerator = RandomGenerator<FastRandomEngine>;

/**
 * @brief Thread-local generator bound to `ThreadLocalEngine<Engine>()`.
 * @tparam Engine RandomEngine type used by the generator
 * @return Reference to the thread-local generator
 */
template <RandomEngine Engine>
[[nodiscard]] auto ThreadLocalGenerator() -> RandomGenerator<Engine>& {
  thread_local RandomGenerator<Engine> generator{ThreadLocalEngine<Engine>()};
  return generator;
}

/**
 * @brief Provides access to a thread-local default-quality random generator.
 * @return Reference to `DefaultRandomGenerator` bound to `DefaultEngine()`
 */
[[nodiscard]] inline DefaultRandomGenerator& RandomDefault() {
  return ThreadLocalGenerator<DefaultRandomEngine>();
}

/**
 * @brief Provides access to a thread-local fast random generator.
 * @return Reference to `FastRandomGenerator` bound to `FastEngine()`
 */
[[nodiscard]] inline FastRandomGenerator& RandomFast() {
  return ThreadLocalGenerator<FastRandomEngine>();
}

/**
 * @brief Convenience function to generate a default-distribution value using
 * the default engine.
 * @details Equivalent to `RandomDefault().Value<T>()`, but shorter to call.
 * @tparam T Arithmetic type to generate
 * @return Randomly generated value of type T
 */
template <utils::ArithmeticTrait T>
[[nodiscard]] inline T RandomValue() {
  return RandomDefault().Value<T>();
}

/**
 * @brief Convenience function to generate a value in range using the default
 * engine.
 * @details Equivalent to `RandomDefault().ValueFromRange(min, max)`.
 * @tparam T Arithmetic type
 * @tparam U Arithmetic type
 * @param min Lower bound of the range
 * @param max Upper bound of the range
 * @return Random value of `std::common_type_t<T, U>` in [min, max] or [min,
 * max)
 */
template <utils::ArithmeticTrait T, utils::ArithmeticTrait U>
[[nodiscard]] inline auto RandomValueFromRange(T min, U max)
    -> std::common_type_t<T, U> {
  return RandomDefault().ValueFromRange(min, max);
}

/**
 * @brief Convenience function to generate a default-distribution value using
 * the fast engine.
 * @details Equivalent to `RandomFast().Value<T>()`, but shorter to call.
 * @tparam T Arithmetic type to generate
 * @return Randomly generated value of type `T` using the fast engine
 */
template <utils::ArithmeticTrait T>
[[nodiscard]] inline T RandomFastValue() {
  return RandomFast().Value<T>();
}

/**
 * @brief Convenience function to generate a value in range using the fast
 * engine.
 * @details Equivalent to `RandomFast().ValueFromRange(min, max)`.
 * @tparam T Arithmetic type
 * @tparam U Arithmetic type
 * @param min Lower bound of the range
 * @param max Upper bound of the range
 * @return Random value of `std::common_type_t<T, U>` in [min, max] or [min,
 * max)
 */
template <utils::ArithmeticTrait T, utils::ArithmeticTrait U>
[[nodiscard]] inline auto RandomFastValueFromRange(T min, U max)
    -> std::common_type_t<T, U> {
  return RandomFast().ValueFromRange(min, max);
}

}  // namespace helios::utils
#endif  // HELIOS_MODULE_CONSUMER_SHIM
