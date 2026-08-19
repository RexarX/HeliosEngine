#pragma once

#include <helios/assert.hpp>
#include <helios/container/multi_type_map.hpp>
#include <helios/ecs/details/profile.hpp>
#include <helios/ecs/message/async_queue.hpp>
#include <helios/ecs/message/consumed_registry.hpp>
#include <helios/ecs/message/id.hpp>
#include <helios/ecs/message/message.hpp>
#include <helios/ecs/message/queue.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace helios::ecs {

/// @brief Metadata for a registered message type.
struct MessageMetadata {
  MessageTypeIndex type_index;  ///< Unique type index for the message
  std::string_view name;        ///< Human-readable name of the message
  MessageClearPolicy clear_policy =
      MessageClearPolicy::kAutomatic;  ///< Message clearing policy
  bool is_async = false;  ///< Whether this message uses the async queue
};

/**
 * @brief Central coordinator for message lifecycle with double buffering and
 * consumed message removal.
 * @details Implements message management with:
 * - Double buffering: messages persist for one full update cycle (two frames)
 * - Monotonic per-type message ids assigned on entry to the global queues
 * - Explicit registration: messages must be registered before use
 * - Automatic clearing: messages are cleared after their lifecycle expires
 * - Manual control: users can opt-out of auto-clearing
 * - Consumed message removal: at Update time, consumed messages are physically
 * removed from queues without additional heap allocation
 *
 * Message Lifecycle:
 * - Frame N: Messages written to current queue (ids assigned)
 * - Frame N+1: Messages readable from previous queue (after swap)
 * - Frame N+2: Messages cleared from previous queue (automatic policy)
 *
 * Delivery to systems is tracked separately via `MessageCursor<T>`. Retention
 * does not imply re-delivery to a cursor that already observed a message.
 *
 * Consumed messages are removed at Update time based on merged per-system
 * `ConsumedMessagesRegistry` instances. Each system writes to its own registry
 * during parallel execution, and all registries are merged and applied here.
 *
 * @note Partially thread-safe.
 */
class MessageManager {
public:
  using size_type = size_t;

  MessageManager() = default;
  MessageManager(const MessageManager&) = delete;
  MessageManager(MessageManager&&) noexcept = default;
  ~MessageManager() = default;

  MessageManager& operator=(const MessageManager&) = delete;
  MessageManager& operator=(MessageManager&&) noexcept = default;

  /**
   * @brief Clears all messages, consumed flags, and registration data.
   * @note Not thread-safe.
   */
  void Clear() noexcept;

  /**
   * @brief Clears all message data without removing registrations.
   * @note Not thread-safe.
   */
  void ClearAllQueues() noexcept;

  /**
   * @brief Updates message lifecycle — applies consumed messages, then ages
   * buffers.
   * @details Performs the following steps:
   * 1. Removes consumed messages from previous and current queues.
   * 2. Merges current queue into previous queue (does **not** clear automatic
   *    previous messages — call the no-arg `Update()` for full frame aging).
   * 3. Clears current messages in place (type registrations retained).
   * @note Not thread-safe. Prefer `ApplyConsumed` + `MergeLocalMessages` from
   * per-system deferred work; use no-arg `Update()` once per frame.
   * @tparam Alloc Allocator type for the consumed registry
   * @param consumed_registry Const References to per-system consumed
   * registry. The caller is responsible for clearing it after this call.
   */
  template <typename Alloc>
  void Update(const ConsumedMessagesRegistry<Alloc>& consumed_registry);

  /**
   * @brief Updates message lifecycle without any consumed message processing.
   * @details Convenience overload when no systems have consumed any messages.
   * @note Not thread-safe.
   */
  void Update();

  /**
   * @brief Applies consumed message ids to both queues, removing them in-place.
   * @details Consumed entries are stable message ids (not transient buffer
   * indices). No buffer swap occurs.
   * @note Not thread-safe.
   * @tparam Alloc Allocator type of the consumed registry
   * @param merged_consumed The combined consumed registry from all systems
   */
  template <typename Alloc>
  void ApplyConsumed(const ConsumedMessagesRegistry<Alloc>& merged_consumed);

  /**
   * @brief Registers multiple message types.
   * @note Not thread-safe.
   * @warning Triggers assertion if any message type is already registered.
   * @tparam Messages Message types to register, satisfying `AnyMessageTrait`
   */
  template <AnyMessageTrait... Messages>
    requires(sizeof...(Messages) > 0)
  void Register();

  /**
   * @brief Writes a single regular message to the current queue.
   * @note Not thread-safe.
   * @warning Triggers assertion if the message type is not registered.
   * @tparam T Message type satisfying `MessageTrait`
   * @param message Message to write
   * @return Assigned message id
   */
  template <MessageTrait T>
  auto Write(T&& message) -> MessageId<std::remove_cvref_t<T>>;

  /**
   * @brief Writes a single async message to the async queue.
   * @note Thread-safe.
   * @warning Triggers assertion if the message type is not registered.
   * @tparam T Async message type satisfying `AsyncMessageTrait`
   * @param message Message to write
   */
  template <AsyncMessageTrait T>
  void WriteAsync(T&& message);

  /**
   * @brief Writes multiple regular messages to the current queue in bulk.
   * @note Not thread-safe.
   * @warning Triggers assertion if the message type is not registered.
   * @tparam R Range whose value_type satisfies `MessageTrait`
   * @param messages Range of messages to write
   */
  template <std::ranges::input_range R>
    requires MessageTrait<std::ranges::range_value_t<R>>
  void WriteBulk(R&& messages);

  /**
   * @brief Writes multiple async messages to the async queue in bulk.
   * @note Thread-safe.
   * @warning Triggers assertion if the message type is not registered.
   * @tparam R Range whose value_type satisfies `AsyncMessageTrait`
   * @param messages Range of async messages to write
   */
  template <std::ranges::input_range R>
    requires AsyncMessageTrait<std::ranges::range_value_t<R>>
  void WriteAsyncBulk(R&& messages);

  /**
   * @brief Manually clears regular messages of a specific type from both
   * queues.
   * @note Not thread-safe.
   * @warning Triggers assertion if the message type is not registered.
   * @tparam T Message type satisfying `MessageTrait`
   */
  template <MessageTrait T>
  void ManualClear();

  /**
   * @brief Manually clears async messages of a specific type from the queue.
   * @note Thread-safe.
   * @warning Triggers assertion if the message type is not registered.
   * @tparam T Async message type satisfying `AsyncMessageTrait`
   */
  template <AsyncMessageTrait T>
  void ManualAsyncClear();

  /**
   * @brief Merges messages from a local `MessageQueue` into the current queue.
   * @note Not thread-safe.
   * @details Assigns monotonic ids, then flushes the per-system write buffer
   * into the global message state. Typically called after a system finishes
   * execution.
   * @tparam OtherAllocator Allocator type used by the local `MessageQueue`
   * @param local Local message queue to merge from (will be left in a valid but
   * empty state for the rvalue overload)
   */
  template <typename OtherAllocator>
  void MergeLocalMessages(const MessageQueue<OtherAllocator>& local);

  /**
   * @brief Merges messages from a local `MessageQueue` into the current queue.
   * @note Not thread-safe.
   * @details Rvalue overload that consumes the local queue.
   * @tparam OtherAllocator Allocator type used by the local `MessageQueue`
   * @param local Local message queue to merge from (will be left in a valid but
   * empty state)
   */
  template <typename OtherAllocator>
  void MergeLocalMessages(MessageQueue<OtherAllocator>&& local);

  /**
   * @brief Checks if an message type (regular or async) is registered.
   * @note Thread safe for concurrent reads.
   * @tparam T Message type satisfying `AnyMessageTrait`
   * @return True if the message type is registered, false otherwise
   */
  template <AnyMessageTrait T>
  [[nodiscard]] bool IsRegistered() const noexcept {
    return registered_messages_.Contains(MessageTypeIndex::From<T>());
  }

  /**
   * @brief Checks if any messages exist across regular and async queues.
   * @note Thread safe for concurrent reads.
   * @return True if at least one message exists, false otherwise
   */
  [[nodiscard]] bool HasMessages() const noexcept {
    return current_messages_.HasMessages() ||
           previous_messages_.HasMessages() || async_messages_.HasMessages();
  }

  /**
   * @brief Checks if regular messages of a specific type exist in either queue.
   * @note Thread safe for concurrent reads.
   * @tparam T Message type satisfying `MessageTrait`
   * @return True if messages exist, false otherwise
   */
  template <MessageTrait T>
  [[nodiscard]] bool HasMessages() const noexcept;

  /**
   * @brief Checks if async messages of a specific type exist.
   * @note Thread safe.
   * @tparam T Async message type satisfying `AsyncMessageTrait`
   * @return True if async messages exist, false otherwise
   */
  template <AsyncMessageTrait T>
  [[nodiscard]] bool HasAsyncMessages() const noexcept;

  /**
   * @brief Gets a const span of messages of a specific type from the previous
   * queue.
   * @note Thread safe for concurrent reads. Bypasses cursor delivery tracking.
   * @tparam T Message type satisfying `MessageTrait`
   * @return Span of const messages
   */
  template <MessageTrait T>
  [[nodiscard]] auto PreviousMessages() const noexcept -> std::span<const T> {
    return previous_messages_.Messages<T>();
  }

  /**
   * @brief Gets a const span of messages of a specific type from the current
   * queue.
   * @note Thread safe for concurrent reads. Bypasses cursor delivery tracking.
   * @tparam T Message type satisfying `MessageTrait`
   * @return Span of const messages
   */
  template <MessageTrait T>
  [[nodiscard]] auto CurrentMessages() const noexcept -> std::span<const T> {
    return current_messages_.Messages<T>();
  }

  /**
   * @brief Gets ids for previous-buffer messages of type `T`.
   * @note Thread safe for concurrent reads.
   * @tparam T Message type satisfying `MessageTrait`
   * @return Span of message ids aligned with `PreviousMessages<T>()`
   */
  template <MessageTrait T>
  [[nodiscard]] auto PreviousIds() const noexcept
      -> std::span<const AnyMessageId> {
    return IdsFor(previous_ids_, MessageTypeIndex::From<T>());
  }

  /**
   * @brief Gets ids for current-buffer messages of type `T`.
   * @note Thread safe for concurrent reads.
   * @tparam T Message type satisfying `MessageTrait`
   * @return Span of message ids aligned with `CurrentMessages<T>()`
   */
  template <MessageTrait T>
  [[nodiscard]] auto CurrentIds() const noexcept
      -> std::span<const AnyMessageId> {
    return IdsFor(current_ids_, MessageTypeIndex::From<T>());
  }

  /**
   * @brief Gets the next message id that will be assigned for type `T`.
   * @note Thread safe for concurrent reads.
   * @tparam T Message type satisfying `MessageTrait`
   * @return Next id value
   */
  template <MessageTrait T>
  [[nodiscard]] auto MessageCount() const noexcept -> MessageId<T>;

  /**
   * @brief Gets the oldest retained message id for type `T`.
   * @details Equal to `MessageCount<T>()` when no messages are retained.
   * @note Thread safe for concurrent reads.
   * @tparam T Message type satisfying `MessageTrait`
   * @return Oldest retained id, or the next id when empty
   */
  template <MessageTrait T>
  [[nodiscard]] auto OldestMessageCount() const noexcept -> MessageId<T>;

  /**
   * @brief Counts retained messages with id >= `last_message_count`.
   * @note Thread safe for concurrent reads.
   * @tparam T Message type satisfying `MessageTrait`
   * @param last_message_count Cursor position
   * @return Unread retained count
   */
  template <MessageTrait T>
  [[nodiscard]] size_type UnreadCount(
      MessageId<T> last_message_count) const noexcept;

  /**
   * @brief Gets metadata for a registered message type.
   * @note Thread safe for concurrent reads.
   * @tparam T Message type satisfying `AnyMessageTrait`
   * @return Pointer to metadata, or nullptr if not registered
   */
  template <AnyMessageTrait T>
  [[nodiscard]] const MessageMetadata* Metadata() const noexcept {
    return registered_messages_.TryGet(MessageTypeIndex::From<T>());
  }

  /**
   * @brief Gets the number of registered message types (regular + async).
   * @note Thread safe for concurrent reads.
   * @return Count of registered messages
   */
  [[nodiscard]] size_type RegisteredMessageCount() const noexcept {
    return registered_messages_.Size();
  }

  /**
   * @brief Gets const reference to current message queue.
   * @note Thread-safe.
   * @return Const reference to current queue
   */
  [[nodiscard]] const MessageQueue<>& CurrentQueue() const noexcept {
    return current_messages_;
  }

  /**
   * @brief Gets mutable reference to current message queue.
   * @note Not thread-safe. Prefer `Write` / `MergeLocalMessages` so ids stay
   * consistent.
   * @return Mutable reference to current queue
   */
  [[nodiscard]] MessageQueue<>& CurrentQueue() noexcept {
    return current_messages_;
  }

  /**
   * @brief Gets const reference to previous message queue.
   * @note Thread-safe.
   * @return Const reference to previous queue
   */
  [[nodiscard]] const MessageQueue<>& PreviousQueue() const noexcept {
    return previous_messages_;
  }

  /**
   * @brief Gets mutable reference to previous message queue.
   * @note Not thread-safe.
   * @return Mutable reference to previous queue
   */
  [[nodiscard]] MessageQueue<>& PreviousQueue() noexcept {
    return previous_messages_;
  }

  /**
   * @brief Gets the async message queue (for creating tokens, etc.).
   * @note Thread-safe.
   * @return Reference to the async message queue
   */
  [[nodiscard]] AsyncMessageQueue& AsyncQueue() noexcept {
    return async_messages_;
  }

  /**
   * @brief Gets the async message queue (const).
   * @note Thread-safe.
   * @return Const reference to the async message queue
   */
  [[nodiscard]] const AsyncMessageQueue& AsyncQueue() const noexcept {
    return async_messages_;
  }

private:
  using RegisteredMessages = container::MultiTypeMap<MessageMetadata>;
  using MessageIdList = std::vector<AnyMessageId>;
  using MessageIdMap = container::MultiTypeMap<MessageIdList>;
  using MessageCountMap = container::MultiTypeMap<size_type>;

  void AssignIds(MessageTypeIndex type_index, size_type count);
  void ClearIds(MessageTypeIndex type_index) noexcept;
  void ClearAllIds() noexcept;
  void AgeIds();
  void RemoveIds(MessageTypeIndex type_index, MessageIdList& ids,
                 std::span<const size_type> sorted_indices);

  [[nodiscard]] auto IdsFor(const MessageIdMap& map,
                            MessageTypeIndex type_index) const noexcept
      -> std::span<const AnyMessageId>;

  RegisteredMessages
      registered_messages_;  ///< Metadata for all registered message types

  MessageQueue<>
      current_messages_;  ///< Storage for messages written in the current frame
  MessageQueue<>
      previous_messages_;      ///< Storage for messages from the previous frame
  MessageIdMap current_ids_;   ///< Ids aligned with `current_messages_`
  MessageIdMap previous_ids_;  ///< Ids aligned with `previous_messages_`
  MessageCountMap
      message_counts_;  ///< Next id to assign per regular message type
  AsyncMessageQueue async_messages_;  ///< Storage for async messages
                                      ///< (lock-free, not double-buffered)
};

template <typename Alloc>
inline void MessageManager::Update(
    const ConsumedMessagesRegistry<Alloc>& consumed_registry) {
  HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::MessageManager::Update");

  if (!consumed_registry.Empty()) {
    ApplyConsumed(consumed_registry);
  }

  previous_messages_.Merge(current_messages_);
  AgeIds();
  current_messages_.ClearAll();
}

template <typename Alloc>
inline void MessageManager::ApplyConsumed(
    const ConsumedMessagesRegistry<Alloc>& merged_consumed) {
  HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::MessageManager::ApplyConsumed");
  HELIOS_ECS_PROFILE_ZONE_VALUE(merged_consumed.TotalConsumedCount());

  for (const auto& [type_index, consumed_ids] : merged_consumed.Data()) {
    if (consumed_ids.empty()) {
      continue;
    }

    const auto* metadata = registered_messages_.TryGet(type_index);
    if (metadata == nullptr || metadata->is_async) {
      continue;
    }

    auto* prev_ids = previous_ids_.TryGet(type_index);
    auto* curr_ids = current_ids_.TryGet(type_index);
    if ((prev_ids == nullptr || prev_ids->empty()) &&
        (curr_ids == nullptr || curr_ids->empty())) {
      continue;
    }

    std::vector<size_type> prev_indices;
    std::vector<size_type> curr_indices;
    prev_indices.reserve(consumed_ids.size());
    curr_indices.reserve(consumed_ids.size());

    for (const AnyMessageId id : consumed_ids) {
      if (prev_ids != nullptr) {
        const auto it = std::ranges::lower_bound(*prev_ids, id);
        if (it != prev_ids->end() && *it == id) {
          prev_indices.push_back(
              static_cast<size_type>(it - prev_ids->begin()));
          continue;
        }
      }
      if (curr_ids != nullptr) {
        const auto it = std::ranges::lower_bound(*curr_ids, id);
        if (it != curr_ids->end() && *it == id) {
          curr_indices.push_back(
              static_cast<size_type>(it - curr_ids->begin()));
        }
      }
    }

    if (!prev_indices.empty()) {
      previous_messages_.RemoveIndices(type_index, prev_indices);
      RemoveIds(type_index, *prev_ids, prev_indices);
    }
    if (!curr_indices.empty()) {
      current_messages_.RemoveIndices(type_index, curr_indices);
      RemoveIds(type_index, *curr_ids, curr_indices);
    }
  }
}

template <AnyMessageTrait... Ts>
  requires(sizeof...(Ts) > 0)
inline void MessageManager::Register() {
  [[maybe_unused]] constexpr std::array type_indices = {
      MessageTypeIndex::From<Ts>()...};
  [[maybe_unused]] constexpr std::array names = {MessageNameOf<Ts>()...};

#ifdef HELIOS_ENABLE_ASSERTS
  std::string already_registered;
  for (size_t i = 0; i < sizeof...(Ts); ++i) {
    if (!registered_messages_.Contains(type_indices[i])) {
      continue;
    }

    if (!already_registered.empty()) {
      already_registered.append(", ");
    }
    already_registered.append(names[i]);
  }

  HELIOS_ASSERT(already_registered.empty(),
                "Message type(s) '{}' already registered!", already_registered);
#endif

  (
      [this]<typename T>() {
        registered_messages_.template Emplace<T>(
            MessageMetadata{.type_index = MessageTypeIndex::From<T>(),
                            .name = MessageNameOf<T>(),
                            .clear_policy = MessageClearPolicyOf<T>(),
                            .is_async = AsyncMessageTrait<T>});

        if constexpr (AsyncMessageTrait<T>) {
          async_messages_.template Register<T>();
        } else if constexpr (MessageTrait<T>) {
          current_messages_.template Register<T>();
          previous_messages_.template Register<T>();
          current_ids_.template Ensure<T>();
          previous_ids_.template Ensure<T>();
          message_counts_.template Ensure<T>() = 0;
        }
      }.template operator()<Ts>(),
      ...);
}

template <MessageTrait T>
inline auto MessageManager::Write(T&& message)
    -> MessageId<std::remove_cvref_t<T>> {
  using DecayedT = std::remove_cvref_t<T>;
  HELIOS_ASSERT(
      registered_messages_.Contains(MessageTypeIndex::From<DecayedT>()),
      "Message type '{}' is not registered!", MessageNameOf<DecayedT>());

  auto& next_id = message_counts_.template Get<DecayedT>();
  const MessageId<DecayedT> id{next_id++};
  current_messages_.Enqueue(std::forward<T>(message));
  current_ids_.template Get<DecayedT>().push_back(AnyMessageId::From(id));
  return id;
}

template <AsyncMessageTrait T>
inline void MessageManager::WriteAsync(T&& message) {
  using DecayedT = std::remove_cvref_t<T>;
  HELIOS_ASSERT(
      registered_messages_.Contains(MessageTypeIndex::From<DecayedT>()),
      "Message type '{}' is not registered!", MessageNameOf<DecayedT>());
  async_messages_.Enqueue<DecayedT>(std::forward<T>(message));
}

template <std::ranges::input_range R>
  requires MessageTrait<std::ranges::range_value_t<R>>
inline void MessageManager::WriteBulk(R&& messages) {
  using T = std::ranges::range_value_t<R>;
  HELIOS_ASSERT(registered_messages_.Contains(MessageTypeIndex::From<T>()),
                "Message type '{}' is not registered!", MessageNameOf<T>());

  const auto before = current_messages_.template MessageCount<T>();
  current_messages_.EnqueueBulk(std::forward<R>(messages));
  const auto after = current_messages_.template MessageCount<T>();
  AssignIds(MessageTypeIndex::From<T>(), after - before);
}

template <std::ranges::input_range R>
  requires AsyncMessageTrait<std::ranges::range_value_t<R>>
inline void MessageManager::WriteAsyncBulk(R&& messages) {
  using T [[maybe_unused]] = std::ranges::range_value_t<R>;
  HELIOS_ASSERT(registered_messages_.Contains(MessageTypeIndex::From<T>()),
                "Message type '{}' is not registered!", MessageNameOf<T>());
  async_messages_.EnqueueBulk(std::forward<R>(messages));
}

template <MessageTrait T>
inline void MessageManager::ManualClear() {
  constexpr auto type_index = MessageTypeIndex::From<T>();
  HELIOS_ASSERT(registered_messages_.Contains(type_index),
                "Message type '{}' is not registered!", MessageNameOf<T>());
  current_messages_.template Clear<T>();
  previous_messages_.template Clear<T>();
  ClearIds(type_index);
}

template <AsyncMessageTrait T>
inline void MessageManager::ManualAsyncClear() {
  constexpr auto type_index = MessageTypeIndex::From<T>();
  HELIOS_ASSERT(registered_messages_.Contains(type_index),
                "Message type '{}' is not registered!", MessageNameOf<T>());
  async_messages_.template Clear<T>();
}

template <typename OtherAllocator>
inline void MessageManager::MergeLocalMessages(
    const MessageQueue<OtherAllocator>& local) {
  for (const auto& [type_index, metadata] : registered_messages_) {
    if (metadata.is_async) {
      continue;
    }
    AssignIds(type_index, local.MessageCount(type_index));
  }
  current_messages_.Merge(local);
}

template <typename OtherAllocator>
inline void MessageManager::MergeLocalMessages(
    MessageQueue<OtherAllocator>&& local) {
  for (const auto& [type_index, metadata] : registered_messages_) {
    if (metadata.is_async) {
      continue;
    }
    AssignIds(type_index, local.MessageCount(type_index));
  }
  current_messages_.Merge(std::move(local));
}

template <MessageTrait T>
inline bool MessageManager::HasMessages() const noexcept {
  if (!registered_messages_.Contains(MessageTypeIndex::From<T>()))
      [[unlikely]] {
    return false;
  }
  return current_messages_.template HasMessages<T>() ||
         previous_messages_.template HasMessages<T>();
}

template <AsyncMessageTrait T>
inline bool MessageManager::HasAsyncMessages() const noexcept {
  if (!registered_messages_.Contains(MessageTypeIndex::From<T>()))
      [[unlikely]] {
    return false;
  }
  return async_messages_.template HasMessages<T>();
}

template <MessageTrait T>
inline auto MessageManager::MessageCount() const noexcept -> MessageId<T> {
  const auto* count = message_counts_.template TryGet<T>();
  return MessageId<T>{count != nullptr ? *count : 0};
}

template <MessageTrait T>
inline auto MessageManager::OldestMessageCount() const noexcept
    -> MessageId<T> {
  const auto prev = PreviousIds<T>();
  if (!prev.empty()) {
    return MessageId<T>::From(prev.front());
  }
  const auto curr = CurrentIds<T>();
  if (!curr.empty()) {
    return MessageId<T>::From(curr.front());
  }
  return MessageCount<T>();
}

template <MessageTrait T>
inline auto MessageManager::UnreadCount(
    MessageId<T> last_message_count) const noexcept -> size_type {
  const AnyMessageId cursor_pos = last_message_count.ToAny();
  const auto count_from = [cursor_pos](std::span<const AnyMessageId> ids) {
    const auto it = std::ranges::lower_bound(ids, cursor_pos);
    return static_cast<size_type>(ids.end() - it);
  };
  return count_from(PreviousIds<T>()) + count_from(CurrentIds<T>());
}

}  // namespace helios::ecs
