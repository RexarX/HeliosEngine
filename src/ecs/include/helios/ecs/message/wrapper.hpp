#pragma once

#include <helios/assert.hpp>
#include <helios/ecs/message/consumed_registry.hpp>
#include <helios/ecs/message/id.hpp>

#include <cstddef>
#include <functional>
#include <memory_resource>
#include <string_view>

namespace helios::ecs {

/**
 * @brief A wrapper around a message that provides convenient access to the
 * message's data, type information, and the ability to mark the message as
 * consumed.
 * @details It holds a const reference to the underlying message, a reference to
 * the `ConsumedMessagesRegistry`, and the stable message id.
 *
 * Marking a message as consumed writes into the per-system registry,
 * which is later merged and applied at `MessageManager::Update` time.
 *
 * @note Thread-safe, except for the `Consume` method which is NOT thread-safe.
 * @tparam T The type of the message being wrapped. Must satisfy
 * `ConsumableMessageTrait`.
 * @tparam Alloc Allocator type for the consumed messages registry
 */
template <ConsumableMessageTrait T,
          typename Alloc = std::pmr::polymorphic_allocator<std::byte>>
class ConsumableMessageWrapper {
public:
  /**
   * @brief Constructs a `ConsumableMessageWrapper`.
   * @details Intended to be called by `ConsumableMessageReader`; not part of
   * the public construction API.
   * @param message Const reference to the message
   * @param registry Reference to the per-system consumed messages registry
   * @param id Stable message id
   */
  constexpr ConsumableMessageWrapper(const T& message,
                                     ConsumedMessagesRegistry<Alloc>& registry,
                                     MessageId<T> id) noexcept
      : message_(message), id_(id), registry_(registry) {}

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
    registry_.get().template MarkConsumed<T>(id_);
  }

  /**
   * @brief Checks whether this message has been marked as consumed by the
   * owning system.
   * @return True if the message has been consumed, false otherwise
   */
  [[nodiscard]] bool IsConsumed() const noexcept {
    return registry_.get().template IsConsumed<T>(id_);
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
   * @brief Gets the stable id of this message.
   * @return Message id
   */
  [[nodiscard]] constexpr auto Id() const noexcept -> MessageId<T> {
    return id_;
  }

private:
  std::reference_wrapper<const T> message_;  ///< Const reference to the message
  MessageId<T> id_;                          ///< Stable message id
  /// Per-system consumed registry
  std::reference_wrapper<ConsumedMessagesRegistry<Alloc>> registry_;
};

/**
 * @brief A wrapper around a message that provides convenient access to the
 * message's data and type information.
 * @details It holds a const reference to the underlying message and its stable
 * message id.
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
   * @param id Stable message id
   */
  constexpr MessageWrapper(const T& message, MessageId<T> id) noexcept
      : message_(message), id_(id) {}

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
   * @brief Gets the stable id of this message.
   * @return Message id
   */
  [[nodiscard]] constexpr auto Id() const noexcept -> MessageId<T> {
    return id_;
  }

private:
  std::reference_wrapper<const T> message_;  ///< Const reference to the message
  MessageId<T> id_;                          ///< Stable message id
};

}  // namespace helios::ecs
