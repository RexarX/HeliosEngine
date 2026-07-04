#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>

namespace helios::utils {

/**
 * @brief Implements pimpl idiom without dynamic memory allocation.
 * @details FastPimpl doesn't require memory allocation or indirect memory
 * access. You must manually set object size and alignment when instantiating
 * FastPimpl.
 * @tparam T The implementation type
 * @tparam Size The size in bytes to allocate for T
 * @tparam Alignment The alignment requirement for T
 * @tparam RequireStrictMatch If true, requires exact size/alignment match
 */
template <class T, size_t Size, size_t Alignment,
          bool RequireStrictMatch = false>
class FastPimpl {
public:
  template <typename... Args>
  explicit(sizeof...(Args) == 1) FastPimpl(Args&&... args) noexcept(
      std::is_nothrow_constructible_v<T, Args...>) {
    std::construct_at(Impl(), std::forward<Args>(args)...);
  }

  FastPimpl(const FastPimpl& other) noexcept(
      noexcept(FastPimpl(std::declval<const T&>())))
      : FastPimpl(*other) {}

  FastPimpl(FastPimpl&& other) noexcept(
      noexcept(FastPimpl(std::declval<T&&>())))
      : FastPimpl(std::move(*other)) {}

  ~FastPimpl() noexcept(noexcept(std::destroy_at(std::declval<T*>())));

  FastPimpl& operator=(const FastPimpl& rhs) noexcept(
      noexcept(std::declval<T&>() = std::declval<const T&>()));

  FastPimpl& operator=(FastPimpl&& rhs) noexcept(
      noexcept(std::declval<T&>() = std::declval<T&&>()));

  /**
   * @brief Copy-assigns from a T instance.
   * @param value Source value
   * @return Reference to this instance
   */
  FastPimpl& operator=(const T& value) noexcept(
      noexcept(std::declval<T&>() = std::declval<const T&>()));

  /**
   * @brief Move-assigns from a T instance.
   * @param value Source value
   * @return Reference to this instance
   */
  FastPimpl& operator=(T&& value) noexcept(
      noexcept(std::declval<T&>() = std::declval<T&&>()));

  T* operator->() noexcept { return Impl(); }
  const T* operator->() const noexcept { return Impl(); }
  T& operator*() noexcept { return *Impl(); }
  const T& operator*() const noexcept { return *Impl(); }

private:
  static consteval void ValidateConstraints() noexcept;

  T* Impl() noexcept {
    return std::launder(reinterpret_cast<T*>(storage_.data()));
  }

  const T* Impl() const noexcept {
    return std::launder(reinterpret_cast<const T*>(storage_.data()));
  }

  alignas(Alignment) std::array<std::byte, Size> storage_;
};

template <class T, size_t Size, size_t Alignment, bool RequireStrictMatch>
inline FastPimpl<T, Size, Alignment, RequireStrictMatch>::~FastPimpl() noexcept(
    noexcept(std::destroy_at(std::declval<T*>()))) {
  ValidateConstraints();
  std::destroy_at(Impl());
}

template <class T, size_t Size, size_t Alignment, bool RequireStrictMatch>
inline auto FastPimpl<T, Size, Alignment, RequireStrictMatch>::operator=(
    const FastPimpl& rhs) noexcept(noexcept(std::declval<T&>() =
                                                std::declval<const T&>()))
    -> FastPimpl& {
  if (this != &rhs) [[likely]] {
    *Impl() = *rhs;
  }
  return *this;
}

template <class T, size_t Size, size_t Alignment, bool RequireStrictMatch>
inline auto FastPimpl<T, Size, Alignment, RequireStrictMatch>::operator=(
    FastPimpl&& rhs) noexcept(noexcept(std::declval<T&>() =
                                           std::declval<T&&>())) -> FastPimpl& {
  if (this != &rhs) [[likely]] {
    *Impl() = std::move(*rhs);
  }
  return *this;
}

template <class T, size_t Size, size_t Alignment, bool RequireStrictMatch>
inline auto FastPimpl<T, Size, Alignment, RequireStrictMatch>::operator=(
    const T& value) noexcept(noexcept(std::declval<T&>() =
                                          std::declval<const T&>()))
    -> FastPimpl& {
  if (Impl() != &value) [[likely]] {
    *Impl() = value;
  }
  return *this;
}

template <class T, size_t Size, size_t Alignment, bool RequireStrictMatch>
inline auto FastPimpl<T, Size, Alignment, RequireStrictMatch>::operator=(
    T&& value) noexcept(noexcept(std::declval<T&>() = std::declval<T&&>()))
    -> FastPimpl& {
  if (Impl() != &value) [[likely]] {
    *Impl() = std::move(value);
  }
  return *this;
}

template <class T, size_t Size, size_t Alignment, bool RequireStrictMatch>
consteval void FastPimpl<T, Size, Alignment,
                         RequireStrictMatch>::ValidateConstraints() noexcept {
  static_assert(Size >= sizeof(T), "FastPimpl: Size must be >= sizeof(T)");
  static_assert(!RequireStrictMatch || Size == sizeof(T),
                "FastPimpl: Strict match required but Size != sizeof(T)");

  static_assert(Alignment % alignof(T) == 0,
                "FastPimpl: Alignment must be a multiple of alignof(T)");
  static_assert(!RequireStrictMatch || Alignment == alignof(T),
                "FastPimpl: Strict match required but Alignment != alignof(T)");
}

}  // namespace helios::utils
