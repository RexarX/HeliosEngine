#pragma once

#include <helios/ecs/message/message.hpp>
#include <helios/ecs/message/queue.hpp>

#include <concepts>
#include <functional>
#include <memory_resource>
#include <ranges>
#include <utility>

namespace helios::ecs {

/**
 * @brief Type-safe writer for regular messages with a configurable queue
 * allocator.
 * @details Messages written through `BasicMessageWriter` are buffered in a
 * message queue and merged into the global `MessageManager` at sync time when
 * used from system local data.
 * @note Not thread-safe.
 * @tparam T Message type satisfying `MessageTrait`
 * @tparam Allocator Allocator type for the underlying message queue (default:
 * `std::allocator<std::byte>`)
 */
template <MessageTrait T, typename Allocator = std::allocator<std::byte>>
class BasicMessageWriter {
public:
  /**
   * @brief Constructs a `BasicMessageWriter` that writes to a message queue.
   * @param queue Reference to the message queue
   */
  explicit constexpr BasicMessageWriter(MessageQueue<Allocator>& queue) noexcept
      : queue_(queue) {}
  BasicMessageWriter(const BasicMessageWriter&) = delete;
  constexpr BasicMessageWriter(BasicMessageWriter&&) noexcept = default;
  constexpr ~BasicMessageWriter() noexcept = default;

  BasicMessageWriter& operator=(const BasicMessageWriter&) = delete;
  constexpr BasicMessageWriter& operator=(BasicMessageWriter&&) noexcept =
      default;

  /**
   * @brief Writes a single message to the local queue.
   * @tparam U Message type, must be the same as `T`
   * @param message Message to write
   */
  template <typename U = T>
    requires std::same_as<std::remove_cvref_t<U>, T>
  void Write(U&& message) const {
    queue_.get().Enqueue(std::forward<U>(message));
  }

  /**
   * @brief Writes multiple messages to the local queue in bulk.
   * @tparam R Range of messages
   * @param messages Range of messages to write
   */
  template <std::ranges::input_range R>
    requires std::same_as<std::ranges::range_value_t<R>, T>
  void WriteBulk(R&& messages) const {
    queue_.get().EnqueueBulk(std::forward<R>(messages));
  }

  /**
   * @brief Constructs an message in-place and writes it to the local queue.
   * @tparam Args Constructor argument types
   * @param args Arguments to forward to the message constructor
   */
  template <typename... Args>
    requires std::constructible_from<T, Args...>
  void Emplace(Args&&... args) const {
    queue_.get().Enqueue(T{std::forward<Args>(args)...});
  }

private:
  std::reference_wrapper<MessageQueue<Allocator>>
      queue_;  ///< Reference to the message queue
};

template <MessageTrait T>
using PmrBasicMessageWriter =
    BasicMessageWriter<T, std::pmr::polymorphic_allocator<>>;

/**
 * @brief Alias for `BasicMessageWriter` bound to a PMR allocator.
 * @details The canonical message writer type for system parameters. Users who
 * need a different allocator can use `BasicMessageWriter` directly.
 * @tparam T Message type satisfying `MessageTrait`
 *
 * @code
 * void MySystem(MessageWriter<DeathEvent> writer) {
 *   writer.Write(DeathEvent{...});
 * }
 * @endcode
 */
template <MessageTrait T>
using MessageWriter = PmrBasicMessageWriter<T>;

}  // namespace helios::ecs
