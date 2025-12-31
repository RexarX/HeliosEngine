#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/app/details/scheduler.hpp>
#include <helios/core/app/schedule.hpp>
#include <helios/core/app/system_set.hpp>
#include <helios/core/ecs/system.hpp>
#include <helios/core/utils/common_traits.hpp>

#include <array>
#include <cstddef>
#include <functional>
#include <span>
#include <utility>
#include <vector>

namespace helios::app {

// Forward declaration
class SubApp;

/**
 * @brief Fluent builder for configuring systems with ordering and sets.
 * @details Provides a chainable API for specifying system dependencies, set membership and
 * relationships with other system sets. The configuration is applied when the builder is
 * destroyed or when explicitly finalized.
 * @tparam Schedule Schedule type where systems are added
 * @tparam Systems System types being configured
 *
 * @example
 * @code
 * app.AddSystems<MovementSystem, CollisionSystem>(kUpdate)
 *     .After<InputSystem>()
 *     .Before<RenderSystem>()
 *     .InSet<PhysicsSet>()
 *     .AfterSet<InputSet>()
 *     .BeforeSet<RenderSet>()
 *     .Sequence();
 * @endcode
 */
template <ScheduleTrait Schedule, ecs::SystemTrait... Systems>
  requires(sizeof...(Systems) > 0 && utils::UniqueTypes<Systems...>)
class SystemConfig {
public:
  /**
   * @brief Constructs a system configuration builder.
   * @param sub_app Reference to the sub-app where systems will be added
   * @param schedule Schedule instance where systems are added
   */
  explicit SystemConfig(SubApp& sub_app, Schedule schedule = {}) noexcept : sub_app_(sub_app), schedule_(schedule) {}
  SystemConfig(const SystemConfig&) = delete;
  SystemConfig(SystemConfig&&) noexcept = default;

  /**
   * @brief Destructor - automatically applies configuration if not already applied.
   */
  ~SystemConfig();

  SystemConfig& operator=(const SystemConfig&) = delete;
  SystemConfig& operator=(SystemConfig&&) noexcept = default;

  /**
   * @brief Adds ordering constraint: these systems run after specified systems.
   * @details Creates a dependency edge from each specified system to each system in this config.
   * @tparam AfterSystems System types that must run before these systems
   * @return Reference to this config for method chaining
   */
  template <ecs::SystemTrait... AfterSystems>
    requires(sizeof...(AfterSystems) > 0 && utils::UniqueTypes<AfterSystems...>)
  SystemConfig& After();

  /**
   * @brief Adds ordering constraint: these systems run before specified systems.
   * @details Creates a dependency edge from each system in this config to each specified system.
   * @tparam BeforeSystems System types that must run after these systems
   * @return Reference to this config for method chaining
   */
  template <ecs::SystemTrait... BeforeSystems>
    requires(sizeof...(BeforeSystems) > 0 && utils::UniqueTypes<BeforeSystems...>)
  SystemConfig& Before();

  /**
   * @brief Adds these systems to specified system sets.
   * @details Systems inherit ordering constraints and run conditions from their sets.
   * Systems can belong to multiple sets.
   * @tparam Sets System set types
   * @return Reference to this config for method chaining
   */
  template <SystemSetTrait... Sets>
    requires(sizeof...(Sets) > 0 && utils::UniqueTypes<Sets...>)
  SystemConfig& InSet();

  /**
   * @brief Adds ordering constraint: these systems run after specified system sets.
   * @details Creates a dependency edge from each specified set to all systems in this config.
   * All systems that belong to the specified sets will run before these systems.
   * @tparam AfterSets System set types that must run before these systems
   * @return Reference to this config for method chaining
   */
  template <SystemSetTrait... AfterSets>
    requires(sizeof...(AfterSets) > 0 && utils::UniqueTypes<AfterSets...>)
  SystemConfig& AfterSet();

  /**
   * @brief Adds ordering constraint: these systems run before specified system sets.
   * @details Creates a dependency edge from all systems in this config to each specified set.
   * All systems in this config will run before systems that belong to the specified sets.
   * @tparam BeforeSets System set types that must run after these systems
   * @return Reference to this config for method chaining
   */
  template <SystemSetTrait... BeforeSets>
    requires(sizeof...(BeforeSets) > 0 && utils::UniqueTypes<BeforeSets...>)
  SystemConfig& BeforeSet();

  /**
   * @brief Creates an ordered sequence of systems.
   * @details Each system runs after the previous one in the template parameter order.
   * Equivalent to manually adding After constraints: System2.After<System1>(), System3.After<System2>(), etc.
   * @warning Only valid when configuring multiple systems (sizeof...(Systems) > 1).
   * @return Reference to this config for method chaining
   */
  SystemConfig& Sequence() noexcept
    requires(sizeof...(Systems) > 1);

  /**
   * @brief Explicitly applies the configuration and adds systems to the sub-app.
   * @details Called automatically by destructor if not already applied.
   * Can be called explicitly to control when configuration is finalized.
   */
  void Apply();

private:
  std::reference_wrapper<SubApp> sub_app_;  ///< Sub-app where systems are added
  Schedule schedule_;                       ///< Schedule where systems are added
  bool sequence_ = false;                   ///< Whether to create sequential ordering
  bool applied_ = false;                    ///< Whether configuration has been applied

  std::vector<ecs::SystemTypeId> before_systems_;  ///< Systems that run after these systems
  std::vector<ecs::SystemTypeId> after_systems_;   ///< Systems that run before these systems
  std::vector<SystemSetId> system_sets_;           ///< System sets these systems belong to
  std::vector<SystemSetId> before_sets_;           ///< System sets that must run after these systems
  std::vector<SystemSetId> after_sets_;            ///< System sets that must run before these systems
};

template <ScheduleTrait Schedule, ecs::SystemTrait... Systems>
  requires(sizeof...(Systems) > 0 && utils::UniqueTypes<Systems...>)
inline SystemConfig<Schedule, Systems...>::~SystemConfig() {
  if (!applied_) {
    Apply();
  }
}

template <ScheduleTrait Schedule, ecs::SystemTrait... Systems>
  requires(sizeof...(Systems) > 0 && utils::UniqueTypes<Systems...>)
template <ecs::SystemTrait... AfterSystems>
  requires(sizeof...(AfterSystems) > 0 && utils::UniqueTypes<AfterSystems...>)
inline auto SystemConfig<Schedule, Systems...>::After() -> SystemConfig& {
  (after_systems_.push_back(ecs::SystemTypeIdOf<AfterSystems>()), ...);
  return *this;
}

template <ScheduleTrait Schedule, ecs::SystemTrait... Systems>
  requires(sizeof...(Systems) > 0 && utils::UniqueTypes<Systems...>)
template <ecs::SystemTrait... BeforeSystems>
  requires(sizeof...(BeforeSystems) > 0 && utils::UniqueTypes<BeforeSystems...>)
inline auto SystemConfig<Schedule, Systems...>::Before() -> SystemConfig& {
  (before_systems_.push_back(ecs::SystemTypeIdOf<BeforeSystems>()), ...);
  return *this;
}

template <ScheduleTrait Schedule, ecs::SystemTrait... Systems>
  requires(sizeof...(Systems) > 0 && utils::UniqueTypes<Systems...>)
template <SystemSetTrait... Sets>
  requires(sizeof...(Sets) > 0 && utils::UniqueTypes<Sets...>)
inline auto SystemConfig<Schedule, Systems...>::InSet() -> SystemConfig& {
  (system_sets_.push_back(SystemSetIdOf<Sets>()), ...);
  return *this;
}

template <ScheduleTrait Schedule, ecs::SystemTrait... Systems>
  requires(sizeof...(Systems) > 0 && utils::UniqueTypes<Systems...>)
template <SystemSetTrait... AfterSets>
  requires(sizeof...(AfterSets) > 0 && utils::UniqueTypes<AfterSets...>)
inline auto SystemConfig<Schedule, Systems...>::AfterSet() -> SystemConfig& {
  (after_sets_.push_back(SystemSetIdOf<AfterSets>()), ...);
  return *this;
}

template <ScheduleTrait Schedule, ecs::SystemTrait... Systems>
  requires(sizeof...(Systems) > 0 && utils::UniqueTypes<Systems...>)
template <SystemSetTrait... BeforeSets>
  requires(sizeof...(BeforeSets) > 0 && utils::UniqueTypes<BeforeSets...>)
inline auto SystemConfig<Schedule, Systems...>::BeforeSet() -> SystemConfig& {
  (before_sets_.push_back(SystemSetIdOf<BeforeSets>()), ...);
  return *this;
}

template <ScheduleTrait Schedule, ecs::SystemTrait... Systems>
  requires(sizeof...(Systems) > 0 && utils::UniqueTypes<Systems...>)
inline auto SystemConfig<Schedule, Systems...>::Sequence() noexcept -> SystemConfig&
  requires(sizeof...(Systems) > 1)
{
  sequence_ = true;
  return *this;
}

template <ScheduleTrait Schedule, ecs::SystemTrait... Systems>
  requires(sizeof...(Systems) > 0 && utils::UniqueTypes<Systems...>)
inline void SystemConfig<Schedule, Systems...>::Apply() {
  if (applied_) [[unlikely]] {
    return;
  }

  applied_ = true;

  auto& sub_app = sub_app_.get();
  auto& scheduler = sub_app.GetScheduler();

  // Add systems to the schedule first so that scheduler storage is populated.
  (sub_app.template AddSystem<Systems>(schedule_), ...);

  // Helper to register explicit system-to-system ordering with scheduler.
  const auto register_ordering_for = [&scheduler, this]<typename T>(std::span<const ecs::SystemTypeId> before_ids,
                                                                    std::span<const ecs::SystemTypeId> after_ids) {
    if (!before_ids.empty() || !after_ids.empty()) {
      details::SystemOrdering ordering{};
      ordering.before.assign(before_ids.begin(), before_ids.end());
      ordering.after.assign(after_ids.begin(), after_ids.end());
      scheduler.template RegisterOrdering<T>(schedule_, std::move(ordering));
    }
  };

  // Helper to append metadata to SystemInfo via Scheduler helper APIs and register set membership.
  const auto append_metadata_for = [&scheduler, this]<typename T>() {
    if (!before_systems_.empty() || !after_systems_.empty()) {
      scheduler.template AppendSystemOrderingMetadata<T>(schedule_, std::span<const ecs::SystemTypeId>(before_systems_),
                                                         std::span<const ecs::SystemTypeId>(after_systems_));
    }
    if (!system_sets_.empty()) {
      // Record membership in each configured set
      constexpr ecs::SystemTypeId system_id = ecs::SystemTypeIdOf<T>();
      for (const auto set_id : system_sets_) {
        scheduler.AddSystemToSet(set_id, system_id);
      }

      scheduler.template AppendSystemSetMetadata<T>(schedule_, std::span<const SystemSetId>(system_sets_));
    }
  };

  if (sequence_ && sizeof...(Systems) > 1) {
    // Sequence: each system runs after the previous one in template parameter order.
    constexpr std::array system_ids = {ecs::SystemTypeIdOf<Systems>()...};

    size_t idx = 0;

    (
        [&]<typename T>() {
          std::vector<ecs::SystemTypeId> before_ids(before_systems_.begin(), before_systems_.end());
          std::vector<ecs::SystemTypeId> after_ids(after_systems_.begin(), after_systems_.end());

          if (idx > 0) {
            // Current system must run after the previous system in the sequence.
            after_ids.push_back(system_ids[idx - 1]);
          }

          register_ordering_for.template operator()<T>(before_ids, after_ids);
          append_metadata_for.template operator()<T>();

          ++idx;
        }.template operator()<Systems>(),
        ...);
  } else {
    // No sequence: each system just gets the accumulated before/after system constraints.
    (
        [&]<typename T>() {
          register_ordering_for.template operator()<T>(std::span<const ecs::SystemTypeId>(before_systems_),
                                                       std::span<const ecs::SystemTypeId>(after_systems_));
          append_metadata_for.template operator()<T>();
        }.template operator()<Systems>(),
        ...);
  }

  // NOTE:
  //  - before_sets_ / after_sets_ are collected by the builder API,
  //    but the current scheduler implementation does not yet track SystemSetInfo
  //    or set-level ordering constraints. Once such infrastructure exists,
  //    these vectors can be propagated similarly to the system-level constraints.
}

}  // namespace helios::app
