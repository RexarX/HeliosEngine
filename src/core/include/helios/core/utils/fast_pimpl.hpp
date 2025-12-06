#pragma once

#include <helios/core_pch.hpp>

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <memory>
#include <type_traits>

namespace helios::utils {

/**
 * @brief Implements pimpl idiom without dynamic memory allocation.
 * @details FastPimpl doesn't require memory allocation or indirect memory access.
 * You must manually set object size and alignment when instantiating FastPimpl.
 * @tparam T The implementation type
 * @tparam Size The size in bytes to allocate for T
 * @tparam Alignment The alignment requirement for T
 * @tparam RequireStrictMatch If true, requires exact size/alignment match
 */
template <class T, size_t Size, size_t Alignment, bool RequireStrictMatch = false>
class FastPimpl {
public:
  template <typename... Args>
  explicit(sizeof...(Args) ==
           1) constexpr FastPimpl(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    requires(sizeof...(Args) != 1 || (!std::same_as<std::remove_cvref_t<Args>, FastPimpl> && ...))
  {
    std::construct_at(Impl(), std::forward<Args>(args)...);
  }

  constexpr FastPimpl(const FastPimpl& other) noexcept(std::is_nothrow_copy_constructible_v<T>) : FastPimpl(*other) {}
  constexpr FastPimpl(FastPimpl&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
      : FastPimpl(std::move(*other)) {}

  constexpr ~FastPimpl() noexcept(std::is_nothrow_destructible_v<T>);

  constexpr FastPimpl& operator=(const FastPimpl& rhs) noexcept(std::is_nothrow_copy_assignable_v<T>);
  constexpr FastPimpl& operator=(FastPimpl&& rhs) noexcept(std::is_nothrow_move_assignable_v<T>);

  constexpr T* operator->() noexcept { return Impl(); }
  constexpr const T* operator->() const noexcept { return Impl(); }
  constexpr T& operator*() noexcept { return *Impl(); }
  constexpr const T& operator*() const noexcept { return *Impl(); }

private:
  static consteval void ValidateConstraints() noexcept;

  constexpr T* Impl() noexcept { return std::bit_cast<T*>(storage_.data()); }
  constexpr const T* Impl() const noexcept { return std::bit_cast<T*>(storage_.data()); }

  alignas(Alignment) std::array<std::byte, Size> storage_;
};

template <class T, size_t Size, size_t Alignment, bool RequireStrictMatch>
constexpr FastPimpl<T, Size, Alignment, RequireStrictMatch>::~FastPimpl() noexcept(std::is_nothrow_destructible_v<T>) {
  ValidateConstraints();
  std::destroy_at(Impl());
}

template <class T, size_t Size, size_t Alignment, bool RequireStrictMatch>
constexpr auto FastPimpl<T, Size, Alignment, RequireStrictMatch>::operator=(const FastPimpl& rhs) noexcept(
    std::is_nothrow_copy_assignable_v<T>) -> FastPimpl& {
  if (this != &rhs) [[likely]] {
    *Impl() = *rhs;
  }
  return *this;
}

template <class T, size_t Size, size_t Alignment, bool RequireStrictMatch>
constexpr auto FastPimpl<T, Size, Alignment, RequireStrictMatch>::operator=(FastPimpl&& rhs) noexcept(
    std::is_nothrow_move_assignable_v<T>) -> FastPimpl& {
  if (this != &rhs) [[likely]] {
    *Impl() = std::move(*rhs);
  }
  return *this;
}

template <class T, size_t Size, size_t Alignment, bool RequireStrictMatch>
consteval void FastPimpl<T, Size, Alignment, RequireStrictMatch>::ValidateConstraints() noexcept {
  static_assert(Size >= sizeof(T), "FastPimpl: Size must be >= sizeof(T)");
  static_assert(!RequireStrictMatch || Size == sizeof(T), "FastPimpl: Strict match required but Size != sizeof(T)");

  static_assert(Alignment % alignof(T) == 0, "FastPimpl: Alignment must be a multiple of alignof(T)");
  static_assert(!RequireStrictMatch || Alignment == alignof(T),
                "FastPimpl: Strict match required but Alignment != alignof(T)");
}

}  // namespace helios::utils
