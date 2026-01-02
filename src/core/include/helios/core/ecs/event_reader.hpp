#pragma once

#include <helios/core/ecs/details/event_manager.hpp>
#include <helios/core/ecs/event.hpp>
#include <helios/core/utils/functional_adapters.hpp>

#include <concepts>
#include <cstddef>
#include <iterator>
#include <optional>
#include <span>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace helios::ecs {

/**
 * @brief Simple iterator for EventReader that returns plain events.
 * @tparam T Event type
 */
template <EventTrait T>
class EventSimpleIterator {
public:
  using value_type = std::remove_cvref_t<T>;
  using iterator_category = std::forward_iterator_tag;
  using difference_type = ptrdiff_t;
  using pointer = const value_type*;
  using reference = const value_type&;

  constexpr EventSimpleIterator() noexcept = default;
  constexpr explicit EventSimpleIterator(typename std::vector<T>::const_iterator iter) noexcept : iter_(iter) {}

  constexpr EventSimpleIterator(const EventSimpleIterator&) noexcept = default;
  constexpr EventSimpleIterator(EventSimpleIterator&&) noexcept = default;
  constexpr ~EventSimpleIterator() noexcept = default;

  constexpr EventSimpleIterator& operator=(const EventSimpleIterator&) noexcept = default;
  constexpr EventSimpleIterator& operator=(EventSimpleIterator&&) noexcept = default;

  constexpr reference operator*() const noexcept { return *iter_; }
  constexpr pointer operator->() const noexcept { return &(*iter_); }

  constexpr EventSimpleIterator& operator++() noexcept;
  constexpr EventSimpleIterator operator++(int) noexcept;

  constexpr EventSimpleIterator& operator--() noexcept;
  constexpr EventSimpleIterator operator--(int) noexcept;

  constexpr bool operator==(const EventSimpleIterator& other) const noexcept { return iter_ == other.iter_; }
  constexpr bool operator!=(const EventSimpleIterator& other) const noexcept { return iter_ != other.iter_; }

private:
  typename std::vector<value_type>::const_iterator iter_;
};

template <EventTrait T>
constexpr auto EventSimpleIterator<T>::operator++() noexcept -> EventSimpleIterator& {
  ++iter_;
  return *this;
}

template <EventTrait T>
constexpr auto EventSimpleIterator<T>::operator++(int) noexcept -> EventSimpleIterator {
  auto temp = *this;
  ++iter_;
  return temp;
}

template <EventTrait T>
constexpr auto EventSimpleIterator<T>::operator--() noexcept -> EventSimpleIterator& {
  --iter_;
  return *this;
}

template <EventTrait T>
constexpr auto EventSimpleIterator<T>::operator--(int) noexcept -> EventSimpleIterator {
  auto temp = *this;
  --iter_;
  return temp;
}

/**
 * @brief Iterator wrapper for EventReader.
 * @tparam T Event type
 */
template <EventTrait T>
class EventIterator {
public:
  using value_type = std::tuple<std::remove_cvref_t<T>>;
  using iterator_category = std::forward_iterator_tag;
  using difference_type = ptrdiff_t;
  using pointer = const std::tuple<std::remove_cvref_t<T>>*;
  using reference = std::tuple<std::remove_cvref_t<T>>;

  constexpr EventIterator() noexcept = default;
  constexpr explicit EventIterator(typename std::vector<std::remove_cvref_t<T>>::const_iterator iter) noexcept
      : iter_(iter) {}

  constexpr EventIterator(const EventIterator&) noexcept = default;
  constexpr EventIterator(EventIterator&&) noexcept = default;
  constexpr ~EventIterator() noexcept = default;

  constexpr EventIterator& operator=(const EventIterator&) noexcept = default;
  constexpr EventIterator& operator=(EventIterator&&) noexcept = default;

  constexpr reference operator*() const noexcept { return *iter_; }

  constexpr EventIterator& operator++() noexcept;
  constexpr EventIterator operator++(int) noexcept;

  constexpr EventIterator& operator--() noexcept;
  constexpr EventIterator operator--(int) noexcept;

  constexpr bool operator==(const EventIterator& other) const noexcept { return iter_ == other.iter_; }
  constexpr bool operator!=(const EventIterator& other) const noexcept { return iter_ != other.iter_; }

private:
  typename std::vector<std::remove_cvref_t<T>>::const_iterator iter_;
};

template <EventTrait T>
constexpr auto EventIterator<T>::operator++() noexcept -> EventIterator& {
  ++iter_;
  return *this;
}

template <EventTrait T>
constexpr auto EventIterator<T>::operator++(int) noexcept -> EventIterator {
  auto temp = *this;
  ++iter_;
  return temp;
}

template <EventTrait T>
constexpr auto EventIterator<T>::operator--() noexcept -> EventIterator& {
  --iter_;
  return *this;
}

template <EventTrait T>
constexpr auto EventIterator<T>::operator--(int) noexcept -> EventIterator {
  auto temp = *this;
  --iter_;
  return temp;
}

/**
 * @brief Type-safe reader for events with filtering and query support.
 * @details Provides a clean, ergonomic API for reading events from the event system.
 * EventReader supports iteration, filtering, searching, and collection operations.
 * It uses lazy evaluation with caching to avoid multiple reads from EventManager.
 * @note EventReader is read-only and uses lazy evaluation with internal caching.
 * EventReader is intended to be short-lived (function-scoped).
 * Thread-safe for const operations if EventManager is thread-safe.
 * @tparam T Event type
 *
 * @example
 * @code
 * auto reader = world.ReadEvents<MyEvent>();
 *
 * // Iterate over events (efficient - no copy)
 * for (const auto& event : reader) {
 *   ProcessEvent(event);
 * }
 *
 * // Get events as span (efficient - no copy)
 * auto events = reader.Read();
 * for (const auto& event : events) {
 *   ProcessEvent(event);
 * }
 *
 * // Collect events into vector (when you need ownership)
 * auto owned = reader.Collect();
 *
 * // Functional operations
 * auto high_priority = reader.Filter([](auto& e) { return e.priority > 5; });
 * auto ids = reader.Map([](auto& e) { return e.id; });
 * for (const auto& [idx, event] : reader.Enumerate()) {
 *   // Process with index
 * }
 * @endcode
 */
template <EventTrait T>
class EventReader {
public:
  using AdaptorIterator = EventIterator<T>;  // For adapters - returns tuples
  using AdaptorConstIterator = EventSimpleIterator<T>;

  using value_type = std::remove_cvref_t<T>;
  using const_iterator = AdaptorConstIterator;
  using iterator = const_iterator;  // Read-only access

  /**
   * @brief Constructs an EventReader.
   * @param manager Reference to the event manager
   */
  explicit constexpr EventReader(const details::EventManager& manager) noexcept : manager_(manager) {}
  EventReader(const EventReader&) = delete;
  EventReader(EventReader&&) noexcept = default;
  ~EventReader() noexcept = default;

  EventReader& operator=(const EventReader&) = delete;
  EventReader& operator=(EventReader&&) noexcept = default;

  /**
   * @brief Returns events as a read-only span.
   * @details This is the primary interface for accessing events.
   * The span remains valid as long as the EventReader is alive and no non-const operations are performed.
   * @return Span of const events
   *
   * @example
   * @code
   * auto reader = world.ReadEvents<MyEvent>();
   * auto events = reader.Read();
   * for (const auto& event : events) {
   *   ProcessEvent(event);
   * }
   * @endcode
   */
  [[nodiscard]] auto Read() const& -> std::span<const value_type> {
    EnsureCached();
    return {cached_events_->data(), cached_events_->size()};
  }

  /**
   * @brief Collects all events into a vector.
   * @details Returns a copy of the events. Use this when you need ownership
   * or when the events need to outlive the EventReader.
   * For most cases, prefer using Read() or range-based for loops.
   * @return Vector containing all events
   *
   * @example
   * @code
   * // Use when you need a copy
   * auto events = reader.Collect();
   * ProcessLater(std::move(events));
   * @endcode
   */
  [[nodiscard]] auto Collect() const -> std::vector<value_type> {
    EnsureCached();
    return *cached_events_;
  }

  /**
   * @brief Collects all events into a vector using a custom allocator.
   * @details Returns a copy of the events using the provided allocator.
   * Useful for temporary allocations with frame allocators.
   * @warning When using a frame allocator (via SystemContext::MakeFrameAllocator()),
   * the returned data is only valid for the current frame. All pointers and references
   * to frame-allocated data become invalid after the frame ends. Do not store
   * frame-allocated data in components, resources, or any persistent storage.
   * @tparam Alloc STL-compatible allocator type for value_type
   * @param alloc Allocator instance to use for the result vector
   * @return Vector containing all events, using the provided allocator
   *
   * @example
   * @code
   * void MySystem::Update(app::SystemContext& ctx) {
   *   auto alloc = ctx.MakeFrameAllocator<MyEvent>();
   *   auto reader = ctx.ReadEvents<MyEvent>();
   *   auto events = reader.CollectWith(alloc);
   *   // events uses frame allocator, automatically reclaimed at frame end
   *   // WARNING: Do not store 'events' or pointers to its contents beyond this frame!
   * }
   * @endcode
   */
  template <typename Alloc>
    requires std::same_as<typename Alloc::value_type, std::remove_cvref_t<T>>
  [[nodiscard]] auto CollectWith(Alloc alloc) const -> std::vector<value_type, Alloc>;

  /**
   * @brief Reads events into an output iterator.
   * @details Efficiently writes events to an output iterator without materialization.
   * This method bypasses the cache and reads directly from EventManager for optimal performance.
   * @tparam OutIt Output iterator type
   * @param out Output iterator
   *
   * @example
   * @code
   * std::vector<MyEvent> events;
   * reader.ReadInto(std::back_inserter(events));
   * @endcode
   */
  template <typename OutIt>
    requires std::output_iterator<OutIt, value_type>
  void ReadInto(OutIt out) const {
    manager_.ReadInto<value_type>(std::move(out));
  }

  /**
   * @brief Consumes adapter/iterator and writes events into an output iterator.
   * @details Terminal operation that writes all events to the provided output iterator.
   * More efficient than Collect() when you have a destination container.
   * @tparam OutIt Output iterator type
   * @param out Output iterator
   *
   * @example
   * @code
   * std::vector<MyEvent> filtered_events;
   * reader.Filter([](auto& e) { return e.priority > 5; })
   *       .Into(std::back_inserter(filtered_events));
   * @endcode
   */
  template <typename OutIt>
    requires std::output_iterator<OutIt, std::remove_cvref_t<T>>
  void Into(OutIt out) const;

  /**
   * @brief Returns the first event that matches a predicate.
   * @details The returned pointer is valid only while EventReader is alive.
   * @tparam Pred Predicate type
   * @param predicate Predicate function
   * @return Pointer to first matching event or nullptr if not found
   *
   * @example
   * @code
   * if (auto* event = reader.FindFirst([](auto& e) { return e.priority > 5; })) {
   *   ProcessHighPriority(*event);
   * }
   * @endcode
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, const std::remove_cvref_t<T>&>
  [[nodiscard]] const value_type* FindFirst(const Pred& predicate) const&;

  /**
   * @brief Counts events that match a predicate.
   * @tparam Pred Predicate type
   * @param predicate Predicate function
   * @return Number of matching events
   *
   * @example
   * @code
   * size_t critical_count = reader.CountIf([](auto& e) { return e.is_critical; });
   * @endcode
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, const std::remove_cvref_t<T>&>
  [[nodiscard]] size_t CountIf(const Pred& predicate) const;

  /**
   * @brief Filters events based on a predicate.
   * @details Lazily filters the event results, only yielding elements that satisfy the predicate.
   * @tparam Pred Predicate function type (const T&) -> bool
   * @param predicate Function to test each event
   * @return View that yields only matching events
   *
   * @example
   * @code
   * auto critical = reader.Filter([](const MyEvent& e) { return e.is_critical; });
   * for (const auto& event : critical) {
   *   HandleCritical(event);
   * }
   * @endcode
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, const std::remove_cvref_t<T>&>
  [[nodiscard]] auto Filter(Pred predicate) const& -> utils::FilterAdapter<AdaptorIterator, Pred>;

  /**
   * @brief Transforms each event using a mapping function.
   * @details Lazily transforms event results by applying a function to each element.
   * @tparam Func Transformation function type (const T&) -> U
   * @param transform Function to transform each event
   * @return View that yields transformed elements
   *
   * @example
   * @code
   * auto ids = reader.Map([](const MyEvent& e) { return e.id; });
   * for (auto id : ids) {
   *   ProcessId(id);
   * }
   * @endcode
   */
  template <typename Func>
    requires utils::TransformFor<Func, const std::remove_cvref_t<T>&>
  [[nodiscard]] auto Map(Func transform) const& -> utils::MapAdapter<AdaptorIterator, Func>;

  /**
   * @brief Takes only the first N events.
   * @details Lazily yields at most N elements from the event results.
   * @param count Maximum number of elements to take
   * @return View that yields at most count elements
   *
   * @example
   * @code
   * auto first_five = reader.Take(5);
   * @endcode
   */
  [[nodiscard]] auto Take(size_t count) const& -> utils::TakeAdapter<AdaptorIterator>;

  /**
   * @brief Skips the first N events.
   * @details Lazily skips the first N elements and yields the rest.
   * @param count Number of elements to skip
   * @return View that yields elements after skipping count elements
   *
   * @code
   * auto after_ten = reader.Skip(10);
   * @endcode
   */
  [[nodiscard]] auto Skip(size_t count) const& -> utils::SkipAdapter<AdaptorIterator>;

  /**
   * @brief Takes events while a predicate is true.
   * @details Lazily yields elements until the predicate returns false.
   * @tparam Pred Predicate function type (const T&) -> bool
   * @param predicate Function to test each event
   * @return View that yields elements while predicate is true
   *
   * @example
   * @code
   * auto while_valid = reader.TakeWhile([](const MyEvent& e) { return e.is_valid; });
   * @endcode
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, const std::remove_cvref_t<T>&>
  [[nodiscard]] auto TakeWhile(Pred predicate) const& -> utils::TakeWhileAdapter<AdaptorIterator, Pred>;

  /**
   * @brief Skips events while a predicate is true.
   * @details Lazily skips elements until the predicate returns false, then yields the rest.
   * @tparam Pred Predicate function type (const T&) -> bool
   * @param predicate Function to test each event
   * @return View that yields elements after predicate becomes false
   *
   * @example
   * @code
   * auto after_invalid = reader.SkipWhile([](const MyEvent& e) { return !e.is_valid; });
   * @endcode
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, const std::remove_cvref_t<T>&>
  [[nodiscard]] auto SkipWhile(Pred predicate) const& -> utils::SkipWhileAdapter<AdaptorIterator, Pred>;

  /**
   * @brief Adds an index to each event.
   * @details Lazily yields tuples with an index followed by the event.
   * @return View that yields (index, event) tuples
   *
   * @example
   * @code
   * for (auto [idx, event] : reader.Enumerate()) {
   *   // idx is 0, 1, 2, ...
   *   ProcessWithIndex(idx, event);
   * }
   * @endcode
   */
  [[nodiscard]] auto Enumerate() const& -> utils::EnumerateAdapter<AdaptorIterator>;

  /**
   * @brief Inspects each event without consuming it.
   * @details Calls a function on each element for side effects, then passes it through.
   * @tparam Func Inspection function type (const T&) -> void
   * @param inspector Function to call on each event
   * @return View that yields the same elements after inspection
   *
   * @example
   * @code
   * auto result = reader.Inspect([](const MyEvent& e) {
   *     std::cout << "Event: " << e.id << "\n";
   *   })
   *   .Collect();
   * @endcode
   */
  template <typename Func>
    requires utils::InspectorFor<Func, const std::remove_cvref_t<T>&>
  [[nodiscard]] auto Inspect(Func inspector) const& -> utils::InspectAdapter<AdaptorIterator, Func>;

  /**
   * @brief Yields every Nth event.
   * @details Lazily yields elements at regular intervals.
   * @param step Interval between yielded elements (must be > 0)
   * @return View that yields every step-th element
   *
   * @example
   * @code
   * auto every_third = reader.StepBy(3);
   * @endcode
   */
  [[nodiscard]] auto StepBy(size_t step) const& -> utils::StepByAdapter<AdaptorIterator>;

  /**
   * @brief Reverses the order of events.
   * @details Iterates through events in reverse order.
   * @return View that yields events in reverse order
   * @note Requires bidirectional iterator support
   *
   * @example
   * @code
   * auto reversed = reader.Reverse();
   * @endcode
   */
  [[nodiscard]] auto Reverse() const& -> utils::ReverseAdapter<AdaptorIterator> { return {begin(), end()}; }

  /**
   * @brief Creates sliding windows over events.
   * @details Yields overlapping windows of a fixed size.
   * @param window_size Size of the sliding window
   * @return View that yields windows of events
   * @warning window_size must be greater than 0
   *
   * @example
   * @code
   * auto windows = reader.Slide(3);
   * @endcode
   */
  [[nodiscard]] auto Slide(size_t window_size) const& -> utils::SlideAdapter<AdaptorIterator> {
    return {begin(), end(), window_size};
  }

  /**
   * @brief Takes every Nth event.
   * @details Yields events at regular stride intervals.
   * @param stride Number of events to skip between yields
   * @return View that yields every Nth event
   * @warning stride must be greater than 0
   *
   * @example
   * @code
   * auto every_third = reader.Stride(3);
   * @endcode
   */
  [[nodiscard]] auto Stride(size_t stride) const& -> utils::StrideAdapter<AdaptorIterator> {
    return {begin(), end(), stride};
  }

  /**
   * @brief Zips events with another iterator.
   * @details Combines events with another range into tuples.
   * @tparam OtherIter Iterator type to zip with
   * @param other_begin Beginning of the other range
   * @param other_end End of the other range
   * @return View that yields tuples of corresponding elements
   *
   * @example
   * @code
   * std::vector<int> ids = {1, 2, 3};
   * auto zipped = reader.Zip(ids.begin(), ids.end());
   * @endcode
   */
  template <typename OtherIter>
    requires utils::IteratorLike<OtherIter>
  [[nodiscard]] auto Zip(OtherIter other_begin,
                         OtherIter other_end) const& -> utils::ZipAdapter<AdaptorIterator, OtherIter> {
    return {begin(), end(), std::move(other_begin), std::move(other_end)};
  }

  /**
   * @brief Executes a function for each event.
   * @details Calls the provided function with each event for side effects.
   * @tparam Action Function type (const T&) -> void
   * @param action Function to execute for each event
   *
   * @example
   * @code
   * reader.ForEach([](const MyEvent& e) {
   *   ProcessEvent(e);
   * });
   * @endcode
   */
  template <typename Action>
    requires utils::ActionFor<Action, const std::remove_cvref_t<T>&>
  void ForEach(const Action& action) const;

  /**
   * @brief Folds the events into a single value.
   * @details Applies a binary function to an initial value and each event.
   * @tparam Acc Accumulator type
   * @tparam Folder Function type (Acc, const T&) -> Acc
   * @param init Initial value
   * @param folder Function that combines accumulator with each event
   * @return Final folded value
   *
   * @example
   * @code
   * int total = reader.Fold(0, [](int acc, const MyEvent& e) { return acc + e.value; });
   * @endcode
   */
  template <typename Acc, typename Folder>
    requires utils::FolderFor<Folder, Acc, const std::remove_cvref_t<T>&>
  [[nodiscard]] Acc Fold(Acc init, const Folder& folder) const;

  /**
   * @brief Checks if any event matches the predicate.
   * @details Short-circuits on first match for efficiency.
   * @tparam Pred Predicate function type (const T&) -> bool
   * @param predicate Function to test each event
   * @return True if at least one event matches, false otherwise
   *
   * @code
   * bool has_critical = reader.Any([](const MyEvent& e) { return e.is_critical; });
   * @endcode
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, const std::remove_cvref_t<T>&>
  [[nodiscard]] bool Any(const Pred& predicate) const;

  /**
   * @brief Checks if all events match the predicate.
   * @details Short-circuits on first non-match for efficiency.
   * @tparam Pred Predicate function type (const T&) -> bool
   * @param predicate Function to test each event
   * @return True if all events match, false otherwise
   *
   * @example
   * @code
   * bool all_valid = reader.All([](const MyEvent& e) { return e.is_valid; });
   * @endcode
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, const std::remove_cvref_t<T>&>
  [[nodiscard]] bool All(const Pred& predicate) const;

  /**
   * @brief Finds the first event matching the predicate.
   * @details Returns an optional containing the first matching event.
   * @tparam Pred Predicate function type (const T&) -> bool
   * @param predicate Function to test each event
   * @return Optional containing the first matching event, or empty if not found
   *
   * @example
   * @code
   * if (auto event = reader.Find([](const MyEvent& e) { return e.id == 42; })) {
   *   Process(*event);
   * }
   * @endcode
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, const std::remove_cvref_t<T>&>
  [[nodiscard]] auto Find(const Pred& predicate) const -> std::optional<value_type>;

  /**
   * @brief Checks if no events match the predicate.
   * @details Short-circuits on first match for efficiency.
   * @tparam Pred Predicate function type (const T&) -> bool
   * @param predicate Function to test each event
   * @return True if no events match, false otherwise
   *
   * @example
   * @code
   * bool none_critical = reader.None([](const MyEvent& e) { return e.is_critical; });
   * @endcode
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, const std::remove_cvref_t<T>&>
  [[nodiscard]] constexpr bool None(const Pred& predicate) const {
    return !Any(predicate);
  }

  /**
   * @brief Partitions events into two groups based on a predicate.
   * @tparam Pred Predicate function type (const T&) -> bool
   * @param predicate Function to test each event
   * @return Pair of vectors: first contains events matching predicate, second contains non-matching
   *
   * @example
   * @code
   * auto [critical, normal] = reader.Partition([](const MyEvent& e) { return e.is_critical; });
   * @endcode
   */
  template <typename Pred>
    requires utils::PredicateFor<Pred, const std::remove_cvref_t<T>&>
  [[nodiscard]] auto Partition(const Pred& predicate) const
      -> std::pair<std::vector<value_type>, std::vector<value_type>>;

  /**
   * @brief Finds the event with the maximum value using a key function.
   * @details Finds the event that produces the maximum value when passed to the key function.
   * @tparam KeyFunc Function type (const T&) -> Comparable
   * @param key_func Function to extract the comparison value
   * @return Optional containing the event with maximum key, or empty if no events exist
   *
   * @example
   * @code
   * auto highest_priority = reader.MaxBy([](const MyEvent& e) { return e.priority; });
   * @endcode
   */
  template <typename KeyFunc>
    requires std::invocable<KeyFunc, const std::remove_cvref_t<T>&>
  [[nodiscard]] auto MaxBy(const KeyFunc& key_func) const -> std::optional<value_type>;

  /**
   * @brief Finds the event with the minimum value using a key function.
   * @details Finds the event that produces the minimum value when passed to the key function.
   * @tparam KeyFunc Function type (const T&) -> Comparable
   * @param key_func Function to extract the comparison value
   * @return Optional containing the event with minimum key, or empty if no events exist
   *
   * @example
   * @code
   * auto lowest_priority = reader.MinBy([](const MyEvent& e) { return e.priority; });
   * @endcode
   */
  template <typename KeyFunc>
    requires std::invocable<KeyFunc, const std::remove_cvref_t<T>&>
  [[nodiscard]] auto MinBy(const KeyFunc& key_func) const -> std::optional<value_type>;

  /**
   * @brief Groups events by a key function.
   * @details Groups events into a map where keys are produced by the key function.
   * @tparam KeyFunc Function type (const T&) -> Key
   * @param key_func Function to extract the grouping key
   * @return Map from keys to vectors of events with that key
   *
   * @example
   * @code
   * auto by_type = reader.GroupBy([](const MyEvent& e) { return e.type; });
   * @endcode
   */
  template <typename KeyFunc>
    requires std::invocable<KeyFunc, const std::remove_cvref_t<T>&>
  [[nodiscard]] auto GroupBy(const KeyFunc& key_func) const
      -> std::unordered_map<std::invoke_result_t<KeyFunc, const value_type&>, std::vector<value_type>>;

  /**
   * @brief Checks if there are any events.
   * @return true if no events exist, false otherwise
   */
  [[nodiscard]] bool Empty() const { return !manager_.HasEvents<value_type>(); }

  /**
   * @brief Returns the number of events.
   * @return Number of events in current and previous queues
   */
  [[nodiscard]] size_t Count() const;

  /**
   * @brief Returns an iterator to the beginning of events.
   * @details Triggers lazy initialization of event cache on first call.
   * @return Const iterator to first event (returns plain T for range-based for loops)
   */
  [[nodiscard]] const_iterator begin() const;

  /**
   * @brief Returns an iterator to the end of events.
   * @details Triggers lazy initialization of event cache on first call.
   * @return Const iterator to end
   */
  [[nodiscard]] const_iterator end() const;

private:
  /**
   * @brief Ensures events are cached.
   * @details Lazily initializes the event cache on first access.
   * Subsequent calls are no-ops since the cache persists.
   */
  void EnsureCached() const;

  const details::EventManager& manager_;                          ///< Reference to the event manager
  mutable std::optional<std::vector<value_type>> cached_events_;  ///< Lazy-initialized event cache
};

template <EventTrait T>
template <typename OutIt>
  requires std::output_iterator<OutIt, std::remove_cvref_t<T>>
inline void EventReader<T>::Into(OutIt out) const {
  EnsureCached();
  for (const auto& event : *cached_events_) {
    *out++ = event;
  }
}

template <EventTrait T>
template <typename Pred>
  requires utils::PredicateFor<Pred, const std::remove_cvref_t<T>&>
inline auto EventReader<T>::FindFirst(const Pred& predicate) const& -> const value_type* {
  EnsureCached();
  for (const auto& event : *cached_events_) {
    if (predicate(event)) {
      return &event;
    }
  }
  return nullptr;
}

template <EventTrait T>
template <typename Pred>
  requires utils::PredicateFor<Pred, const std::remove_cvref_t<T>&>
inline size_t EventReader<T>::CountIf(const Pred& predicate) const {
  EnsureCached();
  size_t count = 0;
  for (const auto& event : *cached_events_) {
    if (predicate(event)) {
      ++count;
    }
  }
  return count;
}

template <EventTrait T>
inline size_t EventReader<T>::Count() const {
  EnsureCached();
  return cached_events_->size();
}

template <EventTrait T>
template <typename Alloc>
  requires std::same_as<typename Alloc::value_type, std::remove_cvref_t<T>>
inline auto EventReader<T>::CollectWith(Alloc alloc) const -> std::vector<value_type, Alloc> {
  EnsureCached();
  std::vector<value_type, Alloc> result{std::move(alloc)};
  result.reserve(cached_events_->size());
  for (const auto& event : *cached_events_) {
    result.push_back(event);
  }
  return result;
}

template <EventTrait T>
template <typename Pred>
  requires utils::PredicateFor<Pred, const std::remove_cvref_t<T>&>
inline auto EventReader<T>::Filter(Pred predicate) const& -> utils::FilterAdapter<AdaptorIterator, Pred> {
  EnsureCached();
  return {AdaptorIterator(cached_events_->begin()), AdaptorIterator(cached_events_->end()), std::move(predicate)};
}

template <EventTrait T>
template <typename Func>
  requires utils::TransformFor<Func, const std::remove_cvref_t<T>&>
inline auto EventReader<T>::Map(Func transform) const& -> utils::MapAdapter<AdaptorIterator, Func> {
  EnsureCached();
  return {AdaptorIterator(cached_events_->begin()), AdaptorIterator(cached_events_->end()), std::move(transform)};
}

template <EventTrait T>
inline auto EventReader<T>::Take(size_t count) const& -> utils::TakeAdapter<AdaptorIterator> {
  EnsureCached();
  return {AdaptorIterator(cached_events_->begin()), AdaptorIterator(cached_events_->end()), count};
}

template <EventTrait T>
inline auto EventReader<T>::Skip(size_t count) const& -> utils::SkipAdapter<AdaptorIterator> {
  EnsureCached();
  return {AdaptorIterator(cached_events_->begin()), AdaptorIterator(cached_events_->end()), count};
}

template <EventTrait T>
template <typename Pred>
  requires utils::PredicateFor<Pred, const std::remove_cvref_t<T>&>
inline auto EventReader<T>::TakeWhile(Pred predicate) const& -> utils::TakeWhileAdapter<AdaptorIterator, Pred> {
  EnsureCached();
  return {AdaptorIterator(cached_events_->begin()), AdaptorIterator(cached_events_->end()), std::move(predicate)};
}

template <EventTrait T>
template <typename Pred>
  requires utils::PredicateFor<Pred, const std::remove_cvref_t<T>&>
inline auto EventReader<T>::SkipWhile(Pred predicate) const& -> utils::SkipWhileAdapter<AdaptorIterator, Pred> {
  EnsureCached();
  return {AdaptorIterator(cached_events_->begin()), AdaptorIterator(cached_events_->end()), std::move(predicate)};
}

template <EventTrait T>
inline auto EventReader<T>::Enumerate() const& -> utils::EnumerateAdapter<AdaptorIterator> {
  EnsureCached();
  return {AdaptorIterator(cached_events_->begin()), AdaptorIterator(cached_events_->end())};
}

template <EventTrait T>
template <typename Func>
  requires utils::InspectorFor<Func, const std::remove_cvref_t<T>&>
inline auto EventReader<T>::Inspect(Func inspector) const& -> utils::InspectAdapter<AdaptorIterator, Func> {
  EnsureCached();
  return {AdaptorIterator(cached_events_->begin()), AdaptorIterator(cached_events_->end()), std::move(inspector)};
}

template <EventTrait T>
inline auto EventReader<T>::StepBy(size_t step) const& -> utils::StepByAdapter<AdaptorIterator> {
  EnsureCached();
  return {AdaptorIterator(cached_events_->begin()), AdaptorIterator(cached_events_->end()), step};
}

template <EventTrait T>
template <typename Pred>
  requires utils::PredicateFor<Pred, const std::remove_cvref_t<T>&>
inline bool EventReader<T>::Any(const Pred& predicate) const {
  EnsureCached();
  for (const auto& event : *cached_events_) {
    if (predicate(event)) {
      return true;
    }
  }
  return false;
}

template <EventTrait T>
template <typename Pred>
  requires utils::PredicateFor<Pred, const std::remove_cvref_t<T>&>
inline bool EventReader<T>::All(const Pred& predicate) const {
  EnsureCached();
  for (const auto& event : *cached_events_) {
    if (!predicate(event)) {
      return false;
    }
  }
  return true;
}

template <EventTrait T>
template <typename Pred>
  requires utils::PredicateFor<Pred, const std::remove_cvref_t<T>&>
inline auto EventReader<T>::Find(const Pred& predicate) const -> std::optional<value_type> {
  EnsureCached();
  for (const auto& event : *cached_events_) {
    if (predicate(event)) {
      return event;
    }
  }
  return std::nullopt;
}

template <EventTrait T>
template <typename Action>
  requires utils::ActionFor<Action, const std::remove_cvref_t<T>&>
inline void EventReader<T>::ForEach(const Action& action) const {
  EnsureCached();
  for (const auto& event : *cached_events_) {
    if constexpr (std::invocable<Action, const value_type&>) {
      action(event);
    } else {
      std::apply(action, event);
    }
  }
}

template <EventTrait T>
template <typename Acc, typename Folder>
  requires utils::FolderFor<Folder, Acc, const std::remove_cvref_t<T>&>
inline Acc EventReader<T>::Fold(Acc init, const Folder& folder) const {
  EnsureCached();
  for (const auto& event : *cached_events_) {
    if constexpr (std::invocable<Folder, Acc, const value_type&>) {
      init = folder(std::move(init), event);
    } else {
      init = std::apply([&folder, &init](const auto&... args) { return folder(std::move(init), args...); }, event);
    }
  }
  return init;
}

template <EventTrait T>
template <typename Pred>
  requires utils::PredicateFor<Pred, const std::remove_cvref_t<T>&>
inline auto EventReader<T>::Partition(const Pred& predicate) const
    -> std::pair<std::vector<value_type>, std::vector<value_type>> {
  EnsureCached();
  std::pair<std::vector<value_type>, std::vector<value_type>> result;
  for (const auto& event : *cached_events_) {
    if (predicate(event)) {
      result.first.push_back(event);
    } else {
      result.second.push_back(event);
    }
  }
  return result;
}

template <EventTrait T>
template <typename KeyFunc>
  requires std::invocable<KeyFunc, const std::remove_cvref_t<T>&>
inline auto EventReader<T>::MaxBy(const KeyFunc& key_func) const -> std::optional<value_type> {
  EnsureCached();
  if (cached_events_->empty()) {
    return std::nullopt;
  }

  auto max_iter = cached_events_->begin();
  auto max_key = key_func(*max_iter);

  for (auto iter = cached_events_->begin() + 1; iter != cached_events_->end(); ++iter) {
    auto current_key = key_func(*iter);
    if (current_key > max_key) {
      max_key = std::move(current_key);
      max_iter = iter;
    }
  }

  return *max_iter;
}

template <EventTrait T>
template <typename KeyFunc>
  requires std::invocable<KeyFunc, const std::remove_cvref_t<T>&>
inline auto EventReader<T>::MinBy(const KeyFunc& key_func) const -> std::optional<value_type> {
  EnsureCached();
  if (cached_events_->empty()) {
    return std::nullopt;
  }

  auto min_iter = cached_events_->begin();
  auto min_key = key_func(*min_iter);

  for (auto iter = cached_events_->begin() + 1; iter != cached_events_->end(); ++iter) {
    auto current_key = key_func(*iter);
    if (current_key < min_key) {
      min_key = std::move(current_key);
      min_iter = iter;
    }
  }

  return *min_iter;
}

template <EventTrait T>
template <typename KeyFunc>
  requires std::invocable<KeyFunc, const std::remove_cvref_t<T>&>
inline auto EventReader<T>::GroupBy(const KeyFunc& key_func) const
    -> std::unordered_map<std::invoke_result_t<KeyFunc, const value_type&>, std::vector<value_type>> {
  EnsureCached();
  std::unordered_map<std::invoke_result_t<KeyFunc, const value_type&>, std::vector<value_type>> result;
  for (const auto& event : *cached_events_) {
    result[key_func(event)].push_back(event);
  }
  return result;
}

template <EventTrait T>
inline auto EventReader<T>::begin() const -> const_iterator {
  EnsureCached();
  return const_iterator(cached_events_->begin());
}

template <EventTrait T>
inline auto EventReader<T>::end() const -> const_iterator {
  EnsureCached();
  return const_iterator(cached_events_->end());
}

template <EventTrait T>
inline void EventReader<T>::EnsureCached() const {
  if (!cached_events_.has_value()) {
    cached_events_ = manager_.Read<value_type>();
  }
}

}  // namespace helios::ecs
