#pragma once

#include <helios/utils/type_info.hpp>

#include <concepts>
#include <cstddef>
#include <cstring>
#include <memory>
#include <type_traits>

namespace helios::container {

/**
 * @brief Concept for types storable in `TypedBuffer` / `TypedBufferArray`.
 * @details Requires the type to be destructible, not polymorphic,
 * and have alignment less than or equal to `std::max_align_t`.
 */
template <typename T>
concept TypedBufferStorable =
    std::destructible<T> && !std::is_polymorphic_v<std::remove_cvref_t<T>> &&
    (alignof(std::remove_cvref_t<T>) <= alignof(std::max_align_t));

namespace details {

using TypeBufferSizeType = size_t;
using TypeBufferDestroyFn = void (*)(void*, TypeBufferSizeType);
using TypeBufferCopyFn = void (*)(void*, const void*, TypeBufferSizeType);
using TypeBufferMoveFn = void (*)(void*, void*, TypeBufferSizeType);
using TypeBufferDefaultCtorFn = void (*)(void*, TypeBufferSizeType);
using TypeBufferRelocateFn = void (*)(void*, void*, TypeBufferSizeType);

/// @brief Runtime type information for type-erased buffer elements.
struct TypeBufferInfo {
  utils::TypeIndex type_index;
  TypeBufferSizeType element_size = 0;
  TypeBufferSizeType element_align = 0;
  TypeBufferDestroyFn destroy = nullptr;
  TypeBufferCopyFn copy_construct = nullptr;
  TypeBufferMoveFn move_construct = nullptr;
  TypeBufferDefaultCtorFn default_construct = nullptr;
  TypeBufferRelocateFn relocate =
      nullptr;  ///< Move + destroy source (for shifting).
  bool is_trivially_copyable = false;
  bool is_copy_constructible = false;

  /**
   * @brief Creates a `TypeBufferInfo` populated with function pointers for type
   * `T`.
   * @tparam T The type to create info for
   * @return Populated `TypeBufferInfo` for type `T`
   */
  template <typename T>
  [[nodiscard]] static constexpr TypeBufferInfo From() noexcept;

  /// @brief Resets all fields to their default values.
  constexpr void Reset() noexcept {
    type_index = utils::TypeIndex();
    element_size = 0;
    element_align = 0;
    destroy = nullptr;
    copy_construct = nullptr;
    move_construct = nullptr;
    default_construct = nullptr;
    relocate = nullptr;
    is_trivially_copyable = false;
    is_copy_constructible = false;
  }

  /// @brief Returns true if the type info has been set (non-empty type index).
  [[nodiscard]] constexpr bool IsValid() const noexcept {
    return !type_index.Empty();
  }
};

}  // namespace details

}  // namespace helios::container

template <typename T>
constexpr auto helios::container::details::TypeBufferInfo::From() noexcept
    -> TypeBufferInfo {
  TypeBufferInfo info{};
  info.type_index = utils::TypeIndex::From<T>();
  info.element_size = sizeof(T);
  info.element_align = alignof(T);
  info.is_trivially_copyable = std::is_trivially_copyable_v<T>;
  info.is_copy_constructible = std::copy_constructible<T>;

  if constexpr (!std::is_trivially_destructible_v<T>) {
    info.destroy = [](void* ptr, TypeBufferSizeType count) {
      auto* typed_ptr = static_cast<T*>(ptr);
      std::destroy_n(typed_ptr, count);
    };
  }

  if constexpr (std::copy_constructible<T>) {
    info.copy_construct = [](void* dest, const void* src,
                             TypeBufferSizeType count) {
      auto* typed_dest = static_cast<T*>(dest);
      const auto* typed_src = static_cast<const T*>(src);
      if constexpr (std::is_trivially_copyable_v<T>) {
        std::memcpy(typed_dest, typed_src, count * sizeof(T));
      } else {
        std::uninitialized_copy_n(typed_src, count, typed_dest);
      }
    };
  }

  if constexpr (std::move_constructible<T>) {
    info.move_construct = [](void* dest, void* src, TypeBufferSizeType count) {
      auto* typed_dest = static_cast<T*>(dest);
      auto* typed_src = static_cast<T*>(src);
      if constexpr (std::is_trivially_copyable_v<T>) {
        std::memcpy(typed_dest, typed_src, count * sizeof(T));
      } else {
        std::uninitialized_move_n(typed_src, count, typed_dest);
      }
    };
  }

  if constexpr (std::default_initializable<T>) {
    info.default_construct = [](void* dest, TypeBufferSizeType count) {
      auto* typed_dest = static_cast<T*>(dest);
      std::uninitialized_default_construct_n(typed_dest, count);
    };
  }

  if constexpr (std::is_trivially_copyable_v<T>) {
    info.relocate = [](void* dest, void* src, TypeBufferSizeType count) {
      std::memmove(dest, src, count * sizeof(T));
    };
  } else if constexpr (std::is_nothrow_move_constructible_v<T>) {
    info.relocate = [](void* dest, void* src, TypeBufferSizeType count) {
      auto* typed_dest = static_cast<T*>(dest);
      auto* typed_src = static_cast<T*>(src);
      std::uninitialized_move_n(typed_src, count, typed_dest);
      std::destroy_n(typed_src, count);
    };
  }

  return info;
}
