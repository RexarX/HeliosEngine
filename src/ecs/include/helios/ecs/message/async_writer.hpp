#pragma once

#include <helios/ecs/message/async_queue.hpp>
#include <helios/ecs/message/manager.hpp>
#include <helios/ecs/message/message.hpp>

#include <concepts>
#include <ranges>
#include <utility>

namespace helios::ecs {

/**
 * @brief Type-safe writer for async messages.
 * @details Async messages use a lock-free concurrent queue, making writes
 * thread-safe. This is suitable for messages produced from multiple threads
 * concurrently (e.g., I/O callbacks, background workers). `AsyncMessageWriter`
 * is intended to be short-lived (function-scoped).
 * @note Thread-safe.
 * @tparam T Async message type satisfying `AsyncMessageTrait`
 */
template <AsyncMessageTrait T>
class AsyncMessageWriter {
public:
  /**
   * @brief Constructs an `AsyncMessageWriter` from the message manager.
   * @param manager Mutable reference to the message manager
   */
  explicit constexpr AsyncMessageWriter(MessageManager& manager) noexcept
      : AsyncMessageWriter(manager.AsyncQueue()) {}

  /**
   * @brief Constructs an `AsyncMessageWriter` that writes to the shared async
   * message queue.
   * @param async_queue Reference to the async message queue
   */
  explicit AsyncMessageWriter(AsyncMessageQueue& async_queue)
      : async_queue_(async_queue), token_(async_queue.MakeProducerToken<T>()) {}
  AsyncMessageWriter(const AsyncMessageWriter&) = delete;
  AsyncMessageWriter(AsyncMessageWriter&&) noexcept = default;
  ~AsyncMessageWriter() noexcept = default;

  AsyncMessageWriter& operator=(const AsyncMessageWriter&) = delete;
  AsyncMessageWriter& operator=(AsyncMessageWriter&&) noexcept = default;

  /**
   * @brief Writes a single message to the async queue (move).
   * @param message Message to write
   */
  void Write(T&& message) {
    async_queue_.get().Enqueue<T>(token_, std::move(message));
  }

  /**
   * @brief Writes a single message to the async queue (copy).
   * @param message Message to write
   */
  void Write(const T& message)
    requires std::copy_constructible<T>
  {
    async_queue_.get().Enqueue<T>(token_, T{message});
  }

  /**
   * @brief Writes multiple messages to the async queue in bulk.
   * @tparam R Range of messages
   * @param messages Range of messages to write
   */
  template <std::ranges::input_range R>
    requires std::same_as<std::ranges::range_value_t<R>, T>
  void WriteBulk(R&& messages) {
    async_queue_.get().EnqueueBulk(token_, std::forward<R>(messages));
  }

  /**
   * @brief Constructs an message in-place and writes it to the async queue.
   * @tparam Args Constructor argument types
   * @param args Arguments to forward to the message constructor
   */
  template <typename... Args>
    requires std::constructible_from<T, Args...>
  void Emplace(Args&&... args) {
    async_queue_.get().Enqueue<T>(token_, T{std::forward<Args>(args)...});
  }

private:
  std::reference_wrapper<AsyncMessageQueue>
      async_queue_;  ///< Reference to the shared async message queue
  AsyncMessageQueueProducerToken
      token_;  ///< Per-writer producer token for improved throughput
};

}  // namespace helios::ecs
