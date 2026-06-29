#pragma once

#include <helios/ecs/schedule/run_condition.hpp>
#include <helios/ecs/schedule/system_handle.hpp>
#include <helios/ecs/schedule/system_set.hpp>
#include <helios/ecs/system/system.hpp>

#include <functional>
#include <string>

namespace helios::ecs {

class Schedule;
class SystemHandle;
class SystemGroupHandle;

/// @brief Handle returned by Schedule::Set for configuring a named system set.
class SystemSetHandle {
public:
  /**
   * @brief Constructs a `SystemSetHandle` with the given system set id and
   * schedule.
   * @param id The system set id
   * @param schedule The schedule that the system set belongs to
   */
  constexpr SystemSetHandle(SystemSetId id, Schedule& schedule) noexcept
      : id_(id), schedule_(schedule) {}

  SystemSetHandle(const SystemSetHandle&) = delete;
  constexpr SystemSetHandle(SystemSetHandle&&) noexcept = default;
  constexpr ~SystemSetHandle() noexcept = default;

  SystemSetHandle& operator=(const SystemSetHandle&) = delete;
  constexpr SystemSetHandle& operator=(SystemSetHandle&&) noexcept = default;

  /**
   * @brief Adds an ordering constraint: all members of this set must run before
   * the given system.
   * @param target System id that this set must precede
   * @return Reference to this handle for chaining
   */
  constexpr auto Before(this auto&& self, SystemId target)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds an ordering constraint: all members of this set must run
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
   * @brief Adds an ordering constraint: all members of this set must run before
   * the given system handle.
   * @param target System handle that this set must precede
   * @return Reference to this handle for chaining
   */
  constexpr auto Before(this auto&& self, const SystemHandle& target)
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).Before(target.Id().id);
  }

  /**
   * @brief Adds an ordering constraint: all members of this set must run
   * before all members of the given set.
   * @param target Set id that this set must precede
   * @return Reference to this handle for chaining
   */
  constexpr auto Before(this auto&& self, SystemSetId target)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds an ordering constraint: all members of this set must run before
   * all members of the given set type.
   * @tparam T Set type satisfying `SystemSetTrait`
   * @return Reference to this handle for chaining
   */
  template <SystemSetTrait T>
  constexpr auto BeforeSet(this auto&& self)
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).Before(SystemSetId::From<T>());
  }

  /**
   * @brief Adds an ordering constraint: all members of this set must run before
   * the given system set handle.
   * @param target System set handle that this set must precede
   * @return Reference to this handle for chaining
   */
  constexpr auto Before(this auto&& self, const SystemSetHandle& target)
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).Before(target.Id());
  }

  /**
   * @brief Adds an ordering constraint: all members of this set must run before
   * all members of the given system group.
   * @param target System group handle that this set must precede
   * @return Reference to this handle for chaining
   */
  constexpr auto Before(this auto&& self, const SystemGroupHandle& target)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds an ordering constraint: all members of this set must run after
   * the given system.
   * @param target System id that this set must follow
   * @return Reference to this handle for chaining
   */
  constexpr auto After(this auto&& self, SystemId target)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds an ordering constraint: all members of this set must run after
   * the given system type.
   * @tparam T System type satisfying `SystemTrait`
   * @return Reference to this handle for chaining
   */
  template <SystemTrait T>
  constexpr auto After(this auto&& self)
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).After(SystemId::From<T>());
  }

  /**
   * @brief Adds an ordering constraint: all members of this set must run after
   * the given system handle.
   * @param target System handle that this set must follow
   * @return Reference to this handle for chaining
   */
  constexpr auto After(this auto&& self, const SystemHandle& target)
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).After(target.Id().id);
  }

  /**
   * @brief Adds an ordering constraint: all members of this set must run after
   * all members of the given set.
   * @param target Set id that this set must follow
   * @return Reference to this handle for chaining
   */
  constexpr auto After(this auto&& self, SystemSetId target)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds an ordering constraint: all members of this set must run after
   * all members of the given set type.
   * @tparam T Set type satisfying `SystemSetTrait`
   * @return Reference to this handle for chaining
   */
  template <SystemSetTrait T>
  constexpr auto AfterSet(this auto&& self)
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).After(SystemSetId::From<T>());
  }

  /**
   * @brief Adds an ordering constraint: all members of this set must run after
   * the given system set handle.
   * @param target System set handle that this set must follow
   * @return Reference to this handle for chaining
   */
  constexpr auto After(this auto&& self, const SystemSetHandle& target)
      -> decltype(std::forward<decltype(self)>(self)) {
    return std::forward<decltype(self)>(self).After(target.Id());
  }

  /**
   * @brief Adds an ordering constraint: all members of this set must run after
   * all members of the given system group.
   * @param target System group handle that this set must follow
   * @return Reference to this handle for chaining
   */
  constexpr auto After(this auto&& self, const SystemGroupHandle& target)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds a run condition predicate that applies to all members.
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
   * @brief Adds a run condition functor that applies to all members.
   * @details Lambdas are rejected; use `RunIf(std::string, T&&)`.
   * @tparam T Run condition type satisfying `FunctorSystemTrait`
   * @param condition Functor with declared access policy
   * @return Reference to this handle for chaining
   */
  template <FunctorSystemTrait T>
  constexpr auto RunIf(this auto&& self, T&& condition)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds a run condition to all set members with an explicit name.
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
   * @brief Marks this set as a sequence: systems run in insertion order.
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
   * @brief Gets the identifier for this system set.
   * @return System set id
   */
  [[nodiscard]] constexpr SystemSetId Id() const noexcept { return id_; }

private:
  SystemSetId id_;
  std::reference_wrapper<Schedule> schedule_;
};

}  // namespace helios::ecs
