#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/ecs/resource.hpp>

#include <chrono>
#include <cstdint>
#include <string_view>

namespace helios::app {

/**
 * @brief Resource for tracking frame timing information.
 * @details Provides delta time (time since last frame), total elapsed time, and frame count.
 * Updated automatically by the engine before each frame.
 * @note Thread-safe for read operations. Written only by the runner before Update().
 *
 * @example
 * @code
 * void MovementSystem(SystemContext& ctx) {
 *   const auto& time = ctx.ReadResource<Time>();
 *   auto query = ctx.Query().Get<Position&, const Velocity&>();
 *   for (auto&& [entity, pos, vel] : query) {
 *     pos.x += vel.x * time.DeltaSeconds();
 *     pos.y += vel.y * time.DeltaSeconds();
 *   }
 * }
 * @endcode
 */
class Time {
public:
  using ClockType = std::chrono::steady_clock;
  using TimePoint = ClockType::time_point;
  using Duration = ClockType::duration;

  /**
   * @brief Default constructor.
   * @details Initializes time with zero delta and records start time.
   */
  constexpr Time() noexcept = default;
  constexpr Time(const Time&) noexcept = default;
  constexpr Time(Time&&) noexcept = default;
  constexpr ~Time() noexcept = default;

  constexpr Time& operator=(const Time&) noexcept = default;
  constexpr Time& operator=(Time&&) noexcept = default;

  /**
   * @brief Updates timing information for a new frame.
   * @details Should be called at the beginning of each frame by the runner.
   * Calculates delta time based on the time since last update.
   */
  void Tick() noexcept;

  /**
   * @brief Resets all timing information.
   * @details Sets delta to zero, resets elapsed time, and resets frame count.
   */
  void Reset() noexcept;

  /**
   * @brief Checks if this is the first frame.
   * @return True if frame count is 0
   */
  [[nodiscard]] constexpr bool IsFirstFrame() const noexcept { return frame_count_ == 0; }

  /**
   * @brief Gets the raw delta duration.
   * @return Delta time as std::chrono::duration
   */
  [[nodiscard]] constexpr Duration Delta() const noexcept { return delta_; }

  /**
   * @brief Gets the time elapsed since the last frame in seconds.
   * @return Delta time in seconds as float
   */
  [[nodiscard]] constexpr float DeltaSeconds() const noexcept { return delta_seconds_; }

  /**
   * @brief Gets the time elapsed since the last frame in milliseconds.
   * @return Delta time in milliseconds as float
   */
  [[nodiscard]] constexpr float DeltaMilliseconds() const noexcept { return delta_seconds_ * 1000.0F; }

  /**
   * @brief Gets the total elapsed time since timing started.
   * @return Total elapsed time as std::chrono::duration
   */
  [[nodiscard]] constexpr Duration Elapsed() const noexcept { return elapsed_; }

  /**
   * @brief Gets the total elapsed time since timing started in seconds.
   * @return Total elapsed time in seconds as float
   */
  [[nodiscard]] constexpr float ElapsedSeconds() const noexcept { return elapsed_seconds_; }

  /**
   * @brief Gets the current frame number.
   * @details Incremented each time Tick() is called.
   * @return Current frame count (0-indexed, first frame is 0)
   */
  [[nodiscard]] constexpr uint64_t FrameCount() const noexcept { return frame_count_; }

  /**
   * @brief Calculates the current frames per second.
   * @details Based on the current delta time.
   * @return FPS as float, or 0 if delta is zero
   */
  [[nodiscard]] constexpr float Fps() const noexcept { return delta_seconds_ > 0.0F ? 1.0F / delta_seconds_ : 0.0F; }

  /**
   * @brief Gets the resource name for trait requirements.
   * @return Resource name
   */
  static constexpr std::string_view GetName() noexcept { return "Time"; }

private:
  TimePoint last_tick_{ClockType::now()};   ///< Time point of the last tick
  TimePoint start_time_{ClockType::now()};  ///< Time point when timing started

  Duration delta_{Duration::zero()};    ///< Duration since last frame
  Duration elapsed_{Duration::zero()};  ///< Total elapsed duration

  float delta_seconds_ = 0.0F;    ///< Delta time in seconds (cached for performance)
  float elapsed_seconds_ = 0.0F;  ///< Elapsed time in seconds (cached for performance)

  uint64_t frame_count_ = 0;  ///< Number of frames since timing started
};

inline void Time::Tick() noexcept {
  const TimePoint now = ClockType::now();

  delta_ = now - last_tick_;
  elapsed_ = now - start_time_;

  delta_seconds_ = std::chrono::duration<float>(delta_).count();
  elapsed_seconds_ = std::chrono::duration<float>(elapsed_).count();

  last_tick_ = now;
  ++frame_count_;
}

inline void Time::Reset() noexcept {
  const TimePoint now = ClockType::now();

  last_tick_ = now;
  start_time_ = now;
  delta_ = Duration::zero();
  elapsed_ = Duration::zero();
  delta_seconds_ = 0.0F;
  elapsed_seconds_ = 0.0F;
  frame_count_ = 0;
}

// Ensure Time satisfies ResourceTrait
static_assert(ecs::ResourceTrait<Time>, "Time must satisfy ResourceTrait");

}  // namespace helios::app
