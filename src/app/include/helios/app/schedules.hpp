#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.app;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <string_view>
#endif

HELIOS_MODULE_EXPORT
namespace helios::ecs {

class World;
class Scheduler;

}  // namespace helios::ecs

HELIOS_MODULE_EXPORT
namespace helios::app {

/// @brief Stage for one-time startup execution.
struct StartupStage {
  static constexpr std::string_view kName = "helios::app::StartupStage";
};

/// @brief Builtin stage for startup.
inline constexpr StartupStage kStartupStage{};

/// @brief Stage for per-frame game logic.
struct UpdateStage {
  static constexpr std::string_view kName = "helios::app::UpdateStage";
};

/// @brief Builtin stage for per-frame updates.
inline constexpr UpdateStage kUpdateStage{};

/// @brief Stage for per-frame extraction into sub-apps.
struct ExtractStage {
  static constexpr std::string_view kName = "helios::app::ExtractStage";
};

/// @brief Builtin stage for extraction.
inline constexpr ExtractStage kExtractStage{};

/// @brief Stage for one-time shutdown execution.
struct ShutdownStage {
  static constexpr std::string_view kName = "helios::app::ShutdownStage";
};

/// @brief Builtin stage for shutdown.
inline constexpr ShutdownStage kShutdownStage{};

/// @brief Main-thread startup schedule (window creation, etc.).
struct MainStartup {
  static constexpr std::string_view kName = "helios::app::MainStartup";
};

/// @brief Builtin main-thread startup schedule.
inline constexpr MainStartup kMainStartup{};

/// @brief Pre-startup schedule.
struct PreStartup {
  static constexpr std::string_view kName = "helios::app::PreStartup";
};

/// @brief Builtin pre-startup schedule.
inline constexpr PreStartup kPreStartup{};

/// @brief Startup schedule.
struct Startup {
  static constexpr std::string_view kName = "helios::app::Startup";
};

/// @brief Builtin startup schedule.
inline constexpr Startup kStartup{};

/// @brief Post-startup schedule.
struct PostStartup {
  static constexpr std::string_view kName = "helios::app::PostStartup";
};

/// @brief Builtin post-startup schedule.
inline constexpr PostStartup kPostStartup{};

/// @brief First schedule in a frame.
struct First {
  static constexpr std::string_view kName = "helios::app::First";
};

/// @brief Builtin first schedule.
inline constexpr First kFirst{};

/// @brief Pre-update schedule.
struct PreUpdate {
  static constexpr std::string_view kName = "helios::app::PreUpdate";
};

/// @brief Builtin pre-update schedule.
inline constexpr PreUpdate kPreUpdate{};

/// @brief Update schedule.
struct Update {
  static constexpr std::string_view kName = "helios::app::Update";
};

/// @brief Builtin update schedule.
inline constexpr Update kUpdate{};

/// @brief Post-update schedule.
struct PostUpdate {
  static constexpr std::string_view kName = "helios::app::PostUpdate";
};

/// @brief Builtin post-update schedule.
inline constexpr PostUpdate kPostUpdate{};

/// @brief Last schedule in a frame.
struct Last {
  static constexpr std::string_view kName = "helios::app::Last";
};

/// @brief Builtin last schedule.
inline constexpr Last kLast{};

/// @brief Extract schedule (runs after the last frame schedule).
struct Extract {
  static constexpr std::string_view kName = "helios::app::Extract";
};

/// @brief Builtin extract schedule.
inline constexpr Extract kExtract{};

/// @brief Pre-shutdown schedule.
struct PreShutdown {
  static constexpr std::string_view kName = "helios::app::PreShutdown";
};

/// @brief Builtin pre-shutdown schedule.
inline constexpr PreShutdown kPreShutdown{};

/// @brief Shutdown schedule.
struct Shutdown {
  static constexpr std::string_view kName = "helios::app::Shutdown";
};

/// @brief Builtin shutdown schedule.
inline constexpr Shutdown kShutdown{};

/// @brief Post-shutdown schedule.
struct PostShutdown {
  static constexpr std::string_view kName = "helios::app::PostShutdown";
};

/// @brief Builtin post-shutdown schedule.
inline constexpr PostShutdown kPostShutdown{};

/**
 * @brief Registers default application schedules on an ECS scheduler.
 * @details Creates empty schedules for all built-in types and assigns default
 * executor kinds, stage membership, and ordering.
 * @param scheduler ECS scheduler to populate
 */
void RegisterBuiltinSchedules(ecs::Scheduler& scheduler);

/**
 * @brief Inserts default `MainFrameOrder` and `FramePumpOrder` resources.
 * @details Safe to call multiple times (`TryInsertResources`).
 * @param world World that owns the frame-order resources
 */
void RegisterBuiltinFrameOrders(ecs::World& world);

/**
 * @brief Registers built-in schedules for a sub-app ECS scheduler.
 * @details Same as @ref RegisterBuiltinSchedules except extract stage and
 * schedules are omitted; extraction is driven by the main app via
 * `SubApp::Extract`.
 * @param scheduler ECS scheduler to populate
 */
void RegisterBuiltinSubAppSchedules(ecs::Scheduler& scheduler);

}  // namespace helios::app
#endif  // HELIOS_MODULE_CONSUMER_SHIM
