#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.ecs;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/utils/common_traits.hpp>

#include <array>
#include <cstddef>
#include <functional>
#endif
#include <helios/assert.hpp>
#include <helios/ecs/component/component.hpp>
#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/message/message.hpp>
#include <helios/ecs/resource/resource.hpp>
#include <helios/ecs/system/param.hpp>
#include <helios/ecs/world.hpp>

HELIOS_MODULE_EXPORT
namespace helios::ecs {

class AccessPolicy;
class AccessPolicyBuilder;

/**
 * @brief Thread-safe read-only view into the world.
 * @details Provides read-only introspection methods for entity, component,
 * resource, and message queries. All operations are thread-safe for read
 * access.
 * @note Thread-safe for read operations.
 */
class WorldView {
public:
  /**
   * @brief Constructs a WorldView from a World reference.
   * @param world Reference to the World to view.
   */
  explicit constexpr WorldView(const World& world) noexcept : world_(world) {}
  WorldView(const WorldView&) = delete;
  WorldView(WorldView&&) = delete;
  constexpr ~WorldView() noexcept = default;

  WorldView& operator=(const WorldView&) = delete;
  WorldView& operator=(WorldView&&) = delete;

  /**
   * @brief Checks if entity exists in the world.
   * @note Thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * @param entity Entity to check
   * @return True if entity exists, false otherwise
   */
  [[nodiscard]] bool EntityExists(Entity entity) const noexcept;

  /**
   * @brief Checks if entity has component.
   * @note Thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * @tparam T Component type to check
   * @param entity Entity to check
   * @return True if entity has the component
   */
  template <ComponentTrait T>
  [[nodiscard]] bool HasComponent(Entity entity) const;

  /**
   * @brief Checks if entity has components.
   * @note Thread-safe.
   * @warning Triggers assertion in next cases:
   * - Entity is invalid.
   * - World does not own entity.
   * @tparam Ts Components types
   * @param entity Entity to check
   * @return Array of bools indicating whether entity has each component
   */
  template <ComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
  [[nodiscard]] auto HasComponents(Entity entity) const
      -> std::array<bool, sizeof...(Ts)>;

  /**
   * @brief Checks if a resource exists.
   * @note Thread-safe.
   * @tparam T Resource type
   * @return True if resource exists, false otherwise
   */
  template <ResourceTrait T>
  [[nodiscard]] bool HasResource() const {
    return world_.get().template HasResource<T>();
  }

  /**
   * @brief Checks if a message type is registered.
   * @note Thread-safe.
   * @tparam T Message type
   * @return True if message type is registered, false otherwise
   */
  template <AnyMessageTrait T>
  [[nodiscard]] bool HasMessage() const noexcept {
    return world_.get().template HasMessage<T>();
  }

  /**
   * @brief Checks if messages of a specific type exist in message queue.
   * @note Thread-safe.
   * @tparam T Message type
   * @return True if messages exist, false otherwise
   */
  template <AnyMessageTrait T>
  [[nodiscard]] bool HasMessages() const noexcept {
    return world_.get().template HasMessages<T>();
  }

  /**
   * @brief Gets the number of entities in the world.
   * @note Thread-safe.
   * @return Number of entities in the world
   */
  [[nodiscard]] size_t EntityCount() const noexcept {
    return world_.get().EntityCount();
  }

  /**
   * @brief Gets the number of resources in the world.
   * @note Thread-safe.
   * @return Number of resources in the world
   */
  [[nodiscard]] size_t ResourceCount() const noexcept {
    return world_.get().ResourceCount();
  }

private:
  std::reference_wrapper<const World> world_;
};

inline bool WorldView::EntityExists(Entity entity) const noexcept {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  const auto& world = world_.get();
  HELIOS_ASSERT(world.Exists(entity),
                "Entity '{}' does not exist in the world!", entity);
  return world.Exists(entity);
}

template <ComponentTrait T>
inline bool WorldView::HasComponent(Entity entity) const {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  const auto& world = world_.get();
  HELIOS_ASSERT(world.Exists(entity),
                "Entity '{}' does not exist in the world!", entity);
  return world.HasComponent<T>(entity);
}

template <ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 0)
inline auto WorldView::HasComponents(Entity entity) const
    -> std::array<bool, sizeof...(Ts)> {
  HELIOS_ASSERT(entity.Valid(), "Entity '{}' is invalid!", entity);
  const auto& world = world_.get();
  HELIOS_ASSERT(world.Exists(entity),
                "Entity '{}' does not exist in the world!", entity);
  return world.template HasComponents<Ts...>(entity);
}

template <>
struct SystemParamTraits<WorldView> {
  static constexpr WorldView Make(World& world, SystemLocalData& /*data*/,
                                  const AccessPolicy& /*policy*/) noexcept {
    return WorldView(world);
  }

  static constexpr void RegisterAccess(
      AccessPolicyBuilder& /*builder*/) noexcept {}
};

}  // namespace helios::ecs
#endif  // HELIOS_MODULE_CONSUMER_SHIM
