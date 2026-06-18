#pragma once

#include <helios/ecs/message/message.hpp>

#include <string_view>
#include <type_traits>
#include <utility>

namespace helios::ecs {

/**
 * @brief A wrapper around an async message that provides convenient access to
 * the message's data and type information.
 * @details Unlike `MessageWrapper` for regular messages, `AsyncMessageWrapper`
 * owns the message.
 * @note Thread-safe.
 * @tparam T The type of the message being wrapped. Must satisfy
 * `AsyncMessageTrait`
 */
template <AsyncMessageTrait T>
class AsyncMessageWrapper {
public:
  /**
   * @brief Constructs an AsyncMessageWrapper by moving in an message.
   * @param message Message to wrap (moved)
   */
  explicit constexpr AsyncMessageWrapper(T&& message) noexcept(
      std::is_nothrow_move_constructible_v<T>)
      : message_(std::move(message)) {}
  constexpr AsyncMessageWrapper(const AsyncMessageWrapper&) noexcept(
      std::is_nothrow_copy_constructible_v<T>) = default;
  constexpr AsyncMessageWrapper(AsyncMessageWrapper&&) noexcept(
      std::is_nothrow_move_constructible_v<T>) = default;
  constexpr ~AsyncMessageWrapper() noexcept(std::is_nothrow_destructible_v<T>) =
      default;

  constexpr AsyncMessageWrapper& operator=(const AsyncMessageWrapper&) noexcept(
      std::is_nothrow_copy_assignable_v<T>) = default;
  constexpr AsyncMessageWrapper& operator=(AsyncMessageWrapper&&) noexcept(
      std::is_nothrow_move_assignable_v<T>) = default;

  /**
   * @brief Provides member access to the underlying message (mutable).
   * @return A pointer to the underlying message data
   */
  constexpr T* operator->() noexcept { return &message_; }

  /**
   * @brief Provides member access to the underlying message (const).
   * @return A const pointer to the underlying message data
   */
  constexpr const T* operator->() const noexcept { return &message_; }

  /**
   * @brief Dereferences to the underlying message (mutable).
   * @return A reference to the underlying message data
   */
  [[nodiscard]] constexpr T& operator*() noexcept { return message_; }

  /**
   * @brief Dereferences to the underlying message (const).
   * @return A const reference to the underlying message data
   */
  [[nodiscard]] constexpr const T& operator*() const noexcept {
    return message_;
  }

  /**
   * @brief Gets the name of the message type.
   * @return A string view containing the name of the message type
   */
  [[nodiscard]] constexpr std::string_view Name() const noexcept {
    return MessageNameOf<T>();
  }

  /**
   * @brief Gets the type index of the message.
   * @return An `MessageTypeIndex` representing the type index of the message
   */
  [[nodiscard]] constexpr MessageTypeIndex TypeIndex() const noexcept {
    return MessageTypeIndex::From<T>();
  }

  /**
   * @brief Unwraps the message wrapper, returning a mutable reference.
   * @return A mutable reference to the underlying message data
   */
  [[nodiscard]] constexpr T& Unwrap() noexcept { return message_; }

  /**
   * @brief Unwraps the message wrapper, returning a const reference.
   * @return A const reference to the underlying message data
   */
  [[nodiscard]] constexpr const T& Unwrap() const noexcept { return message_; }

  /**
   * @brief Moves the message out of the wrapper.
   * @details After calling this, the wrapper's message is in a moved-from
   * state.
   * @return The message, moved out of the wrapper
   */
  [[nodiscard]] constexpr T Take() noexcept(
      std::is_nothrow_move_constructible_v<T>) {
    return std::move(message_);
  }

private:
  T message_;  ///< The owned message
};

}  // namespace helios::ecs
