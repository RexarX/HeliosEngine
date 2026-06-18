#pragma once

#include <helios/compiler/compiler.hpp>
#include <helios/ecs/schedule/system_local_data.hpp>
#include <helios/ecs/system/access_policy.hpp>
#include <helios/ecs/system/param_policy.hpp>
#include <helios/ecs/system/system.hpp>

#include <functional>
#include <string>
#include <type_traits>
#include <utility>

namespace helios::ecs {

class World;

#ifdef HELIOS_MOVEONLY_FUNCTION_AVAILABLE
using SystemCallable = std::move_only_function<void(World&, SystemLocalData&)>;
#else
using SystemCallable = std::function<void(World&, SystemLocalData&)>;
#endif

/// @brief Storage for a system.
struct SystemStorage {
  SystemId id;
  std::string name;
  AccessPolicy access_policy;

  /// Type-erased system callable.
  /// Receives World& and SystemLocalData& directly. The wrapper
  /// lambda constructs params and calls Update after the system runs.
  SystemCallable system;

  SystemLocalData local_data;

  SystemStorage() = default;

  /**
   * @brief Constructs storage from individual members and options.
   * @param id System identifier
   * @param name System name
   * @param policy Access policy
   * @param system Callable that receives (World&, SystemLocalData&)
   * @param options System local data options
   */
  SystemStorage(SystemId sid, std::string sname, AccessPolicy spolicy,
                SystemCallable ssystem, SystemLocalDataOptions options);
  SystemStorage(const SystemStorage&) = delete;
  SystemStorage(SystemStorage&&) noexcept = default;
  ~SystemStorage() noexcept = default;

  SystemStorage& operator=(const SystemStorage&) = delete;
  SystemStorage& operator=(SystemStorage&&) noexcept = default;

  /**
   * @brief Factory from a param-style system functor.
   * @details Auto-deduces the access policy from the functor's parameter
   * types. Wraps invocation so that Update is called after the system runs.
   * Only accepts functor structs; lambdas must use `FromParamNamed`.
   * @tparam T System type satisfying `FunctorSystemTrait`
   * @param system System instance
   * @param options System local data options
   * @return System storage
   */
  template <FunctorSystemTrait T>
  [[nodiscard]] static SystemStorage FromParam(
      T&& system, SystemLocalDataOptions options = {});

  /**
   * @brief Factory from a param-style system functor with an explicit name.
   * @details Identical to `FromParam` but uses the provided name instead of
   * the auto-deduced type name for both the storage name and the `SystemId`
   * derivation. Required for lambdas and optional for functor duplicates.
   * @tparam T System type satisfying `SystemTrait`
   * @param name Human-readable system name
   * @param system System instance
   * @param options System local data options
   * @return System storage
   */
  template <SystemTrait T>
  [[nodiscard]] static SystemStorage FromParamNamed(
      std::string name, T&& system, SystemLocalDataOptions options = {});

  /**
   * @brief Factory for a raw callable with explicit name and access policy.
   * @param name System name
   * @param system Callable receiving (`World&`, `SystemLocalData&`)
   * @param access_policy Access policy for scheduling
   * @param options System local data options
   * @return System storage
   */
  [[nodiscard]] static SystemStorage From(std::string name,
                                          SystemCallable system,
                                          AccessPolicy access_policy = {},
                                          SystemLocalDataOptions options = {}) {
    return {SystemId::From(name), std::move(name), std::move(access_policy),
            std::move(system), options};
  }

  /**
   * @brief Factory from a system type index with explicit name and policy.
   * @param name System name
   * @param index System type index
   * @param system Callable receiving (`World&`, `SystemLocalData&`)
   * @param access_policy Access policy for scheduling
   * @param options System local data options
   * @return System storage
   */
  [[nodiscard]] static SystemStorage From(std::string name,
                                          SystemTypeIndex index,
                                          SystemCallable system,
                                          AccessPolicy access_policy = {},
                                          SystemLocalDataOptions options = {}) {
    return {SystemId::From(index), std::move(name), std::move(access_policy),
            std::move(system), options};
  }

  /**
   * @brief Factory from a system type id with explicit name and policy.
   * @param name System name
   * @param id System type id
   * @param system Callable receiving (`World&`, `SystemLocalData&`)
   * @param access_policy Access policy for scheduling
   * @param options System local data options
   * @return System storage
   */
  [[nodiscard]] static SystemStorage From(std::string name, SystemTypeId id,
                                          SystemCallable system,
                                          AccessPolicy access_policy = {},
                                          SystemLocalDataOptions options = {}) {
    return From(std::move(name), id.Index(), std::move(system),
                std::move(access_policy), options);
  }
};

inline SystemStorage::SystemStorage(SystemId sid, std::string sname,
                                    AccessPolicy spolicy,
                                    SystemCallable ssystem,
                                    SystemLocalDataOptions options)
    : id(sid),
      name(std::move(sname)),
      access_policy(std::move(spolicy)),
      system(std::move(ssystem)),
      local_data(options) {}

template <FunctorSystemTrait T>
inline SystemStorage SystemStorage::FromParam(T&& system,
                                              SystemLocalDataOptions options) {
  using Decayed = std::remove_cvref_t<T>;

  constexpr auto name = SystemNameOf<Decayed>();
  constexpr SystemId id = SystemId::From<Decayed>();
  AccessPolicy policy = BuildPolicyFromSystem<Decayed>();

  auto wrapped = [system = std::forward<T>(system), policy_ = policy](
                     World& world, SystemLocalData& local) mutable {
    []<typename... Params>(std::tuple<Params...>*, auto& inner_system,
                           World& inner_world, SystemLocalData& inner_data,
                           const AccessPolicy& inner_policy) {
      inner_system(SystemParamTraits<std::remove_cvref_t<Params>>::Make(
          inner_world, inner_data, inner_policy)...);
    }(static_cast<typename details::MemberFnArgs<
          decltype(&Decayed::operator())>::ArgsTuple*>(nullptr),
      system, world, local, policy_);
  };

  return {id, std::string(name), std::move(policy), std::move(wrapped),
          options};
}

template <SystemTrait T>
inline SystemStorage SystemStorage::FromParamNamed(
    std::string name, T&& system, SystemLocalDataOptions options) {
  using Decayed = std::remove_cvref_t<T>;

  SystemId id = SystemId::From(name);
  AccessPolicy policy = BuildPolicyFromSystem<Decayed>();

  auto wrapped = [system = std::forward<T>(system), policy_ = policy](
                     World& world, SystemLocalData& local) mutable {
    []<typename... Params>(std::tuple<Params...>*, auto& inner_system,
                           World& inner_world, SystemLocalData& inner_data,
                           const AccessPolicy& inner_policy) {
      inner_system(SystemParamTraits<std::remove_cvref_t<Params>>::Make(
          inner_world, inner_data, inner_policy)...);
    }(static_cast<typename details::MemberFnArgs<
          decltype(&Decayed::operator())>::ArgsTuple*>(nullptr),
      system, world, local, policy_);
  };

  return {id, std::move(name), std::move(policy), std::move(wrapped), options};
}

}  // namespace helios::ecs
