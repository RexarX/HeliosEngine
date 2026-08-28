#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.ecs;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/container/flat_map.hpp>

#include <algorithm>
#include <cstddef>
#include <memory_resource>
#include <span>
#include <utility>
#include <vector>
#endif
#include <helios/assert.hpp>
#include <helios/ecs/message/id.hpp>
#include <helios/ecs/message/message.hpp>

HELIOS_MODULE_EXPORT
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
 */
class ConsumedMessagesRegistry {
  using size_type = size_t;

  using ConsumedIds = std::pmr::vector<AnyMessageId>;
  using ConsumedMap = container::FlatMap<MessageTypeIndex, ConsumedIds>;

public:
  constexpr ConsumedMessagesRegistry() = default;

  explicit constexpr ConsumedMessagesRegistry(
      std::pmr::memory_resource* resource)
      : consumed_(resource) {}

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
  void MarkConsumed(MessageId<T> message_id) {
    MarkConsumed(MessageTypeIndex::From<T>(), message_id.ToAny());
  }

  /**
   * @brief Marks an message as consumed by its type index and message id.
   * @param type_index Type index of the message
   * @param message_id Stable id of the message
   */
  void MarkConsumed(MessageTypeIndex type_index, AnyMessageId message_id);

  /**
   * @brief Merges consumed entries from another registry into this one.
   * @details Performs a sorted union of consumed indices for each type.
   * Avoids additional heap allocation by appending and using in-place merge.
   * @param other Registry to merge from
   */
  void MergeFrom(const ConsumedMessagesRegistry& other);

  /**
   * @brief Merges consumed entries from another registry into this one.
   * @details Rvalue overload that consumes the source registry.
   * @param other Registry to merge from
   */
  void MergeFrom(ConsumedMessagesRegistry&& other);

  /// @brief Clears all consumed entries.
  void Clear() noexcept { consumed_.Clear(); }

  /**
   * @brief Clears consumed entries for a specific message type.
   * @tparam T Consumable message type
   */
  template <ConsumableMessageTrait T>
  void Clear() noexcept {
    Clear(MessageTypeIndex::From<T>());
  }

  /**
   * @brief Clears consumed entries for a specific message type.
   * @param type_index Type index to clear
   */
  void Clear(MessageTypeIndex type_index) noexcept {
    consumed_.Erase(type_index);
  }

  /**
   * @brief Gets the sorted consumed message ids for a specific message type.
   * @tparam T Consumable message type
   * @return Span of sorted consumed ids, or empty span if none
   */
  template <ConsumableMessageTrait T>
  [[nodiscard]] auto ConsumedIndicesFor() const noexcept
      -> std::span<const AnyMessageId> {
    return ConsumedIndicesFor(MessageTypeIndex::From<T>());
  }

  /**
   * @brief Gets the sorted consumed message ids for a specific message type.
   * @param type_index Type index of the message
   * @return Span of sorted consumed ids, or empty span if none
   */
  [[nodiscard]] auto ConsumedIndicesFor(MessageTypeIndex type_index)
      const noexcept -> std::span<const AnyMessageId>;

  /**
   * @brief Checks if a specific message is marked as consumed.
   * @tparam T Consumable message type
   * @param message_id Stable id of the message
   * @return True if the message is consumed, false otherwise
   */
  template <ConsumableMessageTrait T>
  [[nodiscard]] bool IsConsumed(MessageId<T> message_id) const noexcept {
    return IsConsumed(MessageTypeIndex::From<T>(), message_id.ToAny());
  }

  /**
   * @brief Checks if a specific message is marked as consumed.
   * @param type_index Type index of the message
   * @param message_id Stable id of the message
   * @return True if the message is consumed, false otherwise
   */
  [[nodiscard]] bool IsConsumed(MessageTypeIndex type_index,
                                AnyMessageId message_id) const noexcept;

  /**
   * @brief Checks if any message type has consumed entries.
   * @return True if no consumed entries exist, false otherwise
   */
  [[nodiscard]] bool Empty() const noexcept { return consumed_.Empty(); }

  /**
   * @brief Checks if a specific message type has consumed entries.
   * @tparam T Consumable message type
   * @return True if consumed entries exist for this type, false otherwise
   */
  template <ConsumableMessageTrait T>
  [[nodiscard]] bool HasConsumed() const noexcept {
    return HasConsumed(MessageTypeIndex::From<T>());
  }

  /**
   * @brief Checks if a specific message type has consumed entries.
   * @param type_index Type index to check
   * @return True if consumed entries exist for this type, false otherwise
   */
  [[nodiscard]] bool HasConsumed(MessageTypeIndex type_index) const noexcept;

  /**
   * @brief Gets the total number of consumed messages across all types.
   * @return Total consumed message count
   */
  [[nodiscard]] size_type TotalConsumedCount() const noexcept;

  /**
   * @brief Gets the number of consumed messages for a specific type.
   * @tparam T Consumable message type
   * @return Number of consumed messages for this type
   */
  template <ConsumableMessageTrait T>
  [[nodiscard]] size_type ConsumedCount() const noexcept {
    return ConsumedCount(MessageTypeIndex::From<T>());
  }

  /**
   * @brief Gets the number of consumed messages for a specific type.
   * @param type_index Type index to query
   * @return Number of consumed messages for this type
   */
  [[nodiscard]] size_type ConsumedCount(
      MessageTypeIndex type_index) const noexcept;

  /**
   * @brief Provides direct access to the underlying consumed map.
   * @return Const reference to the consumed map
   */
  [[nodiscard]] const ConsumedMap& Data() const noexcept { return consumed_; }

  /**
   * @brief Returns the memory resource used for internal storage.
   * @return Memory resource passed to the constructor, or the default resource
   */
  [[nodiscard]] std::pmr::memory_resource* GetMemoryResource() const noexcept {
    return consumed_.GetMemoryResource();
  }

private:
  [[nodiscard]] ConsumedIds& EnsureIds(MessageTypeIndex type_index);

  ConsumedMap consumed_;  ///< Map of message type index to sorted consumed
                          ///< message ids
};

inline auto ConsumedMessagesRegistry::EnsureIds(MessageTypeIndex type_index)
    -> ConsumedIds& {
  const auto it = consumed_.LowerBound(type_index);
  if (it != consumed_.end() && it->first == type_index) {
    return it->second;
  }

  return consumed_
      .EmplaceHint(it, type_index, ConsumedIds{GetMemoryResource()})
      ->second;
}

inline void ConsumedMessagesRegistry::MarkConsumed(MessageTypeIndex type_index,
                                                   AnyMessageId message_id) {
  HELIOS_ASSERT(message_id.type == type_index,
                "AnyMessageId type does not match type_index!");
  auto& ids = EnsureIds(type_index);
  // Insert in sorted order, maintaining uniqueness
  const auto pos = std::ranges::lower_bound(ids, message_id);
  if (pos == ids.end() || *pos != message_id) {
    ids.insert(pos, message_id);
  }
}

inline auto ConsumedMessagesRegistry::ConsumedIndicesFor(
    MessageTypeIndex type_index) const noexcept
    -> std::span<const AnyMessageId> {
  const auto it = consumed_.Find(type_index);
  if (it == consumed_.end()) [[unlikely]] {
    return {};
  }
  return {it->second.data(), it->second.size()};
}

inline bool ConsumedMessagesRegistry::IsConsumed(
    MessageTypeIndex type_index, AnyMessageId message_id) const noexcept {
  const auto it = consumed_.Find(type_index);
  if (it == consumed_.end()) [[unlikely]] {
    return false;
  }
  return std::ranges::binary_search(it->second, message_id);
}

inline bool ConsumedMessagesRegistry::HasConsumed(
    MessageTypeIndex type_index) const noexcept {
  const auto it = consumed_.Find(type_index);
  return it != consumed_.end() && !it->second.empty();
}

inline auto ConsumedMessagesRegistry::ConsumedCount(
    MessageTypeIndex type_index) const noexcept -> size_type {
  const auto it = consumed_.Find(type_index);
  if (it == consumed_.end()) [[unlikely]] {
    return 0;
  }
  return it->second.size();
}

}  // namespace helios::ecs
#endif  // HELIOS_MODULE_CONSUMER_SHIM
