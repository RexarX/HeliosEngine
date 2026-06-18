#pragma once

#include <helios/ecs/schedule/run_condition.hpp>
#include <helios/ecs/system/system.hpp>
#include <helios/utils/type_info.hpp>

#include <algorithm>
#include <compare>
#include <concepts>
#include <cstddef>
#include <ranges>
#include <string_view>
#include <type_traits>
#include <vector>

namespace helios::ecs {

/// @brief Type index for system sets.
using SystemSetTypeIndex = utils::TypeIndex;

/// @brief Type id for system sets.
using SystemSetTypeId = utils::TypeId;

/**
 * @brief Concept for system sets.
 * @details A system set must be an object and be empty.
 */
template <typename T>
concept SystemSetTrait = std::is_object_v<std::remove_cvref_t<T>> &&
                         std::is_empty_v<std::remove_cvref_t<T>>;

/**
 * @brief Concept for system sets that provide a name.
 * @details A system set with name trait must satisfy `SystemSetTrait`
 * and provide:
 * - `static constexpr std::string_view kName` variable
 */
template <typename T>
concept SystemSetWithNameTrait = SystemSetTrait<T> && requires {
  { std::remove_cvref_t<T>::kName } -> std::convertible_to<std::string_view>;
};

/**
 * @brief System set name for debugging and serialization.
 * @tparam T System set type
 * @return System set name
 */
template <SystemSetTrait T>
[[nodiscard]] constexpr std::string_view SystemSetNameOf() noexcept {
  if constexpr (SystemSetWithNameTrait<T>) {
    return T::kName;
  } else {
    return utils::QualifiedTypeNameOf<T>();
  }
}

/**
 * @brief System set name for debugging and serialization.
 * @tparam T System set type
 * @param instance System set instance
 * @return System set name
 */
template <SystemSetTrait T>
[[nodiscard]] constexpr std::string_view SystemSetNameOf(
    const T& /*instance*/) noexcept {
  return SystemSetNameOf<std::remove_cvref_t<T>>();
}

/// @brief Identifier for a system set.
struct SystemSetId {
  size_t id = 0;

  /**
   * @brief Creates system set id from a system set type index.
   * @param index System set type index
   * @return System set id
   */
  [[nodiscard]] static constexpr SystemSetId From(
      SystemSetTypeIndex index) noexcept {
    return SystemSetId{.id = index.Hash()};
  }

  /**
   * @brief Creates system set id from a system set type id.
   * @param id System set type id
   * @return System set id
   */
  [[nodiscard]] static constexpr SystemSetId From(SystemSetTypeId id) noexcept {
    return From(id.Index());
  }

  /**
   * @brief Creates system set id from a system set type.
   * @tparam T System set type
   * @return System set id
   */
  template <SystemSetTrait T>
  [[nodiscard]] static constexpr SystemSetId From() noexcept {
    return From(SystemSetTypeIndex::From<T>());
  }

  /**
   * @brief Creates system set id from a system set type.
   * @tparam T System set type
   * @param instance System set instance
   * @return System set id
   */
  template <SystemSetTrait T>
  [[nodiscard]] static constexpr SystemSetId From(
      const T& /*instance*/) noexcept {
    return From<std::remove_cvref_t<T>>();
  }

  /**
   * @brief Creates system set id from a range of system ids.
   * @tparam R Range type containing SystemId elements
   * @param ids Range of system ids
   * @return System set id
   */
  template <std::ranges::input_range R>
    requires std::same_as<std::ranges::range_value_t<R>, SystemId>
  [[nodiscard]] static constexpr SystemSetId From(const R& ids) noexcept;

  [[nodiscard]] constexpr std::strong_ordering operator<=>(
      const SystemSetId&) const noexcept = default;
};

template <std::ranges::input_range R>
  requires std::same_as<std::ranges::range_value_t<R>, SystemId>
constexpr auto SystemSetId::From(const R& ids) noexcept -> SystemSetId {
  const auto hasher = [](size_t lhs, size_t rhs) {
    return lhs ^ (rhs + 0x9e3779b9 + (lhs << 6) + (rhs >> 2));
  };

  size_t hash = 0;
  for (const auto& system_id : ids) {
    hash = hasher(hash, system_id.id);
  }
  return SystemSetId{.id = hash};
}

/**
 * @brief A group of systems that share ordering, run conditions, and other
 * properties.
 * @details When a property is applied to a set (e.g., `Before`, `After`,
 * `RunIf`), it is inherited by all systems that belong to the set.
 */
class SystemSet {
public:
  /**
   * @brief Constructs a `SystemSet` with the given ID.
   * @param id The ID of the system set
   */
  explicit constexpr SystemSet(SystemSetId id) : id_(id) {}
  SystemSet(const SystemSet&) = delete;
  constexpr SystemSet(SystemSet&&) = default;
  constexpr ~SystemSet() = default;

  SystemSet& operator=(const SystemSet&) = delete;
  constexpr SystemSet& operator=(SystemSet&&) = default;

  /**
   * @brief Adds an ordering constraint: all members must run before the given
   * system.
   * @param target System that members must precede
   * @return Reference to this set for chaining
   */
  constexpr auto Before(this auto&& self, SystemId target)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds an ordering constraint: all members must run before all
   * members of the given set.
   * @param target Set that members must precede
   * @return Reference to this set for chaining
   */
  constexpr auto Before(this auto&& self, SystemSetId target)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds an ordering constraint: all members must run after the given
   * system.
   * @param target System that members must follow
   * @return Reference to this set for chaining
   */
  constexpr auto After(this auto&& self, SystemId target)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds an ordering constraint: all members must run after all
   * members of the given set.
   * @param target Set that members must follow
   * @return Reference to this set for chaining
   */
  constexpr auto After(this auto&& self, SystemSetId target)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds a run condition to the set.
   * @details All systems in this set will only run if all conditions return
   * true.
   * @param condition Run condition predicate with access policy
   * @param policy Access policy for the run condition
   * @param options Options for run condition local data construction
   * @return Reference to this set for chaining
   */
  constexpr auto RunIf(this auto&& self, RunConditionStorage condition,
                       AccessPolicy policy = {},
                       SystemLocalDataOptions options = {})
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds a run condition functor to the set.
   * @details Lambdas are rejected; use `RunIf(std::string, T&&)`.
   * @tparam T Run condition type satisfying `FunctorSystemTrait`
   * @param condition Functor with declared access policy
   * @return Reference to this set for chaining
   */
  template <FunctorSystemTrait T>
  constexpr auto RunIf(this auto&& self, T&& condition)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Adds a run condition to the set with an explicit name.
   * @details Required for lambdas.
   * @tparam T Run condition type satisfying `SystemTrait`
   * @param name Human-readable name
   * @param condition Functor with declared access policy
   * @return Reference to this set for chaining
   */
  template <SystemTrait T>
  constexpr auto RunIf(this auto&& self, std::string name, T&& condition)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Marks this set as a sequence: systems run in insertion order.
   * @return Reference to this set for chaining
   */
  constexpr auto Sequence(this auto&& self)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Checks if this set is a sequence set.
   * @return True if this set is a sequence set, false otherwise
   */
  [[nodiscard]] constexpr bool IsSequence() const noexcept {
    return is_sequence_;
  }

  /**
   * @brief Gets the system set id.
   * @return System set id
   */
  [[nodiscard]] constexpr SystemSetId Id() const noexcept { return id_; }

  /**
   * @brief Takes ownership of all run conditions from this set.
   * @return Vector of run conditions that were previously owned by this set
   */
  [[nodiscard]] constexpr auto TakeConditions()
      -> std::vector<RunConditionStorage> {
    return std::exchange(conditions_, {});
  }

  /**
   * @brief Gets the system ordering targets for "before" constraints.
   * @return Vector of system ids that are targets of "before" constraints
   * from this set
   */
  [[nodiscard]] constexpr auto BeforeTargets() const noexcept
      -> const std::vector<SystemId>& {
    return before_targets_;
  }

  /**
   * @brief Gets the system ordering targets for "after" constraints.
   * @return Vector of system ids that are targets of "after" constraints from
   * this set
   */
  [[nodiscard]] constexpr auto AfterTargets() const noexcept
      -> const std::vector<SystemId>& {
    return after_targets_;
  }

  /**
   * @brief Gets the set ordering targets for "before" constraints.
   * @return Vector of system set ids that are targets of "before" constraints
   * from this set
   */
  [[nodiscard]] constexpr auto BeforeSetTargets() const noexcept
      -> const std::vector<SystemSetId>& {
    return before_set_targets_;
  }

  /**
   * @brief Gets the set ordering targets for "after" constraints.
   * @return Vector of system set ids that are targets of "after" constraints
   * from this set
   */
  [[nodiscard]] constexpr auto AfterSetTargets() const noexcept
      -> const std::vector<SystemSetId>& {
    return after_set_targets_;
  }

  /**
   * @brief Gets the run conditions owned by this set.
   * @return Vector of run conditions owned by this set
   */
  [[nodiscard]] constexpr auto Conditions() const noexcept
      -> const std::vector<RunConditionStorage>& {
    return conditions_;
  }

private:
  SystemSetId id_;
  std::vector<SystemId> before_targets_;
  std::vector<SystemId> after_targets_;
  std::vector<SystemSetId> before_set_targets_;
  std::vector<SystemSetId> after_set_targets_;
  std::vector<RunConditionStorage> conditions_;
  bool is_sequence_ = false;
};

constexpr auto SystemSet::Before(this auto&& self, SystemId target)
    -> decltype(std::forward<decltype(self)>(self)) {
  if (std::ranges::find(self.before_targets_, target) ==
      self.before_targets_.end()) {
    self.before_targets_.push_back(target);
  }
  return std::forward<decltype(self)>(self);
}

constexpr auto SystemSet::Before(this auto&& self, SystemSetId target)
    -> decltype(std::forward<decltype(self)>(self)) {
  if (std::ranges::find(self.before_set_targets_, target) ==
      self.before_set_targets_.end()) {
    self.before_set_targets_.push_back(target);
  }
  return std::forward<decltype(self)>(self);
}

constexpr auto SystemSet::After(this auto&& self, SystemId target)
    -> decltype(std::forward<decltype(self)>(self)) {
  if (std::ranges::find(self.after_targets_, target) ==
      self.after_targets_.end()) {
    self.after_targets_.push_back(target);
  }
  return std::forward<decltype(self)>(self);
}

constexpr auto SystemSet::After(this auto&& self, SystemSetId target)
    -> decltype(std::forward<decltype(self)>(self)) {
  if (std::ranges::find(self.after_set_targets_, target) ==
      self.after_set_targets_.end()) {
    self.after_set_targets_.push_back(target);
  }
  return std::forward<decltype(self)>(self);
}

constexpr auto SystemSet::RunIf(this auto&& self, RunConditionStorage condition,
                                AccessPolicy /*policy*/,
                                SystemLocalDataOptions /*options*/)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.conditions_.push_back(std::move(condition));
  return std::forward<decltype(self)>(self);
}

template <FunctorSystemTrait T>
constexpr auto SystemSet::RunIf(this auto&& self, T&& condition)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.conditions_.push_back(
      RunConditionStorage::FromParam(std::forward<T>(condition)));
  return std::forward<decltype(self)>(self);
}

template <SystemTrait T>
constexpr auto SystemSet::RunIf(this auto&& self, std::string name,
                                T&& condition)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.conditions_.push_back(RunConditionStorage::FromParamNamed(
      std::move(name), std::forward<T>(condition)));
  return std::forward<decltype(self)>(self);
}

constexpr auto SystemSet::Sequence(this auto&& self)
    -> decltype(std::forward<decltype(self)>(self)) {
  self.is_sequence_ = true;
  return std::forward<decltype(self)>(self);
}

}  // namespace helios::ecs
