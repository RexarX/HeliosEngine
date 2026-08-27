#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.container;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#include <helios/compiler/compiler.hpp>
#ifndef HELIOS_BUILDING_MODULE
#include <helios/utils/string_hash.hpp>

#include <algorithm>
#include <compare>
#include <concepts>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory_resource>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#endif
#include <helios/assert.hpp>

HELIOS_MODULE_EXPORT
namespace helios::container {

/**
 * @brief Default `Compare` selected for `Key` when none is specified.
 * @details Uses transparent `std::less<>` so lookup/erase accept any type
 * ordered against `Key` via `<` without constructing a temporary key.
 * Specialized for string-like keys to pick `utils::StringLess`, which compares
 * through `std::string_view` and avoids `const char*` pointer ordering.
 */
template <typename Key>
struct DefaultCompare {
  using type = std::less<>;
};

template <>
struct DefaultCompare<std::string> {
  using type = utils::StringLess;
};

template <>
struct DefaultCompare<std::string_view> {
  using type = utils::StringLess;
};

template <>
struct DefaultCompare<const char*> {
  using type = utils::StringLess;
};

template <typename Key>
using DefaultCompareT = typename DefaultCompare<Key>::type;

/**
 * @brief Default `Hash` selected for `Key` when none is specified.
 * @details Specialized for string-like keys to pick `utils::StringHash`. Not
 * used by lookup in the current sorted-vector implementation; kept in sync
 * with `DefaultCompare` for future use.
 */
template <typename Key>
struct DefaultHash {
  using type = std::hash<Key>;
};

template <>
struct DefaultHash<std::string> {
  using type = utils::StringHash;
};

template <>
struct DefaultHash<std::string_view> {
  using type = utils::StringHash;
};

template <>
struct DefaultHash<const char*> {
  using type = utils::StringHash;
};

template <typename Key>
using DefaultHashT = typename DefaultHash<Key>::type;

template <typename T, typename Key>
concept FlatMapCompare = std::strict_weak_order<T, Key, Key>;

template <typename T, typename Key>
concept FlatMapHash = requires(const T& hasher, const Key& key) {
  { hasher(key) } -> std::convertible_to<size_t>;
};

/// @brief Detects a transparent comparator/hasher (exposes `is_transparent`).
template <typename T>
concept TransparentFunctor = requires { typename T::is_transparent; };

/// @brief `K` is `Key`, or is ordered against `Key` by a transparent `Compare`.
template <typename Compare, typename Key, typename K>
concept FlatMapLookupKey =
    std::same_as<std::remove_cvref_t<K>, Key> ||
    (TransparentFunctor<Compare> &&
     std::predicate<const Compare&, const K&, const Key&> &&
     std::predicate<const Compare&, const Key&, const K&>);

/**
 * @brief Sorted, contiguous key/value container optimized for lookup-heavy
 * workloads.
 * @details Backed by a single `std::pmr::vector` kept sorted by `Compare`,
 * giving O(log n) lookup via binary search and cache-friendly linear iteration.
 * Insertion/erase are O(n) and are intended to happen only during startup, not
 * on hot paths.
 * @tparam Key Key type, ordered by `Compare`
 * @tparam Value Mapped value type
 * @tparam Compare Strict-weak-order comparator over `Key`. Defaults to
 * transparent `std::less<>`, or `utils::StringLess` for string-like keys
 * @tparam Hash Hasher over `Key`, reserved for future use. Defaults to
 * `std::hash<Key>`
 */
template <typename Key, typename Value, typename Compare = DefaultCompareT<Key>,
          typename Hash = DefaultHashT<Key>>
  requires(FlatMapCompare<Compare, Key> && FlatMapHash<Hash, Key>)
class FlatMap {
public:
  using key_type = Key;
  using mapped_type = Value;
  using value_type = std::pair<Key, Value>;
  using container_type = std::pmr::vector<value_type>;
  using size_type = typename container_type::size_type;
  using difference_type = typename container_type::difference_type;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = typename container_type::pointer;
  using const_pointer = typename container_type::const_pointer;
  using iterator = typename container_type::iterator;
  using const_iterator = typename container_type::const_iterator;
  using reverse_iterator = typename container_type::reverse_iterator;
  using const_reverse_iterator =
      typename container_type::const_reverse_iterator;
  using sentinel = iterator;
  using const_sentinel = const_iterator;

  /**
   * @brief Constructs an empty map using the default memory resource.
   * @param compare Key comparator instance
   * @param hash Key hasher instance, reserved for future use
   */
  explicit constexpr FlatMap(Compare compare = Compare{},
                             Hash hash = Hash{}) noexcept
      : FlatMap(std::pmr::get_default_resource(), std::move(compare),
                std::move(hash)) {}

  /**
   * @brief Constructs an empty map backed by `resource`.
   * @warning Triggers `HELIOS_ASSERT` if `resource` is `nullptr`.
   * @param resource Memory resource used for all allocations. Must outlive the
   * map
   * @param compare Key comparator instance
   * @param hash Key hasher instance, reserved for future use
   */
  explicit constexpr FlatMap(std::pmr::memory_resource* resource,
                             Compare compare = Compare{},
                             Hash hash = Hash{}) noexcept;

  explicit constexpr FlatMap(std::nullptr_t) = delete;

  constexpr FlatMap(const FlatMap&) = default;
  constexpr FlatMap(FlatMap&&) noexcept = default;
  constexpr ~FlatMap() = default;

  constexpr FlatMap& operator=(const FlatMap&) = default;
  constexpr FlatMap& operator=(FlatMap&&) noexcept = default;

  /// @brief Removes all entries without releasing storage.
  constexpr void Clear() noexcept { entries_.clear(); }

  /**
   * @brief Reserves storage for at least `capacity` entries.
   * @param capacity Minimum number of entries storage should hold without
   * reallocating
   */
  constexpr void Reserve(size_type capacity) { entries_.reserve(capacity); }

  /// @brief Releases unused capacity so storage fits `Size()`.
  constexpr void ShrinkToFit() { entries_.shrink_to_fit(); }

  /**
   * @brief Inserts `key`/`value` if `key` is not already present.
   * @details O(n) - intended for startup-time population only.
   * @param key Key to insert
   * @param value Value to associate with `key`
   * @return Iterator to the inserted entry, or the existing one if `key` was
   * present
   */
  constexpr iterator Insert(Key key, Value value);

  /**
   * @brief Insert overload taking a single `value_type` instead of separate
   * key/value.
   * @param value Entry to insert
   * @return Iterator to the inserted entry, or the existing one if the key was
   * present
   */
  constexpr iterator Insert(value_type value) {
    return Insert(std::move(value.first), std::move(value.second));
  }

  /**
   * @brief Bulk-inserts every element of `range`, skipping keys already
   * present.
   * @details Moves each element out of `range` when `range` is an rvalue (e.g.
   * a temporary container), falling back to copying when it is an lvalue.
   * Incoming entries are sorted if needed, duplicate incoming keys are dropped,
   * then linear-merged with existing entries. Existing keys always win.
   * @tparam R An input range whose values convert to `value_type`
   * @param range Range of entries to insert
   */
  template <std::ranges::input_range R>
    requires std::convertible_to<
        std::ranges::range_value_t<R>,
        typename FlatMap<Key, Value, Compare, Hash>::value_type>
  constexpr void Insert(R&& range);

  /**
   * @brief Bulk-inserts `[first, last)`, skipping keys already present.
   * @tparam InputIt An input iterator dereferencing to something convertible to
   * `value_type`
   * @param first Beginning of the range to insert
   * @param last End of the range to insert
   */
  template <std::input_iterator InputIt>
    requires std::convertible_to<
        std::iter_value_t<InputIt>,
        typename FlatMap<Key, Value, Compare, Hash>::value_type>
  constexpr void Insert(InputIt first, InputIt last);

  /**
   * @brief Inserts `key`/`value`, overwriting any existing value for `key`.
   * @param key Key to insert or update
   * @param value New value for `key`
   * @return Iterator to the stored entry
   */
  constexpr iterator InsertOrAssign(Key key, Value value);

  /**
   * @brief Constructs a `Value` from `args` and inserts it for `key` if absent.
   * @details Always constructs a temporary `Value`, even if `key` is already
   * present - use `TryEmplace` to avoid that when construction is expensive.
   * @tparam Args Constructor argument types for `Value`
   * @param key Key to insert
   * @param args Arguments forwarded to `Value`'s constructor
   * @return Iterator to the inserted entry, or the existing one if `key` was
   * present
   */
  template <typename... Args>
    requires std::constructible_from<Value, Args...>
  constexpr iterator Emplace(const Key& key, Args&&... args) {
    return Insert(key, Value(std::forward<Args>(args)...));
  }

  /**
   * @brief Constructs a `Value` from `args` and inserts it for a moved `key`
   * if absent.
   * @details Always constructs a temporary `Value`, even if `key` is already
   * present - use `TryEmplace` to avoid that when construction is expensive.
   * @tparam Args Constructor argument types for `Value`
   * @param key Key to insert
   * @param args Arguments forwarded to `Value`'s constructor
   * @return Iterator to the inserted entry, or the existing one if `key` was
   * present
   */
  template <typename... Args>
    requires std::constructible_from<Value, Args...>
  constexpr iterator Emplace(Key&& key, Args&&... args) {
    return Insert(std::move(key), Value(std::forward<Args>(args)...));
  }

  /**
   * @brief Inserts `key`/`args` as close as possible to `hint` if `key` is
   * absent.
   * @details `hint` is a best-effort insertion position. If it does not
   * identify the correct sorted slot, the map falls back to a binary search.
   * Constructs `Value` from `args` only when inserting.
   * @tparam Args Constructor argument types for `Value`
   * @param hint Suggested insertion position
   * @param key Key to insert
   * @param args Arguments forwarded to `Value`'s constructor, used only on
   * insertion
   * @return Iterator to the inserted entry, or the existing one if `key` was
   * present
   */
  template <typename... Args>
    requires std::constructible_from<Value, Args...>
  constexpr iterator EmplaceHint(const_iterator hint, const Key& key,
                                 Args&&... args);

  /**
   * @brief Inserts a moved `key` as close as possible to `hint` if `key` is
   * absent.
   * @tparam Args Constructor argument types for `Value`
   * @param hint Suggested insertion position
   * @param key Key to insert
   * @param args Arguments forwarded to `Value`'s constructor, used only on
   * insertion
   * @return Iterator to the inserted entry, or the existing one if `key` was
   * present
   */
  template <typename... Args>
    requires std::constructible_from<Value, Args...>
  constexpr iterator EmplaceHint(const_iterator hint, Key&& key,
                                 Args&&... args);

  /**
   * @brief Constructs a `Value` in place from `args` only if `key` is absent.
   * @tparam Args Constructor argument types for `Value`
   * @param key Key to insert
   * @param args Arguments forwarded to `Value`'s constructor, used only on
   * insertion
   * @return Pair of an iterator to the entry and whether an insertion took
   * place
   */
  template <typename... Args>
    requires std::constructible_from<Value, Args...>
  constexpr auto TryEmplace(const Key& key, Args&&... args)
      -> std::pair<iterator, bool>;

  /**
   * @brief Constructs a `Value` in place from `args` only if a moved `key` is
   * absent.
   * @tparam Args Constructor argument types for `Value`
   * @param key Key to insert
   * @param args Arguments forwarded to `Value`'s constructor, used only on
   * insertion
   * @return Pair of an iterator to the entry and whether an insertion took
   * place
   */
  template <typename... Args>
    requires std::constructible_from<Value, Args...>
  constexpr auto TryEmplace(Key&& key, Args&&... args)
      -> std::pair<iterator, bool>;

  /**
   * @brief Removes the entry for `key` if present.
   * @tparam K `Key`, or any type comparable against `Key` when `Compare` is
   * transparent
   * @param key Key to remove
   * @return `true` if an entry was removed
   */
  template <typename K>
    requires FlatMapLookupKey<Compare, Key, K>
  constexpr bool Erase(const K& key);

  /**
   * @brief Removes the entry at `pos`.
   * @param pos Iterator to the entry to remove
   * @return Iterator following the removed entry
   */
  constexpr iterator Erase(const_iterator pos) { return entries_.erase(pos); }

  /**
   * @brief Removes entries in `[first, last)`.
   * @param first Beginning of the range to remove
   * @param last End of the range to remove
   * @return Iterator following the last removed entry
   */
  constexpr iterator Erase(const_iterator first, const_iterator last) {
    return entries_.erase(first, last);
  }

  /**
   * @brief Copies entries from `other` whose keys are not already present.
   * @details Both maps stay valid. Existing keys in `*this` always win;
   * `other` is left unchanged.
   * @param other Map to copy unique keys from
   */
  constexpr void Merge(const FlatMap& other);

  /**
   * @brief Moves entries from `other` whose keys are not already present.
   * @details Existing keys in `*this` always win. Keys that were already
   * present remain in `other`; transferred keys are removed from it.
   * @param other Map to steal unique keys from
   */
  constexpr void Merge(FlatMap&& other);

  /**
   * @brief Swaps contents with `other`.
   * @warning Triggers `HELIOS_ASSERT` if `other` uses a different memory
   * resource - `std::pmr::polymorphic_allocator` does not propagate on swap, so
   * swapping storage across resources would leave allocations outliving their
   * owning resource.
   * @param other Map to swap contents with
   */
  constexpr void Swap(FlatMap& other) noexcept;

  /**
   * @brief ADL-visible `swap` for `std::swap`/algorithm compatibility.
   * @param lhs First map to swap
   * @param rhs Second map to swap
   */
  friend constexpr void swap(FlatMap& lhs, FlatMap& rhs) noexcept {
    lhs.Swap(rhs);
  }

  /**
   * @brief Looks up the entry for `key`.
   * @tparam K `Key`, or any type comparable against `Key` when `Compare` is
   * transparent
   * @param key Key to search for
   * @return Iterator to the entry, or `end()` if not found
   */
  template <typename K>
    requires FlatMapLookupKey<Compare, Key, K>
  [[nodiscard]] constexpr iterator Find(const K& key) noexcept;

  /**
   * @brief Looks up the entry for `key`.
   * @tparam K `Key`, or any type comparable against `Key` when `Compare` is
   * transparent
   * @param key Key to search for
   * @return Const iterator to the entry, or `end()` if not found
   */
  template <typename K>
    requires FlatMapLookupKey<Compare, Key, K>
  [[nodiscard]] constexpr const_iterator Find(const K& key) const noexcept;

  /**
   * @brief Accesses the value for `key`.
   * @warning Triggers `HELIOS_ASSERT` if `key` is not present.
   * @tparam K `Key`, or any type comparable against `Key` when `Compare` is
   * transparent
   * @param key Key to search for
   * @return Reference to the value
   */
  template <typename K>
    requires FlatMapLookupKey<Compare, Key, K>
  [[nodiscard]] constexpr Value& At(const K& key);

  /**
   * @brief Accesses the value for `key`.
   * @warning Triggers `HELIOS_ASSERT` if `key` is not present.
   * @tparam K `Key`, or any type comparable against `Key` when `Compare` is
   * transparent
   * @param key Key to search for
   * @return Const reference to the value
   */
  template <typename K>
    requires FlatMapLookupKey<Compare, Key, K>
  [[nodiscard]] constexpr const Value& At(const K& key) const;

  /**
   * @brief Accesses the value for `key`, default-constructing and inserting one
   * if absent.
   * @param key Key to look up or insert
   * @return Reference to the value
   */
  constexpr Value& operator[](const Key& key) {
    return TryEmplace(key).first->second;
  }

  /**
   * @brief Accesses the value for a moved `key`, default-constructing and
   * inserting one if absent.
   * @param key Key to look up or insert
   * @return Reference to the value
   */
  constexpr Value& operator[](Key&& key) {
    return TryEmplace(std::move(key)).first->second;
  }

  /**
   * @brief Lower bound search for `key`.
   * @tparam K Type comparable with Key
   * @param value Value to search for
   * @return Iterator to first element not less than value
   */
  template <typename K>
    requires FlatMapLookupKey<Compare, Key, K>
  [[nodiscard]] constexpr iterator LowerBound(const K& key) noexcept {
    return std::ranges::lower_bound(entries_, key, compare_,
                                    &value_type::first);
  }

  /**
   * @brief Lower bound search for `key`.
   * @tparam K Type comparable with Key
   * @param value Value to search for
   * @return Const iterator to first element not less than value
   */
  template <typename K>
    requires FlatMapLookupKey<Compare, Key, K>
  [[nodiscard]] constexpr const_iterator LowerBound(
      const K& key) const noexcept {
    return std::ranges::lower_bound(entries_, key, compare_,
                                    &value_type::first);
  }

  /**
   * @brief Upper bound search for `key`.
   * @tparam K Type comparable with Key
   * @param value Value to search for
   * @return Iterator to first element not less than value
   */
  template <typename K>
    requires FlatMapLookupKey<Compare, Key, K>
  [[nodiscard]] constexpr iterator UpperBound(const K& key) noexcept {
    return std::ranges::upper_bound(entries_, key, compare_,
                                    &value_type::first);
  }

  /**
   * @brief Upper bound search for `key`.
   * @tparam K Type comparable with Key
   * @param value Value to search for
   * @return Const iterator to first element not less than value
   */
  template <typename K>
    requires FlatMapLookupKey<Compare, Key, K>
  [[nodiscard]] constexpr const_iterator UpperBound(
      const K& key) const noexcept {
    return std::ranges::upper_bound(entries_, key, compare_,
                                    &value_type::first);
  }

  /**
   * @brief Erases every entry for which `pred` returns `true`.
   * @param map Map to erase entries from
   * @param pred Predicate invoked with each `value_type`
   * @return The number of entries removed
   */
  template <typename Pred>
  constexpr size_type EraseIf(Pred pred);

  /**
   * @brief Erases every entry for which `pred` returns `true`.
   * @param map Map to erase entries from
   * @param pred Predicate invoked with each `value_type`
   * @return The number of entries removed
   */
  template <typename Pred>
  friend constexpr size_type erase_if(FlatMap& map, Pred pred) {
    return map.EraseIf(std::move(pred));
  }

  /**
   * @brief Compares two maps entry-by-entry for equality.
   * @param other Map to compare against
   * @return `true` if both maps hold the same entries in the same order
   */
  [[nodiscard]] constexpr bool operator==(const FlatMap& other) const
    requires std::equality_comparable<Value>
  {
    return entries_ == other.entries_;
  }

  /**
   * @brief Lexicographically compares two maps entry-by-entry.
   * @param other Map to compare against
   * @return The ordering between the two maps' entry sequences
   */
  [[nodiscard]] constexpr auto operator<=>(const FlatMap& other) const
    requires std::three_way_comparable<Value>
  {
    return entries_ <=> other.entries_;
  }

  /**
   * @brief Checks whether `key` is present.
   * @tparam K `Key`, or any type comparable against `Key` when `Compare` is
   * transparent
   * @param key Key to search for
   * @return `true` if `key` is present, `false` otherwise
   */
  template <typename K>
    requires FlatMapLookupKey<Compare, Key, K>
  [[nodiscard]] constexpr bool Contains(const K& key) const noexcept {
    return Find(key) != end();
  }

  /**
   * @brief Checks whether the map holds no entries.
   * @return `true` if the map is empty, `false` otherwise
   */
  [[nodiscard]] constexpr bool Empty() const noexcept {
    return entries_.empty();
  }

  constexpr iterator begin() noexcept { return entries_.begin(); }
  constexpr iterator end() noexcept { return entries_.end(); }
  constexpr const_iterator begin() const noexcept { return entries_.begin(); }
  constexpr const_iterator end() const noexcept { return entries_.end(); }
  constexpr const_iterator cbegin() const noexcept { return entries_.cbegin(); }
  constexpr const_iterator cend() const noexcept { return entries_.cend(); }
  constexpr reverse_iterator rbegin() noexcept { return entries_.rbegin(); }
  constexpr reverse_iterator rend() noexcept { return entries_.rend(); }
  constexpr const_reverse_iterator rbegin() const noexcept {
    return entries_.rbegin();
  }

  constexpr const_reverse_iterator rend() const noexcept {
    return entries_.rend();
  }

  constexpr const_reverse_iterator crbegin() const noexcept {
    return entries_.crbegin();
  }

  constexpr const_reverse_iterator crend() const noexcept {
    return entries_.crend();
  }

  /**
   * @brief Gets the number of entries.
   * @return The number of entries
   */
  [[nodiscard]] constexpr size_type Size() const noexcept {
    return entries_.size();
  }

  /**
   * @brief Gets the number of entries storage can hold without reallocating.
   * @return The number of entries
   */
  [[nodiscard]] constexpr size_type Capacity() const noexcept {
    return entries_.capacity();
  }

  /**
   * @brief Returns the memory resource used for internal storage.
   * @return Memory resource passed to the constructor, or the default
   * resource
   */
  [[nodiscard]] constexpr std::pmr::memory_resource* GetMemoryResource()
      const noexcept {
    return entries_.get_allocator().resource();
  }

private:
  template <typename A, typename B>
  [[nodiscard]] constexpr bool Equivalent(const A& lhs,
                                          const B& rhs) const noexcept {
    return !compare_(lhs, rhs) && !compare_(rhs, lhs);
  }

  [[nodiscard]] constexpr iterator ToIterator(const_iterator it) noexcept {
    return entries_.begin() + (it - entries_.cbegin());
  }

  template <typename K>
    requires FlatMapLookupKey<Compare, Key, K>
  [[nodiscard]] constexpr iterator HintPosition(const_iterator hint,
                                                const K& key) noexcept;

  template <typename KeyArg, typename... Args>
  constexpr iterator EmplaceHintImpl(const_iterator hint, KeyArg&& key,
                                     Args&&... args);

  template <typename KeyArg, typename... Args>
  constexpr auto TryEmplaceImpl(KeyArg&& key, Args&&... args)
      -> std::pair<iterator, bool>;

  constexpr void SortUniqueIncoming(container_type& incoming);
  constexpr void MergeSortedUnique(container_type&& incoming);
  constexpr void MergeSortedUniqueSteal(container_type& incoming);

  container_type entries_;
  HELIOS_NO_UNIQUE_ADDRESS Compare compare_;
  HELIOS_NO_UNIQUE_ADDRESS Hash hash_;
};

template <typename Key, typename Value, typename Compare, typename Hash>
  requires(FlatMapCompare<Compare, Key> && FlatMapHash<Hash, Key>)
constexpr FlatMap<Key, Value, Compare, Hash>::FlatMap(
    std::pmr::memory_resource* resource, Compare compare, Hash hash) noexcept
    : entries_(resource), compare_(std::move(compare)), hash_(std::move(hash)) {
  HELIOS_ASSERT(resource != nullptr,
                "FlatMap constructed with a null memory_resource");
}

template <typename Key, typename Value, typename Compare, typename Hash>
  requires(FlatMapCompare<Compare, Key> && FlatMapHash<Hash, Key>)
constexpr auto FlatMap<Key, Value, Compare, Hash>::Insert(Key key, Value value)
    -> iterator {
  const iterator it = LowerBound(key);
  if (it != entries_.end() && Equivalent(key, it->first)) {
    return it;
  }
  return entries_.emplace(it, std::move(key), std::move(value));
}

template <typename Key, typename Value, typename Compare, typename Hash>
  requires(FlatMapCompare<Compare, Key> && FlatMapHash<Hash, Key>)
template <std::ranges::input_range R>
  requires std::convertible_to<
      std::ranges::range_value_t<R>,
      typename FlatMap<Key, Value, Compare, Hash>::value_type>
constexpr void FlatMap<Key, Value, Compare, Hash>::Insert(R&& range) {
  container_type incoming(entries_.get_allocator());
  if constexpr (std::ranges::sized_range<R>) {
    incoming.reserve(std::ranges::size(range));
  }

  if constexpr (std::is_lvalue_reference_v<R>) {
    for (auto&& entry : range) {
      incoming.emplace_back(entry);
    }
  } else {
    for (auto&& entry : std::forward<R>(range)) {
      incoming.emplace_back(std::forward<decltype(entry)>(entry));
    }
  }

  SortUniqueIncoming(incoming);
  MergeSortedUnique(std::move(incoming));
}

template <typename Key, typename Value, typename Compare, typename Hash>
  requires(FlatMapCompare<Compare, Key> && FlatMapHash<Hash, Key>)
template <std::input_iterator InputIt>
  requires std::convertible_to<
      std::iter_value_t<InputIt>,
      typename FlatMap<Key, Value, Compare, Hash>::value_type>
constexpr void FlatMap<Key, Value, Compare, Hash>::Insert(InputIt first,
                                                          InputIt last) {
  container_type incoming(entries_.get_allocator());
  if constexpr (std::sized_sentinel_for<InputIt, InputIt>) {
    incoming.reserve(static_cast<size_type>(std::distance(first, last)));
  }

  for (; first != last; ++first) {
    incoming.emplace_back(*first);
  }

  SortUniqueIncoming(incoming);
  MergeSortedUnique(std::move(incoming));
}

template <typename Key, typename Value, typename Compare, typename Hash>
  requires(FlatMapCompare<Compare, Key> && FlatMapHash<Hash, Key>)
constexpr auto FlatMap<Key, Value, Compare, Hash>::InsertOrAssign(Key key,
                                                                  Value value)
    -> iterator {
  const iterator it = LowerBound(key);
  if (it != entries_.end() && Equivalent(key, it->first)) {
    it->second = std::move(value);
    return it;
  }
  return entries_.emplace(it, std::move(key), std::move(value));
}

template <typename Key, typename Value, typename Compare, typename Hash>
  requires(FlatMapCompare<Compare, Key> && FlatMapHash<Hash, Key>)
template <typename K>
  requires FlatMapLookupKey<Compare, Key, K>
constexpr auto FlatMap<Key, Value, Compare, Hash>::HintPosition(
    const_iterator hint, const K& key) noexcept -> iterator {
  iterator pos = ToIterator(hint);

  const bool prev_ok = pos == entries_.begin() ||
                       compare_(std::prev(pos)->first, key) ||
                       Equivalent(std::prev(pos)->first, key);
  const bool next_ok = pos == entries_.end() || !compare_(pos->first, key);

  if (prev_ok && next_ok) {
    if (pos != entries_.begin() && Equivalent(std::prev(pos)->first, key)) {
      return std::prev(pos);
    }
    return pos;
  }
  return LowerBound(key);
}

template <typename Key, typename Value, typename Compare, typename Hash>
  requires(FlatMapCompare<Compare, Key> && FlatMapHash<Hash, Key>)
template <typename KeyArg, typename... Args>
constexpr auto FlatMap<Key, Value, Compare, Hash>::EmplaceHintImpl(
    const_iterator hint, KeyArg&& key, Args&&... args) -> iterator {
  const iterator it = HintPosition(hint, key);
  if (it != entries_.end() && Equivalent(key, it->first)) {
    return it;
  }
  return entries_.emplace(it, std::piecewise_construct,
                          std::forward_as_tuple(std::forward<KeyArg>(key)),
                          std::forward_as_tuple(std::forward<Args>(args)...));
}

template <typename Key, typename Value, typename Compare, typename Hash>
  requires(FlatMapCompare<Compare, Key> && FlatMapHash<Hash, Key>)
template <typename... Args>
  requires std::constructible_from<Value, Args...>
constexpr auto FlatMap<Key, Value, Compare, Hash>::EmplaceHint(
    const_iterator hint, const Key& key, Args&&... args) -> iterator {
  return EmplaceHintImpl(hint, key, std::forward<Args>(args)...);
}

template <typename Key, typename Value, typename Compare, typename Hash>
  requires(FlatMapCompare<Compare, Key> && FlatMapHash<Hash, Key>)
template <typename... Args>
  requires std::constructible_from<Value, Args...>
constexpr auto FlatMap<Key, Value, Compare, Hash>::EmplaceHint(
    const_iterator hint, Key&& key, Args&&... args) -> iterator {
  return EmplaceHintImpl(hint, std::move(key), std::forward<Args>(args)...);
}

template <typename Key, typename Value, typename Compare, typename Hash>
  requires(FlatMapCompare<Compare, Key> && FlatMapHash<Hash, Key>)
template <typename KeyArg, typename... Args>
constexpr auto FlatMap<Key, Value, Compare, Hash>::TryEmplaceImpl(
    KeyArg&& key, Args&&... args) -> std::pair<iterator, bool> {
  const iterator it = LowerBound(key);
  if (it != entries_.end() && Equivalent(key, it->first)) {
    return {it, false};
  }
  const iterator inserted =
      entries_.emplace(it, std::piecewise_construct,
                       std::forward_as_tuple(std::forward<KeyArg>(key)),
                       std::forward_as_tuple(std::forward<Args>(args)...));
  return {inserted, true};
}

template <typename Key, typename Value, typename Compare, typename Hash>
  requires(FlatMapCompare<Compare, Key> && FlatMapHash<Hash, Key>)
template <typename... Args>
  requires std::constructible_from<Value, Args...>
constexpr auto FlatMap<Key, Value, Compare, Hash>::TryEmplace(const Key& key,
                                                              Args&&... args)
    -> std::pair<iterator, bool> {
  return TryEmplaceImpl(key, std::forward<Args>(args)...);
}

template <typename Key, typename Value, typename Compare, typename Hash>
  requires(FlatMapCompare<Compare, Key> && FlatMapHash<Hash, Key>)
template <typename... Args>
  requires std::constructible_from<Value, Args...>
constexpr auto FlatMap<Key, Value, Compare, Hash>::TryEmplace(Key&& key,
                                                              Args&&... args)
    -> std::pair<iterator, bool> {
  return TryEmplaceImpl(std::move(key), std::forward<Args>(args)...);
}

template <typename Key, typename Value, typename Compare, typename Hash>
  requires(FlatMapCompare<Compare, Key> && FlatMapHash<Hash, Key>)
template <typename K>
  requires FlatMapLookupKey<Compare, Key, K>
constexpr bool FlatMap<Key, Value, Compare, Hash>::Erase(const K& key) {
  const iterator it = LowerBound(key);
  if (it == entries_.end() || compare_(key, it->first)) {
    return false;
  }
  entries_.erase(it);
  return true;
}

template <typename Key, typename Value, typename Compare, typename Hash>
  requires(FlatMapCompare<Compare, Key> && FlatMapHash<Hash, Key>)
constexpr void FlatMap<Key, Value, Compare, Hash>::Merge(const FlatMap& other) {
  if (this == &other || other.Empty()) [[unlikely]] {
    return;
  }

  container_type incoming(other.entries_.begin(), other.entries_.end(),
                          entries_.get_allocator());
  MergeSortedUnique(std::move(incoming));
}

template <typename Key, typename Value, typename Compare, typename Hash>
  requires(FlatMapCompare<Compare, Key> && FlatMapHash<Hash, Key>)
constexpr void FlatMap<Key, Value, Compare, Hash>::Merge(FlatMap&& other) {
  if (this == &other || other.Empty()) [[unlikely]] {
    return;
  }

  MergeSortedUniqueSteal(other.entries_);
}

template <typename Key, typename Value, typename Compare, typename Hash>
  requires(FlatMapCompare<Compare, Key> && FlatMapHash<Hash, Key>)
constexpr void FlatMap<Key, Value, Compare, Hash>::Swap(
    FlatMap& other) noexcept {
  HELIOS_ASSERT(
      entries_.get_allocator() == other.entries_.get_allocator(),
      "FlatMap::Swap: cannot swap maps backed by different memory resources");
  using std::swap;
  entries_.swap(other.entries_);
  swap(compare_, other.compare_);
  swap(hash_, other.hash_);
}

template <typename Key, typename Value, typename Compare, typename Hash>
  requires(FlatMapCompare<Compare, Key> && FlatMapHash<Hash, Key>)
template <typename K>
  requires FlatMapLookupKey<Compare, Key, K>
constexpr auto FlatMap<Key, Value, Compare, Hash>::Find(const K& key) noexcept
    -> iterator {
  const iterator it = LowerBound(key);
  if (it == entries_.end() || compare_(key, it->first)) {
    return entries_.end();
  }
  return it;
}

template <typename Key, typename Value, typename Compare, typename Hash>
  requires(FlatMapCompare<Compare, Key> && FlatMapHash<Hash, Key>)
template <typename K>
  requires FlatMapLookupKey<Compare, Key, K>
constexpr auto FlatMap<Key, Value, Compare, Hash>::Find(
    const K& key) const noexcept -> const_iterator {
  const const_iterator it = LowerBound(key);
  if (it == entries_.end() || compare_(key, it->first)) {
    return entries_.end();
  }
  return it;
}

template <typename Key, typename Value, typename Compare, typename Hash>
  requires(FlatMapCompare<Compare, Key> && FlatMapHash<Hash, Key>)
template <typename K>
  requires FlatMapLookupKey<Compare, Key, K>
constexpr Value& FlatMap<Key, Value, Compare, Hash>::At(const K& key) {
  const iterator it = Find(key);
  HELIOS_ASSERT(it != end(), "FlatMap::At: key not found");
  return it->second;
}

template <typename Key, typename Value, typename Compare, typename Hash>
  requires(FlatMapCompare<Compare, Key> && FlatMapHash<Hash, Key>)
template <typename K>
  requires FlatMapLookupKey<Compare, Key, K>
constexpr const Value& FlatMap<Key, Value, Compare, Hash>::At(
    const K& key) const {
  const const_iterator it = Find(key);
  HELIOS_ASSERT(it != end(), "FlatMap::At: key not found");
  return it->second;
}

template <typename Key, typename Value, typename Compare, typename Hash>
  requires(FlatMapCompare<Compare, Key> && FlatMapHash<Hash, Key>)
template <typename Pred>
constexpr auto FlatMap<Key, Value, Compare, Hash>::EraseIf(Pred pred)
    -> size_type {
  const size_type old_size = entries_.size();
  std::erase_if(entries_, std::move(pred));
  return old_size - entries_.size();
}

template <typename Key, typename Value, typename Compare, typename Hash>
  requires(FlatMapCompare<Compare, Key> && FlatMapHash<Hash, Key>)
constexpr void FlatMap<Key, Value, Compare, Hash>::SortUniqueIncoming(
    container_type& incoming) {
  if (incoming.size() < 2) {
    return;
  }

  if (!std::ranges::is_sorted(incoming, compare_, &value_type::first)) {
    std::ranges::sort(incoming, compare_, &value_type::first);
  }

  const auto unique_end = std::ranges::unique(
      incoming, [this](const value_type& lhs, const value_type& rhs) {
        return Equivalent(lhs.first, rhs.first);
      });
  incoming.erase(unique_end.begin(), incoming.end());
}

template <typename Key, typename Value, typename Compare, typename Hash>
  requires(FlatMapCompare<Compare, Key> && FlatMapHash<Hash, Key>)
constexpr void FlatMap<Key, Value, Compare, Hash>::MergeSortedUnique(
    container_type&& incoming) {
  if (incoming.empty()) {
    return;
  }

  if (entries_.empty()) {
    entries_ = std::move(incoming);
    return;
  }

  container_type merged(entries_.get_allocator());
  merged.reserve(entries_.size() + incoming.size());

  auto left = entries_.begin();
  auto right = incoming.begin();
  const auto left_end = entries_.end();
  const auto right_end = incoming.end();

  while (left != left_end && right != right_end) {
    if (compare_(left->first, right->first)) {
      merged.push_back(std::move(*left));
      ++left;
    } else if (compare_(right->first, left->first)) {
      merged.push_back(std::move(*right));
      ++right;
    } else {
      merged.push_back(std::move(*left));
      ++left;
      ++right;
    }
  }

  while (left != left_end) {
    merged.push_back(std::move(*left));
    ++left;
  }
  while (right != right_end) {
    merged.push_back(std::move(*right));
    ++right;
  }

  entries_ = std::move(merged);
}

template <typename Key, typename Value, typename Compare, typename Hash>
  requires(FlatMapCompare<Compare, Key> && FlatMapHash<Hash, Key>)
constexpr void FlatMap<Key, Value, Compare, Hash>::MergeSortedUniqueSteal(
    container_type& incoming) {
  if (incoming.empty()) {
    return;
  }
  if (entries_.empty()) {
    if (entries_.get_allocator() == incoming.get_allocator()) {
      entries_ = std::move(incoming);
    } else {
      MergeSortedUnique(container_type(incoming.begin(), incoming.end(),
                                       entries_.get_allocator()));
      incoming.clear();
    }
    return;
  }

  container_type merged(entries_.get_allocator());
  container_type leftover(incoming.get_allocator());
  merged.reserve(entries_.size() + incoming.size());
  leftover.reserve(incoming.size());

  auto left = entries_.begin();
  auto right = incoming.begin();
  const auto left_end = entries_.end();
  const auto right_end = incoming.end();

  while (left != left_end && right != right_end) {
    if (compare_(left->first, right->first)) {
      merged.push_back(std::move(*left));
      ++left;
    } else if (compare_(right->first, left->first)) {
      merged.push_back(std::move(*right));
      ++right;
    } else {
      merged.push_back(std::move(*left));
      leftover.push_back(std::move(*right));
      ++left;
      ++right;
    }
  }

  while (left != left_end) {
    merged.push_back(std::move(*left));
    ++left;
  }
  while (right != right_end) {
    merged.push_back(std::move(*right));
    ++right;
  }

  entries_ = std::move(merged);
  incoming = std::move(leftover);
}

}  // namespace helios::container
#endif  // HELIOS_MODULE_CONSUMER_SHIM
