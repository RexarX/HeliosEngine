#pragma once

#include <helios/compiler/compiler.hpp>
#include <helios/ecs/schedule/system_local_data.hpp>
#include <helios/ecs/system/param_policy.hpp>
#include <helios/ecs/system/system.hpp>
#include <helios/ecs/world.hpp>

#include <functional>
#include <string>
#include <type_traits>

namespace helios::ecs {

/// @brief Type-erased run condition: returns bool, receives World& + local
/// data.
#ifdef HELIOS_MOVEONLY_FUNCTION_AVAILABLE
using RunCondition = std::move_only_function<bool(World&, SystemLocalData&)>;
#else
using RunCondition = std::function<bool(World&, SystemLocalData&)>;
#endif

/// @brief Storage for a run condition — predicate + access policy + local data.
struct RunConditionStorage {
  RunCondition run_condition;
  std::string name;
  AccessPolicy access_policy;
  SystemLocalData local_data;

  RunConditionStorage() = default;

  /**
   * @brief Constructs storage from individual members and options.
   * @param condition Run condition callable
   * @param cond_name Human-readable name for the condition
   * @param policy Access policy
   * @param options System local data options
   */
  RunConditionStorage(RunCondition condition, std::string cond_name,
                      AccessPolicy policy, SystemLocalDataOptions options);
  RunConditionStorage(const RunConditionStorage&) = delete;
  RunConditionStorage(RunConditionStorage&&) noexcept = default;
  ~RunConditionStorage() noexcept = default;

  RunConditionStorage& operator=(const RunConditionStorage&) = delete;
  RunConditionStorage& operator=(RunConditionStorage&&) noexcept = default;

  /**
   * @brief Factory from a param-style run condition functor.
   * @details Accepts only functor structs; lambdas must use
   * `FromParamNamed`.
   * @tparam T Run condition type satisfying `FunctorSystemTrait` and
   * returning bool
   * @param condition Run condition functor
   * @param options System local data options
   * @return RunConditionStorage
   */
  template <FunctorSystemTrait T>
  [[nodiscard]] static RunConditionStorage FromParam(
      T&& instance, SystemLocalDataOptions options = {});

  /**
   * @brief Factory from a param-style run condition with an explicit name.
   * @details Required for lambdas. Derives identity from the provided name.
   * @tparam T Run condition type satisfying `SystemTrait` and returning bool
   * @param name Human-readable name
   * @param condition Run condition functor
   * @param options System local data options
   * @return RunConditionStorage
   */
  template <SystemTrait T>
  [[nodiscard]] static RunConditionStorage FromParamNamed(
      std::string name, T&& instance, SystemLocalDataOptions options = {});

  /**
   * @brief Factory from a raw predicate + explicit policy.
   * @param fn Predicate function
   * @param policy Access policy
   * @param options Local data options
   * @return Run condition storage
   */
  [[nodiscard]] static RunConditionStorage From(
      RunCondition fn, AccessPolicy policy = {},
      SystemLocalDataOptions options = {}, std::string name = {}) {
    return {std::move(fn), std::move(name), std::move(policy), options};
  }
};

inline RunConditionStorage::RunConditionStorage(RunCondition condition,
                                                std::string cond_name,
                                                AccessPolicy policy,
                                                SystemLocalDataOptions options)
    : run_condition(std::move(condition)),
      name(std::move(cond_name)),
      access_policy(std::move(policy)),
      local_data(options) {}

template <FunctorSystemTrait T>
inline auto RunConditionStorage::FromParam(T&& instance,
                                           SystemLocalDataOptions options)
    -> RunConditionStorage {
  using Decayed = std::remove_cvref_t<T>;
  using ArgsTuple =
      typename details::MemberFnArgs<decltype(&Decayed::operator())>::ArgsTuple;

  constexpr auto cond_name = SystemNameOf<Decayed>();
  AccessPolicy policy = BuildPolicyFromSystem<Decayed>();

  auto wrapped = [cond = std::forward<T>(instance), policy_ = policy](
                     World& world, SystemLocalData& data) mutable -> bool {
    return []<typename... Params>(
               std::tuple<Params...>*, auto& inner_cond, World& inner_world,
               SystemLocalData& inner_data,
               [[maybe_unused]] const AccessPolicy& inner_policy) -> bool {
      return inner_cond(SystemParamTraits<std::remove_cvref_t<Params>>::Make(
          inner_world, inner_data, inner_policy)...);
    }(static_cast<ArgsTuple*>(nullptr), cond, world, data, policy_);
  };

  return {std::move(wrapped), std::string(cond_name), std::move(policy),
          options};
}

template <SystemTrait T>
inline auto RunConditionStorage::FromParamNamed(std::string name, T&& instance,
                                                SystemLocalDataOptions options)
    -> RunConditionStorage {
  using Decayed = std::remove_cvref_t<T>;
  using ArgsTuple =
      typename details::MemberFnArgs<decltype(&Decayed::operator())>::ArgsTuple;

  AccessPolicy policy = BuildPolicyFromSystem<Decayed>();

  auto wrapped = [cond = std::forward<T>(instance), policy_ = policy](
                     World& world, SystemLocalData& data) mutable -> bool {
    return []<typename... Params>(
               std::tuple<Params...>*, auto& inner_cond, World& inner_world,
               SystemLocalData& inner_data,
               [[maybe_unused]] const AccessPolicy& inner_policy) -> bool {
      return inner_cond(SystemParamTraits<std::remove_cvref_t<Params>>::Make(
          inner_world, inner_data, inner_policy)...);
    }(static_cast<ArgsTuple*>(nullptr), cond, world, data, policy_);
  };

  return {std::move(wrapped), std::move(name), std::move(policy), options};
}

}  // namespace helios::ecs
