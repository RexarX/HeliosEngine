#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/ecs/entity.hpp>
#include <helios/core/ecs/event.hpp>

#include <string_view>
#include <type_traits>

namespace helios::ecs::events {

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

// Static assertions to ensure events meet requirements
static_assert(std::is_trivially_copyable_v<EntitySpawnedEvent>, "EntitySpawnedEvent must be trivially copyable");
static_assert(std::is_trivially_copyable_v<EntityDestroyedEvent>, "EntityDestroyedEvent must be trivially copyable");

static_assert(sizeof(EntitySpawnedEvent) <= 128, "EntitySpawnedEvent should be reasonably sized");
static_assert(sizeof(EntityDestroyedEvent) <= 128, "EntityDestroyedEvent should be reasonably sized");

}  // namespace helios::ecs::events
