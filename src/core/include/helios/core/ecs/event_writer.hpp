#pragma once

#include <helios/core/ecs/details/event_manager.hpp>
#include <helios/core/ecs/event.hpp>

#include <concepts>
#include <ranges>

namespace helios::ecs {

/**
 * @brief Type-safe writer for events.
 * @details Provides a clean, ergonomic API for writing events to the event system.
 * EventWriter is a lightweight wrapper around EventManager that enforces type safety
 * and provides convenient methods for bulk operations and in-place construction.
 * @note EventWriter holds a non-const reference to EventManager and is not thread-safe.
 * EventWriter is intended to be short-lived (function-scoped).
 * @tparam T Event type
 *
 * @code
 * auto writer = world.WriteEvents<MyEvent>();
 * writer.Write(MyEvent{42});
 * writer.Emplace(42);  // Construct in-place
 * writer.WriteBulk(event_span);
 * @endcode
 */
template <EventTrait T>
class EventWriter {
public:
  /**
   * @brief Constructs an EventWriter.
   * @param manager Reference to the event manager
   */
  explicit constexpr EventWriter(details::EventManager& manager) noexcept : manager_(manager) {}
  EventWriter(const EventWriter&) = delete;
  constexpr EventWriter(EventWriter&&) noexcept = default;
  constexpr ~EventWriter() noexcept = default;

  EventWriter& operator=(const EventWriter&) = delete;
  constexpr EventWriter& operator=(EventWriter&&) noexcept = default;

  /**
   * @brief Writes a single event (copy).
   * @details Copies the event into the current event queue.
   * @param event Event to write
   */
  void Write(const T& event) { manager_.Write(event); }

  /**
   * @brief Writes multiple events to the queue in bulk.
   * @tparam R Range of events
   * @param events Range of events to store
   */
  template <std::ranges::sized_range R>
    requires std::same_as<std::ranges::range_value_t<R>, T>
  void WriteBulk(const R& events) {
    manager_.WriteBulk(events);
  }

  /**
   * @brief Emplaces an event in-place.
   * @details Constructs the event directly in the event queue,
   * avoiding unnecessary copies or moves.
   * @tparam Args Constructor argument types
   * @param args Arguments to forward to event constructor
   */
  template <typename... Args>
    requires std::constructible_from<T, Args...>
  void Emplace(Args&&... args) {
    manager_.Write(T{std::forward<Args>(args)...});
  }

private:
  details::EventManager& manager_;  ///< Reference to the event manager
};

}  // namespace helios::ecs
