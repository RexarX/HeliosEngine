#pragma once

#include <helios/ecs/schedule/run_condition.hpp>
#include <helios/ecs/schedule/system_handle.hpp>
#include <helios/ecs/schedule/system_set.hpp>
#include <helios/ecs/schedule/system_set_handle.hpp>
#include <helios/ecs/system/system.hpp>

#include <functional>
#include <string>

namespace helios::ecs {

class Schedule;
class SystemHandle;
class SystemSetHandle;

/**
 * @brief Handle returned by variadic Schedule::Add for configuring a batch of
 * systems added together.
 * @details Each variadic `Add` call creates a unique anonymous group so
 * configuration such as `Sequence()` does not leak between batches with the
 * same system types. Use `InSet` to assign every member of the batch to a named
 * set in one call.
 */
class SystemGroupHandle {
public:
  /**
   * @brief Constructs a `SystemGroupHandle` with the given group id and
   * schedule.
   * @param id The anonymous group id
   * @param schedule The schedule that the group belongs to
   */
  constexpr SystemGroupHandle(SystemSetId id, Schedule& schedule) noexcept
      : id_(id), schedule_(schedule) {}

  SystemGroupHandle(const SystemGroupHandle&) = delete;
  constexpr SystemGroupHandle(SystemGroupHandle&&) noexcept = default;
  constexpr ~SystemGroupHandle() noexcept = default;

  SystemGroupHandle& operator=(const SystemGroupHandle&) = delete;
  constexpr SystemGroupHandle& operator=(SystemGroupHandle&&) noexcept =
      default;

  /**
   * @brief Adds an ordering constraint: all members of this group must run
   * before the given system.
   * @param target System id that this group must precede
   * @return Reference to this handle for chaining
   */
  constexpr auto Before(this auto&& self, SystemId target)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds an ordering constraint: all members of this group must run
   * before the given system type.
   * @tparam T System type satisfying `SystemTrait`
   * @return Reference to this handle for chaining
   */
  template <SystemTrait T>
  constexpr auto Before(this auto&& self)
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).Before(SystemId::From<T>());
  }

  /**
   * @brief Adds an ordering constraint: all members of this group must run
   * before the given system handle.
   * @param target System handle that this group must precede
   * @return Reference to this handle for chaining
   */
  constexpr auto Before(this auto&& self, const SystemHandle& target)
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).Before(target.Id().id);
  }

  /**
   * @brief Adds an ordering constraint: all members of this group must run
   * before all members of the given set.
   * @param target Set id that this group must precede
   * @return Reference to this handle for chaining
   */
  constexpr auto Before(this auto&& self, SystemSetId target)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds an ordering constraint: all members of this group must run
   * before all members of the given set type.
   * @tparam T Set type satisfying `SystemSetTrait`
   * @return Reference to this handle for chaining
   */
  template <SystemSetTrait T>
  constexpr auto BeforeSet(this auto&& self)
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).Before(SystemSetId::From<T>());
  }

  /**
   * @brief Adds an ordering constraint: all members of this group must run
   * before the given system set handle.
   * @param target System set handle that this group must precede
   * @return Reference to this handle for chaining
   */
  constexpr auto Before(this auto&& self, const SystemSetHandle& target)
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).Before(target.Id());
  }

  /**
   * @brief Adds an ordering constraint: all members of this group must run
   * before all members of the given system group.
   * @param target System group handle that this group must precede
   * @return Reference to this handle for chaining
   */
  constexpr auto Before(this auto&& self, const SystemGroupHandle& target)
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).Before(target.Id());
  }

  /**
   * @brief Adds an ordering constraint: all members of this group must run
   * after the given system.
   * @param target System id that this group must follow
   * @return Reference to this handle for chaining
   */
  constexpr auto After(this auto&& self, SystemId target)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds an ordering constraint: all members of this group must run
   * after the given system type.
   * @tparam T System type satisfying `SystemTrait`
   * @return Reference to this handle for chaining
   */
  template <SystemTrait T>
  constexpr auto After(this auto&& self)
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).After(SystemId::From<T>());
  }

  /**
   * @brief Adds an ordering constraint: all members of this group must run
   * after the given system handle.
   * @param target System handle that this group must follow
   * @return Reference to this handle for chaining
   */
  constexpr auto After(this auto&& self, const SystemHandle& target)
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).After(target.Id().id);
  }

  /**
   * @brief Adds an ordering constraint: all members of this group must run
   * after all members of the given set.
   * @param target Set id that this group must follow
   * @return Reference to this handle for chaining
   */
  constexpr auto After(this auto&& self, SystemSetId target)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds an ordering constraint: all members of this group must run
   * after all members of the given set type.
   * @tparam T Set type satisfying `SystemSetTrait`
   * @return Reference to this handle for chaining
   */
  template <SystemSetTrait T>
  constexpr auto AfterSet(this auto&& self)
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).After(SystemSetId::From<T>());
  }

  /**
   * @brief Adds an ordering constraint: all members of this group must run
   * after the given system set handle.
   * @param target System set handle that this group must follow
   * @return Reference to this handle for chaining
   */
  constexpr auto After(this auto&& self, const SystemSetHandle& target)
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).After(target.Id());
  }

  /**
   * @brief Adds an ordering constraint: all members of this group must run
   * after all members of the given system group.
   * @param target System group handle that this group must follow
   * @return Reference to this handle for chaining
   */
  constexpr auto After(this auto&& self, const SystemGroupHandle& target)
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).After(target.Id());
  }

  /**
   * @brief Adds a run condition predicate that applies to all group members.
   * @param condition Predicate evaluated before any member runs
   * @param policy Access policy for the run condition
   * @param options Options for run condition local data construction
   * @return Reference to this handle for chaining
   */
  constexpr auto RunIf(this auto&& self, RunCondition condition,
                       AccessPolicy policy = {},
                       SystemLocalDataOptions options = {})
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds a run condition functor that applies to all group members.
   * @details Lambdas are rejected; use `RunIf(std::string, T&&)`.
   * @tparam T Run condition type satisfying `FunctorSystemTrait`
   * @param condition Functor with declared access policy
   * @return Reference to this handle for chaining
   */
  template <FunctorSystemTrait T>
  constexpr auto RunIf(this auto&& self, T&& condition)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds a run condition to all group members with an explicit name.
   * @details Required for lambdas.
   * @tparam T Run condition type satisfying `SystemTrait`
   * @param name Human-readable name
   * @param condition Functor with declared access policy
   * @return Reference to this handle for chaining
   */
  template <SystemTrait T>
  constexpr auto RunIf(this auto&& self, std::string name, T&& condition)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Assigns every member of this group to the given named set.
   * @details Creates the target set if needed. Members inherit the target set's
   * ordering constraints and run conditions. Repeated calls with the same set
   * id are ignored for each member.
   * @param target Named set to assign group members to
   * @return Reference to this handle for chaining
   * @code
   * schedule.Add(SystemA{}, SystemB{}).InSet(MovementSet{}).Sequence();
   * @endcode
   */
  constexpr auto InSet(this auto&& self, SystemSetId target)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Assigns every member of this group to the given named set type.
   * @tparam T Set type satisfying `SystemSetTrait`
   * @return Reference to this handle for chaining
   */
  template <SystemSetTrait T>
  constexpr auto InSet(this auto&& self, const T& /*set*/ = {})
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).InSet(SystemSetId::From<T>());
  }

  /**
   * @brief Marks this group as a sequence: members run in insertion order.
   * @return Reference to this handle for chaining
   */
  constexpr auto Sequence(this auto&& self)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Finishes configuration and returns a reference to the parent
   * schedule.
   * @return Reference to the parent schedule for continued configuration
   */
  constexpr Schedule& Done() noexcept { return schedule_.get(); }

  /**
   * @brief Gets the identifier for this anonymous group.
   * @return Anonymous group id
   */
  [[nodiscard]] constexpr SystemSetId Id() const noexcept { return id_; }

private:
  SystemSetId id_;
  std::reference_wrapper<Schedule> schedule_;
};

}  // namespace helios::ecs
