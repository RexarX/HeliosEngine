#pragma once

#include <helios/ecs/resource/resource.hpp>

#include <functional>

namespace helios::ecs {

/**
 * @brief Read-only or mutable access to resource.
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

/// @brief Read-only access to thread-safe resources.
template <AsyncResourceTrait T>
using AsyncRes = Res<T>;

}  // namespace helios::ecs
