#pragma once

#include <helios/app/application.hpp>
#include <helios/app/plugin.hpp>
#include <helios/app/schedules.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/utils/common_traits.hpp>

#include <chrono>
#include <cstdint>
#include <string_view>

namespace helios::app {

/// @brief Application frame timing resource.
struct Time {
  using Clock = std::chrono::steady_clock;
  using Duration = Clock::duration;

  static constexpr std::string_view kName = "helios::app::Time";

  Duration delta_time{};
  Duration elapsed{};

  /**
   * @brief Timestamp used to compute the next frame delta.
   * @warning Manually changing this value is not recommended. Prefer allowing
   * `TimePlugin` to maintain frame timing.
   */
  Clock::time_point last_update = Clock::now();

  /**
   * @brief Updates frame delta and elapsed time using `Clock::now()`.
   * @warning Manually calling this function is not recommended. Prefer allowing
   * `TimePlugin` to call it once per frame.
   */
  void Update() noexcept {
    const auto now = Clock::now();
    delta_time = now - last_update;
    elapsed += delta_time;
    last_update = now;
  }

  /**
   * @brief Gets elapsed time converted to an arithmetic value.
   * @tparam Type Arithmetic return type
   * @tparam Units Duration units to convert through
   * @return Elapsed time as `Type`
   */
  template <utils::ArithmeticTrait Type = typename Duration::rep,
            utils::DurationTrait Units = Duration>
  [[nodiscard]] Type Elapsed() const {
    return static_cast<Type>(ElapsedDuration<Units>().count());
  }

  /**
   * @brief Gets elapsed time as a `std::chrono::duration`.
   * @tparam Units Duration type to convert to
   * @return Elapsed time as `Units`
   */
  template <utils::DurationTrait Units = Duration>
  [[nodiscard]] Units ElapsedDuration() const {
    return std::chrono::duration_cast<Units>(elapsed);
  }

  /**
   * @brief Gets elapsed time in seconds.
   * @return Elapsed time in seconds
   */
  [[nodiscard]] double ElapsedSec() const {
    return Elapsed<double, std::chrono::duration<double>>();
  }

  /**
   * @brief Gets elapsed time in milliseconds.
   * @return Elapsed time in milliseconds
   */
  [[nodiscard]] double ElapsedMilliSec() const {
    return Elapsed<double, std::chrono::duration<double, std::milli>>();
  }

  /**
   * @brief Gets elapsed time in microseconds.
   * @return Elapsed time in microseconds
   */
  [[nodiscard]] int64_t ElapsedMicroSec() const {
    return Elapsed<int64_t, std::chrono::microseconds>();
  }

  /**
   * @brief Gets elapsed time in nanoseconds.
   * @return Elapsed time in nanoseconds
   */
  [[nodiscard]] int64_t ElapsedNanoSec() const {
    return Elapsed<int64_t, std::chrono::nanoseconds>();
  }

  /**
   * @brief Gets delta time converted to an arithmetic value.
   * @tparam Type Arithmetic return type
   * @tparam Units Duration units to convert through
   * @return Delta time as `Type`
   */
  template <utils::ArithmeticTrait Type = typename Duration::rep,
            utils::DurationTrait Units = Duration>
  [[nodiscard]] Type Delta() const {
    return static_cast<Type>(DeltaDuration<Units>().count());
  }

  /**
   * @brief Gets delta time as a `std::chrono::duration`.
   * @tparam Units Duration type to convert to
   * @return Delta time as `Units`
   */
  template <utils::DurationTrait Units = Duration>
  [[nodiscard]] Units DeltaDuration() const {
    return std::chrono::duration_cast<Units>(delta_time);
  }

  /**
   * @brief Gets delta time in seconds.
   * @return Delta time in seconds
   */
  [[nodiscard]] double DeltaSec() const {
    return Delta<double, std::chrono::duration<double>>();
  }

  /**
   * @brief Gets delta time in milliseconds.
   * @return Delta time in milliseconds
   */
  [[nodiscard]] double DeltaMilliSec() const {
    return Delta<double, std::chrono::duration<double, std::milli>>();
  }

  /**
   * @brief Gets delta time in microseconds.
   * @return Delta time in microseconds
   */
  [[nodiscard]] int64_t DeltaMicroSec() const {
    return Delta<int64_t, std::chrono::microseconds>();
  }

  /**
   * @brief Gets delta time in nanoseconds.
   * @return Delta time in nanoseconds
   */
  [[nodiscard]] int64_t DeltaNanoSec() const {
    return Delta<int64_t, std::chrono::nanoseconds>();
  }
};

/// @brief Updates the application frame timing resource.
struct UpdateTime {
  void operator()(ecs::Res<Time> time) const noexcept { time->Update(); }
};

/// @brief Adds the `Time` resource and its per-frame update system.
class TimePlugin final : public Plugin {
public:
  static constexpr std::string_view kName = "helios::app::TimePlugin";

  void Build(App& app) override {
    app.TryInsertResources(Time{});
    app.AddSystem(kFirst, UpdateTime{});
  }
};

}  // namespace helios::app
