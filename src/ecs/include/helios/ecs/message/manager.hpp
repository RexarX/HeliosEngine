#pragma once

#include <helios/assert.hpp>
#include <helios/compiler/compiler.hpp>
#include <helios/container/multi_type_map.hpp>
#include <helios/ecs/details/profile.hpp>
#include <helios/ecs/message/async_queue.hpp>
#include <helios/ecs/message/consumed_registry.hpp>
#include <helios/ecs/message/message.hpp>
#include <helios/ecs/message/queue.hpp>

#include <array>
#include <cstddef>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
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
 * - Explicit registration: messages must be registered before use
 * - Automatic clearing: messages are cleared after their lifecycle expires
 * - Manual control: users can opt-out of auto-clearing
 * - Consumed message removal: at Update time, consumed messages are physically
 * removed from queues without additional heap allocation
 *
 * Message Lifecycle:
 * - Frame N: Messages written to current queue
 * - Frame N+1: Messages readable from previous queue (after swap)
 * - Frame N+2: Messages cleared from previous queue (automatic policy)
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
   * @brief Updates message lifecycle — applies consumed messages, swaps
   * buffers, clears old messages.
   * @details Performs the following steps:
   * 1. Removes consumed messages from previous and current queues.
   * 2. Clears automatic messages from previous queue.
   * 3. Merges current queue into previous queue.
   * 4. Prepares a fresh current queue for the next frame.
   * @note Not thread-safe.
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
   * @brief Applies consumed indices to both queues, removing consumed messages
   * in-place.
   * @details For each type with consumed entries, indices [0, prev_count)
   * target previous messages and indices [prev_count, prev_count + curr_count)
   * target current messages (offset by prev_count). No buffer swap occurs.
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
   */
  template <MessageTrait T>
  void Write(T&& message);

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
   * @details Used to flush a per-system write buffer into the global message
   * state. Typically called after a system finishes execution.
   * @tparam OtherAllocator Allocator type used by the local `MessageQueue`
   * @param local Local message queue to merge from (will be left in a valid but
   * empty state)
   */
  template <typename OtherAllocator>
  void MergeLocalMessages(const MessageQueue<OtherAllocator>& local) {
    current_messages_.Merge(local);
  }

  /**
   * @brief Merges messages from a local `MessageQueue` into the current queue.
   * @note Not thread-safe.
   * @details Rvalue overload that consumes the local queue.
   * @tparam OtherAllocator Allocator type used by the local `MessageQueue`
   * @param local Local message queue to merge from (will be left in a valid but
   * empty state)
   */
  template <typename OtherAllocator>
  void MergeLocalMessages(MessageQueue<OtherAllocator>&& local) {
    current_messages_.Merge(std::move(local));
  }

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
   * @note Thread safe for concurrent reads.
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
   * @note Thread safe for concurrent reads.
   * @tparam T Message type satisfying `MessageTrait`
   * @return Span of const messages
   */
  template <MessageTrait T>
  [[nodiscard]] auto CurrentMessages() const noexcept -> std::span<const T> {
    return current_messages_.Messages<T>();
  }

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
   * @note Not thread-safe.
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

  RegisteredMessages
      registered_messages_;  ///< Metadata for all registered message types

  MessageQueue<>
      current_messages_;  ///< Storage for messages written in the current frame
  MessageQueue<>
      previous_messages_;  ///< Storage for messages from the previous frame
  AsyncMessageQueue async_messages_;  ///< Storage for async messages
                                      ///< (lock-free, not double-buffered)
};

inline void MessageManager::Clear() noexcept {
  registered_messages_.ResetAll();
  current_messages_.ResetAll();
  previous_messages_.ResetAll();
  async_messages_.Reset();
}

inline void MessageManager::ClearAllQueues() noexcept {
  current_messages_.ClearAll();
  previous_messages_.ClearAll();
  async_messages_.Clear();
}

template <typename Alloc>
inline void MessageManager::Update(
    const ConsumedMessagesRegistry<Alloc>& consumed_registry) {
  HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::MessageManager::Update");

  // Remove consumed messages from both queues.
  if (!consumed_registry.Empty()) {
    ApplyConsumed(consumed_registry);
  }

  // Merge current queue into previous queue.
  // Previous queue now holds surviving previous messages (manual policy,
  // non-consumed) plus all current messages.
  previous_messages_.Merge(std::move(current_messages_));

  // Re-register types in current_messages_ so it's ready for new
  // writes.
  for (const auto& [type_index, metadata] : registered_messages_) {
    if (!metadata.is_async) {
      current_messages_.Register(type_index);
    }
  }
}

inline void MessageManager::Update() {
  HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::MessageManager::Update");

  // No consumed messages to process; clear aged automatic messages from
  // previous, then merge current into previous.
  for (const auto& [type_index, metadata] : registered_messages_) {
    if (!metadata.is_async &&
        metadata.clear_policy == MessageClearPolicy::kAutomatic) {
      previous_messages_.Clear(type_index);
    }
  }

  previous_messages_.Merge(std::move(current_messages_));

  for (const auto& [type_index, metadata] : registered_messages_) {
    if (!metadata.is_async) {
      current_messages_.Register(type_index);
    }
  }
}

template <typename Alloc>
inline void MessageManager::ApplyConsumed(
    const ConsumedMessagesRegistry<Alloc>& merged_consumed) {
  HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::MessageManager::ApplyConsumed");
  HELIOS_ECS_PROFILE_ZONE_VALUE(merged_consumed.TotalConsumedCount());

  for (const auto& [type_index, consumed_indices] : merged_consumed.Data()) {
    if (consumed_indices.empty()) {
      continue;
    }

    // Skip types that aren't registered as regular messages.
    const auto* metadata = registered_messages_.TryGet(type_index);
    if (metadata == nullptr || metadata->is_async) {
      continue;
    }

    const auto prev_count = previous_messages_.MessageCount(type_index);
    const auto curr_count = current_messages_.MessageCount(type_index);

    if (prev_count == 0 && curr_count == 0) {
      continue;
    }

    // Split consumed indices into those targeting previous vs current
    // messages. Global indices [0, prev_count) -> previous queue Global
    // indices [prev_count, prev_count + curr_count) -> current queue (offset
    // by prev_count)

    // Find the partition point between previous and current indices.
    const auto partition_it =
        std::ranges::lower_bound(consumed_indices, prev_count);

    // Apply to previous queue: indices are already direct indices into the
    // previous buffer.
    if (partition_it != consumed_indices.begin()) {
      auto prev_indices = std::span<const size_type>(
          consumed_indices.data(),
          static_cast<size_type>(partition_it - consumed_indices.begin()));
      previous_messages_.RemoveIndices(type_index, prev_indices);
    }

    // Apply to current queue: offset indices by subtracting prev_count.
    if (partition_it != consumed_indices.end()) {
      // Build offset indices in a small local buffer. Since we're in the
      // single-threaded Update path and consumed counts are typically small,
      // use a stack-friendly approach. We reuse the tail of consumed_indices
      // by creating offset copies.
      const auto curr_consumed_count = static_cast<size_type>(
          consumed_indices.data() + consumed_indices.size() - &(*partition_it));

      // To avoid heap allocation, we compute offset indices into a local
      // small buffer. If the count is small enough, use the stack; otherwise
      // fall back to a vector.
      constexpr size_type kStackThreshold = 64;
      if (curr_consumed_count <= kStackThreshold) {
        std::array<size_type, kStackThreshold> local_buf = {};
        for (size_type i = 0; i < curr_consumed_count; ++i) {
          local_buf[i] =
              *(partition_it + static_cast<ptrdiff_t>(i)) - prev_count;
        }
        current_messages_.RemoveIndices(
            type_index,
            std::span<const size_type>{local_buf.data(), curr_consumed_count});
      } else {
        // Many consumed messages - heap allocate.
        std::vector<size_type> offset_indices;
        offset_indices.reserve(curr_consumed_count);
        for (auto it = partition_it; it != consumed_indices.end(); ++it) {
          offset_indices.push_back(*it - prev_count);
        }
        current_messages_.RemoveIndices(type_index, offset_indices);
      }
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
        registered_messages_.Emplace<T>(
            MessageMetadata{.type_index = MessageTypeIndex::From<T>(),
                            .name = MessageNameOf<T>(),
                            .clear_policy = MessageClearPolicyOf<T>(),
                            .is_async = AsyncMessageTrait<T>});

        if constexpr (AsyncMessageTrait<T>) {
          async_messages_.Register<T>();
        } else if constexpr (MessageTrait<T>) {
          current_messages_.Register<T>();
          previous_messages_.Register<T>();
        }
      }.template operator()<Ts>(),
      ...);
}

template <MessageTrait T>
inline void MessageManager::Write(T&& message) {
  using DecayedT [[maybe_unused]] = std::remove_cvref_t<T>;
  HELIOS_ASSERT(
      registered_messages_.Contains(MessageTypeIndex::From<DecayedT>()),
      "Message type '{}' is not registered!", MessageNameOf<DecayedT>());
  current_messages_.Enqueue(std::forward<T>(message));
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
  using T [[maybe_unused]] = std::ranges::range_value_t<R>;
  HELIOS_ASSERT(registered_messages_.Contains(MessageTypeIndex::From<T>()),
                "Message type '{}' is not registered!", MessageNameOf<T>());
  current_messages_.EnqueueBulk(std::forward<R>(messages));
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
  current_messages_.Clear<T>();
  previous_messages_.Clear<T>();
}

template <AsyncMessageTrait T>
inline void MessageManager::ManualAsyncClear() {
  constexpr auto type_index = MessageTypeIndex::From<T>();
  HELIOS_ASSERT(registered_messages_.Contains(type_index),
                "Message type '{}' is not registered!", MessageNameOf<T>());
  async_messages_.Clear<T>();
}

template <MessageTrait T>
inline bool MessageManager::HasMessages() const noexcept {
  if (!registered_messages_.Contains(MessageTypeIndex::From<T>()))
      [[unlikely]] {
    return false;
  }
  return current_messages_.HasMessages<T>() ||
         previous_messages_.HasMessages<T>();
}

template <AsyncMessageTrait T>
inline bool MessageManager::HasAsyncMessages() const noexcept {
  if (!registered_messages_.Contains(MessageTypeIndex::From<T>()))
      [[unlikely]] {
    return false;
  }
  return async_messages_.HasMessages<T>();
}

}  // namespace helios::ecs
