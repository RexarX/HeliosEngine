#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.window;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/app/schedules.hpp>
#include <helios/ecs/schedule/executor/executor.hpp>
#include <helios/ecs/schedule/schedule.hpp>
#include <helios/ecs/schedule/scheduler.hpp>

#include <string_view>
#include <utility>
#endif

HELIOS_MODULE_EXPORT
namespace helios::window {

/// @brief Stage for platform window event polling and synchronization.
struct WindowStage {
  static constexpr std::string_view kName = "helios::window::WindowStage";
};

/// @brief Builtin stage for window events.
inline constexpr WindowStage kWindowStage{};

/// @brief Main-thread schedule for platform window polling and synchronization.
struct Events {
  static constexpr std::string_view kName = "helios::window::Events";
};

/// @brief Builtin main-thread window events schedule.
inline constexpr Events kEvents{};

/**
 * @brief Registers the window events stage and schedule.
 * @details Creates `kWindowStage` ordered before `app::kUpdateStage`, and a
 * main-thread `kEvents` schedule inside that stage. Safe to call multiple
 * times.
 * @param scheduler Scheduler to populate
 */
inline void RegisterEventsSchedule(ecs::Scheduler& scheduler) {
  if (!scheduler.HasStage(kWindowStage)) {
    scheduler.AddStage(kWindowStage);
  }
  if (!scheduler.HasStage(app::kUpdateStage)) {
    scheduler.AddStage(app::kUpdateStage);
  }
  scheduler.OrderStage(kWindowStage).Before(app::kUpdateStage);

  if (scheduler.TryGetSchedule(kEvents) != nullptr) {
    return;
  }

  auto schedule = ecs::Schedule::From(kEvents);
  schedule.Settings().executor_kind = ecs::ExecutorKind::kMainThread;
  scheduler.Add(kEvents, std::move(schedule)).InStage(kWindowStage);
}

}  // namespace helios::window
#endif  // HELIOS_MODULE_CONSUMER_SHIM
