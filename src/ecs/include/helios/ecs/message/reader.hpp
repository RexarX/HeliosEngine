#pragma once

#include <helios/assert.hpp>
#include <helios/ecs/message/consumed_registry.hpp>
#include <helios/ecs/message/cursor.hpp>
#include <helios/ecs/message/id.hpp>
#include <helios/ecs/message/manager.hpp>
#include <helios/ecs/message/message.hpp>
#include <helios/ecs/message/wrapper.hpp>
#include <helios/utils/functional_adapters.hpp>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <memory_resource>
#include <optional>
#include <ranges>
#include <span>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace helios::ecs {

namespace details {

/**
 * @brief Returns the index of the first id >= `last_message_count`.
 * @param ids Sorted message ids for one buffer
 * @param last_message_count Cursor position
 * @return Offset into `ids`, or `ids.size()` when all ids are older
 */
template <MessageTrait T>
[[nodiscard]] constexpr size_t MessageUnreadOffset(
    std::span<const AnyMessageId> ids,
    MessageId<T> last_message_count) noexcept {
  const AnyMessageId cursor_pos = last_message_count.ToAny();
  const auto it = std::ranges::lower_bound(ids, cursor_pos);
  return static_cast<size_t>(it - ids.begin());
}

}  // namespace details

/**
 * @brief Bidirectional iterator that yields `MessageWrapper<T>` for unread
 * messages.
 * @details Walks previous then current buffers for ids >=
 * `cursor->last_message_count`. Prefixed `operator++` advances the cursor to
 * `current_id + 1`.
 *
 * Derives from `FunctionalAdapterBase` so the iterator itself can be used as a
 * lazy range, enabling chained adapter calls such as `.Filter(...)`,
 * `.Map(...)`, `.Take(...)`, etc.
 *
 * @note The adapter methods operate on `MessageWrapper<T>` values.
 * @tparam T Message type satisfying `MessageTrait`
 */
template <MessageTrait T>
class MessageWrapperIter
    : public utils::FunctionalAdapterBase<MessageWrapperIter<T>> {
public:
  using iterator_concept = std::bidirectional_iterator_tag;
  using iterator_category = std::input_iterator_tag;
  using value_type = MessageWrapper<T>;
  using reference = value_type;
  using pointer = void;
  using difference_type = ptrdiff_t;

  constexpr MessageWrapperIter() noexcept = default;

  /**
   * @brief Constructs an unread-message wrapper iterator.
   * @param previous_messages Span of messages from the previous frame
   * @param current_messages Span of messages from the current frame
   * @param previous_ids Ids aligned with `previous_messages`
   * @param current_ids Ids aligned with `current_messages`
   * @param cursor Delivery cursor advanced by `operator++` (may be null only
   * for a singular iterator)
   * @param previous_offset First unread index in the previous buffer
   * @param current_offset First unread index in the current buffer
   * @param position Logical position in the combined unread sequence
   */
  constexpr MessageWrapperIter(std::span<const T> previous_messages,
                               std::span<const T> current_messages,
                               std::span<const AnyMessageId> previous_ids,
                               std::span<const AnyMessageId> current_ids,
                               MessageCursor<T>* cursor, size_t previous_offset,
                               size_t current_offset, size_t position) noexcept
      : previous_messages_(previous_messages),
        current_messages_(current_messages),
        previous_ids_(previous_ids),
        current_ids_(current_ids),
        cursor_(cursor),
        previous_offset_(previous_offset),
        current_offset_(current_offset),
        position_(position) {}

  constexpr MessageWrapperIter(const MessageWrapperIter&) noexcept = default;
  constexpr MessageWrapperIter(MessageWrapperIter&&) noexcept = default;
  constexpr ~MessageWrapperIter() noexcept = default;

  constexpr MessageWrapperIter& operator=(const MessageWrapperIter&) noexcept =
      default;
  constexpr MessageWrapperIter& operator=(MessageWrapperIter&&) noexcept =
      default;

  /**
   * @brief Gets the current unread message wrapper.
   * @warning Triggers assertion if the iterator is singular or out of range.
   * @return Message wrapper for the current unread message
   */
  [[nodiscard]] constexpr reference operator*() const noexcept;

  pointer operator->() const = delete;

  /**
   * @brief Advances to the next unread message and updates the cursor.
   * @warning Triggers assertion if the cursor is null or the iterator is at
   * end.
   * @return Reference to this iterator
   */
  constexpr MessageWrapperIter& operator++() noexcept;

  /**
   * @brief Advances to the next unread message and updates the cursor.
   * @warning Triggers assertion if the cursor is null or the iterator is at
   * end.
   * @return Copy of this iterator before advancing
   */
  [[nodiscard]] constexpr MessageWrapperIter operator++(int) noexcept;

  /**
   * @brief Moves to the previous unread message.
   * @warning Triggers assertion if the iterator is at the beginning.
   * @return Reference to this iterator
   */
  constexpr MessageWrapperIter& operator--() noexcept;

  /**
   * @brief Moves to the previous unread message.
   * @warning Triggers assertion if the iterator is at the beginning.
   * @return Copy of this iterator before moving
   */
  [[nodiscard]] constexpr MessageWrapperIter operator--(int) noexcept;

  [[nodiscard]] constexpr auto operator<=>(
      const MessageWrapperIter& other) const noexcept {
    return position_ <=> other.position_;
  }

  [[nodiscard]] constexpr bool operator==(
      const MessageWrapperIter& other) const noexcept {
    return position_ == other.position_;
  }

  [[nodiscard]] constexpr bool operator!=(
      const MessageWrapperIter& other) const noexcept {
    return position_ != other.position_;
  }

  /**
   * @brief Returns the current logical position in the unread sequence.
   * @return Zero-based position index
   */
  [[nodiscard]] constexpr size_t Position() const noexcept { return position_; }

  /**
   * @brief Returns a copy of this iterator positioned at the beginning of the
   * unread snapshot.
   * @details Required so `MessageWrapperIter` can act as a self-contained range
   * for `FunctionalAdapterBase` adapter methods.
   * @return Iterator reset to position 0 over the same unread window
   */
  [[nodiscard]] constexpr MessageWrapperIter begin() const noexcept {
    return {previous_messages_,
            current_messages_,
            previous_ids_,
            current_ids_,
            cursor_,
            previous_offset_,
            current_offset_,
            0};
  }

  /**
   * @brief Returns a copy of this iterator positioned one past the last unread
   * element.
   * @details The end iterator does not advance the cursor.
   * @return Iterator at position equal to the unread count
   */
  [[nodiscard]] constexpr MessageWrapperIter end() const noexcept {
    return {previous_messages_, current_messages_, previous_ids_,
            current_ids_,       cursor_,           previous_offset_,
            current_offset_,    UnreadCount()};
  }

private:
  [[nodiscard]] constexpr const T& MessageAt(size_t position) const noexcept;
  [[nodiscard]] constexpr MessageId<T> IdAt(size_t position) const noexcept;

  [[nodiscard]] constexpr size_t UnreadPreviousCount() const noexcept {
    return previous_ids_.size() - previous_offset_;
  }

  [[nodiscard]] constexpr size_t UnreadCount() const noexcept {
    return UnreadPreviousCount() + (current_ids_.size() - current_offset_);
  }

  std::span<const T> previous_messages_;
  std::span<const T> current_messages_;
  std::span<const AnyMessageId> previous_ids_;
  std::span<const AnyMessageId> current_ids_;
  MessageCursor<T>* cursor_ = nullptr;
  size_t previous_offset_ = 0;
  size_t current_offset_ = 0;
  size_t position_ = 0;
};

template <MessageTrait T>
constexpr auto MessageWrapperIter<T>::operator*() const noexcept -> reference {
  HELIOS_ASSERT(position_ < UnreadCount(),
                "MessageWrapperIter dereference past the end!");
  return {MessageAt(position_), IdAt(position_)};
}

template <MessageTrait T>
constexpr auto MessageWrapperIter<T>::operator++() noexcept
    -> MessageWrapperIter& {
  HELIOS_ASSERT(cursor_ != nullptr, "MessageCursor pointer is null!");
  HELIOS_ASSERT(position_ < UnreadCount(),
                "MessageWrapperIter increment past the end!");
  cursor_->last_message_count = MessageId<T>{IdAt(position_).value + 1};
  ++position_;
  return *this;
}

template <MessageTrait T>
constexpr auto MessageWrapperIter<T>::operator++(int) noexcept
    -> MessageWrapperIter {
  auto copy = *this;
  ++(*this);
  return copy;
}

template <MessageTrait T>
constexpr auto MessageWrapperIter<T>::operator--() noexcept
    -> MessageWrapperIter& {
  --position_;
  return *this;
}

template <MessageTrait T>
constexpr auto MessageWrapperIter<T>::operator--(int) noexcept
    -> MessageWrapperIter {
  auto copy = *this;
  --(*this);
  return copy;
}

template <MessageTrait T>
constexpr const T& MessageWrapperIter<T>::MessageAt(
    size_t position) const noexcept {
  const size_t unread_previous = UnreadPreviousCount();
  if (position < unread_previous) {
    return previous_messages_[previous_offset_ + position];
  }
  return current_messages_[current_offset_ + (position - unread_previous)];
}

template <MessageTrait T>
constexpr MessageId<T> MessageWrapperIter<T>::IdAt(
    size_t position) const noexcept {
  const size_t unread_previous = UnreadPreviousCount();
  if (position < unread_previous) {
    return MessageId<T>::From(previous_ids_[previous_offset_ + position]);
  }
  return MessageId<T>::From(
      current_ids_[current_offset_ + (position - unread_previous)]);
}

/**
 * @brief Bidirectional iterator that yields `ConsumableMessageWrapper<T>` for
 * unread messages with consume support.
 * @details Walks previous then current buffers for ids >=
 * `cursor->last_message_count`. Prefixed `operator++` advances the cursor to
 * `current_id + 1`.
 *
 * Derives from `FunctionalAdapterBase` so the iterator itself can be used as a
 * lazy range, enabling chained adapter calls such as `.Filter(...)`,
 * `.Map(...)`, `.Take(...)`, etc.
 *
 * @note The adapter methods operate on `ConsumableMessageWrapper<T>` values.
 * @tparam T Message type satisfying `ConsumableMessageTrait`
 * @tparam Alloc Allocator type for the consumed messages registry
 */
template <ConsumableMessageTrait T,
          typename Alloc = std::pmr::polymorphic_allocator<std::byte>>
class ConsumableMessageWrapperIter
    : public utils::FunctionalAdapterBase<
          ConsumableMessageWrapperIter<T, Alloc>> {
public:
  using iterator_concept = std::bidirectional_iterator_tag;
  using iterator_category = std::input_iterator_tag;
  using value_type = ConsumableMessageWrapper<T, Alloc>;
  using reference = value_type;
  using pointer = void;
  using difference_type = ptrdiff_t;

  constexpr ConsumableMessageWrapperIter() noexcept = default;

  /**
   * @brief Constructs an unread consumable-message wrapper iterator.
   * @param previous_messages Span of messages from the previous frame
   * @param current_messages Span of messages from the current frame
   * @param previous_ids Ids aligned with `previous_messages`
   * @param current_ids Ids aligned with `current_messages`
   * @param registry Per-system consumed messages registry
   * @param cursor Delivery cursor advanced by `operator++` (may be null only
   * for a singular iterator)
   * @param previous_offset First unread index in the previous buffer
   * @param current_offset First unread index in the current buffer
   * @param position Logical position in the combined unread sequence
   */
  constexpr ConsumableMessageWrapperIter(
      std::span<const T> previous_messages, std::span<const T> current_messages,
      std::span<const AnyMessageId> previous_ids,
      std::span<const AnyMessageId> current_ids,
      ConsumedMessagesRegistry<Alloc>& registry, MessageCursor<T>* cursor,
      size_t previous_offset, size_t current_offset, size_t position) noexcept
      : previous_messages_(previous_messages),
        current_messages_(current_messages),
        previous_ids_(previous_ids),
        current_ids_(current_ids),
        registry_(&registry),
        cursor_(cursor),
        previous_offset_(previous_offset),
        current_offset_(current_offset),
        position_(position) {}

  constexpr ConsumableMessageWrapperIter(
      const ConsumableMessageWrapperIter&) noexcept = default;
  constexpr ConsumableMessageWrapperIter(
      ConsumableMessageWrapperIter&&) noexcept = default;
  constexpr ~ConsumableMessageWrapperIter() noexcept = default;

  constexpr ConsumableMessageWrapperIter& operator=(
      const ConsumableMessageWrapperIter&) noexcept = default;
  constexpr ConsumableMessageWrapperIter& operator=(
      ConsumableMessageWrapperIter&&) noexcept = default;

  /**
   * @brief Gets the current unread message consumable wrapper.
   * @warning Triggers assertion if registry is null or the iterator is out of
   * range.
   * @return Consumable message wrapper for the current unread message
   */
  [[nodiscard]] constexpr reference operator*() const noexcept;

  pointer operator->() const = delete;

  /**
   * @brief Advances to the next unread message and updates the cursor.
   * @warning Triggers assertion if the cursor is null or the iterator is at
   * end.
   * @return Reference to this iterator
   */
  constexpr ConsumableMessageWrapperIter& operator++() noexcept;

  /**
   * @brief Advances to the next unread message and updates the cursor.
   * @warning Triggers assertion if the cursor is null or the iterator is at
   * end.
   * @return Copy of this iterator before advancing
   */
  [[nodiscard]] constexpr ConsumableMessageWrapperIter operator++(int) noexcept;

  /**
   * @brief Moves to the previous unread message.
   * @warning Triggers assertion if the iterator is at the beginning.
   * @return Reference to this iterator
   */
  constexpr ConsumableMessageWrapperIter& operator--() noexcept;

  /**
   * @brief Moves to the previous unread message.
   * @warning Triggers assertion if the iterator is at the beginning.
   * @return Copy of this iterator before moving
   */
  [[nodiscard]] constexpr ConsumableMessageWrapperIter operator--(int) noexcept;

  [[nodiscard]] constexpr auto operator<=>(
      const ConsumableMessageWrapperIter& other) const noexcept {
    return position_ <=> other.position_;
  }

  [[nodiscard]] constexpr bool operator==(
      const ConsumableMessageWrapperIter& other) const noexcept {
    return position_ == other.position_;
  }

  [[nodiscard]] constexpr bool operator!=(
      const ConsumableMessageWrapperIter& other) const noexcept {
    return position_ != other.position_;
  }

  /**
   * @brief Returns the current logical position in the unread sequence.
   * @return Zero-based position index
   */
  [[nodiscard]] constexpr size_t Position() const noexcept { return position_; }

  /**
   * @brief Returns a copy of this iterator positioned at the beginning of the
   * unread snapshot.
   * @details Required so `ConsumableMessageWrapperIter` can act as a
   * self-contained range for `FunctionalAdapterBase` adapter methods.
   * @warning Triggers assertion if consumed messages registry is null.
   * @return Iterator reset to position 0 over the same unread window
   */
  [[nodiscard]] constexpr ConsumableMessageWrapperIter begin() const noexcept;

  /**
   * @brief Returns a copy of this iterator positioned one past the last unread
   * element.
   * @details The end iterator does not advance the cursor.
   * @warning Triggers assertion if consumed messages registry is null.
   * @return Iterator at position equal to the unread count
   */
  [[nodiscard]] constexpr ConsumableMessageWrapperIter end() const noexcept;

private:
  [[nodiscard]] constexpr const T& MessageAt(size_t position) const noexcept;
  [[nodiscard]] constexpr MessageId<T> IdAt(size_t position) const noexcept;

  [[nodiscard]] constexpr size_t UnreadPreviousCount() const noexcept {
    return previous_ids_.size() - previous_offset_;
  }
  [[nodiscard]] constexpr size_t UnreadCount() const noexcept {
    return UnreadPreviousCount() + (current_ids_.size() - current_offset_);
  }

  std::span<const T> previous_messages_;
  std::span<const T> current_messages_;
  std::span<const AnyMessageId> previous_ids_;
  std::span<const AnyMessageId> current_ids_;
  ConsumedMessagesRegistry<Alloc>* registry_ = nullptr;
  MessageCursor<T>* cursor_ = nullptr;
  size_t previous_offset_ = 0;
  size_t current_offset_ = 0;
  size_t position_ = 0;
};

template <ConsumableMessageTrait T, typename Alloc>
constexpr auto ConsumableMessageWrapperIter<T, Alloc>::operator*()
    const noexcept -> reference {
  HELIOS_ASSERT(registry_ != nullptr,
                "ConsumedMessagesRegistry pointer is null!");
  HELIOS_ASSERT(position_ < UnreadCount(),
                "ConsumableMessageWrapperIter dereference past the end!");
  return {MessageAt(position_), *registry_, IdAt(position_)};
}

template <ConsumableMessageTrait T, typename Alloc>
constexpr auto ConsumableMessageWrapperIter<T, Alloc>::operator++() noexcept
    -> ConsumableMessageWrapperIter& {
  HELIOS_ASSERT(cursor_ != nullptr, "MessageCursor pointer is null!");
  HELIOS_ASSERT(position_ < UnreadCount(),
                "ConsumableMessageWrapperIter increment past the end!");
  cursor_->last_message_count = MessageId<T>{IdAt(position_).value + 1};
  ++position_;
  return *this;
}

template <ConsumableMessageTrait T, typename Alloc>
constexpr auto ConsumableMessageWrapperIter<T, Alloc>::operator++(int) noexcept
    -> ConsumableMessageWrapperIter {
  auto copy = *this;
  ++(*this);
  return copy;
}

template <ConsumableMessageTrait T, typename Alloc>
constexpr auto ConsumableMessageWrapperIter<T, Alloc>::operator--() noexcept
    -> ConsumableMessageWrapperIter& {
  --position_;
  return *this;
}

template <ConsumableMessageTrait T, typename Alloc>
constexpr auto ConsumableMessageWrapperIter<T, Alloc>::operator--(int) noexcept
    -> ConsumableMessageWrapperIter {
  auto copy = *this;
  --(*this);
  return copy;
}

template <ConsumableMessageTrait T, typename Alloc>
constexpr auto ConsumableMessageWrapperIter<T, Alloc>::begin() const noexcept
    -> ConsumableMessageWrapperIter {
  HELIOS_ASSERT(registry_ != nullptr,
                "ConsumedMessagesRegistry pointer is null!");
  return {previous_messages_, current_messages_, previous_ids_,
          current_ids_,       *registry_,        cursor_,
          previous_offset_,   current_offset_,   0};
}

template <ConsumableMessageTrait T, typename Alloc>
constexpr auto ConsumableMessageWrapperIter<T, Alloc>::end() const noexcept
    -> ConsumableMessageWrapperIter {
  HELIOS_ASSERT(registry_ != nullptr,
                "ConsumedMessagesRegistry pointer is null!");
  return {previous_messages_, current_messages_, previous_ids_,
          current_ids_,       *registry_,        cursor_,
          previous_offset_,   current_offset_,   UnreadCount()};
}

template <ConsumableMessageTrait T, typename Alloc>
constexpr const T& ConsumableMessageWrapperIter<T, Alloc>::MessageAt(
    size_t position) const noexcept {
  const size_t unread_previous = UnreadPreviousCount();
  if (position < unread_previous) {
    return previous_messages_[previous_offset_ + position];
  }
  return current_messages_[current_offset_ + (position - unread_previous)];
}

template <ConsumableMessageTrait T, typename Alloc>
constexpr MessageId<T> ConsumableMessageWrapperIter<T, Alloc>::IdAt(
    size_t position) const noexcept {
  const size_t unread_previous = UnreadPreviousCount();
  if (position < unread_previous) {
    return MessageId<T>::From(previous_ids_[previous_offset_ + position]);
  }
  return MessageId<T>::From(
      current_ids_[current_offset_ + (position - unread_previous)]);
}

/**
 * @brief CRTP base class for message readers providing common functionality.
 * @details This base class contains all reader methods except consumption
 * methods (`ConsumeAll` and `ConsumeIf`) which are only available on
 * `ConsumableMessageReader`.
 *
 * @tparam Derived The derived class (CRTP)
 * @tparam T Message type
 * @tparam IterType The iterator type for this reader
 */
template <typename Derived, MessageTrait T, typename IterType>
class MessageReaderBase {
public:
  using value_type = typename IterType::value_type;
  using size_type = MessageManager::size_type;
  using iterator = IterType;
  using const_iterator = IterType;

  constexpr MessageReaderBase() noexcept = default;
  MessageReaderBase(const MessageReaderBase&) = delete;
  constexpr MessageReaderBase(MessageReaderBase&&) noexcept = default;
  constexpr ~MessageReaderBase() noexcept = default;

  MessageReaderBase& operator=(const MessageReaderBase&) = delete;
  constexpr MessageReaderBase& operator=(MessageReaderBase&&) noexcept =
      default;

  /**
   * @brief Applies an action to every unread message wrapper.
   * @tparam Action Callable type `(const value_type&) -> void`
   * @param action Action to apply
   */
  template <typename Action>
    requires std::invocable<Action, const value_type&>
  constexpr void ForEach(const Action& action) const {
    return GetDerived().begin().ForEach(action);
  }

  /**
   * @brief Collects all unread messages into a vector of raw `T` values.
   * @note This shadows `FunctionalAdapterBase::Collect()`. To collect
   * wrapper values, chain a lazy adapter first (e.g.,
   * `reader.Filter(...).Collect()`). Iteration advances the delivery cursor.
   * @return Vector containing copies of all unread messages
   */
  [[nodiscard]] constexpr auto Collect() const -> std::vector<T>;

  /**
   * @brief Collects all unread messages into a vector of raw `T` values using a
   * custom allocator.
   * @tparam Alloc STL-compatible allocator type for `T`
   * @param alloc Allocator instance
   * @return Vector containing copies of all unread messages, using the provided
   * allocator
   */
  template <typename Alloc>
    requires std::same_as<typename std::allocator_traits<Alloc>::value_type, T>
  [[nodiscard]] constexpr auto CollectWith(const Alloc& alloc) const
      -> std::vector<T, Alloc>;

  /**
   * @brief Collects all unread messages into a vector of raw `T` values using a
   * memory resource.
   * @param resource Memory resource for allocating the vector
   * @return Vector containing copies of all unread messages, using the provided
   * memory resource
   */
  [[nodiscard]] constexpr auto CollectWith(
      std::pmr::memory_resource* resource) const -> std::pmr::vector<T>;

  auto CollectWith(std::nullptr_t) const -> std::pmr::vector<T> = delete;

  /**
   * @brief Reads all unread messages into an output iterator.
   * @details Advances the delivery cursor as messages are copied.
   * @tparam OutIt Output iterator type for `T`
   * @param out Output iterator
   */
  template <typename OutIt>
    requires std::output_iterator<OutIt, T>
  constexpr void ReadInto(OutIt out) const;

  /**
   * @brief Returns a lazy filter adapter over the unread wrapped message
   * sequence.
   * @details The adapter yields only those wrapper elements for
   * which `predicate` returns `true`. Chain further adapters or call terminal
   * operations (`.Collect()`, `.ForEach()`, etc.) on the result.
   * @tparam Pred Predicate type `(const value_type&) -> bool`
   * @param predicate Filter predicate
   * @return `FilterAdapter` over the iterator
   */
  template <typename Pred>
    requires std::predicate<Pred, const value_type&>
  [[nodiscard]] constexpr auto Filter(Pred predicate) const
      noexcept(noexcept(GetDerived().begin().Filter(std::move(predicate))))
          -> utils::FilterAdapter<iterator, Pred> {
    return GetDerived().begin().Filter(std::move(predicate));
  }

  /**
   * @brief Returns a lazy map adapter over the unread wrapped message sequence.
   * @details Each wrapper is transformed by `transform`.
   * @tparam Func Transform type `(const value_type&) -> R`
   * @param transform Transformation function
   * @return `MapAdapter` over the iterator
   */
  template <typename Func>
    requires std::invocable<Func, const value_type&>
  [[nodiscard]] constexpr auto Map(Func transform) const
      noexcept(noexcept(GetDerived().begin().Map(std::move(transform))))
          -> utils::MapAdapter<iterator, Func> {
    return GetDerived().begin().Map(std::move(transform));
  }

  /**
   * @brief Returns a lazy adapter yielding at most `count` unread wrappers.
   * @param count Maximum number of elements to yield
   * @return `TakeAdapter` over the iterator
   */
  [[nodiscard]] constexpr auto Take(size_t count) const
      noexcept(noexcept(GetDerived().begin().Take(count)))
          -> utils::TakeAdapter<iterator> {
    return GetDerived().begin().Take(count);
  }

  /**
   * @brief Returns a lazy adapter that skips the first `count` unread wrappers.
   * @param count Number of elements to skip
   * @return `SkipAdapter` over the iterator
   */
  [[nodiscard]] constexpr auto Skip(size_t count) const
      noexcept(noexcept(GetDerived().begin().Skip(count)))
          -> utils::SkipAdapter<iterator> {
    return GetDerived().begin().Skip(count);
  }

  /**
   * @brief Returns a lazy adapter that yields unread wrappers while `predicate`
   * is true.
   * @tparam Pred Predicate type `(const value_type&) -> bool`
   * @param predicate Stop condition
   * @return `TakeWhileAdapter` over the iterator
   */
  template <typename Pred>
    requires std::predicate<Pred, const value_type&>
  [[nodiscard]] constexpr auto TakeWhile(Pred predicate) const
      noexcept(noexcept(GetDerived().begin().TakeWhile(std::move(predicate))))
          -> utils::TakeWhileAdapter<iterator, Pred> {
    return GetDerived().begin().TakeWhile(std::move(predicate));
  }

  /**
   * @brief Returns a lazy adapter that skips unread wrappers while `predicate`
   * is true.
   * @tparam Pred Predicate type `(const value_type&) -> bool`
   * @param predicate Skip condition
   * @return `SkipWhileAdapter` over the iterator
   */
  template <typename Pred>
    requires std::predicate<Pred, const value_type&>
  [[nodiscard]] constexpr auto SkipWhile(Pred predicate) const
      noexcept(noexcept(GetDerived().begin().SkipWhile(std::move(predicate))))
          -> utils::SkipWhileAdapter<iterator, Pred> {
    return GetDerived().begin().SkipWhile(std::move(predicate));
  }

  /**
   * @brief Returns a lazy adapter that pairs each unread wrapper with its
   * zero-based index.
   * @return `EnumerateAdapter` over the iterator
   */
  [[nodiscard]] constexpr auto Enumerate() const
      noexcept(noexcept(GetDerived().begin().Enumerate()))
          -> utils::EnumerateAdapter<iterator> {
    return GetDerived().begin().Enumerate();
  }

  /**
   * @brief Returns a lazy adapter that calls `inspector` on each unread wrapper
   * as a side-effect.
   * @tparam Func Inspector type `(const value_type&) -> void`
   * @param inspector Side-effect function
   * @return `InspectAdapter` over the iterator
   */
  template <typename Func>
    requires std::invocable<Func, const value_type&>
  [[nodiscard]] constexpr auto Inspect(Func inspector) const
      noexcept(noexcept(GetDerived().begin().Inspect(std::move(inspector))))
          -> utils::InspectAdapter<iterator, Func> {
    return GetDerived().begin().Inspect(std::move(inspector));
  }

  /**
   * @brief Returns a lazy adapter that yields every `step`-th unread wrapper.
   * @param step Step size (must be > 0)
   * @warning Triggers assertion if `step == 0`.
   * @return `StepByAdapter` over the iterator
   */
  [[nodiscard]] constexpr auto StepBy(size_t step) const
      noexcept(noexcept(GetDerived().begin().StepBy(step)))
          -> utils::StepByAdapter<iterator> {
    return GetDerived().begin().StepBy(step);
  }

  /**
   * @brief Returns a lazy adapter that chains another range of wrappers.
   * @tparam Iter Iterator type of the range
   * @param first Iterator to the first element of the range
   * @param last Iterator to the end of the range
   * @return `ChainAdapter` over the iterator
   */
  template <typename Iter>
    requires utils::ChainAdapterRequirements<iterator, Iter>
  [[nodiscard]] constexpr auto Chain(Iter first, Iter last) const
      noexcept(noexcept(GetDerived().begin().Chain(std::move(first),
                                                   std::move(last))))
          -> utils::ChainAdapter<iterator, Iter> {
    return GetDerived().begin().Chain(std::move(first), std::move(last));
  }

  /**
   * @brief Returns a lazy adapter that chains another range of wrappers.
   * @tparam R Range type
   * @param range Range to chain
   * @return `ChainAdapter` over the iterator
   */
  template <std::ranges::input_range R>
    requires utils::ChainAdapterRequirements<iterator,
                                             std::ranges::iterator_t<R>>
  [[nodiscard]] constexpr auto Chain(const R& range) const
      noexcept(noexcept(GetDerived().begin().Chain(range)))
          -> utils::ChainAdapter<iterator, std::ranges::iterator_t<R>> {
    return GetDerived().begin().Chain(range);
  }

  /**
   * @brief Returns a lazy adapter that yields unread wrappers in reverse order.
   * @return `ReverseAdapter` over the iterator
   */
  [[nodiscard]] constexpr auto Reverse() const
      noexcept(noexcept(GetDerived().begin().Reverse()))
          -> utils::ReverseAdapter<iterator> {
    return GetDerived().begin().Reverse();
  }

  /**
   * @brief Returns a lazy adapter that yields windows of `window_size`
   * unread wrappers.
   * @param window_size Number of wrappers to include in each window
   * @return `SlideAdapter` over the iterator
   */
  [[nodiscard]] constexpr auto Slide(size_t window_size) const
      noexcept(noexcept(GetDerived().begin().Slide(window_size)))
          -> utils::SlideAdapter<iterator> {
    return GetDerived().begin().Slide(window_size);
  }

  /**
   * @brief Returns a lazy adapter that yields every `stride`-th unread wrapper.
   * @param stride Number of wrappers to skip between each yield
   * @return `StrideAdapter` over the iterator
   */
  [[nodiscard]] constexpr auto Stride(size_t stride) const
      noexcept(noexcept(GetDerived().begin().Stride(stride)))
          -> utils::StrideAdapter<iterator> {
    return GetDerived().begin().Stride(stride);
  }

  /**
   * @brief Returns a lazy adapter that yields pairs of wrappers from two
   * ranges.
   * @tparam Iter Type of the iterator for the second range
   * @param first Iterator to the first element of the second range
   * @param last Iterator to the end of the second range
   * @return `ZipAdapter` over the iterator
   */
  template <typename Iter>
    requires utils::ZipAdapterRequirements<iterator, Iter>
  [[nodiscard]] constexpr auto Zip(Iter first, Iter last) const
      noexcept(noexcept(GetDerived().begin().Zip(std::move(first),
                                                 std::move(last))))
          -> utils::ZipAdapter<iterator, Iter> {
    return GetDerived().begin().Zip(std::move(first), std::move(last));
  }

  /**
   * @brief Returns a lazy adapter that yields pairs of wrappers from two
   * ranges.
   * @tparam R Type of the range for the second range
   * @param range The range to zip with the first range
   * @return `ZipAdapter` over the iterator
   */
  template <typename R>
    requires utils::ZipAdapterRequirements<iterator, std::ranges::iterator_t<R>>
  [[nodiscard]] constexpr auto Zip(const R& range) const
      noexcept(noexcept(GetDerived().begin().Zip(range)))
          -> utils::ZipAdapter<iterator, std::ranges::iterator_t<R>> {
    return GetDerived().begin().Zip(range);
  }

  /**
   * @brief Left-folds all unread messages with an accumulator.
   * @tparam Acc Accumulator type
   * @tparam Folder Callable type `(Acc, const value_type&) -> Acc`
   * @param init Initial accumulator value
   * @param folder Folding function
   * @return Final accumulated value
   */
  template <typename Acc, typename Folder>
    requires std::invocable<Folder, Acc, const value_type&>
  [[nodiscard]] constexpr Acc Fold(Acc init, const Folder& folder) const {
    return GetDerived().begin().Fold(init, folder);
  }

  /**
   * @brief Returns a pointer to the first unread message matching a predicate.
   * @tparam Pred Predicate type `(const value_type&) -> bool`
   * @param predicate Predicate function
   * @return Optional containing the first matching message, or `std::nullopt`
   * if none found
   */
  template <typename Pred>
    requires std::predicate<Pred, const value_type&>
  [[nodiscard]] constexpr auto Find(const Pred& predicate) const
      -> std::optional<value_type> {
    return GetDerived().begin().Find(predicate);
  }

  /**
   * @brief Counts unread messages matching a predicate.
   * @tparam Pred Predicate type `(const value_type&) -> bool`
   * @param predicate Predicate function
   * @return Number of matching messages
   */
  template <typename Pred>
    requires std::predicate<Pred, const value_type&>
  [[nodiscard]] constexpr size_type CountIf(const Pred& predicate) const {
    return GetDerived().begin().CountIf(predicate);
  }

  /**
   * @brief Partitions unread wrapped messages into matching and non-matching
   * groups.
   * @tparam Pred Predicate type `(const value_type&) -> bool`
   * @param predicate Predicate function
   * @return Pair of vectors: first contains matching wrappers, second contains
   * non-matching wrappers
   */
  template <typename Pred>
    requires std::predicate<Pred, const value_type&>
  [[nodiscard]] constexpr auto Partition(const Pred& predicate) const
      -> std::pair<std::vector<value_type>, std::vector<value_type>> {
    return GetDerived().begin().Partition(predicate);
  }

  /**
   * @brief Finds the unread wrapped message with the maximum extracted key.
   * @tparam KeyFunc Key extractor type `(const value_type&) -> Key`
   * @param key_func Key extraction function
   * @return Matching wrapper, or `std::nullopt` if the reader is empty
   */
  template <typename KeyFunc>
    requires std::invocable<KeyFunc, const value_type&>
  [[nodiscard]] constexpr auto MaxBy(const KeyFunc& key_func) const
      -> std::optional<value_type> {
    return GetDerived().begin().MaxBy(key_func);
  }

  /**
   * @brief Finds the unread wrapped message with the minimum extracted key.
   * @tparam KeyFunc Key extractor type `(const value_type&) -> Key`
   * @param key_func Key extraction function
   * @return Matching wrapper, or `std::nullopt` if the reader is empty
   */
  template <typename KeyFunc>
    requires std::invocable<KeyFunc, const value_type&>
  [[nodiscard]] constexpr auto MinBy(const KeyFunc& key_func) const
      -> std::optional<value_type> {
    return GetDerived().begin().MinBy(key_func);
  }

  /**
   * @brief Groups unread wrapped messages by a key extracted from each wrapper.
   * @tparam KeyFunc Key extractor type `(const value_type&) -> Key`
   * @param key_func Key extraction function
   * @return Hash map from key to grouped wrappers
   */
  template <typename KeyFunc>
    requires std::invocable<KeyFunc, const value_type&>
  [[nodiscard]] constexpr auto GroupBy(const KeyFunc& key_func) const
      -> std::unordered_map<
          std::decay_t<std::invoke_result_t<KeyFunc, const value_type&>>,
          std::vector<value_type>> {
    return GetDerived().begin().GroupBy(key_func);
  }

  /**
   * @brief Checks if any unread message matches a predicate.
   * @tparam Pred Predicate type `(const value_type&) -> bool`
   * @param predicate Predicate function
   * @return `true` if at least one message matches
   */
  template <typename Pred>
    requires std::predicate<Pred, const value_type&>
  [[nodiscard]] constexpr bool Any(const Pred& predicate) const {
    return GetDerived().begin().Any(predicate);
  }

  /**
   * @brief Checks if all unread messages match a predicate.
   * @tparam Pred Predicate type `(const value_type&) -> bool`
   * @param predicate Predicate function
   * @return `true` if all messages match (vacuously true if empty)
   */
  template <typename Pred>
    requires std::predicate<Pred, const value_type&>
  [[nodiscard]] constexpr bool All(const Pred& predicate) const {
    return GetDerived().begin().All(predicate);
  }

  /**
   * @brief Checks if no unread messages match a predicate.
   * @tparam Pred Predicate type `(const value_type&) -> bool`
   * @param predicate Predicate function
   * @return `true` if no messages match (vacuously true if empty)
   */
  template <typename Pred>
    requires std::predicate<Pred, const value_type&>
  [[nodiscard]] constexpr bool None(const Pred& predicate) const {
    return GetDerived().begin().None(predicate);
  }

  /**
   * @brief Checks if there are no unread messages.
   * @return `true` if the delivery cursor has no unread retained messages
   */
  [[nodiscard]] constexpr bool Empty() const noexcept {
    return GetDerived().Empty();
  }

  /**
   * @brief Returns the number of unread retained messages.
   * @return Unread message count
   */
  [[nodiscard]] constexpr size_type Count() const noexcept {
    return GetDerived().Count();
  }

private:
  [[nodiscard]] constexpr Derived& GetDerived() noexcept {
    return static_cast<Derived&>(*this);
  }

  [[nodiscard]] constexpr const Derived& GetDerived() const noexcept {
    return static_cast<const Derived&>(*this);
  }
};

template <typename Derived, MessageTrait T, typename IterType>
constexpr auto MessageReaderBase<Derived, T, IterType>::Collect() const
    -> std::vector<T> {
  std::vector<T> result;
  result.reserve(Count());
  auto it = GetDerived().begin();
  const auto last = GetDerived().end();
  for (; it != last; ++it) {
    result.push_back(**it);
  }
  return result;
}

template <typename Derived, MessageTrait T, typename IterType>
template <typename Alloc>
  requires std::same_as<typename std::allocator_traits<Alloc>::value_type, T>
constexpr auto MessageReaderBase<Derived, T, IterType>::CollectWith(
    const Alloc& alloc) const -> std::vector<T, Alloc> {
  std::vector<T, Alloc> result{alloc};
  result.reserve(Count());
  auto it = GetDerived().begin();
  const auto last = GetDerived().end();
  for (; it != last; ++it) {
    result.push_back(**it);
  }
  return result;
}

template <typename Derived, MessageTrait T, typename IterType>
constexpr auto MessageReaderBase<Derived, T, IterType>::CollectWith(
    std::pmr::memory_resource* resource) const -> std::pmr::vector<T> {
  std::pmr::vector<T> result{resource};
  result.reserve(Count());
  auto it = GetDerived().begin();
  const auto last = GetDerived().end();
  for (; it != last; ++it) {
    result.push_back(**it);
  }
  return result;
}

template <typename Derived, MessageTrait T, typename IterType>
template <typename OutIt>
  requires std::output_iterator<OutIt, T>
constexpr void MessageReaderBase<Derived, T, IterType>::ReadInto(
    OutIt out) const {
  auto it = GetDerived().begin();
  const auto last = GetDerived().end();
  for (; it != last; ++it) {
    *out++ = **it;
  }
}

/**
 * @brief Type-safe, zero-copy reader for regular messages with cursor delivery.
 * @details Provides a read-only view over unread messages (id >=
 * `cursor.last_message_count`) across the previous and current queues. Messages
 * are accessed via spans directly into the underlying `TypedBuffer` storage —
 * no copying is performed.
 *
 * Iteration and `Read()` advance the cursor so each message is observed at most
 * once per cursor. Use `MessageManager::PreviousMessages` /
 * `CurrentMessages` to inspect retained buffers without delivery tracking.
 *
 * @note Thread-safe for concurrent reads of retained messages; cursor updates
 * are not synchronized.
 * @tparam T Message type satisfying `MessageTrait`
 *
 * @code
 * auto cursor = MessageCursor<Damage>::IncludeBacklog();
 * MessageReader<Damage> reader(manager, cursor);
 * for (auto wrapper : reader.Read()) {
 *   // Use wrapper; cursor advances
 * }
 * @endcode
 */
template <MessageTrait T>
  requires(!ConsumableMessageTrait<T>)
class MessageReader final
    : public MessageReaderBase<MessageReader<T>, T, MessageWrapperIter<T>> {
public:
  using value_type = MessageWrapper<T>;
  using size_type = MessageManager::size_type;
  using const_iterator = MessageWrapperIter<T>;
  using iterator = const_iterator;

  /**
   * @brief Constructs a `MessageReader` bound to a manager and delivery cursor.
   * @param manager Const reference to the message manager
   * @param cursor Per-reader cursor tracking which messages have been seen
   */
  constexpr MessageReader(const MessageManager& manager,
                          MessageCursor<T>& cursor) noexcept
      : manager_(manager), cursor_(cursor) {}

  MessageReader(const MessageReader&) = delete;
  constexpr MessageReader(MessageReader&&) noexcept = default;
  constexpr ~MessageReader() noexcept = default;

  MessageReader& operator=(const MessageReader&) = delete;
  constexpr MessageReader& operator=(MessageReader&&) noexcept = default;

  /**
   * @brief Returns an iterator range over unread messages.
   * @details Equivalent to `begin()`; provided for an explicit read API.
   * @return Iterator to the first unread message
   */
  [[nodiscard]] constexpr iterator Read() const noexcept { return begin(); }

  /**
   * @brief Skips all currently retained unread messages for this cursor.
   * @details After `Clear()`, `Empty()` is true until new messages are written.
   */
  constexpr void Clear() const noexcept { cursor_.get().Clear(manager_.get()); }

  /**
   * @brief Returns how many retained messages were dropped before this cursor
   * caught up.
   * @return Count of ids older than the oldest retained message
   */
  [[nodiscard]] constexpr size_type MissedMessages() const noexcept {
    return cursor_.get().MissedMessages(manager_.get());
  }

  /**
   * @brief Returns the number of unread retained messages.
   * @return Unread message count
   */
  [[nodiscard]] constexpr size_type Count() const noexcept {
    return cursor_.get().Count(manager_.get());
  }

  /**
   * @brief Checks if there are no unread messages.
   * @return `true` when `Count()` is zero
   */
  [[nodiscard]] constexpr bool Empty() const noexcept {
    return cursor_.get().Empty(manager_.get());
  }

  /**
   * @brief Returns an iterator to the first unread message.
   * @return Iterator to the beginning of the unread range
   */
  [[nodiscard]] constexpr const_iterator begin() const noexcept {
    return MakeIterator(0);
  }

  /**
   * @brief Returns an iterator past the last unread message.
   * @details Does not advance the cursor.
   * @return Iterator to the end of the unread range
   */
  [[nodiscard]] constexpr const_iterator end() const noexcept {
    return MakeIterator(0).end();
  }

private:
  [[nodiscard]] constexpr const_iterator MakeIterator(
      size_t position) const noexcept;

  std::reference_wrapper<const MessageManager> manager_;
  std::reference_wrapper<MessageCursor<T>> cursor_;
};

template <MessageTrait T>
  requires(!ConsumableMessageTrait<T>)
constexpr auto MessageReader<T>::MakeIterator(size_t position) const noexcept
    -> const_iterator {
  const auto& manager = manager_.get();
  const auto previous_messages = manager.PreviousMessages<T>();
  const auto current_messages = manager.CurrentMessages<T>();
  const auto previous_ids = manager.PreviousIds<T>();
  const auto current_ids = manager.CurrentIds<T>();
  const MessageId<T> last = cursor_.get().last_message_count;
  return {previous_messages,
          current_messages,
          previous_ids,
          current_ids,
          &cursor_.get(),
          details::MessageUnreadOffset(previous_ids, last),
          details::MessageUnreadOffset(current_ids, last),
          position};
}

/**
 * @brief Type-safe, zero-copy reader for consumable messages with cursor
 * delivery and consume support.
 * @details Provides a read-only view over unread messages (id >=
 * `cursor.last_message_count`) across the previous and current queues. Messages
 * are accessed via spans directly into the underlying `TypedBuffer` storage —
 * no copying is performed.
 *
 * Consumed messages are not filtered during iteration within the same frame.
 * They are removed at the next `MessageManager::Update` call. Consumption is
 * tracked by stable message id.
 *
 * @note Thread-safe for concurrent reads of retained messages, but
 * `ConsumableMessageWrapper::Consume()` and cursor updates are NOT thread-safe.
 * @tparam T Message type satisfying `ConsumableMessageTrait`
 * @tparam Alloc Allocator type for the consumed messages registry
 *
 * @code
 * MessageCursor<Score> cursor = MessageCursor<Score>::IncludeBacklog();
 * ConsumableMessageReader<Score> reader(manager, cursor, registry);
 * for (auto wrapper : reader.Read()) {
 *   if ((*wrapper).value > 5) {
 *     wrapper.Consume();
 *   }
 * }
 * @endcode
 */
template <ConsumableMessageTrait T,
          typename Alloc = std::pmr::polymorphic_allocator<std::byte>>
class ConsumableMessageReader final
    : public MessageReaderBase<ConsumableMessageReader<T, Alloc>, T,
                               ConsumableMessageWrapperIter<T, Alloc>> {
public:
  using value_type = ConsumableMessageWrapper<T, Alloc>;
  using size_type = MessageManager::size_type;
  using const_iterator = ConsumableMessageWrapperIter<T, Alloc>;
  using iterator = const_iterator;

  /**
   * @brief Constructs a `ConsumableMessageReader` bound to a manager, cursor,
   * and consumed registry.
   * @param manager Const reference to the message manager
   * @param cursor Per-reader cursor tracking which messages have been seen
   * @param consumed_registry Mutable reference to the per-system consumed
   * messages registry
   */
  constexpr ConsumableMessageReader(
      const MessageManager& manager, MessageCursor<T>& cursor,
      ConsumedMessagesRegistry<Alloc>& consumed_registry) noexcept
      : manager_(manager), cursor_(cursor), registry_(consumed_registry) {}

  ConsumableMessageReader(const ConsumableMessageReader&) = delete;
  constexpr ConsumableMessageReader(ConsumableMessageReader&&) noexcept =
      default;
  constexpr ~ConsumableMessageReader() noexcept = default;

  ConsumableMessageReader& operator=(const ConsumableMessageReader&) = delete;
  constexpr ConsumableMessageReader& operator=(
      ConsumableMessageReader&&) noexcept = default;

  /**
   * @brief Returns an iterator range over unread messages.
   * @details Equivalent to `begin()`; provided for an explicit read API.
   * @return Iterator to the first unread message
   */
  [[nodiscard]] constexpr iterator Read() const noexcept { return begin(); }

  /// @brief Marks all unread messages as consumed by message id.
  constexpr void ConsumeAll() const;

  /**
   * @brief Marks all unread messages matching a predicate as consumed.
   * @tparam Pred Predicate type `(const T&) -> bool`
   * @param predicate Predicate function
   * @return Number of messages marked as consumed
   */
  template <typename Pred>
    requires std::predicate<Pred, const T&>
  constexpr size_type ConsumeIf(const Pred& predicate) const;

  /**
   * @brief Skips all currently retained unread messages for this cursor.
   * @details After `Clear()`, `Empty()` is true until new messages are written.
   * Does not mark messages as consumed.
   */
  constexpr void Clear() const noexcept { cursor_.get().Clear(manager_.get()); }

  /**
   * @brief Returns how many retained messages were dropped before this cursor
   * caught up.
   * @return Count of ids older than the oldest retained message
   */
  [[nodiscard]] constexpr size_type MissedMessages() const noexcept {
    return cursor_.get().MissedMessages(manager_.get());
  }

  /**
   * @brief Returns the number of unread retained messages.
   * @return Unread message count
   */
  [[nodiscard]] constexpr size_type Count() const noexcept {
    return cursor_.get().Count(manager_.get());
  }

  /**
   * @brief Checks if there are no unread messages.
   * @return `true` when `Count()` is zero
   */
  [[nodiscard]] constexpr bool Empty() const noexcept {
    return cursor_.get().Empty(manager_.get());
  }

  /**
   * @brief Returns an iterator to the first unread message.
   * @return Iterator to the beginning of the unread range
   */
  [[nodiscard]] constexpr const_iterator begin() const noexcept {
    return MakeIterator(0);
  }

  /**
   * @brief Returns an iterator past the last unread message.
   * @details Does not advance the cursor.
   * @return Iterator to the end of the unread range
   */
  [[nodiscard]] constexpr const_iterator end() const noexcept {
    return MakeIterator(0).end();
  }

private:
  [[nodiscard]] constexpr const_iterator MakeIterator(
      size_t position) const noexcept;

  std::reference_wrapper<const MessageManager> manager_;
  std::reference_wrapper<MessageCursor<T>> cursor_;
  std::reference_wrapper<ConsumedMessagesRegistry<Alloc>> registry_;
};

template <ConsumableMessageTrait T, typename Alloc>
constexpr void ConsumableMessageReader<T, Alloc>::ConsumeAll() const {
  auto& registry = registry_.get();
  auto it = begin();
  const auto last = end();
  for (; it != last; ++it) {
    registry.template MarkConsumed<T>((*it).Id());
  }
}

template <ConsumableMessageTrait T, typename Alloc>
template <typename Pred>
  requires std::predicate<Pred, const T&>
constexpr auto ConsumableMessageReader<T, Alloc>::ConsumeIf(
    const Pred& predicate) const -> size_type {
  size_type consumed_count = 0;
  auto& registry = registry_.get();
  auto it = begin();
  const auto last = end();
  for (; it != last; ++it) {
    if (predicate(**it)) {
      registry.template MarkConsumed<T>((*it).Id());
      ++consumed_count;
    }
  }
  return consumed_count;
}

template <ConsumableMessageTrait T, typename Alloc>
constexpr auto ConsumableMessageReader<T, Alloc>::MakeIterator(
    size_t position) const noexcept -> const_iterator {
  const auto& manager = manager_.get();
  const auto previous_messages = manager.PreviousMessages<T>();
  const auto current_messages = manager.CurrentMessages<T>();
  const auto previous_ids = manager.PreviousIds<T>();
  const auto current_ids = manager.CurrentIds<T>();
  const MessageId<T> last = cursor_.get().last_message_count;
  return {previous_messages,
          current_messages,
          previous_ids,
          current_ids,
          registry_.get(),
          &cursor_.get(),
          details::MessageUnreadOffset(previous_ids, last),
          details::MessageUnreadOffset(current_ids, last),
          position};
}

}  // namespace helios::ecs
