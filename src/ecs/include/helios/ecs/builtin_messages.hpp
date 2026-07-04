#pragma once

#include <helios/assert.hpp>
#include <helios/ecs/component/component.hpp>
#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/message/message.hpp>
#include <helios/ecs/resource/resource.hpp>

#include <string_view>

namespace helios::ecs {

/// @brief Message sent when an entity is added.
class EntityAddedMsg {
public:
  static constexpr std::string_view kName = "EntityAddedMsg";
  static constexpr auto kClearPolicy = MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  /**
   * @brief Constructs entity added message.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Destroyed entity
   */
  explicit constexpr EntityAddedMsg(Entity entity) noexcept : entity_(entity) {
    HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  }

  constexpr EntityAddedMsg(const EntityAddedMsg&) noexcept = default;
  constexpr EntityAddedMsg(EntityAddedMsg&&) noexcept = default;
  constexpr ~EntityAddedMsg() noexcept = default;

  constexpr EntityAddedMsg& operator=(const EntityAddedMsg&) noexcept = default;
  constexpr EntityAddedMsg& operator=(EntityAddedMsg&&) noexcept = default;

  /**
   * @brief Gets the entity that was added.
   * @return Entity that was added
   */
  [[nodiscard]] constexpr Entity GetEntity() const noexcept { return entity_; }

private:
  Entity entity_;  ///< The entity that was added
};

/// @brief Message sent when an entity is destroyed.
class EntityDestroyedMsg {
public:
  static constexpr std::string_view kName = "EntityDestroyedMsg";
  static constexpr auto kClearPolicy = MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  /**
   * @brief Constructs entity destroyed message.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Destroyed entity
   */
  explicit constexpr EntityDestroyedMsg(Entity entity) noexcept
      : entity_(entity) {
    HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  }

  constexpr EntityDestroyedMsg(const EntityDestroyedMsg&) noexcept = default;
  constexpr EntityDestroyedMsg(EntityDestroyedMsg&&) noexcept = default;
  constexpr ~EntityDestroyedMsg() noexcept = default;

  constexpr EntityDestroyedMsg& operator=(const EntityDestroyedMsg&) noexcept =
      default;
  constexpr EntityDestroyedMsg& operator=(EntityDestroyedMsg&&) noexcept =
      default;

  /**
   * @brief Gets the entity that was destroyed.
   * @return Entity that was destroyed
   */
  [[nodiscard]] constexpr Entity GetEntity() const noexcept { return entity_; }

private:
  Entity entity_;  ///< The entity that was destroyed
};

/// @brief Message sent when a component is added.
template <ComponentTrait T>
class ComponentAddedMsg {
public:
  static constexpr std::string_view kName = "ComponentAddedMsg";
  static constexpr auto kClearPolicy = MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  /**
   * @brief Constructs component added message.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity that component was added to
   */
  explicit constexpr ComponentAddedMsg(Entity entity) noexcept
      : entity_(entity) {
    HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  }

  constexpr ComponentAddedMsg(const ComponentAddedMsg&) noexcept = default;
  constexpr ComponentAddedMsg(ComponentAddedMsg&&) noexcept = default;
  constexpr ~ComponentAddedMsg() noexcept = default;

  constexpr ComponentAddedMsg& operator=(const ComponentAddedMsg&) noexcept =
      default;
  constexpr ComponentAddedMsg& operator=(ComponentAddedMsg&&) noexcept =
      default;

  /**
   * @brief Gets the entity that the component was added to.
   * @return Entity that the component was added to
   */
  [[nodiscard]] constexpr Entity GetEntity() const noexcept { return entity_; }

private:
  Entity entity_;  ///< The entity that the component was added to
};

/// @brief Message sent when a component is removed.
template <ComponentTrait T>
class ComponentRemovedMsg {
public:
  static constexpr std::string_view kName = "ComponentRemovedMsg";
  static constexpr auto kClearPolicy = MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  /**
   * @brief Constructs component removed message.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity that component was removed from
   */
  explicit constexpr ComponentRemovedMsg(Entity entity) noexcept
      : entity_(entity) {
    HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  }

  constexpr ComponentRemovedMsg(const ComponentRemovedMsg&) noexcept = default;
  constexpr ComponentRemovedMsg(ComponentRemovedMsg&&) noexcept = default;
  constexpr ~ComponentRemovedMsg() noexcept = default;

  constexpr ComponentRemovedMsg& operator=(
      const ComponentRemovedMsg&) noexcept = default;
  constexpr ComponentRemovedMsg& operator=(ComponentRemovedMsg&&) noexcept =
      default;

  /**
   * @brief Gets the entity that the component was removed from.
   * @return Entity that the component was removed from
   */
  [[nodiscard]] constexpr Entity GetEntity() const noexcept { return entity_; }

private:
  Entity entity_;  ///< The entity that the component was removed from
};

/// @brief Message sent when all components are cleared from an entity.
class ComponentsClearedMsg {
public:
  static constexpr std::string_view kName = "ComponentsClearedMsg";
  static constexpr auto kClearPolicy = MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;

  /**
   * @brief Constructs components cleared message.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity that components were cleared from
   */
  explicit constexpr ComponentsClearedMsg(Entity entity) noexcept
      : entity_(entity) {
    HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  }

  constexpr ComponentsClearedMsg(const ComponentsClearedMsg&) noexcept =
      default;
  constexpr ComponentsClearedMsg(ComponentsClearedMsg&&) noexcept = default;
  constexpr ~ComponentsClearedMsg() noexcept = default;

  constexpr ComponentsClearedMsg& operator=(
      const ComponentsClearedMsg&) noexcept = default;
  constexpr ComponentsClearedMsg& operator=(ComponentsClearedMsg&&) noexcept =
      default;

  /**
   * @brief Gets the entity that the components were cleared from.
   * @return Entity that the components were cleared from
   */
  [[nodiscard]] constexpr Entity GetEntity() const noexcept { return entity_; }

private:
  Entity entity_;  ///< The entity that the components were cleared from
};

/// @brief Message sent when a resource is inserted.
template <ResourceTrait T>
struct ResourceInsertedMsg {
  static constexpr std::string_view kName = "ResourceInsertedMsg";
  static constexpr auto kClearPolicy = MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;
};

/// @brief Message sent when a resource is removed.
template <ResourceTrait T>
struct ResourceRemovedMsg {
  static constexpr std::string_view kName = "ResourceRemovedMsg";
  static constexpr auto kClearPolicy = MessageClearPolicy::kAutomatic;
  static constexpr bool kConsumable = false;
  static constexpr bool kAsync = false;
};

}  // namespace helios::ecs
