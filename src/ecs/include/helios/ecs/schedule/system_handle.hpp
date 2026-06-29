#pragma once

#include <helios/ecs/schedule/run_condition.hpp>
#include <helios/ecs/schedule/system_set.hpp>
#include <helios/ecs/system/system.hpp>

#include <compare>
#include <cstddef>
#include <functional>
#include <string>

namespace helios::ecs {

class Schedule;
class SystemSetHandle;
class SystemGroupHandle;

/**
 * @brief Unique identifier for a system within a schedule.
 * @details Combines the type-based SystemId with a per-schedule index for
 * uniqueness, enabling the same system type to be added multiple times.
 */
struct ScheduleSystemId {
  SystemId id;
  size_t slot = 0;
  size_t generation = 0;

  [[nodiscard]] constexpr bool operator==(
      const ScheduleSystemId&) const noexcept = default;
  [[nodiscard]] constexpr std::strong_ordering operator<=>(
      const ScheduleSystemId&) const noexcept = default;
};

/// @brief Handle returned by Schedule::Add for configuring a system.
class SystemHandle {
public:
  /**
   * @brief Constructs a `SystemHandle` with the given system id and schedule.
   * @param id The system id
   * @param schedule The schedule that the system belongs to
   */
  constexpr SystemHandle(ScheduleSystemId id, Schedule& schedule) noexcept
      : id_(id), schedule_(schedule) {}

  SystemHandle(const SystemHandle&) = delete;
  constexpr SystemHandle(SystemHandle&&) noexcept = default;
  constexpr ~SystemHandle() noexcept = default;

  SystemHandle& operator=(const SystemHandle&) = delete;
  constexpr SystemHandle& operator=(SystemHandle&&) noexcept = default;

  /**
   * @brief Adds an ordering constraint: this system must run before the given
   * system id.
   * @param target System id that this system must precede
   * @return Reference to this handle for chaining
   */
  constexpr auto Before(this auto&& self, SystemId target)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds an ordering constraint: this system must run before the given
   * system type.
   * @tparam T System type satisfying `SystemTrait`
   * @return Reference to this handle for chaining
   */
  template <SystemTrait T>
  constexpr auto Before(this auto&& self)
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).Before(SystemId::From<T>());
  }

  /**
   * @brief Adds an ordering constraint: this system must run before the given
   * system handle.
   * @param target System handle that this system must precede
   * @return Reference to this handle for chaining
   */
  constexpr auto Before(this auto&& self, const SystemHandle& target)
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).Before(target.Id().id);
  }

  /**
   * @brief Adds an ordering constraint: this system must run before the given
   * system set handle.
   * @param target System set handle that this system must precede
   * @return Reference to this handle for chaining
   */
  constexpr auto Before(this auto&& self, const SystemSetHandle& target)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds an ordering constraint: this system must run before all members
   * of the given system group.
   * @param target System group handle that this system must precede
   * @return Reference to this handle for chaining
   */
  constexpr auto Before(this auto&& self, const SystemGroupHandle& target)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds an ordering constraint: this system must run before all members
   * of the given set.
   * @param target Set that this system must precede
   * @return Reference to this handle for chaining
   */
  constexpr auto Before(this auto&& self, SystemSetId target)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds an ordering constraint: this system must run before all members
   * of the given set type.
   * @tparam T Set type satisfying `SystemSetTrait`
   * @return Reference to this handle for chaining
   */
  template <SystemSetTrait T>
  constexpr auto BeforeSet(this auto&& self)
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).Before(SystemSetId::From<T>());
  }

  /**
   * @brief Adds an ordering constraint: this system must run after the given
   * system id.
   * @param target System id that this system must follow
   * @return Reference to this handle for chaining
   */
  constexpr auto After(this auto&& self, SystemId target)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds an ordering constraint: this system must run after the given
   * system type.
   * @tparam T System type satisfying `SystemTrait`
   * @return Reference to this handle for chaining
   */
  template <SystemTrait T>
  constexpr auto After(this auto&& self)
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).After(SystemId::From<T>());
  }

  /**
   * @brief Adds an ordering constraint: this system must run after the given
   * system handle.
   * @param target System handle that this system must follow
   * @return Reference to this handle for chaining
   */
  constexpr auto After(this auto&& self, const SystemHandle& target)
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).After(target.Id().id);
  }

  /**
   * @brief Adds an ordering constraint: this system must run after all members
   * of the given set.
   * @param target Set that this system must follow
   * @return Reference to this handle for chaining
   */
  constexpr auto After(this auto&& self, SystemSetId target)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds an ordering constraint: this system must run after all members
   * of the given set type.
   * @tparam T Set type satisfying `SystemSetTrait`
   * @return Reference to this handle for chaining
   */
  template <SystemSetTrait T>
  constexpr auto AfterSet(this auto&& self)
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).After(SystemSetId::From<T>());
  }

  /**
   * @brief Adds an ordering constraint: this system must run after all members
   * of the given system set handle.
   * @param target System set handle that this system must follow
   * @return Reference to this handle for chaining
   */
  constexpr auto After(this auto&& self, const SystemSetHandle& target)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds an ordering constraint: this system must run after all members
   * of the given system group.
   * @param target System group handle that this system must follow
   * @return Reference to this handle for chaining
   */
  constexpr auto After(this auto&& self, const SystemGroupHandle& target)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds a run condition predicate that must return true for this system
   * to execute.
   * @param condition Predicate evaluated before the system runs
   * @param policy Access policy for the run condition
   * @param options Options for run condition local data construction
   * @return Reference to this handle for chaining
   */
  constexpr auto RunIf(this auto&& self, RunCondition condition,
                       AccessPolicy policy = {},
                       SystemLocalDataOptions options = {})
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds a run condition functor that must return true for this system
   * to execute.
   * @details Lambdas are rejected; use `RunIf(std::string, T&&)` to register
   * them with an explicit name.
   * @tparam T Run condition type satisfying `FunctorSystemTrait`
   * @param condition Functor with declared access policy
   * @return Reference to this handle for chaining
   */
  template <FunctorSystemTrait T>
  constexpr auto RunIf(this auto&& self, T&& condition)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds a run condition to this system with an explicit name.
   * @details Required for lambdas. Derives identity from the provided name.
   * @tparam T Run condition type satisfying `SystemTrait`
   * @param name Human-readable name
   * @param condition Functor with declared access policy
   * @return Reference to this handle for chaining
   */
  template <SystemTrait T>
  constexpr auto RunIf(this auto&& self, std::string name, T&& condition)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Assigns this system to a labeled set by id.
   * @details Creates the target set if needed. Repeated calls with the same set
   * id are ignored.
   * @param set_id Set to assign this system to
   * @return Reference to this handle for chaining
   */
  constexpr auto InSet(this auto&& self, SystemSetId set_id)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Assigns this system to a labeled set by type.
   * @tparam T Set type satisfying `SystemSetTrait`
   * @return Reference to this handle for chaining
   */
  template <SystemSetTrait T>
  constexpr auto InSet(this auto&& self, const T& /*set*/ = {})
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).InSet(SystemSetId::From<T>());
  }

  /**
   * @brief Finishes configuration and returns a reference to the parent
   * schedule.
   * @return Reference to the parent schedule for continued configuration
   */
  constexpr Schedule& Done() noexcept { return schedule_.get(); }

  /**
   * @brief Gets the unique schedule-local identifier for this system.
   * @return Schedule system id
   */
  [[nodiscard]] constexpr ScheduleSystemId Id() const noexcept { return id_; }

private:
  ScheduleSystemId id_;
  std::reference_wrapper<Schedule> schedule_;
};

}  // namespace helios::ecs
