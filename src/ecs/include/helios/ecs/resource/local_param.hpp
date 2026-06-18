#pragma once

#include <helios/assert.hpp>
#include <helios/ecs/resource/manager.hpp>
#include <helios/ecs/resource/resource.hpp>

#include <concepts>
#include <functional>
#include <utility>

namespace helios::ecs {

/**
 * @brief Read-only or mutable access to a system-local resource.
 * @details Only one system ever accesses it.
 * Use `Local<const T>` for read-only access and `Local<T>` for mutable access.
 * @tparam T Resource type. Qualify with `const` for read-only access
 */
template <ResourceTrait T>
class Local {
private:
  using DecayedT = std::remove_const_t<T>;

public:
  /**
   * @brief Constructs a local resource.
   * @param resources Resource manager
   */
  explicit constexpr Local(ResourceManager& resources) noexcept
      : resources_(resources) {}
  constexpr Local(const Local&) noexcept = default;
  constexpr Local(Local&&) noexcept = default;
  constexpr ~Local() noexcept = default;

  constexpr Local& operator=(const Local&) noexcept = default;
  constexpr Local& operator=(Local&&) noexcept = default;

  /**
   * @brief Inserts a local resource.
   * @details Replaces existing resource if present.
   * @tparam U Resource type
   * @param value Resource value
   */
  template <ResourceTrait U>
    requires std::same_as<DecayedT, std::remove_cvref_t<U>> &&
             (!std::is_const_v<T>)
  void Insert(U&& value) const {
    return resources_.get().Insert(std::forward<U>(value));
  }

  /**
   * @brief Emplaces a local resource.
   * @details Replaces existing resource if present.
   * @tparam Args Resource constructor arguments
   * @param args Resource constructor arguments
   */
  template <typename... Args>
    requires std::constructible_from<DecayedT, Args...> && (!std::is_const_v<T>)
  void Emplace(Args&&... args) const {
    return resources_.get().Emplace<DecayedT>(std::forward<Args>(args)...);
  }

  [[nodiscard]] T& operator*() const noexcept {
    return resources_.get().Get<DecayedT>();
  }

  [[nodiscard]] T* operator->() const noexcept {
    return &resources_.get().Get<DecayedT>();
  }

  /**
   * @brief Gets the local resource.
   * @return Reference to the local resource
   */
  [[nodiscard]] T& Get() const noexcept {
    return resources_.get().Get<DecayedT>();
  }

  /**
   * @brief Gets the local resource manager.
   * @return Reference to the local resource manager
   */
  [[nodiscard]] constexpr ResourceManager& Resources() const noexcept {
    return resources_.get();
  }

private:
  std::reference_wrapper<ResourceManager> resources_;
};

}  // namespace helios::ecs
