#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/ecs/entity.hpp>
#include <helios/core/ecs/event.hpp>

#include <cstdint>
#include <string_view>

namespace helios::ecs {

/**
 * @brief Event emitted when an entity is spawned/created.
 * @details This event is automatically emitted by World::CreateEntity() and World::ReserveEntity()
 * if the event is registered via AddEvent<EntitySpawnedEvent>().
 * @note Must be trivially copyable for event storage requirements.
 */
struct EntitySpawnedEvent {
  Entity entity;  ///< The spawned entity

  /**
   * @brief Gets the event name.
   * @return Event name string
   */
  static constexpr std::string_view GetName() noexcept { return "EntitySpawnedEvent"; }

  /**
   * @brief Gets the clear policy for this event.
   * @return Event clear policy (automatic)
   */
  static constexpr EventClearPolicy GetClearPolicy() noexcept { return EventClearPolicy::kAutomatic; }
};

/**
 * @brief Event emitted when an entity is destroyed.
 * @details This event is automatically emitted by World::DestroyEntity() and related methods
 * if the event is registered via AddEvent<EntityDestroyedEvent>().
 * @note Must be trivially copyable for event storage requirements.
 */
struct EntityDestroyedEvent {
  Entity entity;  ///< The destroyed entity

  /**
   * @brief Gets the event name.
   * @return Event name string
   */
  static constexpr std::string_view GetName() noexcept { return "EntityDestroyedEvent"; }

  /**
   * @brief Gets the clear policy for this event.
   * @return Event clear policy (automatic)
   */
  static constexpr EventClearPolicy GetClearPolicy() noexcept { return EventClearPolicy::kAutomatic; }
};

/**
 * @brief Exit code for application shutdown.
 */
enum class ShutdownExitCode : uint8_t {
  kSuccess = 0,  ///< Normal shutdown
  kFailure = 1,  ///< Shutdown due to error
};

/**
 * @brief Event emitted to request application shutdown.
 * @details This event is read by the default runner to gracefully stop the application loop.
 * Systems can emit this event to request shutdown with an optional exit code.
 * @note Must be trivially copyable for event storage requirements.
 *
 * @example
 * @code
 * void QuitSystem(SystemContext& ctx) {
 *   const auto& input = ctx.ReadResource<Input>();
 *   if (input.IsKeyPressed(Key::Escape)) {
 *     ctx.EmitEvent(ShutdownEvent{});
 *   }
 * }
 * @endcode
 */
struct ShutdownEvent {
  ShutdownExitCode exit_code = ShutdownExitCode::kSuccess;  ///< Exit code for the shutdown

  /**
   * @brief Gets the event name.
   * @return Event name string
   */
  static constexpr std::string_view GetName() noexcept { return "ShutdownEvent"; }

  /**
   * @brief Gets the clear policy for this event.
   * @details Uses manual clear policy since shutdown should persist until processed.
   * @return Event clear policy (manual)
   */
  static constexpr EventClearPolicy GetClearPolicy() noexcept { return EventClearPolicy::kManual; }
};

// Static assertions to ensure events meet requirements
static_assert(EventTrait<EntitySpawnedEvent>);
static_assert(EventTrait<EntityDestroyedEvent>);
static_assert(EventTrait<ShutdownEvent>);

}  // namespace helios::ecs
