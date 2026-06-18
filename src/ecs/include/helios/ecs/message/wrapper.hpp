#pragma once

#include <helios/assert.hpp>
#include <helios/ecs/message/consumed_registry.hpp>
#include <helios/ecs/message/message.hpp>

#include <cstddef>
#include <functional>
#include <string_view>

namespace helios::ecs {

/**
 * @brief A wrapper around a message that provides convenient access to the
 * message's data, type information, and the ability to mark the message as
 * consumed.
 * @details It holds a const reference to the underlying message, a reference to
 * the `ConsumedMessagesRegistry`, and the global index of the message within
 * the combined (previous + current) view.
 *
 * Marking a message as consumed writes into the per-system registry,
 * which is later merged and applied at `MessageManager::Update` time.
 *
 * @note Thread-safe, except for the `Consume` method which is NOT thread-safe.
 * @tparam T The type of the message being wrapped. Must satisfy
 * `ConsumableMessageTrait`.
 */
template <ConsumableMessageTrait T>
class ConsumableMessageWrapper {
public:
  /**
   * @brief Constructs a `ConsumableMessageWrapper`.
   * @details Intended to be called by `ConsumableMessageReader`; not part of
   * the public construction API.
   * @param message Const reference to the message
   * @param registry Reference to the per-system consumed messages registry
   * @param global_index Global index of the message in the combined (previous +
   * current) view
   */
  constexpr ConsumableMessageWrapper(const T& message,
                                     ConsumedMessagesRegistry<>& registry,
                                     size_t global_index) noexcept
      : message_(message), global_index_(global_index), registry_(registry) {}

  constexpr ConsumableMessageWrapper(const ConsumableMessageWrapper&) noexcept =
      default;
  constexpr ConsumableMessageWrapper(ConsumableMessageWrapper&&) noexcept =
      default;
  constexpr ~ConsumableMessageWrapper() noexcept = default;

  constexpr ConsumableMessageWrapper& operator=(
      const ConsumableMessageWrapper&) noexcept = default;
  constexpr ConsumableMessageWrapper& operator=(
      ConsumableMessageWrapper&&) noexcept = default;

  /**
   * @brief Dereferences to the underlying message.
   * @return A const reference to the underlying message data
   */
  [[nodiscard]] constexpr const T& operator*() const noexcept {
    return message_.get();
  }

  /**
   * @brief Provides member access to the underlying message.
   * @return A pointer to the underlying message data
   */
  constexpr const T* operator->() const noexcept { return &message_.get(); }

  /**
   * @brief Marks the message as consumed.
   * @details The message will not be propagated to subsequent frames after
   * `MessageManager::Update` merges all per-system consumed registries.
   * Multiple calls are idempotent (sorted-unique insertion).
   */
  constexpr void Consume() const {
    registry_.get().template MarkConsumed<T>(global_index_);
  }

  /**
   * @brief Checks whether this message has been marked as consumed by the
   * owning system.
   * @return True if the message has been consumed, false otherwise
   */
  [[nodiscard]] bool IsConsumed() const noexcept {
    return registry_.get().template IsConsumed<T>(global_index_);
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
   * @return A `MessageTypeIndex` representing the type index of the message
   */
  [[nodiscard]] constexpr MessageTypeIndex TypeIndex() const noexcept {
    return MessageTypeIndex::From<T>();
  }

  /**
   * @brief Gets the global index of this message in the combined (previous +
   * current) view.
   * @return Global index
   */
  [[nodiscard]] constexpr size_t GlobalIndex() const noexcept {
    return global_index_;
  }

  /**
   * @brief Unwraps the message wrapper, returning a const reference to the
   * underlying message.
   * @return A const reference to the underlying message data
   */
  [[nodiscard]] constexpr const T& Unwrap() const noexcept {
    return message_.get();
  }

private:
  std::reference_wrapper<const T> message_;  ///< Const reference to the message
  size_t global_index_ = 0;                  ///< Global index in combined view
  std::reference_wrapper<ConsumedMessagesRegistry<>>
      registry_;  ///< Per-system consumed registry
};

/**
 * @brief A wrapper around a message that provides convenient access to the
 * message's data and type information.
 * @details It holds a const reference to the underlying message and the global
 * index of the message within the combined (previous + current) view.
 *
 * Unlike `ConsumableMessageWrapper`, this wrapper does not provide message
 * consumption functionality.
 *
 * @note Thread-safe.
 * @tparam T The type of the message being wrapped. Must satisfy `MessageTrait`.
 */
template <MessageTrait T>
class MessageWrapper {
public:
  /**
   * @brief Constructs a `MessageWrapper`.
   * @details Intended to be called by `MessageReader`; not part of the public
   * construction API.
   * @param message Const reference to the message
   * @param global_index Global index of the message in the combined (previous +
   * current) view
   */
  constexpr MessageWrapper(const T& message, size_t global_index) noexcept
      : message_(message), global_index_(global_index) {}

  constexpr MessageWrapper(const MessageWrapper&) noexcept = default;
  constexpr MessageWrapper(MessageWrapper&&) noexcept = default;
  constexpr ~MessageWrapper() noexcept = default;

  constexpr MessageWrapper& operator=(const MessageWrapper&) noexcept = default;
  constexpr MessageWrapper& operator=(MessageWrapper&&) noexcept = default;

  /**
   * @brief Dereferences to the underlying message.
   * @return A const reference to the underlying message data
   */
  [[nodiscard]] constexpr const T& operator*() const noexcept {
    return message_.get();
  }

  /**
   * @brief Provides member access to the underlying message.
   * @return A pointer to the underlying message data
   */
  constexpr const T* operator->() const noexcept { return &message_.get(); }

  /**
   * @brief Gets the name of the message type.
   * @return A string view containing the name of the message type
   */
  [[nodiscard]] constexpr std::string_view Name() const noexcept {
    return MessageNameOf<T>();
  }

  /**
   * @brief Gets the type index of the message.
   * @return A `MessageTypeIndex` representing the type index of the message
   */
  [[nodiscard]] constexpr MessageTypeIndex TypeIndex() const noexcept {
    return MessageTypeIndex::From<T>();
  }

  /**
   * @brief Gets the global index of this message in the combined (previous +
   * current) view.
   * @return Global index
   */
  [[nodiscard]] constexpr size_t GlobalIndex() const noexcept {
    return global_index_;
  }

  /**
   * @brief Unwraps the message wrapper, returning a const reference to the
   * underlying message.
   * @return A const reference to the underlying message data
   */
  [[nodiscard]] constexpr const T& Unwrap() const noexcept {
    return message_.get();
  }

private:
  std::reference_wrapper<const T> message_;  ///< Const reference to the message
  size_t global_index_ = 0;                  ///< Global index in combined view
};

}  // namespace helios::ecs
