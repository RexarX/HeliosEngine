#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/assert.hpp>
#include <helios/core/ecs/event.hpp>
#include <helios/core/logger.hpp>

#include <bit>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <ranges>
#include <span>
#include <vector>

namespace helios::ecs::details {

/**
 * @brief Type-erased storage for events using a byte vector.
 * @details Stores events as raw bytes and provides methods to write/read events with proper type safety through.
 * Events are stored sequentially with their size information for proper deserialization.
 * @note Not thread-safe.
 */
class EventStorage {
public:
  /**
   * @brief Constructs an EventStorage.
   * @param event_size Size of each event in bytes
   */
  explicit EventStorage(size_t event_size) : event_size_(event_size) {};
  EventStorage(const EventStorage&) = delete;
  EventStorage(EventStorage&&) noexcept = default;
  ~EventStorage() = default;

  /**
   * @brief Creates an EventStorage for a specific event type.
   * @tparam T Event type
   * @return EventStorage configured for the event type
   */
  template <EventTrait T>
  [[nodiscard]] static EventStorage FromEvent() {
    return EventStorage(sizeof(T));
  }

  EventStorage& operator=(const EventStorage&) = delete;
  EventStorage& operator=(EventStorage&&) noexcept = default;

  /**
   * @brief Clears all stored events.
   */
  void Clear() noexcept { data_.clear(); }

  /**
   * @brief Reserves space for events.
   * @param capacity_bytes Capacity to reserve in bytes
   */
  void Reserve(size_t capacity_bytes) { data_.reserve(capacity_bytes); }

  /**
   * @brief Writes a single event to storage.
   * @warning Triggers assertion if event size doesn't match storage event size.
   * @tparam T Event type
   * @param event Event to store
   */
  template <EventTrait T>
  void Write(const T& event);

  /**
   * @brief Writes multiple events to storage in bulk.
   * @warning Triggers assertion if event size doesn't match storage event size.
   * @tparam R Range of events
   * @param events Range of events to store
   */
  template <std::ranges::sized_range R>
    requires EventTrait<std::ranges::range_value_t<R>>
  void WriteBulk(const R& events);

  /**
   * @brief Reads all events from storage.
   * @warning Triggers assertion if event size doesn't match storage event size.
   * Span gets invalidated if storage is modified.
   * @tparam T Event type
   * @return Span containing all events
   */
  template <EventTrait T>
  [[nodiscard]] auto ReadAll() const noexcept -> std::span<const T>;

  /**
   * @brief Reads events of a specific type into a provided output iterator.
   * @note Thread-safe for read operations.
   * @warning Triggers assertion if event size doesn't match storage event size.
   * @tparam T Event type
   * @tparam OutIt Output iterator type
   * @param out Output iterator to write events into
   */
  template <EventTrait T, typename OutIt>
    requires std::output_iterator<OutIt, T>
  void ReadInto(OutIt out) const;

  /**
   * @brief Appends raw bytes from another EventStorage.
   * @param data Pointer to raw byte data
   */
  void AppendRawBytes(std::span<const std::byte> data);

  /**
   * @brief Checks if storage is empty.
   * @return True if no events are stored, false otherwise
   */
  [[nodiscard]] bool Empty() const noexcept { return data_.empty(); }

  /**
   * @brief Gets the size of stored data in bytes.
   * @return Size in bytes
   */
  [[nodiscard]] size_t SizeBytes() const noexcept { return data_.size(); }

  /**
   * @brief Gets raw data (const).
   * @return Pointer to raw byte data
   */
  [[nodiscard]] auto Data() const noexcept -> std::span<const std::byte> { return {data_.data(), data_.size()}; }

  /**
   * @brief Gets the size of each event in bytes.
   * @return Event size in bytes
   */
  [[nodiscard]] size_t EventSize() const noexcept { return event_size_; }

private:
  size_t event_size_ = 0;        ///< Size of each event in bytes
  std::vector<std::byte> data_;  ///< Events data stored in bytes
};

template <EventTrait T>
inline void EventStorage::Write(const T& event) {
  constexpr size_t event_size = sizeof(T);
  HELIOS_ASSERT(event_size == event_size_, "Failed to write event: Event size misatch, expected '{}', got '{}'!",
                event_size_, event_size);

  const size_t old_size = data_.size();
  data_.resize(old_size + event_size);
  std::memcpy(data_.data() + old_size, &event, event_size);
}

template <std::ranges::sized_range R>
  requires EventTrait<std::ranges::range_value_t<R>>
inline void EventStorage::WriteBulk(const R& events) {
  using EventType = std::ranges::range_value_t<R>;
  constexpr size_t event_size = sizeof(EventType);
  HELIOS_ASSERT(event_size == event_size_, "Failed to write events bulk: Event size misatch, expected '{}', got '{}'!",
                event_size_, event_size);

  const size_t old_size = data_.size();
  const size_t total_bytes = events.size() * event_size;

  data_.resize(old_size + total_bytes);

  size_t offset = old_size;
  for (const auto& event : events) {
    // Write event data using memcpy (safe for trivially copyable types)
    std::memcpy(data_.data() + offset, &event, event_size);
    offset += event_size;
  }
}

template <EventTrait T>
inline auto EventStorage::ReadAll() const noexcept -> std::span<const T> {
  constexpr size_t event_size = sizeof(T);
  HELIOS_ASSERT(event_size == event_size_, "Failed to write event: Event size misatch, expected '{}', got '{}'!",
                event_size_, event_size);

  const auto* begin = std::bit_cast<const T*>(data_.data());
  const auto* end = std::bit_cast<const T*>(data_.data() + data_.size());
  return {begin, end};
}

template <EventTrait T, typename OutIt>
  requires std::output_iterator<OutIt, T>
inline void EventStorage::ReadInto(OutIt out) const {
  constexpr size_t event_size = sizeof(T);
  HELIOS_ASSERT(event_size == event_size_, "Failed to write event: Event size misatch, expected '{}', got '{}'!",
                event_size_, event_size);

  size_t offset = 0;
  while (offset < data_.size()) {
    // Read event data using memcpy (safe for trivially copyable types)
    T event;
    std::memcpy(&event, data_.data() + offset, event_size);
    *out++ = std::move(event);

    offset += event_size;
  }
}

inline void EventStorage::AppendRawBytes(std::span<const std::byte> data) {
  if (data.empty()) [[unlikely]] {
    return;
  }

  const size_t old_size = data_.size();
  data_.resize(old_size + data.size());
  std::memcpy(data_.data() + old_size, data.data(), data.size());
}

}  // namespace helios::ecs::details
