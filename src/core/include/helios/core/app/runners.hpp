#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/app/app.hpp>
#include <helios/core/app/time.hpp>
#include <helios/core/ecs/events/builtin_events.hpp>
#include <helios/core/ecs/world.hpp>
#include <helios/core/logger.hpp>
#include <helios/core/timer.hpp>

#include <chrono>
#include <cstdint>
#include <exception>
#include <utility>

namespace helios::app {

/**
 * @brief Checks if a shutdown event has been received.
 * @param app Application to check
 * @return Pair of (should_shutdown, exit_code)
 */
[[nodiscard]] inline auto CheckShutdownEvent(App& app) noexcept -> std::pair<bool, ecs::ShutdownExitCode> {
  const auto& world = app.GetMainWorld();

  if (!world.HasEvent<ecs::ShutdownEvent>()) {
    return {false, ecs::ShutdownExitCode::kSuccess};
  }

  const auto reader = world.ReadEvents<ecs::ShutdownEvent>();
  if (reader.Empty()) {
    return {false, ecs::ShutdownExitCode::kSuccess};
  }

  // Return the first shutdown event's exit code
  const auto events = reader.Read();
  return {true, events.front().exit_code};
}

/**
 * @brief Converts shutdown exit code to app exit code.
 */
[[nodiscard]] constexpr AppExitCode ToAppExitCode(ecs::ShutdownExitCode code) noexcept {
  switch (code) {
    case ecs::ShutdownExitCode::kSuccess:
      return AppExitCode::kSuccess;
    default:
      return AppExitCode::kFailure;
  }
}

/**
 * @brief Configuration for the default runner.
 */
struct DefaultRunnerConfig {
  bool update_time_resource = true;  ///< Whether to automatically update the Time resource
};

/**
 * @brief Default runner that runs until a ShutdownEvent is received.
 * @details This runner:
 * - Updates the Time resource each frame (if configured)
 * - Checks for ShutdownEvent to gracefully exit
 * - Handles exceptions and returns appropriate exit codes
 *
 * @param app Reference to the application
 * @param config Runner configuration
 * @return Exit code based on shutdown event or exception
 *
 * @example
 * @code
 * App app;
 * app.SetRunner([](App& app) {
 *   return DefaultRunner(app);
 * });
 * app.Run();
 * @endcode
 */
inline AppExitCode DefaultRunner(App& app, DefaultRunnerConfig config = {}) {
  try {
    while (app.IsRunning()) {
      if (config.update_time_resource) [[likely]] {
        app.TickTime();
      }

      app.Update();

      // Check for shutdown event
      const auto [should_shutdown, exit_code] = CheckShutdownEvent(app);
      if (should_shutdown) [[unlikely]] {
        return ToAppExitCode(exit_code);
      }
    }
  } catch (const std::exception& exception) {
    HELIOS_CRITICAL("Application encountered an unhandled exception and will exit: {}!", exception.what());
    return AppExitCode::kFailure;
  }

  return AppExitCode::kSuccess;
}

/**
 * @brief Configuration for the frame-limited runner.
 */
struct FrameLimitedRunnerConfig {
  uint64_t max_frames = 1;           ///< Maximum number of frames to run
  bool update_time_resource = true;  ///< Whether to automatically update the Time resource
};

/**
 * @brief Runner that executes for a fixed number of frames.
 * @details This runner:
 * - Runs for exactly the specified number of frames
 * - Updates the Time resource each frame (if configured)
 * - Respects ShutdownEvent for early termination
 *
 * @param app Reference to the application
 * @param config Runner configuration with max frame count
 * @return Success if all frames completed, otherwise based on shutdown event
 *
 * @example
 * @code
 * App app;
 * app.SetRunner([](App& app) {
 *   return FrameLimitedRunner(app, {.max_frames = 100});
 * });
 * app.Run();
 * @endcode
 */
inline AppExitCode FrameLimitedRunner(App& app, FrameLimitedRunnerConfig config) {
  try {
    uint64_t frame = 0;
    while (app.IsRunning() && frame < config.max_frames) {
      if (config.update_time_resource) [[likely]] {
        app.TickTime();
      }

      app.Update();
      ++frame;

      // Check for shutdown event
      const auto [should_shutdown, exit_code] = CheckShutdownEvent(app);
      if (should_shutdown) [[unlikely]] {
        return ToAppExitCode(exit_code);
      }
    }
  } catch (const std::exception& exception) {
    HELIOS_CRITICAL("Application encountered an unhandled exception and will exit: {}!", exception.what());
    return AppExitCode::kFailure;
  }

  return AppExitCode::kSuccess;
}

/**
 * @brief Configuration for the timed runner.
 */
struct TimedRunnerConfig {
  float duration_seconds = 1.0F;     ///< Duration to run in seconds
  bool update_time_resource = true;  ///< Whether to automatically update the Time resource
};

/**
 * @brief Runner that executes for a specified duration.
 * @details This runner:
 * - Runs for the specified duration in seconds
 * - Updates the Time resource each frame (if configured)
 * - Respects ShutdownEvent for early termination
 *
 * @param app Reference to the application
 * @param config Runner configuration with duration
 * @return Success if duration completed, otherwise based on shutdown event
 *
 * @example
 * @code
 * App app;
 * app.SetRunner([](App& app) {
 *   return TimedRunner(app, {.duration_seconds = 5.0F});
 * });
 * app.Run();
 * @endcode
 */
inline AppExitCode TimedRunner(App& app, TimedRunnerConfig config) {
  try {
    const auto duration_seconds = static_cast<double>(config.duration_seconds);

    Timer timer;
    while (app.IsRunning()) {
      if (timer.ElapsedSec() >= duration_seconds) {
        break;
      }

      if (config.update_time_resource) [[likely]] {
        app.TickTime();
      }

      app.Update();

      // Check for shutdown event
      const auto [should_shutdown, exit_code] = CheckShutdownEvent(app);
      if (should_shutdown) [[unlikely]] {
        return ToAppExitCode(exit_code);
      }
    }
  } catch (const std::exception& exception) {
    HELIOS_CRITICAL("Application encountered an unhandled exception and will exit: {}!", exception.what());
    return AppExitCode::kFailure;
  }

  return AppExitCode::kSuccess;
}

/**
 * @brief Configuration for the fixed timestep runner.
 */
struct FixedTimestepRunnerConfig {
  float fixed_delta_seconds = 1.0F / 60.0F;  ///< Fixed timestep in seconds (default: 60 FPS)
  uint32_t max_substeps = 10;                ///< Maximum physics substeps per frame to prevent spiral of death
  bool update_time_resource = true;          ///< Whether to automatically update the Time resource
};

/**
 * @brief Runner that uses fixed timestep for deterministic updates.
 * @details This runner:
 * - Uses a fixed delta time for each Update() call
 * - Accumulates real time and catches up with multiple substeps if needed
 * - Limits substeps to prevent spiral of death
 * - Respects ShutdownEvent for termination
 *
 * @param app Reference to the application
 * @param config Runner configuration with fixed timestep settings
 * @return Exit code based on shutdown event or exception
 *
 * @example
 * @code
 * App app;
 * app.SetRunner([](App& app) {
 *   return FixedTimestepRunner(app, {.fixed_delta_seconds = 1.0F / 120.0F});
 * });
 * app.Run();
 * @endcode
 */
inline AppExitCode FixedTimestepRunner(App& app, FixedTimestepRunnerConfig config) {
  try {
    using Duration = std::chrono::duration<float>;

    const Duration fixed_delta{config.fixed_delta_seconds};
    Duration accumulator{0.0F};

    Timer frame_timer;
    while (app.IsRunning()) {
      // Calculate frame time using Timer
      const auto frame_time = frame_timer.ElapsedDuration<Duration>();
      frame_timer.Reset();

      accumulator += frame_time;

      // Limit the number of substeps to prevent spiral of death
      uint32_t substeps = 0;
      while (accumulator >= fixed_delta && substeps < config.max_substeps) {
        if (config.update_time_resource) [[likely]] {
          app.TickTime();
        }

        app.Update();
        accumulator -= fixed_delta;
        ++substeps;

        // Check for shutdown event
        const auto [should_shutdown, exit_code] = CheckShutdownEvent(app);
        if (should_shutdown) [[unlikely]] {
          return ToAppExitCode(exit_code);
        }
      }

      // If we hit max substeps, clamp accumulator to prevent runaway
      if (substeps >= config.max_substeps && accumulator >= fixed_delta) {
        HELIOS_WARN("Fixed timestep runner hit max substeps ({}), clamping accumulator", config.max_substeps);
        accumulator = Duration{0.0F};
      }
    }
  } catch (const std::exception& exception) {
    HELIOS_CRITICAL("Application encountered an unhandled exception and will exit: {}!", exception.what());
    return AppExitCode::kFailure;
  }

  return AppExitCode::kSuccess;
}

/**
 * @brief Configuration for the once runner.
 */
struct OnceRunnerConfig {
  bool update_time_resource = true;  ///< Whether to update the Time resource
};

/**
 * @brief Runner that executes exactly one frame.
 * @details Useful for testing or single-shot operations.
 *
 * @param app Reference to the application
 * @param config Runner configuration
 * @return Success after single frame
 *
 * @example
 * @code
 * App app;
 * app.SetRunner([](App& app) {
 *   return OnceRunner(app);
 * });
 * app.Run();
 * @endcode
 */
inline AppExitCode OnceRunner(App& app, OnceRunnerConfig config = {}) {
  try {
    if (app.IsRunning()) {
      if (config.update_time_resource) [[likely]] {
        app.TickTime();
      }

      app.Update();
    }
  } catch (const std::exception& exception) {
    HELIOS_CRITICAL("Application encountered an unhandled exception and will exit: {}!", exception.what());
    return AppExitCode::kFailure;
  }

  return AppExitCode::kSuccess;
}

}  // namespace helios::app
