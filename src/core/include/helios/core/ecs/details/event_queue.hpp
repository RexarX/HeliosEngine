#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/assert.hpp>
#include <helios/core/ecs/details/event_storage.hpp>
#include <helios/core/ecs/event.hpp>
#include <helios/core/logger.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <span>
#include <unordered_map>
#include <utility>

namespace helios::ecs::details {

/**
 * @brief Queue for managing multiple event types.
 * @details Uses an unordered_map to store EventStorage for each event type,
 * allowing efficient type-based event management.
 * Events can be written and read in batches for better performance.
 * @note Not thread-safe.
 */
class EventQueue {
public:
  EventQueue() = default;
  EventQueue(const EventQueue&) = delete;
  EventQueue(EventQueue&&) noexcept = default;
  ~EventQueue() = default;

  EventQueue& operator=(const EventQueue&) = delete;
  EventQueue& operator=(EventQueue&&) noexcept = default;

  /**
   * @brief Registers an event type with the queue.
   * @tparam T Event type
   */
  template <EventTrait T>
  void Register();

  /**
   * @brief Clears all events from the queue and removes registrations.
   */
  void Clear() noexcept { storages_.clear(); }

  /**
   * @brief Clears all event data but preserves registrations.
   * @details Use this when you want to clear events without losing type registrations.
   */
  void ClearData() noexcept;

  /**
   * @brief Clears events of a specific type.
   * @warning Triggers assertion if event type is not registered.
   * @tparam T Event type
   */
  template <EventTrait T>
  void Clear();

  /**
   * @brief Clears events of a specific type by type ID (runtime).
   * @warning Triggers assertion if event type does not exist.
   * @param type_id Type identifier of events to clear
   */
  void ClearByTypeId(EventTypeId type_id);

  /**
   * @brief Merges events from another EventQueue into this one.
   * @param other EventQueue to merge from
   */
  void Merge(EventQueue& other);

  /**
   * @brief Writes a single event to the queue.
   * @warning Triggers assertion if event type is not registered.
   * @tparam T Event type
   * @param event Event to store
   */
  template <EventTrait T>
  void Write(const T& event);

  /**
   * @brief Writes multiple events to the queue in bulk.
   * @warning Triggers assertion if event type is not registered.
   * @tparam R Range of events
   * @param events Range of events to store
   */
  template <std::ranges::sized_range R>
    requires EventTrait<std::ranges::range_value_t<R>>
  void WriteBulk(const R& events);

  /**
   * @brief Reads all events of a specific type from the queue.
   * @warning Span gets invalidated if storage is modified.
   * @tparam T Event type
   * @return Span containing all events of type T
   */
  template <EventTrait T>
  [[nodiscard]] auto Read() const -> std::span<const T>;

  /**
   * @brief Reads events of a specific type into a provided output iterator.
   * @warning Triggers assertion if event type is not registered.
   * @tparam T Event type
   * @tparam OutIt Output iterator type
   * @param out Output iterator to write events into
   */
  template <EventTrait T, typename OutIt>
    requires std::output_iterator<OutIt, T>
  void ReadInto(OutIt out) const;

  /**
   * @brief Checks if an event type is registered.
   * @tparam T Event type
   * @return True if the event type is registered, false otherwise
   */
  template <EventTrait T>
  [[nodiscard]] bool IsRegistered() const;

  /**
   * @brief Checks if events of a specific type exist in the queue.
   * @tparam T Event type
   * @return True if events of type T exist, false otherwise
   */
  template <EventTrait T>
  [[nodiscard]] bool HasEvents() const;

  /**
   * @brief Checks if the queue is empty.
   * @return True if no events are stored, false otherwise
   */
  [[nodiscard]] bool Empty() const noexcept {
    return std::ranges::all_of(storages_, [](const auto& pair) { return pair.second.Empty(); });
  }

  /**
   * @brief Gets the number of event types stored.
   * @return Number of distinct event types
   */
  [[nodiscard]] size_t TypeCount() const noexcept { return storages_.size(); }

  /**
   * @brief Gets the total size of all stored events in bytes.
   * @return Total size in bytes
   */
  [[nodiscard]] size_t TotalSizeBytes() const noexcept;

private:
  std::unordered_map<EventTypeId, EventStorage> storages_;
};

template <EventTrait T>
inline void EventQueue::Register() {
  constexpr EventTypeId type_id = EventTypeIdOf<T>();
  if (!storages_.contains(type_id)) {
    storages_.emplace(type_id, EventStorage::FromEvent<T>());
  }
}

template <EventTrait T>
inline void EventQueue::Clear() {
  constexpr EventTypeId type_id = EventTypeIdOf<T>();
  HELIOS_ASSERT(IsRegistered<T>(), "Failed to clear events: Event type '{}' is not registered!", EventNameOf<T>());
  storages_.at(type_id).Clear();
}

inline void EventQueue::ClearData() noexcept {
  for (auto& [_, storage] : storages_) {
    storage.Clear();
  }
}

inline void EventQueue::ClearByTypeId(EventTypeId type_id) {
  if (const auto it = storages_.find(type_id); it != storages_.end()) {
    it->second.Clear();
  }
}

inline void EventQueue::Merge(EventQueue& other) {
  for (auto& [type_id, other_storage] : other.storages_) {
    if (other_storage.Empty()) {
      continue;
    }

    if (const auto it = storages_.find(type_id); it == storages_.end()) {
      // If we don't have this type registered, move the entire storage
      storages_.emplace(type_id, std::move(other_storage));
    } else {
      auto& this_storage = it->second;
      if (this_storage.Empty()) {
        // If our storage is empty, just move the other storage
        this_storage = std::move(other_storage);
      } else {
        // Append raw bytes from other storage to this storage
        this_storage.AppendRawBytes(other_storage.Data());
      }
    }
  }
}

template <EventTrait T>
inline void EventQueue::Write(const T& event) {
  constexpr EventTypeId type_id = EventTypeIdOf<T>();
  HELIOS_ASSERT(IsRegistered<T>(), "Failed to write event: Event type '{}' is not registered!", EventNameOf<T>());
  storages_.at(type_id).Write(event);
}

template <std::ranges::sized_range R>
  requires EventTrait<std::ranges::range_value_t<R>>
inline void EventQueue::WriteBulk(const R& events) {
  using EventType = std::ranges::range_value_t<R>;
  constexpr EventTypeId type_id = EventTypeIdOf<EventType>();
  HELIOS_ASSERT(IsRegistered<EventType>(), "Failed to write events bulk: Event type '{}' is not registered!",
                EventNameOf<EventType>());
  storages_.at(type_id).WriteBulk(events);
}

template <EventTrait T>
inline auto EventQueue::Read() const -> std::span<const T> {
  constexpr EventTypeId type_id = EventTypeIdOf<T>();
  const auto it = storages_.find(type_id);
  if (it == storages_.end()) {
    return {};
  }

  return it->second.ReadAll<T>();
}

template <EventTrait T, typename OutIt>
  requires std::output_iterator<OutIt, T>
inline void EventQueue::ReadInto(OutIt out) const {
  constexpr EventTypeId type_id = EventTypeIdOf<T>();
  const auto it = storages_.find(type_id);
  if (it == storages_.end()) {
    return;
  }

  it->second.ReadInto<T>(std::move(out));
}

template <EventTrait T>
inline bool EventQueue::HasEvents() const {
  constexpr EventTypeId type_id = EventTypeIdOf<T>();
  const auto it = storages_.find(type_id);
  return it != storages_.end() && !it->second.Empty();
}

template <EventTrait T>
inline bool EventQueue::IsRegistered() const {
  constexpr EventTypeId type_id = EventTypeIdOf<T>();
  return storages_.contains(type_id);
}

inline size_t EventQueue::TotalSizeBytes() const noexcept {
  size_t total = 0;
  for (const auto& [_, storage] : storages_) {
    total += storage.SizeBytes();
  }
  return total;
}

}  // namespace helios::ecs::details
