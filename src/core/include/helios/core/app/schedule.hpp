#pragma once

#include <helios/core_pch.hpp>

#include <ctti/name.hpp>
#include <ctti/type_id.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace helios::app {

/**
 * @brief Type alias for schedule type IDs.
 * @details Used to uniquely identify schedule types at runtime.
 */
using ScheduleId = size_t;

/**
 * @brief Trait to identify valid schedule types.
 * @details A valid schedule type must be an empty struct or class.
 * This ensures schedules are zero-cost type markers with no runtime overhead.
 * @tparam T Type to check
 *
 * @example
 * @code
 * struct MySchedule {};  // Valid - empty type
 * static_assert(ScheduleTrait<MySchedule>);
 * @endcode
 */
template <typename T>
concept ScheduleTrait = std::is_empty_v<std::remove_cvref_t<T>>;

/**
 * @brief Trait to identify schedules with custom names.
 * @details A schedule with name trait must satisfy ScheduleTrait and provide:
 * - `static constexpr std::string_view GetName() noexcept`
 *
 * @tparam T Type to check
 *
 * @example
 * @code
 * struct MySchedule {
 *   static constexpr std::string_view GetName() noexcept { return "MySchedule"; }
 * };
 * static_assert(ScheduleWithNameTrait<MySchedule>);
 * @endcode
 */
template <typename T>
concept ScheduleWithNameTrait = ScheduleTrait<T> && requires {
  { T::GetName() } -> std::same_as<std::string_view>;
};

/**
 * @brief Gets unique type ID for a schedule type.
 * @details Uses CTTI to generate a unique hash for the schedule type.
 * The ID is consistent across compilation units.
 * @tparam T Schedule type
 * @return Unique type ID for the schedule
 *
 * @example
 * @code
 * struct UpdateSchedule {};
 * constexpr ScheduleId id = ScheduleIdOf<UpdateSchedule>();
 * @endcode
 */
template <ScheduleTrait T>
constexpr ScheduleId ScheduleIdOf() noexcept {
  return ctti::type_index_of<T>().hash();
}

/**
 * @brief Gets the name of a schedule type.
 * @details Returns the custom name if GetName() is provided, otherwise returns the CTTI-generated type name.
 * @tparam T Schedule type
 * @return Name of the schedule
 *
 * @example
 * @code
 * struct Update {
 *   static constexpr std::string_view GetName() noexcept { return "Update"; }
 * };
 * constexpr auto name = ScheduleNameOf<Update>();  // "Update"
 * @endcode
 */
template <ScheduleTrait T>
constexpr std::string_view ScheduleNameOf() noexcept {
  if constexpr (ScheduleWithNameTrait<T>) {
    return T::GetName();
  } else {
    return ctti::name_of<T>();
  }
}

/**
 * @brief Concept for schedules that provide Before() ordering constraints.
 * @details A schedule with Before trait must provide:
 * - `static constexpr auto Before() noexcept -> std::array<ScheduleId, N>` or similar span-like type
 */
template <typename T>
concept ScheduleWithBeforeTrait = ScheduleTrait<T> && requires {
  { T::Before() };
};

/**
 * @brief Concept for schedules that provide After() ordering constraints.
 * @details A schedule with After trait must provide:
 * - `static constexpr auto After() noexcept -> std::array<ScheduleId, N>` or similar span-like type
 */
template <typename T>
concept ScheduleWithAfterTrait = ScheduleTrait<T> && requires {
  { T::After() };
};

/**
 * @brief Gets the Before() ordering constraints for a schedule.
 * @details Returns schedules that must run before this schedule.
 * If the schedule doesn't provide Before(), returns empty array.
 * @tparam T Schedule type
 * @return Array of schedule IDs that must run before T
 */
template <ScheduleTrait T>
constexpr auto ScheduleBeforeOf() noexcept {
  if constexpr (ScheduleWithBeforeTrait<T>) {
    return T::Before();
  } else {
    return std::array<ScheduleId, 0>{};
  }
}

/**
 * @brief Gets the After() ordering constraints for a schedule.
 * @details Returns schedules that must run after this schedule.
 * If the schedule doesn't provide After(), returns empty array.
 * @tparam T Schedule type
 * @return Array of schedule IDs that must run after T
 */
template <ScheduleTrait T>
constexpr auto ScheduleAfterOf() noexcept {
  if constexpr (ScheduleWithAfterTrait<T>) {
    return T::After();
  } else {
    return std::array<ScheduleId, 0>{};
  }
}

/**
 * @brief Concept for schedules that declare stage membership.
 * @details A schedule with stage trait must provide:
 * - `static constexpr ScheduleId GetStage() noexcept`
 *
 * This indicates which stage the schedule belongs to for execution grouping.
 *
 * @tparam T Type to check
 */
template <typename T>
concept ScheduleWithStageTrait = ScheduleTrait<T> && requires {
  { T::GetStage() } -> std::same_as<ScheduleId>;
};

/**
 * @brief Concept for schedules that represent stages.
 * @details A stage schedule must provide:
 * - `static constexpr bool IsStage() noexcept` that returns true
 *
 * Stages are the four core execution phases: StartUp, Main, Update, CleanUp
 * All other schedules are executed within these stages based on Before/After relationships.
 *
 * @tparam T Type to check
 */
template <typename T>
concept StageTrait = ScheduleTrait<T> && requires {
  { T::IsStage() } -> std::same_as<bool>;
};

/**
 * @brief Checks if a schedule type is a stage.
 * @tparam T Schedule type
 * @return True if the schedule is a stage, false otherwise
 */
template <ScheduleTrait T>
constexpr bool IsStage() noexcept {
  if constexpr (StageTrait<T>) {
    return T::IsStage();
  } else {
    return false;
  }
}

/**
 * @brief Gets the stage ID that a schedule belongs to.
 * @details If the schedule declares a stage via GetStage(), returns that stage ID.
 * If the schedule is itself a stage, returns its own ID.
 * Otherwise, returns 0 to indicate it's not associated with any stage.
 *
 * @tparam T Schedule type
 * @return Stage ID that this schedule belongs to, or 0 if not in a stage
 */
template <ScheduleTrait T>
constexpr ScheduleId ScheduleStageOf() noexcept {
  if constexpr (ScheduleWithStageTrait<T>) {
    return T::GetStage();
  } else if constexpr (IsStage<T>()) {
    return ScheduleIdOf<T>();
  } else {
    return 0;
  }
}

}  // namespace helios::app
