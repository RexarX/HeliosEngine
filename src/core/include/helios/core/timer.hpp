#pragma once

#include <chrono>
#include <concepts>
#include <cstdint>
#include <ratio>
#include <type_traits>

#include <helios/core/utils/common_traits.hpp>
#include <helios/core_pch.hpp>

namespace helios {

namespace details {

/**
 * @brief Concept for clock types compatible with std::chrono clocks.
 * @details Requires nested types rep, period, duration and time_point, and a static now() function.
 */
template <typename T>
concept ClockTrait = requires {
  typename T::rep;
  typename T::period;
  typename T::duration;
  typename T::time_point;
  { T::now() } -> std::same_as<typename T::time_point>;
};

/**
 * @brief Concept for duration types based on std::chrono::duration.
 */
template <typename T>
concept DurationTrait = std::same_as<T, std::chrono::duration<typename T::rep, typename T::period>>;

/**
 * @brief Helper alias to normalize a duration to its canonical std::chrono::duration form.
 */
template <typename Rep, typename Period>
using NormalizedDuration = std::chrono::duration<Rep, Period>;

/**
 * @brief Helper alias to normalize a clock's duration to a specific rep/period.
 */
template <typename Clock, typename Rep, typename Period>
using NormalizedClockDuration = std::chrono::duration<Rep, Period>;

}  // namespace details

/**
 * @brief High–resolution timer with configurable clock and rich elapsed API.
 * @details The timer measures elapsed time from the last reset point.
 *
 * - The clock type is configurable via the template parameter @p Clock.
 * - Users can query elapsed time as:
 *   - A raw `Clock::duration`.
 *   - Any `std::chrono::duration` (via `ElapsedDuration`).
 *   - Arbitrary arithmetic type with specified time unit (via `Elapsed`).
 *   - Convenience helpers (seconds, milliseconds, microseconds, nanoseconds).
 *
 * @note The default clock is `std::chrono::steady_clock`.
 * @tparam Clock Clock type used to measure time. Must satisfy `details::ClockTrait`.
 */
template <details::ClockTrait Clock = std::chrono::steady_clock>
class Timer {
public:
  using ClockType = Clock;
  using TimePoint = typename Clock::time_point;
  using Duration = typename Clock::duration;

  /**
   * @brief Constructs timer and immediately resets start timestamp.
   */
  constexpr Timer() noexcept(noexcept(Clock::now())) = default;
  constexpr Timer(const Timer&) noexcept(std::is_nothrow_copy_constructible_v<Clock>) = default;
  constexpr Timer(Timer&&) noexcept(std::is_nothrow_move_constructible_v<Clock>) = default;
  constexpr ~Timer() noexcept(std::is_nothrow_destructible_v<Clock>) = default;

  constexpr Timer& operator=(const Timer&) noexcept(std::is_nothrow_copy_assignable_v<Clock>) = default;
  constexpr Timer& operator=(Timer&&) noexcept(std::is_nothrow_move_assignable_v<Clock>) = default;

  /**
   * @brief Reset the timer start point to current time.
   */
  constexpr void Reset() noexcept(noexcept(Clock::now())) { start_time_ = Clock::now(); }

  /**
   * @brief Get elapsed time as a `std::chrono::duration`.
   * @details This is the primary API for time measurement and should be preferred when working with `std::chrono`.
   * @tparam Units Duration type to convert to. Must satisfy `details::DurationTrait`. Defaults to `Clock::duration`.
   * @return Elapsed time since last reset as `Units`.
   */
  template <details::DurationTrait Units = Duration>
  [[nodiscard]] constexpr Units ElapsedDuration() const;

  /**
   * @brief Get elapsed time converted to an arithmetic value with specified units.
   * @details This method allows querying elapsed time in arbitrary precision and units, for example:
   * - `Elapsed<double, std::chrono::seconds>()`
   * - `Elapsed<std::uint64_t, std::chrono::microseconds>()`
   * @tparam Type Arithmetic type used for the returned value.
   * @tparam Units Duration type representing the desired time unit.
   * @return Elapsed time as `Type`, where the underlying duration is `Units`.
   */
  template <utils::ArithmeticTrait Type = typename Duration::rep, details::DurationTrait Units = Duration>
  [[nodiscard]] constexpr Type Elapsed() const;

  /**
   * @brief Elapsed time in seconds as double.
   * @return Elapsed time since last reset in seconds.
   */
  [[nodiscard]] constexpr double ElapsedSec() const { return Elapsed<double, std::chrono::duration<double>>(); }

  /**
   * @brief Elapsed time in milliseconds as double.
   * @return Elapsed time since last reset in milliseconds.
   */
  [[nodiscard]] constexpr double ElapsedMilliSec() const {
    return Elapsed<double, std::chrono::duration<double, std::milli>>();
  }

  /**
   * @brief Elapsed time in microseconds as 64-bit integer.
   * @return Elapsed time since last reset in microseconds.
   */
  [[nodiscard]] constexpr int64_t ElapsedMicroSec() const { return Elapsed<int64_t, std::chrono::microseconds>(); }

  /**
   * @brief Elapsed time in nanoseconds as 64-bit integer.
   * @return Elapsed time since last reset in nanoseconds.
   */
  [[nodiscard]] constexpr int64_t ElapsedNanoSec() const { return Elapsed<int64_t, std::chrono::nanoseconds>(); }

  /**
   * @brief Get the raw start timestamp of the timer.
   * @return Starting time point.
   */
  [[nodiscard]] constexpr TimePoint Start() const noexcept { return start_time_; }

private:
  TimePoint start_time_{Clock::now()};
};

template <details::ClockTrait Clock>
template <details::DurationTrait Units>
constexpr Units Timer<Clock>::ElapsedDuration() const {
  const auto now = Clock::now();
  const auto diff = now - start_time_;
  return std::chrono::duration_cast<Units>(diff);
}

template <details::ClockTrait Clock>
template <utils::ArithmeticTrait Type, details::DurationTrait Units>
constexpr Type Timer<Clock>::Elapsed() const {
  const auto duration = ElapsedDuration<Units>();
  return static_cast<Type>(duration.count());
}

}  // namespace helios
