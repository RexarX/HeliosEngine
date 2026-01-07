#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/core.hpp>

#include <concepts>
#include <type_traits>
#include <utility>

namespace helios::utils {

/**
 * @brief A utility class that defers the execution of a callable until the object goes out of scope.
 * @tparam F The type of the callable to be executed upon destruction.
 *
 * @example
 * @code
 * // Pattern 1: Pre-defined lambda
 * auto cleanup = [resource]() { delete resource; };
 * HELIOS_DEFER_CALL(cleanup);
 *
 * // Pattern 2: Inline lambda (no capture list needed)
 * HELIOS_DEFER {
 *   delete resource;
 * };
 * @endcode
 */
template <std::invocable F>
class Defer {
public:
  Defer() = delete;

  /**
   * @brief Constructs a Defer object that will execute the provided callable upon destruction.
   * @param func The callable to be executed.
   */
  constexpr explicit Defer(F func) noexcept(std::is_nothrow_move_constructible_v<F>) : func_(std::move(func)) {}
  Defer(Defer&&) = delete;
  Defer(const Defer&) = delete;
  constexpr ~Defer() noexcept(std::is_nothrow_invocable_v<F>) { func_(); }

  Defer& operator=(const Defer&) = delete;
  Defer& operator=(Defer&&) = delete;

private:
  F func_;
};

namespace details {

/**
 * @brief Helper struct for the HELIOS_DEFER macro to enable inline lambda syntax.
 */
struct DeferHelper {
  template <std::invocable F>
  constexpr auto operator+(F&& func) noexcept(std::is_nothrow_constructible_v<Defer<std::decay_t<F>>, F>)
      -> Defer<std::decay_t<F>> {
    return Defer<std::decay_t<F>>(std::forward<F>(func));
  }
};

}  // namespace details

}  // namespace helios::utils

/**
 * @brief Defers execution of a callable until the end of the current scope.
 * Accepts lambdas, functors, function pointers, and std::function objects.
 *
 * @example
 * @code
 * auto cleanup = []() { std::cout << "Cleanup\n"; };
 * HELIOS_DEFER_CALL(cleanup);
 * @endcode
 */
#define HELIOS_DEFER_CALL(callable) auto HELIOS_CONCAT(_defer_, __LINE__) = ::helios::utils::Defer(callable)
/**
 * @brief Defers execution of an inline lambda until the end of the current scope.
 * The lambda is written directly after the macro without explicit capture list.
 *
 * @example
 * @code
 * int* ptr = new int(42);
 * HELIOS_DEFER {
 *   delete ptr;
 * };
 * @endcode
 */
#define HELIOS_DEFER                      \
  auto HELIOS_CONCAT(_defer_, __LINE__) = \
      ::helios::utils::details::DeferHelper() + [&]()  // NOLINT(bugprone-macro-parentheses)
