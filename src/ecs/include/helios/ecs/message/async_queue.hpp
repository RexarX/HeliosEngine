#pragma once

#include <helios/assert.hpp>
#include <helios/compiler/compiler.hpp>
#include <helios/ecs/message/message.hpp>

#include <concurrentqueue/moodycamel/concurrentqueue.h>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <limits>
#include <memory>
#include <ranges>
#include <utility>

#ifdef HELIOS_STL_FLAT_MAP_AVAILABLE
#include <flat_map>
#else
#include <boost/container/flat_map.hpp>
#endif

namespace helios::ecs {

namespace details {

/// @brief Base class for type-erased async message storage.
class AsyncMessageStorage {
public:
  virtual ~AsyncMessageStorage() = default;

  /// @brief Clears all messages from the queue.
  virtual void Clear() = 0;

  /**
   * @brief Checks if the queue is empty.
   * @return True if no messages are stored, false otherwise
   */
  [[nodiscard]] virtual bool Empty() const noexcept = 0;

  /**
   * @brief Gets approximate number of messages stored in the queue.
   * @return Message count
   */
  [[nodiscard]] virtual size_t SizeApprox() const noexcept = 0;
};

}  // namespace details

using AsyncMessageQueueProducerToken = moodycamel::ProducerToken;
using AsyncMessageQueueConsumerToken = moodycamel::ConsumerToken;

/**
 * @brief Typed async message storage backed by a lock-free concurrent queue.
 * @note Thread-safe for concurrent enqueue/dequeue operations.
 * @tparam T Async message type
 */
template <AsyncMessageTrait T>
class TypedAsyncMessageStorage final : public details::AsyncMessageStorage {
public:
  TypedAsyncMessageStorage() = default;
  TypedAsyncMessageStorage(const TypedAsyncMessageStorage&) = delete;
  TypedAsyncMessageStorage(TypedAsyncMessageStorage&&) noexcept = default;
  ~TypedAsyncMessageStorage() override = default;

  TypedAsyncMessageStorage& operator=(const TypedAsyncMessageStorage&) = delete;
  TypedAsyncMessageStorage& operator=(TypedAsyncMessageStorage&&) noexcept =
      default;

  /// @brief Clears all messages from the queue by draining it.
  void Clear() override;

  /**
   * @brief Enqueues a single message into the queue.
   * @param message Message to enqueue
   */
  void Enqueue(const T& message)
    requires std::copy_constructible<T>
  {
    queue_.enqueue(T{message});
  }

  /**
   * @brief Enqueues a single message into the queue.
   * @param message Message to enqueue
   */
  void Enqueue(T&& message) { queue_.enqueue(std::move(message)); }

  /**
   * @brief Enqueues a single message into the queue using a producer token.
   * @param token Producer token for improved throughput
   * @param message Message to enqueue
   */
  void Enqueue(AsyncMessageQueueProducerToken& token, const T& message)
    requires std::copy_constructible<T>
  {
    queue_.enqueue(token, T{message});
  }

  /**
   * @brief Enqueues a single message into the queue using a producer token.
   * @param token Producer token for improved throughput
   * @param message Message to enqueue
   */
  void Enqueue(AsyncMessageQueueProducerToken& token, T&& message) {
    queue_.enqueue(token, std::move(message));
  }

  /**
   * @brief Enqueues multiple messages in bulk.
   * @tparam R Range type
   * @param messages Range of messages to enqueue
   */
  template <std::ranges::input_range R>
    requires std::same_as<std::ranges::range_value_t<R>, T>
  void EnqueueBulk(R&& messages);

  /**
   * @brief Enqueues multiple messages in bulk using a producer token.
   * @tparam R Range type
   * @param token Producer token for improved throughput
   * @param messages Range of messages to enqueue
   */
  template <std::ranges::input_range R>
    requires std::same_as<std::ranges::range_value_t<R>, T>
  void EnqueueBulk(AsyncMessageQueueProducerToken& token, R&& messages);

  /**
   * @brief Dequeues a single message from the queue.
   * @return Dequeued message or default-constructed `T` if the queue is empty
   */
  T Dequeue();

  /**
   * @brief Dequeues a single message from the queue.
   * @param token Consumer token for improved throughput
   * @return Dequeued message or default-constructed `T` if the queue is empty
   */
  T Dequeue(AsyncMessageQueueConsumerToken& token);

  /**
   * @brief Dequeues a single message from the queue.
   * @param dest Reference to store the dequeued message
   * @return True if an message was dequeued and stored in dest, false if the
   * queue was empty (`dest` is unchanged)
   */
  bool Dequeue(T& dest) { return queue_.try_dequeue(dest); }

  /**
   * @brief Dequeues a single message from the queue.
   * @param token Consumer token for improved throughput
   * @param dest Reference to store the dequeued message
   * @return True if an message was dequeued and stored in dest, false if the
   * queue was empty (`dest` is unchanged)
   */
  bool Dequeue(AsyncMessageQueueConsumerToken& token, T& dest) {
    return queue_.try_dequeue(token, dest);
  }

  /**
   * @brief Moves messages into an output iterator (dequeues them).
   * @tparam It Output iterator type
   * @param out Output iterator to receive messages
   * @param max_count Maximum number of messages to move (default: all messages)
   * @return Number of messages actually dequeued
   */
  template <typename It>
    requires std::output_iterator<It, T>
  size_t Into(It out, size_t max_count = std::numeric_limits<size_t>::max());

  /**
   * @brief Moves messages into an output iterator using a consumer token.
   * @tparam It Output iterator type
   * @param token Consumer token for improved throughput
   * @param out Output iterator to receive messages
   * @param max_count Maximum number of messages to move (default: all messages)
   * @return Number of messages actually dequeued
   */
  template <typename It>
    requires std::output_iterator<It, T>
  size_t Into(AsyncMessageQueueConsumerToken& token, It out,
              size_t max_count = std::numeric_limits<size_t>::max());

  /**
   * @brief Gets the producer token for this queue.
   * @return Producer token
   */
  [[nodiscard]] AsyncMessageQueueProducerToken MakeProducerToken() {
    return AsyncMessageQueueProducerToken(queue_);
  }

  /**
   * @brief Gets the consumer token for this queue.
   * @return Consumer token
   */
  [[nodiscard]] AsyncMessageQueueConsumerToken MakeConsumerToken() {
    return AsyncMessageQueueConsumerToken(queue_);
  }

  /**
   * @brief Checks if the queue is empty.
   * @return True if no messages are stored, false otherwise
   */
  [[nodiscard]] bool Empty() const noexcept override {
    return queue_.size_approx() == 0;
  }

  /**
   * @brief Gets approximate number of messages stored in the queue.
   * @return Message count
   */
  [[nodiscard]] size_t SizeApprox() const noexcept override {
    return queue_.size_approx();
  }

private:
  moodycamel::ConcurrentQueue<T> queue_;
};

template <AsyncMessageTrait T>
inline void TypedAsyncMessageStorage<T>::Clear() {
  T temp;
  while (queue_.try_dequeue(temp)) {
    // Discard messages
  }
}

template <AsyncMessageTrait T>
template <std::ranges::input_range R>
  requires std::same_as<std::ranges::range_value_t<R>, T>
inline void TypedAsyncMessageStorage<T>::EnqueueBulk(R&& messages) {
  size_t count = 0;
  if constexpr (std::ranges::sized_range<R>) {
    count = std::ranges::size(messages);
  } else {
    count = static_cast<size_t>(std::ranges::distance(messages));
  }

  if constexpr (std::ranges::contiguous_range<R>) {
    queue_.enqueue_bulk(std::ranges::data(messages), count);
  } else if constexpr (std::ranges::borrowed_range<R>) {
    queue_.enqueue_bulk(std::ranges::begin(messages), count);
  } else {
    queue_.enqueue_bulk(std::make_move_iterator(std::ranges::begin(messages)),
                        count);
  }
}

template <AsyncMessageTrait T>
template <std::ranges::input_range R>
  requires std::same_as<std::ranges::range_value_t<R>, T>
inline void TypedAsyncMessageStorage<T>::EnqueueBulk(
    AsyncMessageQueueProducerToken& token, R&& messages) {
  size_t count = 0;
  if constexpr (std::ranges::sized_range<R>) {
    count = std::ranges::size(messages);
  } else {
    count = static_cast<size_t>(std::ranges::distance(messages));
  }

  if constexpr (std::ranges::contiguous_range<R>) {
    queue_.enqueue_bulk(token, std::ranges::data(messages), count);
  } else if constexpr (std::ranges::borrowed_range<R>) {
    queue_.enqueue_bulk(token, std::ranges::begin(messages), count);
  } else {
    queue_.enqueue_bulk(
        token, std::make_move_iterator(std::ranges::begin(messages)), count);
  }
}

template <AsyncMessageTrait T>
inline T TypedAsyncMessageStorage<T>::Dequeue() {
  T result;
  queue_.try_dequeue(result);
  return result;
}

template <AsyncMessageTrait T>
inline T TypedAsyncMessageStorage<T>::Dequeue(
    AsyncMessageQueueConsumerToken& token) {
  T result;
  queue_.try_dequeue(token, result);
  return result;
}

template <AsyncMessageTrait T>
template <typename It>
  requires std::output_iterator<It, T>
inline size_t TypedAsyncMessageStorage<T>::Into(It out, size_t max_count) {
  size_t dequeued = 0;
  T temp;
  while (dequeued < max_count && queue_.try_dequeue(temp)) {
    *out = std::move(temp);
    ++out;
    ++dequeued;
  }
  return dequeued;
}

template <AsyncMessageTrait T>
template <typename It>
  requires std::output_iterator<It, T>
inline size_t TypedAsyncMessageStorage<T>::Into(
    AsyncMessageQueueConsumerToken& token, It out, size_t max_count) {
  size_t dequeued = 0;
  T temp;
  while (dequeued < max_count && queue_.try_dequeue(token, temp)) {
    *out = std::move(temp);
    ++out;
    ++dequeued;
  }
  return dequeued;
}

/**
 * @brief Async queue for managing multiple message types via lock-free
 * concurrent queues.
 * @details Each registered async message type gets its own
 * `TypedAsyncMessageStorage` backed by a `moodycamel::ConcurrentQueue`.
 * @note Thread-safe.
 */
class AsyncMessageQueue {
public:
  using size_type = size_t;

  AsyncMessageQueue() = default;
  AsyncMessageQueue(const AsyncMessageQueue&) = delete;
  AsyncMessageQueue(AsyncMessageQueue&&) noexcept = default;
  ~AsyncMessageQueue() = default;

  AsyncMessageQueue& operator=(const AsyncMessageQueue&) = delete;
  AsyncMessageQueue& operator=(AsyncMessageQueue&&) noexcept = default;

  /**
   * @brief Registers an async message type with the queue.
   * @tparam T Async message type
   */
  template <AsyncMessageTrait T>
  void Register();

  /// @brief Clears all messages from the queue maintaining all registered
  /// types.
  void Clear();

  /**
   * @brief Clears messages of a specific type.
   * @tparam T Async message type
   */
  template <AsyncMessageTrait T>
  void Clear();

  /// @brief Resets the queue by clearing all messages and unregistering all
  /// message types.
  void Reset() noexcept { messages_.clear(); }

  /**
   * @brief Resets the queue by clearing and unregistering a specific message
   * type.
   * @tparam T Async message type
   */
  template <AsyncMessageTrait T>
  void Reset() noexcept {
    messages_.erase(MessageTypeIndex::From<T>());
  }

  /**
   * @brief Enqueues a single message into the queue.
   * @warning Triggers assertion if message type is not registered.
   * @tparam T Async message type
   * @param message Message to enqueue
   */
  template <AsyncMessageTrait T>
  void Enqueue(T&& message) {
    TypedStorage<T>().Enqueue(std::forward<T>(message));
  }

  /**
   * @brief Enqueues a single message using a producer token for improved
   * throughput.
   * @warning Triggers assertion if message type is not registered.
   * @tparam T Async message type
   * @param token Producer token
   * @param message Message to enqueue
   */
  template <AsyncMessageTrait T>
  void Enqueue(AsyncMessageQueueProducerToken& token, T&& message) {
    TypedStorage<T>().Enqueue(token, std::forward<T>(message));
  }

  /**
   * @brief Enqueues multiple messages in bulk.
   * @warning Triggers assertion if message type is not registered.
   * @tparam R Range type whose value_type satisfies `AsyncMessageTrait`
   * @param messages Range of messages to enqueue
   */
  template <std::ranges::input_range R>
    requires AsyncMessageTrait<std::ranges::range_value_t<R>>
  void EnqueueBulk(R&& messages);

  /**
   * @brief Enqueues multiple messages in bulk using a producer token.
   * @warning Triggers assertion if message type is not registered.
   * @tparam R Range type whose value_type satisfies `AsyncMessageTrait`
   * @param token Producer token
   * @param messages Range of messages to enqueue
   */
  template <std::ranges::input_range R>
    requires AsyncMessageTrait<std::ranges::range_value_t<R>>
  void EnqueueBulk(AsyncMessageQueueProducerToken& token, R&& messages);

  /**
   * @brief Creates a producer token for a specific message type.
   * @warning Triggers assertion if message type is not registered.
   * @tparam T Async message type
   * @return Producer token
   */
  template <AsyncMessageTrait T>
  [[nodiscard]] AsyncMessageQueueProducerToken MakeProducerToken() {
    return TypedStorage<T>().MakeProducerToken();
  }

  /**
   * @brief Creates a consumer token for a specific message type.
   * @warning Triggers assertion if message type is not registered.
   * @tparam T Async message type
   * @return Consumer token
   */
  template <AsyncMessageTrait T>
  [[nodiscard]] AsyncMessageQueueConsumerToken MakeConsumerToken() {
    return TypedStorage<T>().MakeConsumerToken();
  }

  /**
   * @brief Swaps the contents of this queue with another.
   * @param other Another AsyncMessageQueue to swap with
   */
  void Swap(AsyncMessageQueue& other) noexcept {
    std::swap(messages_, other.messages_);
  }

  /**
   * @brief Swaps the contents of two AsyncMessageQueues.
   * @param lhs First AsyncMessageQueue
   * @param rhs Second AsyncMessageQueue
   */
  friend void swap(AsyncMessageQueue& lhs, AsyncMessageQueue& rhs) noexcept {
    lhs.Swap(rhs);
  }

  /**
   * @brief Checks if an message type is registered.
   * @tparam T Async message type
   * @return True if the message type is registered, false otherwise
   */
  template <AsyncMessageTrait T>
  [[nodiscard]] bool IsRegistered() const noexcept {
    return messages_.contains(MessageTypeIndex::From<T>());
  }

  /**
   * @brief Checks if any registered type has messages.
   * @return True if at least one message exists across all types, false
   * otherwise
   */
  [[nodiscard]] bool HasMessages() const noexcept {
    return std::ranges::any_of(
        messages_, [](const auto& pair) { return !pair.second->Empty(); });
  }

  /**
   * @brief Checks if messages of a specific type exist in the queue.
   * @tparam T Async message type
   * @return True if messages of type T exist, false otherwise or if the type is
   * not registered
   */
  template <AsyncMessageTrait T>
  [[nodiscard]] bool HasMessages() const noexcept;

  /**
   * @brief Gets the number of registered message types.
   * @return Number of distinct message types
   */
  [[nodiscard]] size_type TypeCount() const noexcept {
    return messages_.size();
  }

  /**
   * @brief Gets the approximate total number of messages across all types.
   * @return Approximate total message count
   */
  [[nodiscard]] size_type MessageCount() const noexcept;

  /**
   * @brief Gets the approximate number of messages for a specific type.
   * @tparam T Async message type
   * @return Approximate message count or 0 if the type is not registered
   */
  template <AsyncMessageTrait T>
  [[nodiscard]] size_type MessageCount() const noexcept;

  /**
   * @brief Gets the typed storage for a specific message type.
   * @warning Triggers assertion if type is not registered.
   * @tparam T Async message type
   * @return Reference to the typed storage
   */
  template <AsyncMessageTrait T>
  [[nodiscard]] auto TypedStorage() noexcept -> TypedAsyncMessageStorage<T>&;

  /**
   * @brief Gets the typed storage for a specific message type (const).
   * @warning Triggers assertion if type is not registered.
   * @tparam T Async message type
   * @return Const reference to the typed storage
   */
  template <AsyncMessageTrait T>
  [[nodiscard]] auto TypedStorage() const noexcept
      -> const TypedAsyncMessageStorage<T>&;

private:
#ifdef HELIOS_STL_FLAT_MAP_AVAILABLE
  using MessageStorage =
      std::flat_map<MessageTypeIndex,
                    std::unique_ptr<details::AsyncMessageStorage>>;
#else
  using MessageStorage =
      boost::container::flat_map<MessageTypeIndex,
                                 std::unique_ptr<details::AsyncMessageStorage>>;
#endif

  MessageStorage messages_;  ///< Storage for async messages of different types
};

template <AsyncMessageTrait T>
inline void AsyncMessageQueue::Register() {
  if (!IsRegistered<T>()) [[likely]] {
    messages_.emplace(MessageTypeIndex::From<T>(),
                      std::make_unique<TypedAsyncMessageStorage<T>>());
  }
}

inline void AsyncMessageQueue::Clear() {
  for (auto&& [_, storage] : messages_) {
    storage->Clear();
  }
}

template <AsyncMessageTrait T>
inline void AsyncMessageQueue::Clear() {
  const auto it = messages_.find(MessageTypeIndex::From<T>());
  if (it == messages_.end()) [[unlikely]] {
    return;
  }
  static_cast<TypedAsyncMessageStorage<T>&>(*it->second).Clear();
}

template <std::ranges::input_range R>
  requires AsyncMessageTrait<std::ranges::range_value_t<R>>
inline void AsyncMessageQueue::EnqueueBulk(R&& messages) {
  using T = std::ranges::range_value_t<R>;
  HELIOS_ASSERT(IsRegistered<T>(), "Async message type '{}' is not registered!",
                MessageNameOf<T>());
  TypedStorage<T>().EnqueueBulk(std::forward<R>(messages));
}

template <std::ranges::input_range R>
  requires AsyncMessageTrait<std::ranges::range_value_t<R>>
inline void AsyncMessageQueue::EnqueueBulk(
    AsyncMessageQueueProducerToken& token, R&& messages) {
  using T = std::ranges::range_value_t<R>;
  HELIOS_ASSERT(IsRegistered<T>(), "Async message type '{}' is not registered!",
                MessageNameOf<T>());
  TypedStorage<T>().EnqueueBulk(token, std::forward<R>(messages));
}

template <AsyncMessageTrait T>
inline bool AsyncMessageQueue::HasMessages() const noexcept {
  const auto it = messages_.find(MessageTypeIndex::From<T>());
  if (it == messages_.end()) [[unlikely]] {
    return false;
  }
  return !static_cast<const TypedAsyncMessageStorage<T>&>(*it->second).Empty();
}

inline auto AsyncMessageQueue::MessageCount() const noexcept -> size_type {
  size_type total = 0;
  for (const auto& [_, storage] : messages_) {
    total += storage->SizeApprox();
  }
  return total;
}

template <AsyncMessageTrait T>
inline auto AsyncMessageQueue::MessageCount() const noexcept -> size_type {
  const auto it = messages_.find(MessageTypeIndex::From<T>());
  if (it == messages_.end()) [[unlikely]] {
    return 0;
  }
  return static_cast<const TypedAsyncMessageStorage<T>&>(*it->second)
      .SizeApprox();
}

template <AsyncMessageTrait T>
inline auto AsyncMessageQueue::TypedStorage() noexcept
    -> TypedAsyncMessageStorage<T>& {
  HELIOS_ASSERT(IsRegistered<T>(), "Async message type '{}' is not registered!",
                MessageNameOf<T>());
  return static_cast<TypedAsyncMessageStorage<T>&>(
      *messages_.at(MessageTypeIndex::From<T>()));
}

template <AsyncMessageTrait T>
inline auto AsyncMessageQueue::TypedStorage() const noexcept
    -> const TypedAsyncMessageStorage<T>& {
  HELIOS_ASSERT(IsRegistered<T>(), "Async message type '{}' is not registered!",
                MessageNameOf<T>());
  return static_cast<const TypedAsyncMessageStorage<T>&>(
      *messages_.at(MessageTypeIndex::From<T>()));
}

}  // namespace helios::ecs
