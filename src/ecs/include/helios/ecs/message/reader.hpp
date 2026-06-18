#pragma once

#include <helios/assert.hpp>
#include <helios/ecs/message/consumed_registry.hpp>
#include <helios/ecs/message/manager.hpp>
#include <helios/ecs/message/message.hpp>
#include <helios/ecs/message/wrapper.hpp>
#include <helios/utils/functional_adapters.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <memory_resource>
#include <optional>
#include <span>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace helios::ecs {

/**
 * @brief Bidirectional iterator that yields `MessageWrapper<T>` instances.
 * @details Each wrapper carries a const reference to the message and the
 * global index, enabling the caller to access the message during iteration.
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
   * @brief Constructs a wrapper iterator over two contiguous message spans.
   * @param first Span of messages from the previous frame
   * @param second Span of messages from the current frame
   * @param position Logical position in the combined [first | second] sequence
   */
  constexpr MessageWrapperIter(std::span<const T> first,
                               std::span<const T> second,
                               size_t position) noexcept
      : first_(first), second_(second), position_(position) {}

  constexpr MessageWrapperIter(const MessageWrapperIter&) noexcept = default;
  constexpr MessageWrapperIter(MessageWrapperIter&&) noexcept = default;
  constexpr ~MessageWrapperIter() noexcept = default;

  constexpr MessageWrapperIter& operator=(const MessageWrapperIter&) noexcept =
      default;
  constexpr MessageWrapperIter& operator=(MessageWrapperIter&&) noexcept =
      default;

  [[nodiscard]] constexpr reference operator*() const noexcept {
    const T& msg = (position_ < first_.size())
                       ? first_[position_]
                       : second_[position_ - first_.size()];
    return {msg, position_};
  }

  pointer operator->() const = delete;

  constexpr MessageWrapperIter& operator++() noexcept {
    ++position_;
    return *this;
  }

  [[nodiscard]] constexpr MessageWrapperIter operator++(int) noexcept {
    auto copy = *this;
    ++(*this);
    return copy;
  }

  constexpr MessageWrapperIter& operator--() noexcept {
    --position_;
    return *this;
  }

  [[nodiscard]] constexpr MessageWrapperIter operator--(int) noexcept {
    auto copy = *this;
    --(*this);
    return copy;
  }

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
   * @brief Returns the current logical position in the combined sequence.
   * @return Zero-based position index
   */
  [[nodiscard]] constexpr size_t Position() const noexcept { return position_; }

  /**
   * @brief Returns a copy of this iterator positioned at the beginning of the
   * sequence.
   * @details Required so `MessageWrapperIter` can act as a self-contained range
   * for `FunctionalAdapterBase` adapter methods.
   * @return Iterator reset to position 0
   */
  [[nodiscard]] constexpr MessageWrapperIter begin() const noexcept {
    return {first_, second_, 0};
  }

  /**
   * @brief Returns a copy of this iterator positioned one past the last
   * element.
   * @return Iterator at position `first_.size() + second_.size()`
   */
  [[nodiscard]] constexpr MessageWrapperIter end() const noexcept {
    return {first_, second_, first_.size() + second_.size()};
  }

private:
  std::span<const T> first_;
  std::span<const T> second_;
  size_t position_ = 0;
};

/**
 * @brief Bidirectional iterator that yields `ConsumableMessageWrapper<T>`
 * instances with consume support.
 * @details Each wrapper carries a const reference to the message, a reference
 * to `ConsumedMessagesRegistry`, and the global index, enabling the caller to
 * mark individual messages as consumed during iteration.
 *
 * Derives from `FunctionalAdapterBase` so the iterator itself can be used as a
 * lazy range, enabling chained adapter calls such as `.Filter(...)`,
 * `.Map(...)`, `.Take(...)`, etc.
 *
 * @note The adapter methods operate on `ConsumableMessageWrapper<T>` values.
 * @tparam T Message type satisfying `ConsumableMessageTrait`
 */
template <ConsumableMessageTrait T>
class ConsumableMessageWrapperIter
    : public utils::FunctionalAdapterBase<ConsumableMessageWrapperIter<T>> {
public:
  using iterator_concept = std::bidirectional_iterator_tag;
  using iterator_category = std::input_iterator_tag;
  using value_type = ConsumableMessageWrapper<T>;
  using reference = value_type;
  using pointer = void;
  using difference_type = ptrdiff_t;

  constexpr ConsumableMessageWrapperIter() noexcept = default;

  /**
   * @brief Constructs a wrapper iterator over two contiguous message spans.
   * @param first Span of messages from the previous frame
   * @param second Span of messages from the current frame
   * @param registry Per-system consumed messages registry
   * @param position Logical position in the combined [first | second] sequence
   */
  constexpr ConsumableMessageWrapperIter(
      std::span<const T> first, std::span<const T> second,
      std::reference_wrapper<ConsumedMessagesRegistry<>> registry,
      size_t position) noexcept
      : first_(first),
        second_(second),
        position_(position),
        registry_(&registry.get()) {}

  constexpr ConsumableMessageWrapperIter(
      const ConsumableMessageWrapperIter&) noexcept = default;
  constexpr ConsumableMessageWrapperIter(
      ConsumableMessageWrapperIter&&) noexcept = default;
  constexpr ~ConsumableMessageWrapperIter() noexcept = default;

  constexpr ConsumableMessageWrapperIter& operator=(
      const ConsumableMessageWrapperIter&) noexcept = default;
  constexpr ConsumableMessageWrapperIter& operator=(
      ConsumableMessageWrapperIter&&) noexcept = default;

  [[nodiscard]] constexpr reference operator*() const noexcept {
    const T& msg = (position_ < first_.size())
                       ? first_[position_]
                       : second_[position_ - first_.size()];
    return {msg, *registry_, position_};
  }

  pointer operator->() const = delete;

  constexpr ConsumableMessageWrapperIter& operator++() noexcept {
    ++position_;
    return *this;
  }

  [[nodiscard]] constexpr ConsumableMessageWrapperIter operator++(
      int) noexcept {
    auto copy = *this;
    ++(*this);
    return copy;
  }

  constexpr ConsumableMessageWrapperIter& operator--() noexcept {
    --position_;
    return *this;
  }

  [[nodiscard]] constexpr ConsumableMessageWrapperIter operator--(
      int) noexcept {
    auto copy = *this;
    --(*this);
    return copy;
  }

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
   * @brief Returns the current logical position in the combined sequence.
   * @return Zero-based position index
   */
  [[nodiscard]] constexpr size_t Position() const noexcept { return position_; }

  /**
   * @brief Returns a copy of this iterator positioned at the beginning of the
   * sequence.
   * @details Required so `ConsumableMessageWrapperIter` can act as a
   * self-contained range for `FunctionalAdapterBase` adapter methods.
   * @return Iterator reset to position 0
   */
  [[nodiscard]] constexpr ConsumableMessageWrapperIter begin() const noexcept {
    return {first_, second_, *registry_, 0};
  }

  /**
   * @brief Returns a copy of this iterator positioned one past the last
   * element.
   * @return Iterator at position `first_.size() + second_.size()`
   */
  [[nodiscard]] constexpr ConsumableMessageWrapperIter end() const noexcept {
    return {first_, second_, *registry_, first_.size() + second_.size()};
  }

private:
  std::span<const T> first_;
  std::span<const T> second_;
  size_t position_ = 0;
  ConsumedMessagesRegistry<>* registry_ = nullptr;
};

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
   * @brief Applies an action to every message (unwrapped `const T&`).
   * @tparam Action Callable type `(const value_type&) -> void`
   * @param action Action to apply
   */
  template <typename Action>
    requires std::invocable<Action, const value_type&>
  constexpr void ForEach(const Action& action) const {
    return GetDerived().begin().ForEach(action);
  }

  /**
   * @brief Collects all messages (previous + current) into a vector of raw `T`
   * values.
   * @note This shadows `FunctionalAdapterBase::Collect()`. To collect
   * wrapper values, chain a lazy adapter first (e.g.,
   * `reader.Filter(...).Collect()`).
   * @return Vector containing copies of all messages
   */
  [[nodiscard]] constexpr auto Collect() const -> std::vector<T>;

  /**
   * @brief Collects all messages into a vector of raw `T` values using a custom
   * allocator.
   * @tparam Alloc STL-compatible allocator type for `T`
   * @param alloc Allocator instance
   * @return Vector containing copies of all messages, using the provided
   * allocator
   */
  template <typename Alloc>
    requires std::same_as<typename std::allocator_traits<Alloc>::value_type, T>
  [[nodiscard]] constexpr auto CollectWith(const Alloc& alloc) const
      -> std::vector<T, Alloc>;

  /**
   * @brief Collects all messages into a vector of raw `T` values using a memory
   * resource.
   * @param resource Memory resource for allocating the vector
   * @return Vector containing copies of all messages, using the provided
   * memory resource
   */
  [[nodiscard]] constexpr auto CollectWith(
      std::pmr::memory_resource* resource) const -> std::pmr::vector<T>;

  auto CollectWith(std::nullptr_t) const -> std::pmr::vector<T> = delete;

  /**
   * @brief Reads all messages into an output iterator.
   * @tparam OutIt Output iterator type for `T`
   * @param out Output iterator
   */
  template <typename OutIt>
    requires std::output_iterator<OutIt, T>
  constexpr void ReadInto(OutIt out) const;

  /**
   * @brief Returns a lazy filter adapter over the wrapped message sequence.
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
   * @brief Returns a lazy map adapter over the wrapped message sequence.
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
   * @brief Returns a lazy adapter yielding at most `count` wrappers.
   * @param count Maximum number of elements to yield
   * @return `TakeAdapter` over the iterator
   */
  [[nodiscard]] constexpr auto Take(size_t count) const
      noexcept(noexcept(GetDerived().begin().Take(count)))
          -> utils::TakeAdapter<iterator> {
    return GetDerived().begin().Take(count);
  }

  /**
   * @brief Returns a lazy adapter that skips the first `count` wrappers.
   * @param count Number of elements to skip
   * @return `SkipAdapter` over the iterator
   */
  [[nodiscard]] constexpr auto Skip(size_t count) const
      noexcept(noexcept(GetDerived().begin().Skip(count)))
          -> utils::SkipAdapter<iterator> {
    return GetDerived().begin().Skip(count);
  }

  /**
   * @brief Returns a lazy adapter that yields wrappers while `predicate` is
   * true.
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
   * @brief Returns a lazy adapter that skips wrappers while `predicate` is
   * true.
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
   * @brief Returns a lazy adapter that pairs each wrapper with its zero-based
   * index.
   * @return `EnumerateAdapter` over the iterator
   */
  [[nodiscard]] constexpr auto Enumerate() const
      noexcept(noexcept(GetDerived().begin().Enumerate()))
          -> utils::EnumerateAdapter<iterator> {
    return GetDerived().begin().Enumerate();
  }

  /**
   * @brief Returns a lazy adapter that calls `inspector` on each wrapper as a
   * side-effect.
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
   * @brief Returns a lazy adapter that yields every `step`-th wrapper.
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
   * @brief Returns a lazy adapter that yields wrappers in reverse order.
   * @return `ReverseAdapter` over the iterator
   */
  [[nodiscard]] constexpr auto Reverse() const
      noexcept(noexcept(GetDerived().begin().Reverse()))
          -> utils::ReverseAdapter<iterator> {
    return GetDerived().begin().Reverse();
  }

  /**
   * @brief Returns a lazy adapter that yields windows of `window_size`
   * wrappers.
   * @param window_size Number of wrappers to include in each window
   * @return `SlideAdapter` over the iterator
   */
  [[nodiscard]] constexpr auto Slide(size_t window_size) const
      noexcept(noexcept(GetDerived().begin().Slide(window_size)))
          -> utils::SlideAdapter<iterator> {
    return GetDerived().begin().Slide(window_size);
  }

  /**
   * @brief Returns a lazy adapter that yields every `stride`-th wrapper.
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
   * @brief Left-folds all messages with an accumulator.
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
   * @brief Returns a pointer to the first message matching a predicate.
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
   * @brief Counts messages matching a predicate.
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
   * @brief Partitions wrapped messages into matching and non-matching groups.
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
   * @brief Finds the wrapped message with the maximum extracted key.
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
   * @brief Finds the wrapped message with the minimum extracted key.
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
   * @brief Groups wrapped messages by a key extracted from each wrapper.
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
   * @brief Checks if any message matches a predicate.
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
   * @brief Checks if all messages match a predicate.
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
   * @brief Checks if no messages match a predicate.
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
   * @brief Checks if there are no messages.
   * @return `true` if both previous and current spans are empty
   */
  [[nodiscard]] constexpr bool Empty() const noexcept {
    return GetDerived().PreviousMessages().empty() &&
           GetDerived().CurrentMessages().empty();
  }

  /**
   * @brief Returns the total number of messages (previous + current).
   * @return Total message count
   */
  [[nodiscard]] constexpr size_type Count() const noexcept {
    return GetDerived().PreviousMessages().size() +
           GetDerived().CurrentMessages().size();
  }

private:
  [[nodiscard]] constexpr Derived& GetDerived() noexcept {
    return static_cast<Derived&>(*this);
  }

  [[nodiscard]] constexpr const Derived& GetDerived() const noexcept {
    return static_cast<const Derived&>(*this);
  }
};

/**
 * @brief Type-safe, zero-copy reader for regular messages.
 * @details Provides a read-only view over the combined (previous + current)
 * message queues for a specific message type. Messages are accessed via spans
 * directly into the underlying `TypedBuffer` storage — no copying is performed.
 *
 * For regular (non-consumable) messages, no consumption support is available.
 *
 * @note Thread-safe.
 * @tparam T Message type satisfying `MessageTrait`
 *
 * @code
 * // Wrapper iteration
 * for (auto wrapper : reader) {
 *   // Use wrapper
 * }
 *
 * // Lazy adapter chaining
 * reader.Filter([](const auto& w) { return (*w).amount > 10; })
 *       .ForEach([](const auto& w) { ... });
 *
 * // Direct span access
 * auto prev = reader.PreviousMessages();
 * auto curr = reader.CurrentMessages();
 * @endcode
 */
template <MessageTrait T>
  requires(!ConsumableMessageTrait<T>)
class MessageReader final
    : public MessageReaderBase<MessageReader<T>, T, MessageWrapperIter<T>> {
  friend class MessageReaderBase<MessageReader<T>, T, MessageWrapperIter<T>>;

public:
  using value_type = MessageWrapper<T>;
  using size_type = MessageManager::size_type;
  using const_iterator = MessageWrapperIter<T>;
  using iterator = const_iterator;

  /**
   * @brief Constructs a `MessageReader` from explicit spans.
   * @param previous Span of messages from the previous frame
   * @param current Span of messages from the current frame
   */
  constexpr MessageReader(std::span<const T> previous,
                          std::span<const T> current) noexcept
      : previous_(previous), current_(current) {}

  /**
   * @brief Constructs a `MessageReader` from the message manager.
   * @param manager Const reference to the message manager
   */
  explicit constexpr MessageReader(const MessageManager& manager) noexcept
      : MessageReader(manager.PreviousMessages<T>(),
                      manager.CurrentMessages<T>()) {}

  MessageReader(MessageReader&) = delete;
  constexpr MessageReader(MessageReader&&) noexcept = default;
  constexpr ~MessageReader() noexcept = default;

  MessageReader& operator=(MessageReader&) = delete;
  constexpr MessageReader& operator=(MessageReader&&) noexcept = default;

  /**
   * @brief Gets the messages from the previous frame.
   * @return Span of const messages from the previous queue
   */
  [[nodiscard]] constexpr auto PreviousMessages() const noexcept
      -> std::span<const T> {
    return previous_;
  }

  /**
   * @brief Gets the messages from the current frame.
   * @return Span of const messages from the current queue
   */
  [[nodiscard]] constexpr auto CurrentMessages() const noexcept
      -> std::span<const T> {
    return current_;
  }

  /**
   * @brief Returns a const iterator to the first message in the combined view.
   * @return Const iterator to the beginning
   */
  [[nodiscard]] constexpr const_iterator begin() const noexcept {
    return {previous_, current_, 0};
  }

  /**
   * @brief Returns a const iterator past the last message in the combined view.
   * @return Const iterator to the end
   */
  [[nodiscard]] constexpr const_iterator end() const noexcept {
    return {previous_, current_, previous_.size() + current_.size()};
  }

private:
  std::span<const T> previous_;
  std::span<const T> current_;
};

/**
 * @brief Type-safe, zero-copy reader for consumable messages with consume
 * support.
 * @details Provides a read-only view over the combined (previous + current)
 * message queues for a specific message type. Messages are accessed via spans
 * directly into the underlying `TypedBuffer` storage — no copying is performed.
 *
 * Consumed messages are not filtered during iteration within the same frame.
 * They are removed at the next `MessageManager::Update` call.
 *
 * @note Thread-safe, but beware that `ConsumableMessageWrapper`'s `Consume()`
 * method is NOT thread-safe.
 * @tparam T Message type satisfying `ConsumableMessageTrait`
 *
 * @code
 * // Wrapper iteration — with consume support
 * for (auto wrapper : reader) {
 *   if ((*wrapper).priority > 5) {
 *     wrapper.Consume();
 *   }
 * }
 *
 * // Lazy adapter chaining — operates on ConsumableMessageWrapper<T>
 * reader.Filter([](const auto& w) { return (*w).amount > 10; })
 *       .ForEach([](const auto& w) { w.Consume(); });
 *
 * // Direct span access
 * auto prev = reader.PreviousMessages();
 * auto curr = reader.CurrentMessages();
 * @endcode
 */
template <ConsumableMessageTrait T>
class ConsumableMessageReader final
    : public MessageReaderBase<ConsumableMessageReader<T>, T,
                               ConsumableMessageWrapperIter<T>> {
  friend class MessageReaderBase<ConsumableMessageReader<T>, T,
                                 ConsumableMessageWrapperIter<T>>;

public:
  using value_type = ConsumableMessageWrapper<T>;
  using size_type = MessageManager::size_type;
  using const_iterator = ConsumableMessageWrapperIter<T>;
  using iterator = const_iterator;

  /**
   * @brief Constructs a `ConsumableMessageReader` from explicit spans and a
   * consumed registry.
   * @param previous Span of messages from the previous frame
   * @param current Span of messages from the current frame
   * @param consumed_registry Mutable reference to the per-system consumed
   * messages registry
   */
  constexpr ConsumableMessageReader(
      std::span<const T> previous, std::span<const T> current,
      ConsumedMessagesRegistry<>& consumed_registry) noexcept
      : previous_(previous), current_(current), registry_(consumed_registry) {}

  /**
   * @brief Constructs a `ConsumableMessageReader` from the message manager and
   * a per-system consumed registry.
   * @param manager Const reference to the message manager
   * @param consumed_registry Mutable reference to the per-system consumed
   * messages registry
   */
  constexpr ConsumableMessageReader(
      const MessageManager& manager,
      ConsumedMessagesRegistry<>& consumed_registry) noexcept
      : ConsumableMessageReader(manager.PreviousMessages<T>(),
                                manager.CurrentMessages<T>(),
                                consumed_registry) {}

  ConsumableMessageReader(ConsumableMessageReader&) = delete;
  constexpr ConsumableMessageReader(ConsumableMessageReader&&) noexcept =
      default;
  constexpr ~ConsumableMessageReader() noexcept = default;

  ConsumableMessageReader& operator=(ConsumableMessageReader&) = delete;
  constexpr ConsumableMessageReader& operator=(
      ConsumableMessageReader&&) noexcept = default;

  /// @brief Marks all messages as consumed.
  constexpr void ConsumeAll() const;

  /**
   * @brief Marks all messages matching a predicate as consumed.
   * @tparam Pred Predicate type `(const T&) -> bool`
   * @param predicate Predicate function
   * @return Number of messages marked as consumed
   */
  template <typename Pred>
    requires std::predicate<Pred, const T&>
  constexpr size_type ConsumeIf(const Pred& predicate) const;

  /**
   * @brief Gets the messages from the previous frame.
   * @return Span of const messages from the previous queue
   */
  [[nodiscard]] constexpr auto PreviousMessages() const noexcept
      -> std::span<const T> {
    return previous_;
  }

  /**
   * @brief Gets the messages from the current frame.
   * @return Span of const messages from the current queue
   */
  [[nodiscard]] constexpr auto CurrentMessages() const noexcept
      -> std::span<const T> {
    return current_;
  }

  /**
   * @brief Returns a const iterator to the first message in the combined view.
   * @return Const iterator to the beginning
   */
  [[nodiscard]] constexpr const_iterator begin() const noexcept {
    return {previous_, current_, registry_, 0};
  }

  /**
   * @brief Returns a const iterator past the last message in the combined view.
   * @return Const iterator to the end
   */
  [[nodiscard]] constexpr const_iterator end() const noexcept {
    return {previous_, current_, registry_, previous_.size() + current_.size()};
  }

private:
  std::span<const T> previous_;
  std::span<const T> current_;
  std::reference_wrapper<ConsumedMessagesRegistry<>> registry_;
};

template <typename Derived, MessageTrait T, typename IterType>
[[nodiscard]] constexpr auto MessageReaderBase<Derived, T, IterType>::Collect()
    const -> std::vector<T> {
  std::vector<T> result;
  result.reserve(Count());
  const auto& derived = GetDerived();
  std::ranges::copy(derived.PreviousMessages(), std::back_inserter(result));
  std::ranges::copy(derived.CurrentMessages(), std::back_inserter(result));
  return result;
}

template <typename Derived, MessageTrait T, typename IterType>
template <typename Alloc>
  requires std::same_as<typename std::allocator_traits<Alloc>::value_type, T>
[[nodiscard]] constexpr auto
MessageReaderBase<Derived, T, IterType>::CollectWith(const Alloc& alloc) const
    -> std::vector<T, Alloc> {
  std::vector<T, Alloc> result{alloc};
  result.reserve(Count());
  const auto& derived = GetDerived();
  std::ranges::copy(derived.PreviousMessages(), std::back_inserter(result));
  std::ranges::copy(derived.CurrentMessages(), std::back_inserter(result));
  return result;
}

template <typename Derived, MessageTrait T, typename IterType>
[[nodiscard]] constexpr auto
MessageReaderBase<Derived, T, IterType>::CollectWith(
    std::pmr::memory_resource* resource) const -> std::pmr::vector<T> {
  std::pmr::vector<T> result{resource};
  result.reserve(Count());
  const auto& derived = GetDerived();
  std::ranges::copy(derived.PreviousMessages(), std::back_inserter(result));
  std::ranges::copy(derived.CurrentMessages(), std::back_inserter(result));
  return result;
}

template <typename Derived, MessageTrait T, typename IterType>
template <typename OutIt>
  requires std::output_iterator<OutIt, T>
constexpr void MessageReaderBase<Derived, T, IterType>::ReadInto(
    OutIt out) const {
  const auto& derived = GetDerived();
  std::ranges::copy(derived.PreviousMessages(), out);
  std::ranges::copy(derived.CurrentMessages(), out);
}

template <ConsumableMessageTrait T>
constexpr void ConsumableMessageReader<T>::ConsumeAll() const {
  auto& registry = registry_.get();
  const auto total = this->Count();
  for (size_type i = 0; i < total; ++i) {
    registry.template MarkConsumed<T>(i);
  }
}

template <ConsumableMessageTrait T>
template <typename Pred>
  requires std::predicate<Pred, const T&>
constexpr auto ConsumableMessageReader<T>::ConsumeIf(
    const Pred& predicate) const -> size_type {
  size_type consumed_count = 0;
  auto& registry = registry_.get();
  size_type global_index = 0;

  for (const auto& message : previous_) {
    if (predicate(message)) {
      registry.template MarkConsumed<T>(global_index);
      ++consumed_count;
    }
    ++global_index;
  }
  for (const auto& message : current_) {
    if (predicate(message)) {
      registry.template MarkConsumed<T>(global_index);
      ++consumed_count;
    }
    ++global_index;
  }
  return consumed_count;
}

}  // namespace helios::ecs
