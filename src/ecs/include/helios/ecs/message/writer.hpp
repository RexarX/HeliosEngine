#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.ecs;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <concepts>
#include <functional>
#include <ranges>
#include <type_traits>
#include <utility>
#endif
#include <helios/ecs/message/id.hpp>
#include <helios/ecs/message/manager.hpp>
#include <helios/ecs/message/message.hpp>
#include <helios/ecs/message/queue.hpp>

HELIOS_MODULE_EXPORT
namespace helios::ecs {

/**
 * @brief Type-safe writer for regular messages.
 * @details Messages written through `BasicMessageWriter` are buffered in a
 * message queue and merged into the global `MessageManager` at sync time when
 * used from system local data. Ids are assigned during that merge.
 * @note Not thread-safe.
 * @tparam T Message type satisfying `MessageTrait`
 */
template <MessageTrait T>
class BasicMessageWriter {
public:
  /**
   * @brief Constructs a `BasicMessageWriter` that writes to a message queue.
   * @param queue Reference to the message queue
   */
  explicit constexpr BasicMessageWriter(MessageQueue& queue) noexcept
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
  constexpr void Write(U&& message) const {
    queue_.get().Enqueue(std::forward<U>(message));
  }

  /**
   * @brief Writes multiple messages to the local queue in bulk.
   * @tparam R Range of messages
   * @param messages Range of messages to write
   */
  template <std::ranges::input_range R>
    requires std::same_as<std::ranges::range_value_t<R>, T>
  constexpr void WriteBulk(R&& messages) const {
    queue_.get().EnqueueBulk(std::forward<R>(messages));
  }

  /**
   * @brief Constructs an message in-place and writes it to the local queue.
   * @tparam Args Constructor argument types
   * @param args Arguments to forward to the message constructor
   */
  template <typename... Args>
    requires std::constructible_from<T, Args...>
  constexpr void Emplace(Args&&... args) const {
    queue_.get().Enqueue(T{std::forward<Args>(args)...});
  }

private:
  std::reference_wrapper<MessageQueue> queue_;  ///< Reference to message queue
};

/**
 * @brief Writer that inserts messages directly into a `MessageManager`.
 * @details Assigns stable message ids immediately. Used by
 * `World::WriteMessages`.
 * @note Not thread-safe.
 * @tparam T Message type satisfying `MessageTrait`
 */
template <MessageTrait T>
class ManagedMessageWriter {
public:
  /**
   * @brief Constructs a writer bound to a message manager.
   * @param manager Message manager that owns the global queues
   */
  explicit constexpr ManagedMessageWriter(MessageManager& manager) noexcept
      : manager_(manager) {}

  ManagedMessageWriter(const ManagedMessageWriter&) = delete;
  constexpr ManagedMessageWriter(ManagedMessageWriter&&) noexcept = default;
  constexpr ~ManagedMessageWriter() noexcept = default;

  ManagedMessageWriter& operator=(const ManagedMessageWriter&) = delete;
  constexpr ManagedMessageWriter& operator=(ManagedMessageWriter&&) noexcept =
      default;

  /**
   * @brief Writes a single message into the manager's current queue.
   * @tparam U Message type, must be the same as `T`
   * @param message Message to write
   * @return Assigned message id
   */
  template <typename U = T>
    requires std::same_as<std::remove_cvref_t<U>, T>
  constexpr auto Write(U&& message) const -> MessageId<T> {
    return manager_.get().Write(std::forward<U>(message));
  }

  /**
   * @brief Writes multiple messages into the manager's current queue.
   * @tparam R Range of messages
   * @param messages Range of messages to write
   */
  template <std::ranges::input_range R>
    requires std::same_as<std::ranges::range_value_t<R>, T>
  constexpr void WriteBulk(R&& messages) const {
    manager_.get().WriteBulk(std::forward<R>(messages));
  }

  /**
   * @brief Constructs a message in-place and writes it into the manager.
   * @tparam Args Constructor argument types
   * @param args Arguments to forward to the message constructor
   * @return Assigned message id
   */
  template <typename... Args>
    requires std::constructible_from<T, Args...>
  constexpr auto Emplace(Args&&... args) const -> MessageId<T> {
    return manager_.get().Write(T{std::forward<Args>(args)...});
  }

private:
  std::reference_wrapper<MessageManager> manager_;
};

/**
 * @brief Canonical message writer type for system parameters.
 * @tparam T Message type satisfying `MessageTrait`
 *
 * @code
 * void MySystem(MessageWriter<DeathEvent> writer) {
 *   writer.Write(DeathEvent{...});
 * }
 * @endcode
 */
template <MessageTrait T>
using MessageWriter = BasicMessageWriter<T>;

}  // namespace helios::ecs
#endif  // HELIOS_MODULE_CONSUMER_SHIM
