#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.ecs;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <functional>
#include <optional>
#endif
#include <helios/ecs/resource/resource.hpp>
#include <helios/ecs/schedule/system_local_data.hpp>
#include <helios/ecs/system/access_policy.hpp>
#include <helios/ecs/system/param.hpp>
#include <helios/ecs/world.hpp>

HELIOS_MODULE_EXPORT
namespace helios::ecs {

/**
 * @brief Access to resource.
 * @details Pointer-like interface for resource access.
 * Use `Res<const T>` for read-only access and `Res<T>` for mutable
 * access.
 * @tparam T Resource type
 */
template <ResourceTrait T>
class Res {
public:
  /**
   * @brief Constructs a resource reference.
   * @param resource Resource value
   */
  explicit constexpr Res(T& resource) noexcept : resource_(resource) {}
  constexpr Res(const Res&) noexcept = default;
  constexpr Res(Res&&) noexcept = default;
  constexpr ~Res() noexcept = default;

  constexpr Res& operator=(const Res&) noexcept = default;
  constexpr Res& operator=(Res&&) noexcept = default;

  [[nodiscard]] constexpr T* operator->() noexcept { return &resource_.get(); }
  [[nodiscard]] constexpr T& operator*() noexcept { return resource_.get(); }

  [[nodiscard]] constexpr const T* operator->() const noexcept {
    return &resource_.get();
  }

  [[nodiscard]] constexpr const T& operator*() const noexcept {
    return resource_.get();
  }

private:
  std::reference_wrapper<T> resource_;
};

/**
 * @brief Access to thread-safe resource.
 * @details Just an alias for `Res<T>`, but documents that the resource is
 * thread-safe and can be accessed in parallel.
 * @tparam T Resource type
 */
template <AsyncResourceTrait T>
using AsyncRes = Res<T>;

/**
 * @brief Optional access to resource.
 * @details Alias for `std::optional<Res<T>>`.
 * @tparam T Resource type
 */
template <ResourceTrait T>
using OptRes = std::optional<Res<T>>;

/**
 * @brief Optional access to thread-safe resource.
 * @details Alias for `std::optional<AsyncRes<T>>`.
 * @tparam T Resource type
 */
template <ResourceTrait T>
using OptAsyncRes = std::optional<AsyncRes<T>>;

/**
 * @brief Read-only or mutable access to a system-local resource.
 * @details Only one system ever accesses it.
 * Use `Local<const T>` for read-only access and `Local<T>` for mutable access.
 * @tparam T Resource type. Qualify with `const` for read-only access
 */
template <ResourceTrait T>
class Local {
public:
  using DecayedT = std::remove_cvref_t<T>;

  /**
   * @brief Constructs a local resource.
   * @param resource Resource value
   */
  explicit constexpr Local(T& resource) noexcept : resource_(resource) {}
  constexpr Local(const Local&) noexcept = default;
  constexpr Local(Local&&) noexcept = default;
  constexpr ~Local() noexcept = default;

  constexpr Local& operator=(const Local&) noexcept = default;
  constexpr Local& operator=(Local&&) noexcept = default;

  /**
   * @brief Assigns a new value to the local resource.
   * @tparam U Resource type
   * @param value Resource value
   * @return Reference to the assigned resource
   */
  template <ResourceTrait U>
    requires std::same_as<DecayedT, std::remove_cvref_t<U>> &&
             std::is_assignable_v<T&, U> && (!std::is_const_v<T>)
  constexpr T& operator=(U&& value) const
      noexcept(std::is_nothrow_assignable_v<T&, U>) {
    resource_.get() = std::forward<U>(value);
    return resource_.get();
  }

  [[nodiscard]] constexpr T& operator*() const noexcept {
    return resource_.get();
  }

  [[nodiscard]] constexpr T* operator->() const noexcept {
    return &resource_.get();
  }

  /**
   * @brief Gets the local resource.
   * @return Reference to the local resource
   */
  [[nodiscard]] constexpr T& Get() const noexcept { return resource_.get(); }

private:
  std::reference_wrapper<T> resource_;
};

template <ResourceTrait T>
struct SystemParamTraits<Res<T>> {
  static auto Make(World& world, SystemLocalData& /*data*/,
                   const AccessPolicy& /*policy*/) noexcept -> Res<T> {
    return Res<T>(world.WriteResource<T>());
  }

  static constexpr void RegisterAccess(AccessPolicyBuilder& builder) {
    if constexpr (!AsyncResourceTrait<T>) {
      builder.WriteResources<T>();
    }
  }
};

template <ResourceTrait T>
struct SystemParamTraits<Res<const T>> {
  static auto Make(World& world, SystemLocalData& /*data*/,
                   const AccessPolicy& /*policy*/) noexcept -> Res<const T> {
    return Res<const T>(world.ReadResource<T>());
  }

  static constexpr void RegisterAccess(AccessPolicyBuilder& builder) {
    if constexpr (!AsyncResourceTrait<T>) {
      builder.ReadResources<T>();
    }
  }
};

template <ResourceTrait T>
struct SystemParamTraits<OptRes<T>> {
  static auto Make(World& world, SystemLocalData& /*data*/,
                   const AccessPolicy& /*policy*/) noexcept -> OptRes<T> {
    if (T* ptr = world.TryWriteResource<T>()) {
      return Res<T>(*ptr);
    }
    return std::nullopt;
  }

  static constexpr void RegisterAccess(AccessPolicyBuilder& builder) {
    if constexpr (!AsyncResourceTrait<T>) {
      builder.WriteResources<T>();
    }
  }
};

template <ResourceTrait T>
struct SystemParamTraits<OptRes<const T>> {
  static auto Make(World& world, SystemLocalData& /*data*/,
                   const AccessPolicy& /*policy*/) noexcept -> OptRes<const T> {
    if (const T* ptr = world.TryReadResource<T>()) {
      return Res<const T>(*ptr);
    }
    return std::nullopt;
  }

  static constexpr void RegisterAccess(AccessPolicyBuilder& builder) {
    if constexpr (!AsyncResourceTrait<T>) {
      builder.ReadResources<T>();
    }
  }
};

template <ResourceTrait T>
  requires std::default_initializable<T>
struct SystemParamTraits<Local<T>> {
  static auto Make(World& /*world*/, SystemLocalData& data,
                   const AccessPolicy& /*policy*/) noexcept -> Local<T> {
    data.resource_manager.TryEmplace<T>();
    return Local<T>(data.resource_manager.Get<T>());
  }

  static constexpr void RegisterAccess(
      AccessPolicyBuilder& /*builder*/) noexcept {}
};

template <ResourceTrait T>
  requires std::default_initializable<T>
struct SystemParamTraits<Local<const T>> {
  static auto Make(World& /*world*/, SystemLocalData& data,
                   const AccessPolicy& /*policy*/) noexcept -> Local<const T> {
    data.resource_manager.TryEmplace<T>();
    return Local<const T>(data.resource_manager.Get<T>());
  }

  static constexpr void RegisterAccess(
      AccessPolicyBuilder& /*builder*/) noexcept {}
};

}  // namespace helios::ecs
#endif  // HELIOS_MODULE_CONSUMER_SHIM
