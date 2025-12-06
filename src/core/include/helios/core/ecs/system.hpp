#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/app/access_policy.hpp>

#include <ctti/name.hpp>
#include <ctti/type_id.hpp>

#include <concepts>
#include <cstddef>
#include <string_view>

namespace helios::app {

class SystemContext;

}  // namespace helios::app

namespace helios::ecs {

/**
 * @brief Base class for all systems.
 * @details Systems are responsible for processing entities and their components.
 * Derived classes must implement:
 * - `void Update(helios::ecs::SystemContext&)` for per-frame or per-tick updates
 * - `static constexpr helios::app::AccessPolicy GetAccessPolicy()` to declare data access requirements
 * - `static constexpr std::string_view GetName() noexcept` (optional)
 *
 * @example
 * @code
 * struct MovementSystem : public helios::ecs::System {
 * public:
 *   ~System() override = default;
 *
 *   static constexpr std::string_view GetName() noexcept { return "MovementSystem"; }
 *
 *   static constexpr auto GetAccessPolicy() {
 *     return helios::app::AccessPolicy()
 *         .Query<Transform&, const Velocity&>()
 *         .ReadResources<CommandLineArgs>();
 *   }
 *
 *   void Update(helios::app::SystemContext& ctx) override {
 *     const auto& args = ctx.ReadResource<CommandLineArgs>();
 *     const int fps = args.TryGet<int>("fps");
 *     const float dt = 1.0F / fps;
 *
 *     auto query = ctx.Query().With<Transform, Velocity>().Get<Transform&, const Velocity&>();
 *     for (auto&& [transform, velocity] : query) {
 *       transform.position += velocity.value * dt;
 *     }
 *   }
 * };
 * @endcode
 */
class System {
public:
  virtual ~System() = default;

  /**
   * @brief Updates the system. This method is called every frame or tick.
   * @param ctx Context providing validated access to world and resources
   */
  virtual void Update(app::SystemContext& ctx) = 0;
};

/**
 * @brief Concept for valid system types.
 * @details A valid system must:
 * - Derive from System
 * - Be default constructible
 * - Provide `static constexpr app::AccessPolicy GetAccessPolicy() noexcept`
 */
template <typename T>
concept SystemTrait = std::derived_from<T, System> && std::constructible_from<T> && requires {
  { T::GetAccessPolicy() } -> std::same_as<app::AccessPolicy>;
};

/**
 * @brief Concept for systems with custom names.
 * @details A system with name trait must satisfy SystemTrait and provide:
 * - `static constexpr std::string_view GetName() noexcept`
 */
template <typename T>
concept SystemWithNameTrait = SystemTrait<T> && requires {
  { T::GetName() } -> std::same_as<std::string_view>;
};

/**
 * @brief Type ID for systems.
 */
using SystemTypeId = size_t;

/**
 * @brief Gets type ID for a system type.
 * @tparam T System type
 * @return Unique type ID for the system
 */
template <typename T>
constexpr SystemTypeId SystemTypeIdOf() noexcept {
  return ctti::type_index_of<T>().hash();
}

/**
 * @brief Gets the name of a system.
 * @details Returns provided name or type name as fallback.
 * @tparam T System type
 * @return System name
 */
template <SystemTrait T>
[[nodiscard]] constexpr std::string_view SystemNameOf() noexcept {
  if constexpr (SystemWithNameTrait<T>) {
    return T::GetName();
  } else {
    return ctti::name_of<T>();
  }
}

}  // namespace helios::ecs
