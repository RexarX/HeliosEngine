#pragma once

#include <helios/assert.hpp>
#include <helios/compiler/compiler.hpp>
#include <helios/ecs/message/id.hpp>
#include <helios/ecs/message/message.hpp>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <memory_resource>
#include <span>
#include <utility>
#include <vector>

#ifdef HELIOS_STL_FLAT_MAP_AVAILABLE
#include <flat_map>
#else
#include <boost/container/flat_map.hpp>
#endif

namespace helios::ecs {

/**
 * @brief Per-system registry for tracking consumed message ids.
 * @details Each system running in parallel gets its own
 * `ConsumedMessagesRegistry` instance. Systems mark messages as consumed by
 * writing stable message ids into their registry, avoiding data races. At
 * Update time, all registries are merged and applied to message queues in
 * `MessageManager`.
 *
 * Entries are monotonic message ids assigned when messages enter the global
 * manager — not transient indices into the previous/current buffers.
 *
 * @note Not therad-safe.
 * @tparam Alloc Allocator type for internal storage (default:
 * `std::allocator<std::byte>`)
 */
template <typename Alloc = std::allocator<std::byte>>
class ConsumedMessagesRegistry {
public:
  using size_type = size_t;
  using allocator_type = Alloc;

private:
  using IdAllocator = typename std::allocator_traits<
      allocator_type>::template rebind_alloc<AnyMessageId>;
  using ConsumedIds = std::vector<AnyMessageId, IdAllocator>;

#ifdef HELIOS_STL_FLAT_MAP_AVAILABLE
  using KeyContainer =
      std::vector<MessageTypeIndex,
                  typename std::allocator_traits<
                      allocator_type>::template rebind_alloc<MessageTypeIndex>>;
  using MappedContainer =
      std::vector<ConsumedIds, typename std::allocator_traits<allocator_type>::
                                   template rebind_alloc<ConsumedIds>>;

  using ConsumedMap =
      std::flat_map<MessageTypeIndex, ConsumedIds, std::less<MessageTypeIndex>,
                    KeyContainer, MappedContainer>;
#else
  using MapValueType = std::pair<MessageTypeIndex, ConsumedIds>;
  using MapAllocator = typename std::allocator_traits<
      allocator_type>::template rebind_alloc<MapValueType>;

  using ConsumedMap =
      boost::container::flat_map<MessageTypeIndex, ConsumedIds,
                                 std::less<MessageTypeIndex>, MapAllocator>;
#endif

#ifdef HELIOS_STL_FLAT_MAP_AVAILABLE
public:
  explicit constexpr ConsumedMessagesRegistry(allocator_type alloc = {})
      : consumed_(alloc), allocator_(std::move(alloc)) {}
#else
public:
  explicit constexpr ConsumedMessagesRegistry(allocator_type alloc = {})
      : consumed_(MapAllocator(alloc)), allocator_(std::move(alloc)) {}
#endif

  /**
   * @brief Constructs a `ConsumedMessagesRegistry` from a PMR memory resource.
   * @details Enabled only when `allocator_type` is constructible from
   * `std::pmr::memory_resource*`.
   * @param resource Memory resource used to construct allocator
   */
  explicit constexpr ConsumedMessagesRegistry(
      std::pmr::memory_resource* resource)
    requires std::constructible_from<allocator_type, std::pmr::memory_resource*>
      : ConsumedMessagesRegistry(allocator_type{resource}) {}

  ConsumedMessagesRegistry(std::nullptr_t) = delete;

  ConsumedMessagesRegistry(const ConsumedMessagesRegistry&) = default;
  ConsumedMessagesRegistry(ConsumedMessagesRegistry&&) noexcept = default;
  ~ConsumedMessagesRegistry() = default;

  ConsumedMessagesRegistry& operator=(const ConsumedMessagesRegistry&) =
      default;
  ConsumedMessagesRegistry& operator=(ConsumedMessagesRegistry&&) noexcept =
      default;

  /**
   * @brief Marks an message as consumed by its type and message id.
   * @tparam T Consumable message type
   * @param message_id Stable id of the message
   */
  template <ConsumableMessageTrait T>
  constexpr void MarkConsumed(MessageId<T> message_id) {
    MarkConsumed(MessageTypeIndex::From<T>(), message_id.ToAny());
  }

  /**
   * @brief Marks an message as consumed by its type index and message id.
   * @param type_index Type index of the message
   * @param message_id Stable id of the message
   */
  constexpr void MarkConsumed(MessageTypeIndex type_index,
                              AnyMessageId message_id);

  /**
   * @brief Merges consumed entries from another registry into this one.
   * @details Performs a sorted union of consumed indices for each type.
   * Avoids additional heap allocation by appending and using in-place merge.
   * @tparam OtherAlloc Allocator type of the source registry
   * @param other Registry to merge from
   */
  template <typename OtherAlloc>
  constexpr void MergeFrom(const ConsumedMessagesRegistry<OtherAlloc>& other);

  /**
   * @brief Merges consumed entries from another registry into this one.
   * @details Rvalue overload that consumes the source registry.
   * @tparam OtherAlloc Allocator type of the source registry
   * @param other Registry to merge from
   */
  template <typename OtherAlloc>
  constexpr void MergeFrom(ConsumedMessagesRegistry<OtherAlloc>&& other);

  /// @brief Clears all consumed entries.
  constexpr void Clear() noexcept { consumed_.clear(); }

  /**
   * @brief Clears consumed entries for a specific message type.
   * @tparam T Consumable message type
   */
  template <ConsumableMessageTrait T>
  constexpr void Clear() noexcept {
    Clear(MessageTypeIndex::From<T>());
  }

  /**
   * @brief Clears consumed entries for a specific message type.
   * @param type_index Type index to clear
   */
  constexpr void Clear(MessageTypeIndex type_index) noexcept {
    consumed_.erase(type_index);
  }

  /**
   * @brief Gets the sorted consumed message ids for a specific message type.
   * @tparam T Consumable message type
   * @return Span of sorted consumed ids, or empty span if none
   */
  template <ConsumableMessageTrait T>
  [[nodiscard]] constexpr auto ConsumedIndicesFor() const noexcept
      -> std::span<const AnyMessageId> {
    return ConsumedIndicesFor(MessageTypeIndex::From<T>());
  }

  /**
   * @brief Gets the sorted consumed message ids for a specific message type.
   * @param type_index Type index of the message
   * @return Span of sorted consumed ids, or empty span if none
   */
  [[nodiscard]] constexpr auto ConsumedIndicesFor(MessageTypeIndex type_index)
      const noexcept -> std::span<const AnyMessageId>;

  /**
   * @brief Checks if a specific message is marked as consumed.
   * @tparam T Consumable message type
   * @param message_id Stable id of the message
   * @return True if the message is consumed, false otherwise
   */
  template <ConsumableMessageTrait T>
  [[nodiscard]] constexpr bool IsConsumed(
      MessageId<T> message_id) const noexcept {
    return IsConsumed(MessageTypeIndex::From<T>(), message_id.ToAny());
  }

  /**
   * @brief Checks if a specific message is marked as consumed.
   * @param type_index Type index of the message
   * @param message_id Stable id of the message
   * @return True if the message is consumed, false otherwise
   */
  [[nodiscard]] constexpr bool IsConsumed(
      MessageTypeIndex type_index, AnyMessageId message_id) const noexcept;

  /**
   * @brief Checks if any message type has consumed entries.
   * @return True if no consumed entries exist, false otherwise
   */
  [[nodiscard]] constexpr bool Empty() const noexcept {
    return consumed_.empty();
  }

  /**
   * @brief Checks if a specific message type has consumed entries.
   * @tparam T Consumable message type
   * @return True if consumed entries exist for this type, false otherwise
   */
  template <ConsumableMessageTrait T>
  [[nodiscard]] constexpr bool HasConsumed() const noexcept {
    return HasConsumed(MessageTypeIndex::From<T>());
  }

  /**
   * @brief Checks if a specific message type has consumed entries.
   * @param type_index Type index to check
   * @return True if consumed entries exist for this type, false otherwise
   */
  [[nodiscard]] constexpr bool HasConsumed(
      MessageTypeIndex type_index) const noexcept;

  /**
   * @brief Gets the total number of consumed messages across all types.
   * @return Total consumed message count
   */
  [[nodiscard]] constexpr size_type TotalConsumedCount() const noexcept;

  /**
   * @brief Gets the number of consumed messages for a specific type.
   * @tparam T Consumable message type
   * @return Number of consumed messages for this type
   */
  template <ConsumableMessageTrait T>
  [[nodiscard]] constexpr size_type ConsumedCount() const noexcept {
    return ConsumedCount(MessageTypeIndex::From<T>());
  }

  /**
   * @brief Gets the number of consumed messages for a specific type.
   * @param type_index Type index to query
   * @return Number of consumed messages for this type
   */
  [[nodiscard]] constexpr size_type ConsumedCount(
      MessageTypeIndex type_index) const noexcept;

  /**
   * @brief Provides direct access to the underlying consumed map.
   * @return Const reference to the consumed map
   */
  [[nodiscard]] constexpr const ConsumedMap& Data() const noexcept {
    return consumed_;
  }

private:
  [[nodiscard]] constexpr ConsumedIds& EnsureIds(MessageTypeIndex type_index);

  ConsumedMap consumed_;  ///< Map of message type index to sorted consumed
                          ///< message ids
  HELIOS_NO_UNIQUE_ADDRESS allocator_type allocator_;
};

template <typename Alloc>
constexpr auto ConsumedMessagesRegistry<Alloc>::EnsureIds(
    MessageTypeIndex type_index) -> ConsumedIds& {
  const auto it = consumed_.lower_bound(type_index);
  if (it != consumed_.end() && it->first == type_index) {
    return it->second;
  }

  return consumed_
      .emplace_hint(it, type_index, ConsumedIds{IdAllocator{allocator_}})
      ->second;
}

template <typename Alloc>
constexpr void ConsumedMessagesRegistry<Alloc>::MarkConsumed(
    MessageTypeIndex type_index, AnyMessageId message_id) {
  HELIOS_ASSERT(message_id.type == type_index,
                "AnyMessageId type does not match type_index!");
  auto& ids = EnsureIds(type_index);
  // Insert in sorted order, maintaining uniqueness
  const auto pos = std::ranges::lower_bound(ids, message_id);
  if (pos == ids.end() || *pos != message_id) {
    ids.insert(pos, message_id);
  }
}

template <typename Alloc>
template <typename OtherAlloc>
constexpr void ConsumedMessagesRegistry<Alloc>::MergeFrom(
    const ConsumedMessagesRegistry<OtherAlloc>& other) {
  for (const auto& [type_index, other_indices] : other.Data()) {
    if (other_indices.empty()) {
      continue;
    }

    auto& our_ids = EnsureIds(type_index);
    if (our_ids.empty()) {
      our_ids.insert(our_ids.end(), other_indices.begin(), other_indices.end());
      continue;
    }

    // In-place merge: append other ids, then inplace_merge, then deduplicate.
    // This reuses existing capacity in our_ids, avoiding a separate allocation.
    const auto original_size = our_ids.size();
    our_ids.insert(our_ids.end(), other_indices.begin(), other_indices.end());
    std::ranges::inplace_merge(
        our_ids, our_ids.begin() + static_cast<ptrdiff_t>(original_size));
    const auto [first, last] = std::ranges::unique(our_ids);
    our_ids.erase(first, last);
  }
}

template <typename Alloc>
template <typename OtherAlloc>
constexpr void ConsumedMessagesRegistry<Alloc>::MergeFrom(
    ConsumedMessagesRegistry<OtherAlloc>&& other) {
  for (auto&& [type_index, other_indices] : other.consumed_) {
    if (other_indices.empty()) {
      continue;
    }

    auto& our_ids = EnsureIds(type_index);
    if (our_ids.empty()) {
      our_ids = std::move(other_indices);
      continue;
    }

    const auto original_size = our_ids.size();
    our_ids.insert(our_ids.end(),
                   std::make_move_iterator(other_indices.begin()),
                   std::make_move_iterator(other_indices.end()));
    std::ranges::inplace_merge(
        our_ids, our_ids.begin() + static_cast<ptrdiff_t>(original_size));
    const auto [first, last] = std::ranges::unique(our_ids);
    our_ids.erase(first, last);
  }

  other.consumed_.clear();
}

template <typename Alloc>
constexpr auto ConsumedMessagesRegistry<Alloc>::ConsumedIndicesFor(
    MessageTypeIndex type_index) const noexcept
    -> std::span<const AnyMessageId> {
  const auto it = consumed_.find(type_index);
  if (it == consumed_.end()) {
    return {};
  }
  return {it->second.data(), it->second.size()};
}

template <typename Alloc>
constexpr bool ConsumedMessagesRegistry<Alloc>::IsConsumed(
    MessageTypeIndex type_index, AnyMessageId message_id) const noexcept {
  const auto it = consumed_.find(type_index);
  if (it == consumed_.end()) {
    return false;
  }
  return std::ranges::binary_search(it->second, message_id);
}

template <typename Alloc>
constexpr bool ConsumedMessagesRegistry<Alloc>::HasConsumed(
    MessageTypeIndex type_index) const noexcept {
  const auto it = consumed_.find(type_index);
  return it != consumed_.end() && !it->second.empty();
}

template <typename Alloc>
constexpr auto ConsumedMessagesRegistry<Alloc>::TotalConsumedCount()
    const noexcept -> size_type {
  size_type total = 0;
  for (const auto& [_, indices] : consumed_) {
    total += indices.size();
  }
  return total;
}

template <typename Alloc>
constexpr auto ConsumedMessagesRegistry<Alloc>::ConsumedCount(
    MessageTypeIndex type_index) const noexcept -> size_type {
  const auto it = consumed_.find(type_index);
  if (it == consumed_.end()) {
    return 0;
  }
  return it->second.size();
}

using PmrConsumedMessagesRegistry =
    ConsumedMessagesRegistry<std::pmr::polymorphic_allocator<std::byte>>;

}  // namespace helios::ecs
