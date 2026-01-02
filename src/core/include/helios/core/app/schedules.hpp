#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/app/schedule.hpp>

#include <array>
#include <string_view>

namespace helios::app {

//==============================================================================
// STAGE SCHEDULES (Hardcoded)
//==============================================================================
// These are the four core execution stages that establish the application lifecycle.
// All other schedules are executed within these stages based on Before/After relationships.

/**
 * @brief StartUpStage - application initialization phase.
 * @details This is the first stage executed during application initialization.
 * Runs once at application start. Systems in this stage initialize resources,
 * load configuration, and prepare the application for the main loop.
 *
 * Schedules in this stage: PreStartup, Startup, PostStartup
 */
struct StartUpStage {
  static constexpr bool IsStage() noexcept { return true; }
  static constexpr std::string_view GetName() noexcept { return "StartUpStage"; }
};

/**
 * @brief MainStage - main thread execution phase.
 * @details Executes synchronously on the main thread for tasks that require
 * main thread context (e.g., window events, input polling).
 * This stage runs on every frame before the UpdateStage.
 *
 * Schedules in this stage: Main
 */
struct MainStage {
  static constexpr bool IsStage() noexcept { return true; }
  static constexpr std::string_view GetName() noexcept { return "MainStage"; }
};

/**
 * @brief UpdateStage - main update logic phase.
 * @details This is where most game/simulation logic runs on every frame.
 * Executes after MainStage and can run systems in parallel on worker threads.
 *
 * Schedules in this stage: First, PreUpdate, Update, PostUpdate, Last
 */
struct UpdateStage {
  static constexpr bool IsStage() noexcept { return true; }
  static constexpr std::string_view GetName() noexcept { return "UpdateStage"; }
};

/**
 * @brief CleanUpStage - cleanup/shutdown phase.
 * @details This is the final stage executed during application shutdown.
 * Runs once at application exit. Systems in this stage release resources,
 * save state, and perform cleanup operations.
 *
 * Schedules in this stage: PreCleanUp, CleanUp, PostCleanUp
 */
struct CleanUpStage {
  static constexpr bool IsStage() noexcept { return true; }
  static constexpr std::string_view GetName() noexcept { return "CleanUpStage"; }
};

//==============================================================================
// STARTUP STAGE SCHEDULES
//==============================================================================

// Forward declarations
struct PreStartup;
struct Startup;
struct PostStartup;

/**
 * @brief PreStartup schedule - runs before startup initialization.
 * @details First schedule in the StartUpStage.
 * Used for early initialization tasks that must complete before main startup.
 * Executes: Before(Startup)
 */
struct PreStartup {
  static constexpr ScheduleId GetStage() noexcept { return ScheduleIdOf<StartUpStage>(); }
  static constexpr auto Before() noexcept -> std::array<ScheduleId, 1>;
  static constexpr auto After() noexcept -> std::array<ScheduleId, 0> { return {}; }
  static constexpr std::string_view GetName() noexcept { return "PreStartup"; }
};

/**
 * @brief Startup schedule - main initialization.
 * @details Main initialization schedule in the StartUpStage.
 * Used for setting up systems and resources.
 * Executes: After(PreStartup) and Before(PostStartup)
 */
struct Startup {
  static constexpr ScheduleId GetStage() noexcept { return ScheduleIdOf<StartUpStage>(); }
  static constexpr auto Before() noexcept -> std::array<ScheduleId, 1>;
  static constexpr auto After() noexcept -> std::array<ScheduleId, 1> { return {ScheduleIdOf<PreStartup>()}; }
  static constexpr std::string_view GetName() noexcept { return "Startup"; }
};

/**
 * @brief PostStartup schedule - runs after startup initialization.
 * @details Final schedule in the StartUpStage.
 * Used for tasks that depend on main startup completion.
 * Executes: After(Startup)
 */
struct PostStartup {
  static constexpr ScheduleId GetStage() noexcept { return ScheduleIdOf<StartUpStage>(); }
  static constexpr auto Before() noexcept -> std::array<ScheduleId, 0> { return {}; }
  static constexpr auto After() noexcept -> std::array<ScheduleId, 1> { return {ScheduleIdOf<Startup>()}; }
  static constexpr std::string_view GetName() noexcept { return "PostStartup"; }
};

constexpr auto PreStartup::Before() noexcept -> std::array<ScheduleId, 1> {
  return {ScheduleIdOf<Startup>()};
}

constexpr auto Startup::Before() noexcept -> std::array<ScheduleId, 1> {
  return {ScheduleIdOf<PostStartup>()};
}

//==============================================================================
// MAIN STAGE SCHEDULES
//==============================================================================

/**
 * @brief Main schedule - main thread execution.
 * @details Main schedule in the MainStage (executes every frame).
 * Used for tasks that must run on the main thread.
 * Examples: window event handling, input polling, main thread UI updates.
 */
struct Main {
  static constexpr ScheduleId GetStage() noexcept { return ScheduleIdOf<MainStage>(); }
  static constexpr auto Before() noexcept -> std::array<ScheduleId, 0> { return {}; }
  static constexpr auto After() noexcept -> std::array<ScheduleId, 0> { return {}; }
  static constexpr std::string_view GetName() noexcept { return "Main"; }
};

//==============================================================================
// UPDATE STAGE SCHEDULES
//==============================================================================

// Forward declarations for Update stage schedules
struct First;
struct PreUpdate;
struct Update;
struct PostUpdate;
struct Last;

/**
 * @brief First schedule - runs first in the UpdateStage.
 * @details First schedule in the UpdateStage (executes every frame).
 * Runs after the MainStage and before PreUpdate.
 * Used for tasks that need to run at the very beginning of the update phase.
 * Executes: Before(PreUpdate)
 */
struct First {
  static constexpr ScheduleId GetStage() noexcept { return ScheduleIdOf<UpdateStage>(); }
  static constexpr auto Before() noexcept -> std::array<ScheduleId, 1>;
  static constexpr auto After() noexcept -> std::array<ScheduleId, 0> { return {}; }
  static constexpr std::string_view GetName() noexcept { return "First"; }
};

/**
 * @brief PreUpdate schedule - runs before the main update.
 * @details Runs after First schedule in the UpdateStage (executes every frame).
 * Used for pre-processing tasks that must complete before main update logic.
 * Executes: After(First) and Before(Update)
 */
struct PreUpdate {
  static constexpr ScheduleId GetStage() noexcept { return ScheduleIdOf<UpdateStage>(); }
  static constexpr auto Before() noexcept -> std::array<ScheduleId, 1>;
  static constexpr auto After() noexcept -> std::array<ScheduleId, 1> { return {ScheduleIdOf<First>()}; }
  static constexpr std::string_view GetName() noexcept { return "PreUpdate"; }
};

/**
 * @brief Update schedule - main update logic.
 * @details Main update schedule in the UpdateStage (executes every frame).
 * Used for core game/simulation logic.
 * Executes: After(PreUpdate) and Before(PostUpdate)
 */
struct Update {
  static constexpr ScheduleId GetStage() noexcept { return ScheduleIdOf<UpdateStage>(); }
  static constexpr auto Before() noexcept -> std::array<ScheduleId, 1>;
  static constexpr auto After() noexcept -> std::array<ScheduleId, 1> { return {ScheduleIdOf<PreUpdate>()}; }
  static constexpr std::string_view GetName() noexcept { return "Update"; }
};

/**
 * @brief PostUpdate schedule - runs after the main update.
 * @details Runs after Update schedule in the UpdateStage (executes every frame).
 * Used for post-processing tasks after main update logic.
 * Examples: physics cleanup, constraint resolution, late transforms, data extraction.
 * Executes: After(Update) and Before(Last)
 */
struct PostUpdate {
  static constexpr ScheduleId GetStage() noexcept { return ScheduleIdOf<UpdateStage>(); }
  static constexpr auto Before() noexcept -> std::array<ScheduleId, 1>;
  static constexpr auto After() noexcept -> std::array<ScheduleId, 1> { return {ScheduleIdOf<Update>()}; }
  static constexpr std::string_view GetName() noexcept { return "PostUpdate"; }
};

/**
 * @brief Last schedule - runs last in the UpdateStage.
 * @details Final schedule in the UpdateStage (executes every frame).
 * Runs after PostUpdate and before the CleanUpStage.
 * Used for tasks that need to run at the very end of the update phase.
 * Executes: After(PostUpdate)
 */
struct Last {
  static constexpr ScheduleId GetStage() noexcept { return ScheduleIdOf<UpdateStage>(); }
  static constexpr auto Before() noexcept -> std::array<ScheduleId, 0> { return {}; }
  static constexpr auto After() noexcept -> std::array<ScheduleId, 1> { return {ScheduleIdOf<PostUpdate>()}; }
  static constexpr std::string_view GetName() noexcept { return "Last"; }
};

// Define deferred Before() implementations for Update stage schedules
constexpr auto First::Before() noexcept -> std::array<ScheduleId, 1> {
  return {ScheduleIdOf<PreUpdate>()};
}

constexpr auto PreUpdate::Before() noexcept -> std::array<ScheduleId, 1> {
  return {ScheduleIdOf<Update>()};
}

constexpr auto Update::Before() noexcept -> std::array<ScheduleId, 1> {
  return {ScheduleIdOf<PostUpdate>()};
}

constexpr auto PostUpdate::Before() noexcept -> std::array<ScheduleId, 1> {
  return {ScheduleIdOf<Last>()};
}

//==============================================================================
// CLEANUP STAGE SCHEDULES
//==============================================================================

/**
 * @brief CleanUp schedule - main cleanup.
 * @details Main cleanup schedule in the CleanUpStage.
 * Used for releasing resources and shutting down systems.
 * Executes: After(PreCleanUp) and Before(PostCleanUp)
 */
struct CleanUp {
  static constexpr ScheduleId GetStage() noexcept { return ScheduleIdOf<CleanUpStage>(); }
  static constexpr auto Before() noexcept -> std::array<ScheduleId, 0> { return {}; }
  static constexpr auto After() noexcept -> std::array<ScheduleId, 0> { return {}; }
  static constexpr std::string_view GetName() noexcept { return "CleanUp"; }
};

/**
 * @brief PreCleanUp schedule - runs before cleanup/shutdown.
 * @details First schedule in the CleanUpStage.
 * Used for tasks that must run before main cleanup.
 * Examples: saving state, flushing caches, disconnecting from servers.
 * Executes: Before(CleanUp)
 */
struct PreCleanUp {
  static constexpr ScheduleId GetStage() noexcept { return ScheduleIdOf<CleanUpStage>(); }
  static constexpr auto Before() noexcept -> std::array<ScheduleId, 1> { return {ScheduleIdOf<CleanUp>()}; }
  static constexpr auto After() noexcept -> std::array<ScheduleId, 0> { return {}; }
  static constexpr std::string_view GetName() noexcept { return "PreCleanUp"; }
};

/**
 * @brief PostCleanUp schedule - runs after cleanup/shutdown.
 * @details Final schedule in the CleanUpStage.
 * Used for tasks that must run after main cleanup.
 * Examples: final resource deallocation, logger shutdown, profiler finalization.
 * Executes: After(CleanUp)
 */
struct PostCleanUp {
  static constexpr ScheduleId GetStage() noexcept { return ScheduleIdOf<CleanUpStage>(); }
  static constexpr auto Before() noexcept -> std::array<ScheduleId, 0> { return {}; }
  static constexpr auto After() noexcept -> std::array<ScheduleId, 1> { return {ScheduleIdOf<CleanUp>()}; }
  static constexpr std::string_view GetName() noexcept { return "PostCleanUp"; }
};

/**
 * @brief Constexpr instances of schedules for easier user interface.
 * @details These instances can be used directly without having to construct the schedule types.
 *
 * @example
 * @code
 * scheduler.AddSystem(kPreStartup, my_system);
 * @endcode
 */

inline constexpr Main kMain{};

inline constexpr PreStartup kPreStartup{};
inline constexpr Startup kStartup{};
inline constexpr PostStartup kPostStartup{};

inline constexpr First kFirst{};
inline constexpr PreUpdate kPreUpdate{};
inline constexpr Update kUpdate{};
inline constexpr PostUpdate kPostUpdate{};
inline constexpr Last kLast{};

inline constexpr PreCleanUp kPreCleanUp{};
inline constexpr CleanUp kCleanUp{};
inline constexpr PostCleanUp kPostCleanUp{};

}  // namespace helios::app
