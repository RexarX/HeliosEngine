#pragma once

#include <helios/ecs/message/id.hpp>
#include <helios/ecs/message/manager.hpp>
#include <helios/ecs/message/message.hpp>

#include <cstddef>

namespace helios::ecs {

/**
 * @brief Per-reader cursor tracking which messages of type `T` have been seen.
 * @details Retention (previous/current buffers) and delivery (this cursor) are
 * independent. Two systems each keep their own cursor and can both observe the
 * same retained message once. Store via `Local` / `SystemLocalData` for
 * system parameters; pass explicitly to `World::ReadMessages`.
 * @tparam T Message type satisfying `MessageTrait`
 */
template <MessageTrait T>
struct MessageCursor {
  /// @brief Next message id this cursor has not yet observed.
  MessageId<T> last_message_count;

  /**
   * @brief Creates a cursor that includes all currently retained messages.
   * @return Cursor starting at id 0
   */
  [[nodiscard]] static constexpr MessageCursor IncludeBacklog() noexcept {
    return {.last_message_count = MessageId<T>{0}};
  }

  /**
   * @brief Creates a cursor that ignores currently retained messages.
   * @param manager Message manager providing the current stream end
   * @return Cursor positioned at the next id to be written
   */
  [[nodiscard]] static MessageCursor FutureOnly(
      const MessageManager& manager) noexcept {
    return {.last_message_count = manager.template MessageCount<T>()};
  }

  /**
   * @brief Skips all currently retained messages for this cursor.
   * @param manager Message manager providing the current stream end
   */
  void Clear(const MessageManager& manager) noexcept {
    last_message_count = manager.template MessageCount<T>();
  }

  /**
   * @brief Returns whether this cursor has no unread retained messages.
   * @param manager Message manager to query
   * @return `true` when `Count()` is zero
   */
  [[nodiscard]] bool Empty(const MessageManager& manager) const noexcept {
    return Count(manager) == 0;
  }

  /**
   * @brief Returns how many messages were dropped before this cursor caught up.
   * @param manager Message manager to query
   * @return Count of ids older than the oldest retained message
   */
  [[nodiscard]] size_t MissedMessages(
      const MessageManager& manager) const noexcept {
    const auto oldest = manager.template OldestMessageCount<T>();
    return last_message_count.value < oldest.value
               ? oldest.value - last_message_count.value
               : 0;
  }

  /**
   * @brief Returns how many retained messages this cursor has not read.
   * @param manager Message manager to query
   * @return Unread retained count
   */
  [[nodiscard]] size_t Count(const MessageManager& manager) const noexcept {
    return manager.template UnreadCount<T>(last_message_count);
  }
};

}  // namespace helios::ecs
