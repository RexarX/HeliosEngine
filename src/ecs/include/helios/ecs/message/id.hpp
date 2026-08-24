#pragma once

#include <helios/assert.hpp>
#include <helios/ecs/message/message.hpp>

#include <cstddef>
#include <optional>

namespace helios::ecs {

struct AnyMessageId;

/**
 * @brief Monotonic identity for a buffered message of type `T`.
 * @details Assigned when a message enters the global `MessageManager` (direct
 * write or local-queue merge). Identities survive buffer aging and are stable
 * under consumable-message removal.
 * @tparam T Message type satisfying `MessageTrait`
 */
template <MessageTrait T>
struct MessageId {
  size_t value = 0;

  constexpr explicit MessageId(size_t id = 0) noexcept : value(id) {}

  /**
   * @brief Creates a typed `MessageId<T>` from a type-erased `AnyMessageId`.
   * @param id Type-erased message identity
   * @return Typed message identity
   */
  [[nodiscard]] static constexpr MessageId From(AnyMessageId id) noexcept;

  /**
   * @brief Convert from typed `MessageId<T>` to type-erased `AnyMessageId`.
   * @return Type-erased message identity
   */
  [[nodiscard]] constexpr AnyMessageId ToAny() const noexcept;

  [[nodiscard]] constexpr auto operator<=>(const MessageId&) const noexcept =
      default;
};

/**
 * @brief Type-erased, type-checked message identity.
 * @details Stored in `MessageManager` and `ConsumedMessagesRegistry`
 * containers. Carries the originating `MessageTypeIndex` so `As<T>()` /
 * `TryAs<T>()` can recover a typed `MessageId<T>` safely at API boundaries.
 */
struct AnyMessageId {
  size_t value = 0;
  MessageTypeIndex type;

  /**
   * @brief Creates a type-erased `AnyMessageId` from a typed `MessageId<T>`.
   * @tparam T Message type satisfying `MessageTrait`
   * @param id Typed message identity
   * @return Type-erased message identity
   */
  template <MessageTrait T>
  [[nodiscard]] static constexpr AnyMessageId From(MessageId<T> id) noexcept {
    return {.value = id.value, .type = MessageTypeIndex::From<T>()};
  }

  /**
   * @brief Converts a type-erased `AnyMessageId` to a typed `MessageId<T>`.
   * @warning Triggers assertion if the underlying type does not match `T`.
   * @tparam T Message type satisfying `MessageTrait`
   * @return Typed message identity
   */
  template <MessageTrait T>
  [[nodiscard]] constexpr auto As() const noexcept -> MessageId<T> {
    HELIOS_ASSERT(type == MessageTypeIndex::From<T>(),
                  "AnyMessageId type mismatch!");
    return MessageId<T>{value};
  }

  /**
   * @brief Tried to convert a type-erased `AnyMessageId` to a typed
   * `MessageId<T>`.
   * @tparam T Message type satisfying `MessageTrait`
   * @return Optional typed message identity
   */
  template <MessageTrait T>
  [[nodiscard]] constexpr auto TryAs() const noexcept
      -> std::optional<MessageId<T>> {
    if (type != MessageTypeIndex::From<T>()) {
      return std::nullopt;
    }
    return MessageId<T>{value};
  }

  [[nodiscard]] constexpr auto operator<=>(const AnyMessageId&) const noexcept =
      default;
};

template <MessageTrait T>
constexpr auto MessageId<T>::From(AnyMessageId id) noexcept -> MessageId<T> {
  return id.As<T>();
}

template <MessageTrait T>
constexpr AnyMessageId MessageId<T>::ToAny() const noexcept {
  return AnyMessageId::From(*this);
}

}  // namespace helios::ecs
