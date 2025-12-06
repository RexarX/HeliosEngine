#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/assert.hpp>
#include <helios/core/ecs/details/event_queue.hpp>
#include <helios/core/ecs/event.hpp>
#include <helios/core/logger.hpp>

#include <cstddef>
#include <iterator>
#include <ranges>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace helios::ecs::details {

/**
 * @brief Metadata for registered events.
 * @details Stores information about event lifecycle management and registration.
 */
struct EventMetadata {
  EventTypeId type_id = 0;                                       ///< Unique type identifier for the event
  std::string_view name;                                         ///< Human-readable name of the event
  EventClearPolicy clear_policy = EventClearPolicy::kAutomatic;  ///< Event clearing policy
  size_t frame_registered = 0;                                   ///< Frame number when event was registered
};

/**
 * @brief Manages event lifecycle with double buffering and registration tracking.
 * @details Implements Bevy-style event management with:
 * - Double buffering: events persist for one full update cycle
 * - Explicit registration: events must be registered before use
 * - Automatic clearing: events are cleared after their lifecycle expires
 * - Manual control: users can opt-out of auto-clearing for custom events
 *
 * Event Lifecycle:
 * - Frame N: Events written to current queue
 * - Frame N+1: Events readable from previous queue (after swap)
 * - Frame N+2: Events cleared from previous queue
 *
 * @note Not thread-safe.
 */
class EventManager {
public:
  EventManager() = default;
  EventManager(const EventManager&) = delete;
  EventManager(EventManager&&) noexcept = default;
  ~EventManager() = default;

  EventManager& operator=(const EventManager&) = delete;
  EventManager& operator=(EventManager&&) noexcept = default;

  /**
   * @brief Clears all events and registration data.
   */
  void Clear() noexcept;

  /**
   * @brief Clears all event queues without removing registration data.
   * @details Useful for manually clearing events while preserving event types.
   * Events can still be written/read after calling this method.
   * Registration metadata and frame counter are preserved.
   */
  void ClearAllQueues() noexcept;

  /**
   * @brief Manually clears events of a specific type from current and previous queues.
   * @details Should be used for manually-managed events (clear_policy = kManual).
   * Can also be used as an override for automatic events if needed.
   * @warning Triggers assertion if event is not registered.
   * @tparam T Event type
   */
  template <EventTrait T>
  void ManualClear();

  /**
   * @brief Updates event lifecycle - swaps buffers and clears old events.
   * @details Should be called at the end of each update cycle.
   * Clears previous queue, moves current to previous, creates new current queue.
   */
  void Update();

  /**
   * @brief Registers an event type for use.
   * @details Uses the event's GetClearPolicy() if available, otherwise defaults to kAutomatic.
   * @warning Triggers assertion if event is already registered.
   * @tparam T Event type
   */
  template <EventTrait T>
  void RegisterEvent();

  /**
   * @brief Registers multiple event types for use.
   * @details Each event uses its own GetClearPolicy() if available.
   * @warning Triggers assertion if any event is already registered.
   * @tparam Events Event types to register
   */
  template <EventTrait... Events>
    requires(sizeof...(Events) > 1)
  void RegisterEvents() {
    (RegisterEvent<Events>(), ...);
  }

  /**
   * @brief Writes a single event to the current queue.
   * @warning Triggers assertion if event is not registered.
   * @tparam T Event type
   * @param event Event to write
   */
  template <EventTrait T>
  void Write(const T& event);

  /**
   * @brief Writes multiple events to the queue in bulk.
   * @warning Triggers assertion if event type does not exist.
   * @tparam R Range of events
   * @param events Range of events to store
   */
  template <std::ranges::sized_range R>
    requires EventTrait<std::ranges::range_value_t<R>>
  void WriteBulk(const R& events);

  /**
   * @brief Reads all events of a specific type from current and previous queues.
   * @details Events from the current frame and previous frame are returned,
   * implementing double buffering for reliable event delivery.
   * @warning Triggers assertion if event is not registered.
   * @tparam T Event type
   * @return Vector containing all events of type T from both buffers
   */
  template <EventTrait T>
  [[nodiscard]] auto Read() const -> std::vector<T>;

  /**
   * @brief Reads events of a specific type into a provided output iterator.
   * @details Reads from current and previous queues without intermediate allocations.
   * @warning Triggers assertion if event is not registered.
   * @tparam T Event type
   * @tparam OutIt Output iterator type
   * @param out Output iterator to write events into
   */
  template <EventTrait T, typename OutIt>
    requires std::output_iterator<std::remove_reference_t<OutIt>, T>
  void ReadInto(OutIt out) const;

  /**
   * @brief Merges events from another EventQueue into the current queue.
   * @param other Event queue to merge from (typically from SystemLocalStorage)
   */
  void Merge(EventQueue& other);

  /**
   * @brief Checks if an event type is registered.
   * @tparam T Event type
   * @return True if event is registered, false otherwise
   */
  template <EventTrait T>
  [[nodiscard]] bool IsRegistered() const noexcept;

  /**
   * @brief Checks if both queues are empty.
   * @return True if no events exist in either queue, false otherwise
   */
  [[nodiscard]] bool Empty() const noexcept { return current_queue_.Empty() && previous_queue_.Empty(); }

  /**
   * @brief Checks if events of a specific type exist in either queue.
   * @tparam T Event type
   * @return True if events of type T exist, false otherwise
   */
  template <EventTrait T>
  [[nodiscard]] bool HasEvents() const {
    return current_queue_.HasEvents<T>() || previous_queue_.HasEvents<T>();
  }

  /**
   * @brief Gets metadata for a registered event.
   * @tparam T Event type
   * @return Pointer to metadata, or nullptr if not registered
   */
  template <EventTrait T>
  [[nodiscard]] const EventMetadata* GetMetadata() const noexcept;

  /**
   * @brief Gets the current frame number.
   * @return Current frame counter value
   */
  [[nodiscard]] size_t GetCurrentFrame() const noexcept { return current_frame_; }

  /**
   * @brief Gets the number of registered event types.
   * @return Count of registered events
   */
  [[nodiscard]] size_t RegisteredEventCount() const noexcept { return registered_events_.size(); }

  /**
   * @brief Gets reference to current event queue (for testing/debugging).
   * @return Const reference to current queue
   */
  [[nodiscard]] const EventQueue& GetCurrentQueue() const noexcept { return current_queue_; }

  /**
   * @brief Gets reference to previous event queue (for testing/debugging).
   * @return Const reference to previous queue
   */
  [[nodiscard]] const EventQueue& GetPreviousQueue() const noexcept { return previous_queue_; }

private:
  std::unordered_map<EventTypeId, EventMetadata> registered_events_;  ///< Metadata for registered events

  EventQueue current_queue_;   ///< Events written in current frame
  EventQueue previous_queue_;  ///< Events from previous frame (readable for double buffering)

  size_t current_frame_ = 0;  ///< Current frame counter for lifecycle tracking
};

inline void EventManager::Clear() noexcept {
  registered_events_.clear();
  current_queue_.Clear();
  previous_queue_.Clear();
  current_frame_ = 0;
}

inline void EventManager::ClearAllQueues() noexcept {
  current_queue_.Clear();
  previous_queue_.Clear();
}

inline void EventManager::Update() {
  // Double-queue double buffering implementation with selective clearing:
  // 1. Selectively clear auto_clear events from previous_queue_ (events that are now 2 frames old)
  // 2. Merge current queue into previous queue (preserving non-auto_clear events)
  // 3. Create new empty current queue for next frame
  //
  // This ensures:
  // - auto_clear=true events persist for exactly 1 full update cycle
  // - auto_clear=false events persist indefinitely until manually cleared
  //
  // Frame lifecycle:
  // - Frame N: Event written to current_queue_
  // - Frame N+1: Event readable from previous_queue_ (after merge)
  // - Frame N+2: auto_clear=true events cleared, auto_clear=false events remain

  // Step 1: Clear automatic events from previous queue (events that are 2+ frames old)
  for (const auto& [type_id, metadata] : registered_events_) {
    if (metadata.clear_policy == EventClearPolicy::kAutomatic) {
      previous_queue_.ClearByTypeId(type_id);
    }
  }

  // Step 2: Merge current queue into previous queue
  // This preserves non-auto_clear events from previous queue while adding current frame's events
  previous_queue_.Merge(current_queue_);

  // Step 3: Clear current queue for next frame (clear each registered type individually)
  for (const auto& [type_id, metadata] : registered_events_) {
    current_queue_.ClearByTypeId(type_id);
  }

  ++current_frame_;
}

template <EventTrait T>
inline void EventManager::RegisterEvent() {
  constexpr EventTypeId type_id = EventTypeIdOf<T>();
  constexpr std::string_view name = EventNameOf<T>();
  constexpr EventClearPolicy clear_policy = EventClearPolicyOf<T>();

  HELIOS_ASSERT(!registered_events_.contains(type_id), "Failed to register event '{}': Event already registered!",
                name);

  registered_events_.emplace(
      type_id, EventMetadata{
                   .type_id = type_id, .name = name, .clear_policy = clear_policy, .frame_registered = current_frame_});

  current_queue_.Register<T>();
  previous_queue_.Register<T>();

  constexpr std::string_view policy_str = (clear_policy == EventClearPolicy::kAutomatic) ? "automatic" : "manual";
  HELIOS_DEBUG("Registered event '{}' (clear_policy: {})", name, policy_str);
}

template <EventTrait T>
inline void EventManager::Write(const T& event) {
  HELIOS_ASSERT(registered_events_.contains(EventTypeIdOf<T>()), "Failed to write event '{}': Event is not registered!",
                EventNameOf<T>());

  current_queue_.Write(event);
}

template <std::ranges::sized_range R>
  requires EventTrait<std::ranges::range_value_t<R>>
inline void EventManager::WriteBulk(const R& events) {
  using EventType = std::ranges::range_value_t<R>;
  HELIOS_ASSERT(registered_events_.contains(EventTypeIdOf<EventType>()),
                "Failed to write bulk event '{}': Event is not registered!", EventNameOf<EventType>());

  current_queue_.WriteBulk(events);
}

template <EventTrait T>
inline auto EventManager::Read() const -> std::vector<T> {
  HELIOS_ASSERT(registered_events_.contains(EventTypeIdOf<T>()), "Failed to read events '{}': Event is not registered!",
                EventNameOf<T>());

  std::vector<T> result;
  result.reserve(previous_queue_.TypeCount() + current_queue_.TypeCount());

  // Read from previous queue (events from last frame)
  previous_queue_.ReadInto<T>(std::back_inserter(result));

  // Read from current queue (events from this frame)
  current_queue_.ReadInto<T>(std::back_inserter(result));

  return result;
}

template <EventTrait T, typename OutIt>
  requires std::output_iterator<std::remove_reference_t<OutIt>, T>
inline void EventManager::ReadInto(OutIt out) const {
  HELIOS_ASSERT(registered_events_.contains(EventTypeIdOf<T>()),
                "Failed to read events '{}' into: Event is not registered!", EventNameOf<T>());

  previous_queue_.ReadInto<T>(out);
  current_queue_.ReadInto<T>(std::move(out));
}

inline void EventManager::Merge(EventQueue& other) {
  current_queue_.Merge(other);
  other.Clear();
}

template <EventTrait T>
inline void EventManager::ManualClear() {
  HELIOS_ASSERT(registered_events_.contains(EventTypeIdOf<T>()),
                "Failed to manual clear events '{}': Event is not registered!", EventNameOf<T>());

  // Note: We allow manual clearing for all events (automatic or manual) as an override
  // This can be useful for debugging or special cases

  current_queue_.Clear<T>();
  previous_queue_.Clear<T>();
}

template <EventTrait T>
inline const EventMetadata* EventManager::GetMetadata() const noexcept {
  constexpr EventTypeId type_id = EventTypeIdOf<T>();
  const auto it = registered_events_.find(type_id);
  return it != registered_events_.end() ? &it->second : nullptr;
}

template <EventTrait T>
inline bool EventManager::IsRegistered() const noexcept {
  constexpr EventTypeId type_id = EventTypeIdOf<T>();
  return registered_events_.contains(type_id);
}

}  // namespace helios::ecs::details
