#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/utils/common_traits.hpp>

#include <concepts>
#include <cstdint>
#include <functional>
#include <limits>
#include <random>
#include <type_traits>

namespace helios {

/**
 * @brief Concept for random number engines compatible with std distributions.
 * @details Requires presence of result_type, operator(), and min()/max() members.
 */
template <typename T>
concept RandomEngine = requires(T engine) {
  typename T::result_type;
  { engine() } -> std::convertible_to<typename T::result_type>;
  { T::min() } -> std::convertible_to<typename T::result_type>;
  { T::max() } -> std::convertible_to<typename T::result_type>;
};

/**
 * @brief Concept for standard-like distributions.
 * @details Requires presence of result_type and callable operator(engine).
 */
template <typename T, typename Engine>
concept Distribution = requires(T dist, Engine engine) {
  typename T::result_type;
  { dist(engine) } -> std::convertible_to<typename T::result_type>;
};

/**
 * @brief Default engine type used by random utilities.
 * @details Uses a 64-bit Mersenne Twister for quality pseudorandom numbers.
 */
using DefaultRandomEngine = std::mt19937_64;

/**
 * @brief Fast but lower-quality engine type used by random utilities.
 * @details Uses a 32-bit linear congruential engine suitable for non-cryptographic, performance-critical scenarios.
 */
using FastRandomEngine = std::minstd_rand;

/**
 * @brief Internal helper to obtain seed from std::random_device.
 * @details This function is intentionally small and header-only to avoid static initialization of engines in user code.
 * @return 64-bit seed value from random_device.
 */
[[nodiscard]] inline uint64_t RandomDeviceSeed() {
  std::random_device rd{};
  using result_type = std::random_device::result_type;
  constexpr auto result_bits = sizeof(result_type) * 8U;
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
 * @brief Creates a default-quality random engine seeded with std::random_device.
 * @details Useful when the caller wants an engine instance but does not care
 * a specific engine type beyond the default choice.
 * @return DefaultRandomEngine instance seeded with random_device.
 */
[[nodiscard]] inline DefaultRandomEngine MakeDefaultEngine() {
  return DefaultRandomEngine{static_cast<DefaultRandomEngine::result_type>(RandomDeviceSeed())};
}

/**
 * @brief Creates a fast linear random engine seeded with std::random_device.
 * @details Intended for performance-critical code where statistical quality is less important.
 * Not suitable for cryptographic purposes.
 * @return FastRandomEngine instance seeded with random_device.
 */
[[nodiscard]] inline FastRandomEngine MakeFastEngine() {
  return FastRandomEngine{static_cast<FastRandomEngine::result_type>(RandomDeviceSeed())};
}

/**
 * @brief Thread-local default-quality engine.
 * @details Uses Meyer's singleton pattern per thread to avoid global static initialization order issues
 * while providing a convenient default.
 * @return Reference to thread-local DefaultRandomEngine instance.
 */
[[nodiscard]] inline DefaultRandomEngine& DefaultEngine() {
  thread_local auto engine = MakeDefaultEngine();
  return engine;
}

/**
 * @brief Thread-local fast engine.
 * @details Uses Meyer's singleton pattern per thread to avoid global static initialization order issues
 * while providing a fast default.
 * @return Reference to thread-local FastRandomEngine instance.
 */
[[nodiscard]] inline FastRandomEngine& FastEngineInstance() {
  thread_local auto engine = MakeFastEngine();
  return engine;
}

/**
 * @brief Random number utilities with a user-provided engine.
 * @details This wrapper delegates all random generation to an underlying engine instance supplied by the user.
 * It never owns the engine and does not perform any static initialization of engines itself.
 * @tparam Engine RandomEngine type used for generation.
 */
template <RandomEngine Engine>
class RandomGenerator {
public:
  /**
   * @brief Constructs a RandomGenerator from an existing engine reference.
   * @details The engine is not owned and must outlive this object.
   * @param engine Reference to engine used for random generation.
   */
  explicit RandomGenerator(Engine& engine) noexcept : engine_(engine) {}
  RandomGenerator(const RandomGenerator&) noexcept = default;
  RandomGenerator(RandomGenerator&&) noexcept = default;
  ~RandomGenerator() noexcept = default;

  RandomGenerator& operator=(const RandomGenerator&) noexcept = default;
  RandomGenerator& operator=(RandomGenerator&&) noexcept = default;

  /**
   * @brief Generates a value using the provided distribution.
   * @details This is a low-level interface that accepts an arbitrary distribution object.
   * Intended for cases where caller needs full control over distribution parameters.
   * @tparam Dist Distribution type compatible with Engine.
   * @param dist Distribution instance used for generation.
   * @return Generated random value of type Dist::result_type.
   */
  template <typename Dist>
    requires Distribution<Dist, Engine>
  [[nodiscard]] auto Next(Dist& dist) noexcept(std::is_nothrow_invocable_v<Dist, Engine&>) ->
      typename Dist::result_type {
    return dist(engine_.get());
  }

  /**
   * @brief Generates a random arithmetic value using a reasonable default distribution.
   * @details For integral types, uses std::uniform_int_distribution over the full representable range,
   * except for bool which uses a uniform {false, true}.
   * For floating point types, uses std::uniform_real_distribution in the [0, 1) range
   * to avoid dependence on std::numeric_limits<>::min().
   * @tparam T Arithmetic type to generate.
   * @return Randomly generated value of type T.
   */
  template <utils::ArithmeticTrait T>
  [[nodiscard]] T Value();

  /**
   * @brief Generates a random arithmetic value within the specified range.
   * @details For integral types, uses std::uniform_int_distribution with closed interval [min, max].
   * For floating point types, uses std::uniform_real_distribution with interval [min, max).
   * @tparam T Arithmetic type.
   * @tparam U Arithmetic type.
   * @param min Lower bound of the range.
   * @param max Upper bound of the range.
   * @return Random value of common_type_t<T, U> within [min, max] or [min, max).
   */
  template <utils::ArithmeticTrait T, utils::ArithmeticTrait U>
  [[nodiscard]] auto ValueFromRange(T min, U max) -> std::common_type_t<T, U>;

  /**
   * @brief Provides access to the underlying engine.
   * @return Reference to the engine used by this generator.
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
      std::uniform_int_distribution<T> dist(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
      return dist(engine);
    }
  } else {
    std::uniform_real_distribution<T> dist(static_cast<T>(0), static_cast<T>(1));
    return dist(engine);
  }
}

template <RandomEngine Engine>
template <utils::ArithmeticTrait T, utils::ArithmeticTrait U>
inline auto RandomGenerator<Engine>::ValueFromRange(T min, U max) -> std::common_type_t<T, U> {
  using Common = std::common_type_t<T, U>;
  const auto cmin = static_cast<Common>(min);
  const auto cmax = static_cast<Common>(max);

  Engine& engine = engine_.get();
  if constexpr (std::integral<Common>) {
    using DistType = std::conditional_t<std::signed_integral<Common>, int64_t, uint64_t>;
    std::uniform_int_distribution<DistType> dist(static_cast<DistType>(cmin), static_cast<DistType>(cmax));
    return static_cast<Common>(dist(engine));
  } else {
    std::uniform_real_distribution<Common> dist(cmin, cmax);
    return dist(engine);
  }
}

/**
 * @brief Convenience alias for a generator using the default-quality engine.
 * @details Uses thread-local DefaultEngine() as the underlying engine.
 */
using DefaultRandomGenerator = RandomGenerator<DefaultRandomEngine>;

/**
 * @brief Convenience alias for a generator using the fast engine.
 * @details Uses thread-local FastEngineInstance() as the underlying engine.
 */
using FastRandomGeneratorType = RandomGenerator<FastRandomEngine>;

/**
 * @brief Provides access to a thread-local default-quality random generator.
 * @details Uses Meyer's singleton pattern per thread and avoids any static engine objects
 * other than thread-local instances that are lazily initialized on first use.
 * @return Reference to DefaultRandomGenerator bound to DefaultEngine().
 */
[[nodiscard]] inline DefaultRandomGenerator& RandomDefault() {
  thread_local DefaultRandomGenerator generator{DefaultEngine()};
  return generator;
}

/**
 * @brief Provides access to a thread-local fast random generator.
 * @details Uses Meyer's singleton pattern per thread and avoids any static engine objects
 * other than thread-local instances that are lazily initialized on first use.
 * @return Reference to FastRandomGeneratorType bound to FastEngineInstance().
 */
[[nodiscard]] inline FastRandomGeneratorType& RandomFast() {
  thread_local FastRandomGeneratorType generator{FastEngineInstance()};
  return generator;
}

/**
 * @brief Convenience function to generate a default-distribution value using the default engine.
 * @details Equivalent to RandomDefault().Value<T>(), but shorter to call.
 * @tparam T Arithmetic type to generate.
 * @return Randomly generated value of type T.
 */
template <utils::ArithmeticTrait T>
[[nodiscard]] inline T RandomValue() {
  return RandomDefault().Value<T>();
}

/**
 * @brief Convenience function to generate a value in range using the default engine.
 * @details Equivalent to RandomDefault().ValueFromRange(min, max).
 * @tparam T Arithmetic type.
 * @tparam U Arithmetic type.
 * @param min Lower bound of the range.
 * @param max Upper bound of the range.
 * @return Random value of common_type_t<T, U> in [min, max] or [min, max).
 */
template <utils::ArithmeticTrait T, utils::ArithmeticTrait U>
[[nodiscard]] inline auto RandomValueFromRange(T min, U max) -> std::common_type_t<T, U> {
  return RandomDefault().ValueFromRange(min, max);
}

/**
 * @brief Convenience function to generate a default-distribution value using the fast engine.
 * @details Equivalent to RandomFast().Value<T>(), but shorter to call.
 * @tparam T Arithmetic type to generate.
 * @return Randomly generated value of type T using the fast engine.
 */
template <utils::ArithmeticTrait T>
[[nodiscard]] inline T RandomFastValue() {
  return RandomFast().Value<T>();
}

/**
 * @brief Convenience function to generate a value in range using the fast engine.
 * @details Equivalent to RandomFast().ValueFromRange(min, max).
 * @tparam T Arithmetic type.
 * @tparam U Arithmetic type.
 * @param min Lower bound of the range.
 * @param max Upper bound of the range.
 * @return Random value of common_type_t<T, U> in [min, max] or [min, max).
 */
template <utils::ArithmeticTrait T, utils::ArithmeticTrait U>
[[nodiscard]] inline auto RandomFastValueFromRange(T min, U max) -> std::common_type_t<T, U> {
  return RandomFast().ValueFromRange(min, max);
}

}  // namespace helios
