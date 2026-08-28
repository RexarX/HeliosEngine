#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.input;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <bitset>
#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>
#endif
#include <helios/assert.hpp>

HELIOS_MODULE_EXPORT
namespace helios::input {

/**
 * @brief Concept for button enums used by `ButtonInput`.
 * @details Requires an enum type with a contiguous `kCount` sentinel.
 * @tparam T Button enum type
 */
template <typename T>
concept ButtonTrait = std::is_enum_v<T> && requires {
  { std::to_underlying(T::kCount) } -> std::convertible_to<size_t>;
};

/**
 * @brief Tracks pressed / just-pressed / just-released state for button enums.
 * @tparam T Button enum satisfying `ButtonTrait`
 */
template <ButtonTrait T>
class ButtonInput {
public:
  static constexpr auto kSize =
      static_cast<size_t>(std::to_underlying(T::kCount));

  /**
   * @brief Marks a button as pressed.
   * @param button Button to press
   * @details Sets `just_pressed` only when transitioning from released.
   */
  constexpr void Press(T button) noexcept;

  /**
   * @brief Marks a button as released.
   * @param button Button to release
   * @details Sets `just_released` only when transitioning from pressed.
   */
  constexpr void Release(T button) noexcept;

  /// @brief Clears edge state (`just_pressed` / `just_released`) only.
  constexpr void Clear() noexcept;

  /// @brief Clears pressed and edge state.
  constexpr void Reset() noexcept;

  /**
   * @brief Tests whether a button is currently held.
   * @param button Button to test
   * @return True if the button is pressed
   */
  [[nodiscard]] constexpr bool Pressed(T button) const noexcept {
    return pressed_[Index(button)];
  }

  /**
   * @brief Tests whether a button was pressed this frame.
   * @param button Button to test
   * @return True if the button transitioned to pressed this frame
   */
  [[nodiscard]] constexpr bool JustPressed(T button) const noexcept {
    return just_pressed_[Index(button)];
  }

  /**
   * @brief Tests whether a button was released this frame.
   * @param button Button to test
   * @return True if the button transitioned to released this frame
   */
  [[nodiscard]] constexpr bool JustReleased(T button) const noexcept {
    return just_released_[Index(button)];
  }

  /**
   * @brief Tests whether any button is currently held.
   * @return True if at least one button is pressed
   */
  [[nodiscard]] constexpr bool AnyPressed() const noexcept {
    return pressed_.any();
  }

  /**
   * @brief Tests whether all given buttons are currently held.
   * @tparam Ts Button types (must match `T`)
   * @param buttons Buttons to test
   * @return True if every listed button is pressed
   */
  template <typename... Ts>
    requires(std::same_as<T, Ts> && ...)
  [[nodiscard]] constexpr bool AllPressed(Ts... buttons) const noexcept {
    return (Pressed(buttons) && ...);
  }

private:
  /**
   * @brief Converts a button enum to a bitset index.
   * @param button Button value
   * @return Zero-based index into the bitsets
   * @warning Asserts when `button` is out of range (`>= kCount`).
   */
  [[nodiscard]] static constexpr size_t Index(T button) noexcept;

  std::bitset<kSize> pressed_{};
  std::bitset<kSize> just_pressed_{};
  std::bitset<kSize> just_released_{};
};

template <ButtonTrait T>
constexpr void ButtonInput<T>::Press(T button) noexcept {
  const size_t index = Index(button);
  if (!pressed_[index]) {
    just_pressed_[index] = true;
  }
  pressed_[index] = true;
}

template <ButtonTrait T>
constexpr void ButtonInput<T>::Release(T button) noexcept {
  const size_t index = Index(button);
  if (pressed_[index]) {
    just_released_[index] = true;
    pressed_[index] = false;
  }
}

template <ButtonTrait T>
constexpr void ButtonInput<T>::Clear() noexcept {
  just_pressed_.reset();
  just_released_.reset();
}

template <ButtonTrait T>
constexpr void ButtonInput<T>::Reset() noexcept {
  pressed_.reset();
  Clear();
}

template <ButtonTrait T>
constexpr size_t ButtonInput<T>::Index(T button) noexcept {
  const auto underlying = std::to_underlying(button);
  HELIOS_ASSERT(underlying < std::to_underlying(T::kCount),
                "button index out of range!");
  return static_cast<size_t>(underlying);
}

/**
 * @brief Tracks pressed / just-pressed / just-released state by index.
 * @tparam N Maximum number of buttons
 */
template <size_t N>
class IndexedButtonInput {
public:
  static constexpr size_t kSize = N;

  /**
   * @brief Marks a button as pressed.
   * @param index Button index
   * @details Sets `just_pressed` only when transitioning from released.
   * @warning Asserts when `index` is out of range (`>= N`).
   */
  constexpr void Press(size_t index) noexcept;

  /**
   * @brief Marks a button as released.
   * @param index Button index
   * @details Sets `just_released` only when transitioning from pressed.
   * @warning Asserts when `index` is out of range (`>= N`).
   */
  constexpr void Release(size_t index) noexcept;

  /// @brief Clears edge state (`just_pressed` / `just_released`) only.
  constexpr void Clear() noexcept;

  /// @brief Clears pressed and edge state.
  constexpr void Reset() noexcept;

  /**
   * @brief Tests whether a button is currently held.
   * @param index Button index
   * @return True if the button is pressed
   * @warning Asserts when `index` is out of range (`>= N`).
   */
  [[nodiscard]] constexpr bool Pressed(size_t index) const noexcept {
    return pressed_[CheckIndex(index)];
  }

  /**
   * @brief Tests whether a button was pressed this frame.
   * @param index Button index
   * @return True if the button transitioned to pressed this frame
   * @warning Asserts when `index` is out of range (`>= N`).
   */
  [[nodiscard]] constexpr bool JustPressed(size_t index) const noexcept {
    return just_pressed_[CheckIndex(index)];
  }

  /**
   * @brief Tests whether a button was released this frame.
   * @param index Button index
   * @return True if the button transitioned to released this frame
   * @warning Asserts when `index` is out of range (`>= N`).
   */
  [[nodiscard]] constexpr bool JustReleased(size_t index) const noexcept {
    return just_released_[CheckIndex(index)];
  }

  /**
   * @brief Tests whether any button is currently held.
   * @return True if at least one button is pressed
   */
  [[nodiscard]] constexpr bool AnyPressed() const noexcept {
    return pressed_.any();
  }

private:
  /**
   * @brief Validates a button index.
   * @param index Button index
   * @return `index` when in range
   * @warning Asserts when `index` is out of range (`>= N`).
   */
  [[nodiscard]] static constexpr size_t CheckIndex(size_t index) noexcept;

  std::bitset<N> pressed_{};
  std::bitset<N> just_pressed_{};
  std::bitset<N> just_released_{};
};

template <size_t N>
constexpr void IndexedButtonInput<N>::Press(size_t index) noexcept {
  const size_t checked = CheckIndex(index);
  if (!pressed_[checked]) {
    just_pressed_[checked] = true;
  }
  pressed_[checked] = true;
}

template <size_t N>
constexpr void IndexedButtonInput<N>::Release(size_t index) noexcept {
  const size_t checked = CheckIndex(index);
  if (pressed_[checked]) {
    just_released_[checked] = true;
    pressed_[checked] = false;
  }
}

template <size_t N>
constexpr void IndexedButtonInput<N>::Clear() noexcept {
  just_pressed_.reset();
  just_released_.reset();
}

template <size_t N>
constexpr void IndexedButtonInput<N>::Reset() noexcept {
  pressed_.reset();
  Clear();
}

template <size_t N>
constexpr size_t IndexedButtonInput<N>::CheckIndex(size_t index) noexcept {
  HELIOS_ASSERT(index < N, "button index out of range!");
  return index;
}

}  // namespace helios::input
#endif  // HELIOS_MODULE_CONSUMER_SHIM
