#pragma once

#include <helios/app/plugin.hpp>
#include <helios/ecs/resource/params.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace helios::app {

class App;
class SubApp;

/// @brief How `FrameLimiter` chooses a frame interval.
enum class FrameLimiterMode : uint8_t {
  kOff,     ///< Do not sleep.
  kManual,  ///< Sleep using `Interval()`.
  kAuto,    ///< Sleep using `RefreshRate()` when it is non-zero.
};

/// @brief Construction / install configuration for `FrameLimiter`.
struct FrameLimiterSettings {
  static constexpr auto kDefaultInterval =
      std::chrono::nanoseconds{16'666'667};  // ~60 FPS

  FrameLimiterMode mode = FrameLimiterMode::kManual;
  std::chrono::nanoseconds interval = kDefaultInterval;

  /**
   * @brief Disables pacing.
   * @return Settings with `kOff`
   */
  [[nodiscard]] static constexpr FrameLimiterSettings Off() noexcept {
    return {.mode = FrameLimiterMode::kOff};
  }

  /**
   * @brief Manual cap at the given frames per second.
   * @param fps Target frames per second (must be > 0)
   * @return Manual settings with the corresponding interval
   */
  [[nodiscard]] static constexpr FrameLimiterSettings FromFPS(
      size_t fps) noexcept {
    constexpr size_t kNanosecsInSec = 1'000'000'000;
    return {.mode = FrameLimiterMode::kManual,
            .interval = std::chrono::nanoseconds{kNanosecsInSec / fps}};
  }

  /**
   * @brief Manual cap at the given frequency in Hz.
   * @param hz Target frequency in Hz (must be > 0)
   * @return Manual settings with the corresponding interval
   */
  [[nodiscard]] static constexpr FrameLimiterSettings FromHz(
      double hz) noexcept {
    const auto nanosec = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::duration<double>{1.0 / hz});
    return {.mode = FrameLimiterMode::kManual, .interval = nanosec};
  }

  /**
   * @brief Manual cap with an explicit interval.
   * @tparam Rep Duration representation type
   * @tparam Period Duration period type
   * @param interval Desired frame interval
   * @return Manual settings with `interval`
   */
  template <typename Rep, typename Period>
  [[nodiscard]] static constexpr FrameLimiterSettings FromInterval(
      std::chrono::duration<Rep, Period> interval) noexcept {
    return {.mode = FrameLimiterMode::kManual,
            .interval =
                std::chrono::duration_cast<std::chrono::nanoseconds>(interval)};
  }

  /**
   * @brief Match display refresh when `FrameLimiter::RefreshRate()` is set.
   * @details Treats a refresh rate of `0` as `kOff` until something (usually
   * the window plugin) reports a monitor Hz.
   * @return Auto settings
   */
  [[nodiscard]] static constexpr FrameLimiterSettings Auto() noexcept {
    return {.mode = FrameLimiterMode::kAuto};
  }
};

/// @brief Stage that runs frame pacing before window polling and update.
struct FramePaceStage {
  static constexpr std::string_view kName = "helios::app::FramePaceStage";
};

/// @brief Builtin frame-pace stage (main app `MainFrameOrder` only).
inline constexpr FramePaceStage kFramePaceStage{};

/// @brief Main-thread schedule that calls `FrameLimiter::Wait()`.
struct FramePace {
  static constexpr std::string_view kName = "helios::app::FramePace";
};

/// @brief Builtin frame-pace schedule.
inline constexpr FramePace kFramePace{};

/// @brief Per-world frame pacer. One instance per `App` / `SubApp` world.
class FrameLimiter {
public:
  using Clock = std::chrono::steady_clock;

  static constexpr std::string_view kName = "helios::app::FrameLimiter";

  /**
   * @brief Constructs a limiter from install settings.
   * @param settings Mode and interval
   */
  explicit FrameLimiter(FrameLimiterSettings settings = {}) noexcept
      : mode_(settings.mode), interval_ns_(settings.interval.count()) {}
  FrameLimiter(const FrameLimiter& other) noexcept;
  FrameLimiter(FrameLimiter&& other) noexcept;
  ~FrameLimiter() = default;

  FrameLimiter& operator=(const FrameLimiter& other) noexcept;
  FrameLimiter& operator=(FrameLimiter&& other) noexcept;

  /**
   * @brief Blocks until the next tick boundary when pacing is active.
   * @details Not thread-safe against itself: call only from the thread that
   * drives this world's `Update()`. The first call after a deadline reset does
   * not sleep. `kOff`, a non-positive interval, and `kAuto` with a zero
   * refresh rate clear the deadline and return immediately.
   */
  void Wait();

  /**
   * @brief Sets the limiter mode. Thread-safe.
   * @param mode New mode
   */
  void SetMode(FrameLimiterMode mode) noexcept {
    mode_.store(mode, std::memory_order_relaxed);
  }

  /**
   * @brief Sets the manual interval. Thread-safe.
   * @param interval Frame interval used by `kManual`
   */
  void SetInterval(std::chrono::nanoseconds interval) noexcept {
    interval_ns_.store(interval.count(), std::memory_order_relaxed);
  }

  /**
   * @brief Sets the display refresh rate used by `kAuto`. Thread-safe.
   * @param hz Refresh rate in Hertz; `0` means unknown
   */
  void SetRefreshRate(uint32_t hz) noexcept {
    refresh_rate_hz_.store(hz, std::memory_order_relaxed);
  }

  /**
   * @brief Returns the current mode. Thread-safe.
   * @return Current `FrameLimiterMode`
   */
  [[nodiscard]] FrameLimiterMode Mode() const noexcept {
    return mode_.load(std::memory_order_relaxed);
  }

  /**
   * @brief Returns the manual interval. Thread-safe.
   * @return Manual frame interval
   */
  [[nodiscard]] std::chrono::nanoseconds Interval() const noexcept {
    return std::chrono::nanoseconds{
        interval_ns_.load(std::memory_order_relaxed)};
  }

  /**
   * @brief Returns the Auto-mode refresh rate. Thread-safe.
   * @return Hertz, or `0` if unknown
   */
  [[nodiscard]] uint32_t RefreshRate() const noexcept {
    return refresh_rate_hz_.load(std::memory_order_relaxed);
  }

private:
  [[nodiscard]] auto ResolveInterval() const noexcept
      -> std::optional<std::chrono::nanoseconds>;

  std::atomic<FrameLimiterMode> mode_{FrameLimiterMode::kManual};
  std::atomic<int64_t> interval_ns_{
      FrameLimiterSettings::kDefaultInterval.count()};
  std::atomic<uint32_t> refresh_rate_hz_{0};
  std::optional<Clock::time_point> next_tick_;
};

inline FrameLimiter::FrameLimiter(const FrameLimiter& other) noexcept
    : mode_(other.mode_.load(std::memory_order_relaxed)),
      interval_ns_(other.interval_ns_.load(std::memory_order_relaxed)),
      refresh_rate_hz_(other.refresh_rate_hz_.load(std::memory_order_relaxed)),
      next_tick_(other.next_tick_) {}

inline FrameLimiter::FrameLimiter(FrameLimiter&& other) noexcept
    : mode_(other.mode_.load(std::memory_order_relaxed)),
      interval_ns_(other.interval_ns_.load(std::memory_order_relaxed)),
      refresh_rate_hz_(other.refresh_rate_hz_.load(std::memory_order_relaxed)),
      next_tick_(other.next_tick_) {}

inline FrameLimiter& FrameLimiter::operator=(
    const FrameLimiter& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  mode_.store(other.mode_.load(std::memory_order_relaxed),
              std::memory_order_relaxed);
  interval_ns_.store(other.interval_ns_.load(std::memory_order_relaxed),
                     std::memory_order_relaxed);
  refresh_rate_hz_.store(other.refresh_rate_hz_.load(std::memory_order_relaxed),
                         std::memory_order_relaxed);
  next_tick_ = other.next_tick_;

  return *this;
}

inline FrameLimiter& FrameLimiter::operator=(FrameLimiter&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  mode_.store(other.mode_.load(std::memory_order_relaxed),
              std::memory_order_relaxed);
  interval_ns_.store(other.interval_ns_.load(std::memory_order_relaxed),
                     std::memory_order_relaxed);
  refresh_rate_hz_.store(other.refresh_rate_hz_.load(std::memory_order_relaxed),
                         std::memory_order_relaxed);
  next_tick_ = other.next_tick_;

  return *this;
}

/// @brief Invokes `FrameLimiter::Wait()` once per scheduled run.
struct LimitFrameRate {
  static constexpr std::string_view kName = "helios::app::LimitFrameRate";

  void operator()(ecs::Res<FrameLimiter> limiter) const { limiter->Wait(); }
};

/**
 * @brief Installs a limiter on the main app world.
 * @details Adds `FrameLimiter`, a main-thread `kFramePace` schedule in
 * `kFramePaceStage`, and prepends that stage to `MainFrameOrder`. Nested
 * `FramePumpOrder` is left unchanged. Prefer this (or `FrameLimiterPlugin`)
 * with `RunDefault` instead of combining with `RunFixed`.
 * @param app Application to configure
 * @param settings Limiter configuration
 */
void InstallFrameLimiter(App& app, FrameLimiterSettings settings = {});

/**
 * @brief Installs a limiter on a sub-app world.
 * @details Adds `FrameLimiter` and a main-thread `kFramePace` schedule in
 * `kUpdateStage` ordered before `kFirst`. Blocking sub-apps still join the
 * main frame (`WaitForSubApps`); independent rates require an async sub-app.
 * @warning Triggers assertion if the sub-app allows overlapping updates.
 * @param sub_app Sub-app to configure
 * @param settings Limiter configuration
 */
void InstallFrameLimiter(SubApp& sub_app, FrameLimiterSettings settings = {});

/// @brief Adds a `FrameLimiter` and paces the main app's `Update()`.
class FrameLimiterPlugin final : public Plugin {
public:
  static constexpr std::string_view kName = "helios::app::FrameLimiterPlugin";

  /**
   * @brief Constructs the plugin.
   * @param settings Limiter configuration
   */
  explicit FrameLimiterPlugin(FrameLimiterSettings settings = {}) noexcept
      : settings_(settings) {}

  void Build(App& app) override { InstallFrameLimiter(app, settings_); }

private:
  FrameLimiterSettings settings_;
};

}  // namespace helios::app
