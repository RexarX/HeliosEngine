#pragma once

#include <helios/assert.hpp>
#include <helios/container/multi_type_map.hpp>
#include <helios/container/typed_buffer_array.hpp>
#include <helios/ecs/message/message.hpp>

#include <algorithm>
#include <memory_resource>
#include <ranges>
#include <span>
#include <type_traits>

namespace helios::ecs {

/**
 * @brief Queue for managing multiple message types using type-erased contiguous
 * storage.
 * @details Messages are stored in a `MultiTypeBuffer`, which provides per-type
 * contiguous storage.
 * @note Not thread-safe.
 * @tparam Allocator Allocator type for internal storage (default:
 * `std::allocator<std::byte>`)
 */
template <typename Allocator = std::allocator<std::byte>>
class MessageQueue {
private:
  using MessageStorage =
      container::MultiTypeMap<container::TypedBufferArray<Allocator>,
                              Allocator>;

public:
  using size_type = MessageStorage::size_type;
  using allocator_type = MessageStorage::allocator_type;

  constexpr MessageQueue() = default;

  /**
   * @brief Constructs an `MessageQueue` with a custom allocator.
   * @param alloc Allocator instance
   */
  explicit constexpr MessageQueue(const allocator_type& alloc)
      : messages_(alloc) {}

  /**
   * @brief Constructs a `MessageQueue` from a PMR memory resource.
   * @details Enabled only when `allocator_type` is constructible from
   * `std::pmr::memory_resource*`.
   * @param resource Memory resource used to construct allocator
   */
  explicit constexpr MessageQueue(std::pmr::memory_resource* resource)
    requires std::constructible_from<allocator_type, std::pmr::memory_resource*>
      : MessageQueue(allocator_type{resource}) {}

  MessageQueue(std::nullptr_t) = delete;

  constexpr MessageQueue(const MessageQueue&) = default;
  constexpr MessageQueue(MessageQueue&&) noexcept = default;
  constexpr ~MessageQueue() = default;

  constexpr MessageQueue& operator=(const MessageQueue&) = default;
  constexpr MessageQueue& operator=(MessageQueue&&) noexcept = default;

  /**
   * @brief Registers an message type with the queue.
   * @tparam T Message type
   */
  template <MessageTrait T>
  constexpr void Register() {
    messages_.template Ensure<T>();
  }

  /**
   * @brief Ensures storage exists for the given runtime type index.
   * @details Used when the concrete type is not available at call-site (e.g.,
   * during Update re-registration). If the type index is already registered,
   * this is a no-op.
   * @param type_index Runtime type index
   */
  constexpr void Register(MessageTypeIndex type_index) {
    messages_.Ensure(type_index);
  }

  /// @brief Clears all messages from the queue maintaining all registered
  /// types.
  constexpr void ClearAll() noexcept { messages_.ClearAll(); }

  /**
   * @brief Clears messages of a specific type.
   * @tparam T Message type
   */
  template <MessageTrait T>
  constexpr void Clear() noexcept {
    messages_.template Clear<T>();
  }

  /**
   * @brief Clears messages of a specific type by runtime type index.
   * @param type_index Type index of messages to clear
   */
  constexpr void Clear(MessageTypeIndex type_index) noexcept {
    messages_.Clear(type_index);
  }

  /// @brief Resets the queue by clearing all messages and unregistering all
  /// types.
  constexpr void ResetAll() noexcept { messages_.ResetAll(); }

  /**
   * @brief Resets a specific message type by clearing its messages and
   * unregistering the type.
   * @tparam T Message type
   */
  template <MessageTrait T>
  constexpr void Reset() noexcept {
    messages_.template Reset<T>();
  }

  /**
   * @brief Merges messages from another MessageQueue into this one.
   * @details For each type in `other`, messages are appended to the
   * corresponding storage in this queue. After merging, `other` is left in a
   * valid but empty state.
   * @tparam OtherAllocator Allocator template of the other queue
   * @param other MessageQueue to merge from
   */
  template <typename OtherAllocator>
  constexpr void Merge(const MessageQueue<OtherAllocator>& other) {
    messages_.Merge(other.messages_);
  }

  /**
   * @brief Merges messages from another MessageQueue into this one.
   * @details Rvalue overload that consumes the source queue.
   * @tparam OtherAllocator Allocator template of the other queue
   * @param other MessageQueue to merge from
   */
  template <typename OtherAllocator>
  constexpr void Merge(MessageQueue<OtherAllocator>&& other) {
    messages_.Merge(std::move(other.messages_));
  }

  /**
   * @brief Enqueues an message into the queue.
   * @warning Triggers assertion if message type is not registered.
   * @tparam T Message type
   * @param message Message to enqueue
   */
  template <MessageTrait T>
  void Enqueue(T&& message);

  /**
   * @brief Enqueues multiple messages in bulk.
   * @warning Triggers assertion if message type is not registered.
   * @tparam R Range type
   * @param messages Range of messages to enqueue
   */
  template <std::ranges::input_range R>
    requires MessageTrait<std::ranges::range_value_t<R>>
  void EnqueueBulk(R&& messages);

  /**
   * @brief Removes messages at the given sorted indices for a specific message
   * type.
   * @warning Triggers assertion if message type is not registered.
   * @tparam T Message type
   * @param sorted_indices Span of sorted, unique global indices to remove
   */
  template <MessageTrait T>
  void RemoveIndices(std::span<const size_type> sorted_indices) {
    RemoveIndices(MessageTypeIndex::From<T>(), sorted_indices);
  }

  /**
   * @brief Removes messages at the given sorted indices for a specific type.
   * @details Erases messages at the provided sorted, deduplicated indices from
   * back to front to avoid index invalidation. No additional allocation is
   * performed.
   * @warning Triggers assertion if type_index is not registered.
   * @param type_index Type index of the message type
   * @param sorted_indices Span of sorted, unique global indices to remove
   */
  void RemoveIndices(MessageTypeIndex type_index,
                     std::span<const size_type> sorted_indices);

  /**
   * @brief Swaps the contents of this queue with another.
   * @param other Another MessageQueue to swap with
   */
  constexpr void Swap(MessageQueue& other) noexcept {
    messages_.Swap(other.messages_);
  }

  /**
   * @brief Swaps the contents of two message queues.
   * @param lhs First MessageQueue
   * @param rhs Second MessageQueue
   */
  friend constexpr void swap(MessageQueue& lhs, MessageQueue& rhs) noexcept {
    lhs.Swap(rhs);
  }

  /**
   * @brief Checks if an message type is registered.
   * @tparam T Message type
   * @return True if the message type is registered, false otherwise
   */
  template <MessageTrait T>
  [[nodiscard]] constexpr bool IsRegistered() const noexcept {
    return messages_.template Contains<T>();
  }

  /**
   * @brief Checks if an message type is registered by runtime type index.
   * @param type_index Type index to check
   * @return True if the message type is registered, false otherwise
   */
  [[nodiscard]] constexpr bool IsRegistered(
      MessageTypeIndex type_index) const noexcept {
    return messages_.Contains(type_index);
  }

  /**
   * @brief Checks if any messages exist in the queue across all types.
   * @return True if at least one message exists, false otherwise
   */
  [[nodiscard]] constexpr bool HasMessages() const noexcept {
    return !messages_.EmptyAll();
  }

  /**
   * @brief Checks if messages of a specific type exist in the queue.
   * @tparam T Message type
   * @return True if messages of type T exist, false otherwise or if the type is
   * not registered
   */
  template <MessageTrait T>
  [[nodiscard]] constexpr bool HasMessages() const noexcept {
    return !messages_.template Empty<T>();
  }

  /**
   * @brief Gets a const span of all messages of a specific type.
   * @tparam T Message type
   * @return Span of const messages, or empty span if type is not registered or
   * has no messages
   */
  template <MessageTrait T>
  [[nodiscard]] constexpr auto Messages() const noexcept -> std::span<const T> {
    const auto* ptr = messages_.template TryGet<T>();
    return ptr ? ptr->template Data<T>() : std::span<const T>{};
  }

  /**
   * @brief Gets a mutable span of all messages of a specific type.
   * @tparam T Message type
   * @return Span of messages, or empty span if type is not registered or has no
   * messages
   */
  template <MessageTrait T>
  [[nodiscard]] constexpr auto Messages() noexcept -> std::span<T> {
    auto* ptr = messages_.template TryGet<T>();
    return ptr ? ptr->template Data<T>() : std::span<T>{};
  }

  /**
   * @brief Gets the number of message types stored.
   * @return Number of distinct message types
   */
  [[nodiscard]] constexpr size_type TypeCount() const noexcept {
    return messages_.TypeCount();
  }

  /**
   * @brief Gets the total number of messages stored across all types.
   * @return Total message count or 0 if no types are registered
   */
  [[nodiscard]] constexpr size_type MessageCount() const noexcept {
    return messages_.Size();
  }

  /**
   * @brief Gets the total number of messages stored for a specific message
   * type.
   * @tparam T Message type
   * @return Total message count or 0 if the type is not registered
   */
  template <MessageTrait T>
  [[nodiscard]] constexpr size_type MessageCount() const noexcept {
    return messages_.template Size<T>();
  }

  /**
   * @brief Gets the number of messages for a type by runtime type index.
   * @param type_index Type index to query
   * @return Number of messages, or 0 if the type is not registered
   */
  [[nodiscard]] constexpr size_type MessageCount(
      MessageTypeIndex type_index) const noexcept {
    return messages_.Size(type_index);
  }

private:
  template <typename OtherAllocator>
  friend class MessageQueue;

  MessageStorage messages_;  ///< Storage for messages of different types
};

template <typename Allocator>
template <MessageTrait T>
inline void MessageQueue<Allocator>::Enqueue(T&& message) {
  HELIOS_ASSERT(IsRegistered<std::remove_cvref_t<T>>(),
                "Message type '{}' is not registered!",
                MessageNameOf<std::remove_cvref_t<T>>());
  messages_.template Get<std::remove_cvref_t<T>>()
      .template EmplaceBack<std::remove_cvref_t<T>>(std::forward<T>(message));
}

template <typename Allocator>
template <std::ranges::input_range R>
  requires MessageTrait<std::ranges::range_value_t<R>>
inline void MessageQueue<Allocator>::EnqueueBulk(R&& messages) {
  using T = std::ranges::range_value_t<R>;
  HELIOS_ASSERT(IsRegistered<T>(), "Message type '{}' is not registered!",
                MessageNameOf<T>());
  messages_.template Get<T>().AppendRange(std::forward<R>(messages));
}

template <typename Allocator>
inline void MessageQueue<Allocator>::RemoveIndices(
    MessageTypeIndex type_index, std::span<const size_type> sorted_indices) {
  if (sorted_indices.empty()) [[likely]] {
    return;
  }

  HELIOS_ASSERT(IsRegistered(type_index),
                "Message type index is not registered!");

  auto& buffer = messages_.Get(type_index);
  if (buffer.Empty()) [[unlikely]] {
    return;
  }

  // Erase contiguous ranges from back to front to preserve preceding indices.
  auto idx = sorted_indices.size();
  while (idx > 0) {
    --idx;
    auto range_end = sorted_indices[idx] + 1;
    auto range_start = sorted_indices[idx];

    // Coalesce consecutive indices into a single range.
    while (idx > 0 && sorted_indices[idx - 1] == range_start - 1) {
      --idx;
      range_start = sorted_indices[idx];
    }

    buffer.Erase(range_start, range_end);
  }
}

using PmrMessageQueue =
    MessageQueue<std::pmr::polymorphic_allocator<std::byte>>;

}  // namespace helios::ecs
