#pragma once

#include <helios/ecs/message/async_queue.hpp>
#include <helios/ecs/message/async_wrapper.hpp>
#include <helios/ecs/message/manager.hpp>
#include <helios/ecs/message/message.hpp>

#include <functional>
#include <iterator>
#include <limits>
#include <utility>

namespace helios::ecs {

/**
 * @brief Type-safe, move-based reader for async messages.
 * @details Drains all async messages of type `T` from the shared
 * `AsyncMessageQueue` into a local buffer on construction. After draining, the
 * messages are owned by the reader and can be iterated over, moved out, or
 * queried.
 * @note Thread-safe.
 * @tparam T Async message type satisfying `AsyncMessageTrait`
 *
 * @code
 * for (auto& message : reader) {
 *   Process(std::move(message));
 * }
 *
 * // Or move all messages out:
 * auto messages = reader.TakeAll();
 * @endcode
 */
template <AsyncMessageTrait T>
class AsyncMessageReader {
public:
  using size_type = AsyncMessageQueue::size_type;

  /**
   * @brief Constructs an `AsyncMessageReader` from the message manager.
   * @param manager Reference to the message manager
   */
  explicit AsyncMessageReader(MessageManager& manager) noexcept
      : AsyncMessageReader(manager.AsyncQueue()) {}

  /**
   * @brief Constructs an `AsyncMessageReader` by draining messages from the
   * async queue.
   * @warning Triggers assertion if type 'T' is not registered.
   * @param async_queue Reference to the async message queue
   */
  explicit AsyncMessageReader(AsyncMessageQueue& async_queue)
      : messages_(async_queue.TypedStorage<T>()),
        token_(messages_.get().MakeConsumerToken()) {}
  AsyncMessageReader(const AsyncMessageReader&) = delete;
  AsyncMessageReader(AsyncMessageReader&&) noexcept = default;
  ~AsyncMessageReader() noexcept = default;

  AsyncMessageReader& operator=(const AsyncMessageReader&) = delete;
  AsyncMessageReader& operator=(AsyncMessageReader&&) noexcept = default;

  /**
   * @brief Dequeues a single message from the queue.
   * @return Dequeued message or default-constructed `T` if the queue is empty
   */
  T Dequeue() const { return messages_.get().Dequeue(token_); }

  /**
   * @brief Dequeues a single message from the queue.
   * @param dest Reference to store the dequeued message
   * @return True if an message was dequeued and stored in dest, false if the
   * queue was empty (`dest` is unchanged)
   */
  bool Dequeue(T& dest) const { return messages_.get().Dequeue(token_, dest); }

  /**
   * @brief Moves messages into an output iterator (dequeues them).
   * @tparam It Output iterator type
   * @param out Output iterator to receive messages
   * @param max_count Maximum number of messages to move (default: all messages)
   * @return Number of messages actually dequeued
   */
  template <std::output_iterator<T> It>
  size_type Into(It out, size_type max_count =
                             std::numeric_limits<size_type>::max()) const {
    return messages_.get().Into(token_, std::move(out), max_count);
  }

  /**
   * @brief Applies an action to every message in the queue (dequeues them).
   * @note It is possible that the queue is modified by other writers while this
   * method is executing, so the number of messages processed may be less than
   * the total number of messages at the time of calling.
   * @tparam Action Callable type `(const T&) -> void`
   * @param action Action to apply
   */
  template <typename Action>
    requires std::invocable<Action, T&>
  void ForEach(const Action& action) const;

  /**
   * @brief Checks if there are no messages.
   * @return True if the reader holds no messages, false otherwise
   */
  [[nodiscard]] bool Empty() const noexcept { return messages_.get().Empty(); }

  /**
   * @brief Returns an approximate count of messages.
   * @return Message count
   */
  [[nodiscard]] size_type CountApprox() const noexcept {
    return messages_.get().SizeApprox();
  }

private:
  std::reference_wrapper<TypedAsyncMessageStorage<T>>
      messages_;  ///< Message storage
  mutable AsyncMessageQueueConsumerToken
      token_;  ///< Per-reader consumer token for improved throughput
};

template <AsyncMessageTrait T>
template <typename Action>
  requires std::invocable<Action, T&>
inline void AsyncMessageReader<T>::ForEach(const Action& action) const {
  T temp;
  while (Dequeue(temp)) {
    action(temp);
  }
}

}  // namespace helios::ecs
