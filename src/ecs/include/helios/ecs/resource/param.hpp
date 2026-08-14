#pragma once

#include <helios/ecs/resource/resource.hpp>

#include <functional>
#include <optional>

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

}  // namespace helios::ecs
