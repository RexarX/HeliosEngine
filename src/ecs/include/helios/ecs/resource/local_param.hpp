#pragma once

#include <helios/ecs/resource/resource.hpp>

#include <concepts>
#include <functional>
#include <type_traits>
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

}  // namespace helios::ecs
