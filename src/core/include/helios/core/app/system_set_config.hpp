#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/app/schedule.hpp>
#include <helios/core/app/system_set.hpp>
#include <helios/core/utils/common_traits.hpp>

#include <functional>
#include <vector>

namespace helios::app {

// Forward declaration
class SubApp;

/**
 * @brief Fluent builder for configuring system sets with ordering.
 * @details Provides a chainable API for specifying system set dependencies.
 * The configuration is applied when the builder is destroyed or when explicitly finalized.
 * @tparam Schedule Schedule type where the set is configured
 * @tparam Set System set type being configured
 *
 * @example
 * @code
 * app.ConfigureSet<PhysicsSet>(kUpdate).After<InputSet>().Before<RenderSet>();
 * @endcode
 */
template <ScheduleTrait Schedule, SystemSetTrait Set>
class SystemSetConfig {
public:
  /**
   * @brief Constructs a system set configuration builder.
   * @param sub_app Reference to the sub-app where the set is configured
   * @param schedule Schedule instance where the set is configured
   */
  explicit SystemSetConfig(SubApp& sub_app, Schedule schedule = {}) noexcept : sub_app_(sub_app), schedule_(schedule) {}
  SystemSetConfig(const SystemSetConfig&) = delete;
  SystemSetConfig(SystemSetConfig&&) noexcept = default;
  ~SystemSetConfig();

  SystemSetConfig& operator=(const SystemSetConfig&) = delete;
  SystemSetConfig& operator=(SystemSetConfig&&) noexcept = default;

  /**
   * @brief Adds ordering constraint: this set runs after specified sets.
   * @details Creates a dependency edge from each specified set to this set.
   * All systems in the specified sets will run before systems in this set.
   * @tparam AfterSets System set types that must run before this set
   * @return Reference to this config for method chaining
   */
  template <SystemSetTrait... AfterSets>
    requires(sizeof...(AfterSets) > 0 && utils::UniqueTypes<AfterSets...>)
  SystemSetConfig& After();

  /**
   * @brief Adds ordering constraint: this set runs before specified sets.
   * @details Creates a dependency edge from this set to each specified set.
   * All systems in this set will run before systems in the specified sets.
   * @tparam BeforeSets System set types that must run after this set
   * @return Reference to this config for method chaining
   */
  template <SystemSetTrait... BeforeSets>
    requires(sizeof...(BeforeSets) > 0 && utils::UniqueTypes<BeforeSets...>)
  SystemSetConfig& Before();

  /**
   * @brief Explicitly applies the configuration.
   * @details Called automatically by destructor if not already applied.
   * Can be called explicitly to control when configuration is finalized.
   */
  void Apply();

private:
  std::reference_wrapper<SubApp> sub_app_;  ///< Sub-app where set is configured
  Schedule schedule_;                       ///< Schedule where set is configured
  bool applied_ = false;                    ///< Whether configuration has been applied

  std::vector<SystemSetId> before_sets_;  ///< System sets that run after this set
  std::vector<SystemSetId> after_sets_;   ///< System sets that run before this set
};

template <ScheduleTrait Schedule, SystemSetTrait Set>
inline SystemSetConfig<Schedule, Set>::~SystemSetConfig() {
  if (!applied_) {
    Apply();
  }
}

template <ScheduleTrait Schedule, SystemSetTrait Set>
template <SystemSetTrait... AfterSets>
  requires(sizeof...(AfterSets) > 0 && utils::UniqueTypes<AfterSets...>)
inline auto SystemSetConfig<Schedule, Set>::After() -> SystemSetConfig& {
  (after_sets_.push_back(SystemSetIdOf<AfterSets>()), ...);
  return *this;
}

template <ScheduleTrait Schedule, SystemSetTrait Set>
template <SystemSetTrait... BeforeSets>
  requires(sizeof...(BeforeSets) > 0 && utils::UniqueTypes<BeforeSets...>)
inline auto SystemSetConfig<Schedule, Set>::Before() -> SystemSetConfig& {
  (before_sets_.push_back(SystemSetIdOf<BeforeSets>()), ...);
  return *this;
}

template <ScheduleTrait Schedule, SystemSetTrait Set>
inline void SystemSetConfig<Schedule, Set>::Apply() {
  if (applied_) {
    return;
  }

  applied_ = true;

  auto& sub_app = sub_app_.get();
  auto& scheduler = sub_app.GetScheduler();

  // Ensure this set exists in the registry
  const auto& this_info = scheduler.template GetOrRegisterSystemSet<Set>();
  const SystemSetId this_id = this_info.id;

  // Register "After" relationships:
  //   ConfigureSet<Set>().After<A, B>()
  // means all systems in A and B must run before systems in Set.
  for (const SystemSetId after_id : after_sets_) {
    scheduler.AddSetRunsBefore(after_id, this_id);
  }

  // Register "Before" relationships:
  //   ConfigureSet<Set>().Before<A, B>()
  // means all systems in Set must run before systems in A and B.
  for (const SystemSetId before_id : before_sets_) {
    scheduler.AddSetRunsBefore(this_id, before_id);
  }
}

}  // namespace helios::app
