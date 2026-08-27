#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.container;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/utils/type_info.hpp>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <memory_resource>
#include <type_traits>
#include <utility>
#endif
#include <helios/assert.hpp>
#include <helios/container/flat_map.hpp>

HELIOS_MODULE_EXPORT
namespace helios::container {

/**
 * @brief Generic type-indexed map that stores one `Storage` instance per
 * registered type key.
 * @details Uses a `FlatMap` keyed by `TypeIndex` to store `Storage` instances.
 * Each type gets its own storage entry identified by its compile-time type
 * index. Type identification uses `helios::utils::TypeIndex`.
 *
 * This class models a *map structure* — use `Ensure<T>()` / `Get<T>()` to
 * obtain the underlying `Storage` and interact with individual entries
 * directly.
 *
 * If `Storage` supports `ChangeType<T>()`, it is called automatically on
 * `Ensure<T>()`. If `Storage` supports `Merge(Storage&&)` or
 * `merge(Storage&&)`, it is called during `Merge()` for overlapping type
 * entries.
 *
 * @tparam Storage  The value type stored per type key. Must be
 * default-constructible.
 */
template <typename Storage>
class MultiTypeMap {
public:
  using TypeIndex = utils::TypeIndex;

  using size_type = size_t;

private:
  using MapType = FlatMap<TypeIndex, Storage>;

public:
  using key_type = TypeIndex;
  using mapped_type = Storage;
  using value_type = typename MapType::value_type;
  using reference = typename MapType::reference;
  using const_reference = typename MapType::const_reference;
  using pointer = typename MapType::pointer;
  using const_pointer = typename MapType::const_pointer;
  using difference_type = typename MapType::difference_type;
  using iterator = typename MapType::iterator;
  using const_iterator = typename MapType::const_iterator;
  using reverse_iterator = typename MapType::reverse_iterator;
  using const_reverse_iterator = typename MapType::const_reverse_iterator;

  constexpr MultiTypeMap() = default;

  /**
   * @brief Constructs with a PMR memory resource.
   * @warning Triggers `HELIOS_ASSERT` if `resource` is `nullptr`.
   * @param resource Memory resource used for internal storage
   */
  explicit constexpr MultiTypeMap(std::pmr::memory_resource* resource)
      : storage_(resource), resource_(resource) {}

  MultiTypeMap(std::nullptr_t) = delete;

  constexpr MultiTypeMap(const MultiTypeMap& other) { Merge(other); }

  /**
   * @brief Move-constructs by swapping storage with an empty map that uses
   * `other`'s allocator.
   * @param other Map to steal from
   */
  constexpr MultiTypeMap(MultiTypeMap&& other) noexcept(
      std::is_nothrow_swappable_v<MapType>)
      : MultiTypeMap(other.GetMemoryResource()) {
    using std::swap;
    swap(storage_, other.storage_);
  }

  constexpr ~MultiTypeMap() = default;

  constexpr MultiTypeMap& operator=(const MultiTypeMap& other);

  /**
   * @brief Move-assigns by taking `other`'s contents and leaving it empty.
   * @details Clears this map, then swaps when both maps share a memory
   * resource. Otherwise entries are merged across resources.
   * @param other Map to steal from
   * @return `*this`
   */
  constexpr MultiTypeMap& operator=(MultiTypeMap&& other) noexcept;

  /**
   * @brief Clears the Storage for type `T` (calls `Storage::Clear()` or
   * `Storage::clear()` if available).
   * @tparam T The type key to clear
   */
  template <typename T>
  constexpr void Clear() noexcept {
    Clear(TypeIndexOf<T>());
  }

  /**
   * @brief Clears the Storage for the given type index.
   * @param index The type index to clear
   */
  constexpr void Clear(TypeIndex index) noexcept;

  /// @brief Clears all per-type storages (calls `Clear()`/`clear()` on each
  /// entry if available).
  constexpr void ClearAll() noexcept;

  /**
   * @brief Resets (removes) the Storage entry for type `T`.
   * @tparam T The type key to reset
   */
  template <typename T>
  constexpr void Reset() noexcept {
    storage_.Erase(TypeIndexOf<T>());
  }

  /**
   * @brief Resets (removes) the Storage entry for the given type index.
   * @param index The type index to reset
   */
  constexpr void Reset(TypeIndex index) noexcept { storage_.Erase(index); }

  /// @brief Removes all Storage entries from the map.
  constexpr void ResetAll() noexcept { storage_.Clear(); }

  /**
   * @brief Reserves storage for at least `count` type entries.
   * @param count Minimum number of type keys the map should hold without
   * reallocating
   */
  constexpr void Reserve(size_type count) { storage_.Reserve(count); }

  /**
   * @brief Creates Storage for type `T` with the given value if absent.
   * @details The type key is derived from `T`
   * @tparam T The type key — `TypeIndexOf<T>` is used as the map key
   * @tparam Args The argument types for constructing Storage
   * @param args The arguments to construct the Storage value
   * (perfect-forwarded)
   * @return Iterator to the entry. If `T` was already present, the existing
   * entry is left unchanged
   */
  template <typename T, typename... Args>
    requires std::constructible_from<Storage, Args...>
  constexpr auto Emplace(Args&&... args) {
    return storage_.Emplace(TypeIndexOf<T>(), std::forward<Args>(args)...);
  }

  /**
   * @brief Creates Storage for type `T` with the given value, only if no entry
   * for `T` already exists.
   * @details The type key is derived from `T`
   * Unlike `Emplace`, this never overwrites an existing entry.
   * @tparam T The type key — `TypeIndexOf<T>` is used as the map key
   * @tparam Args The argument types for constructing Storage
   * @param args The arguments to construct the Storage value
   * (perfect-forwarded)
   * @return Pair of iterator to the entry and bool — `true` if a new entry was
   * inserted
   */
  template <typename T, typename... Args>
    requires std::constructible_from<Storage, Args...>
  constexpr auto TryEmplace(Args&&... args) {
    return storage_.TryEmplace(TypeIndexOf<T>(), std::forward<Args>(args)...);
  }

  /**
   * @brief Removes Storage for type `T` entirely.
   * @tparam T The type key to remove
   * @return true if storage was removed
   */
  template <typename T>
  constexpr bool Remove() noexcept {
    return Remove(TypeIndexOf<T>());
  }

  /**
   * @brief Removes Storage for the given type index entirely.
   * @param index The type index to remove
   * @return true if storage was removed
   */
  constexpr bool Remove(TypeIndex index) noexcept {
    return storage_.Erase(index);
  }

  /**
   * @brief Ensures a Storage entry exists for type `T` and returns a reference
   * to it.
   * @details If no entry exists for `T`, a new default-constructed Storage is
   * inserted. If `Storage` supports `ChangeType<T>()`, it is called on the
   * newly created entry.
   * @tparam T The type key to ensure storage for
   * @return Reference to the Storage for type `T`
   */
  template <typename T>
  constexpr Storage& Ensure();

  /**
   * @brief Ensures a Storage entry exists for the given type index and returns
   * a reference to it.
   * @details If no entry exists, a new default-constructed Storage is inserted.
   * @param index The type index to ensure storage for
   * @return Reference to the Storage for the given type index
   */
  constexpr Storage& Ensure(TypeIndex index);

  /**
   * @brief Gets Storage for type `T`.
   * @warning Triggers assertion if storage for type `T` doesn't exist.
   * @tparam T The type key to get storage for
   * @return Reference to the Storage for type `T`
   */
  template <typename T>
  [[nodiscard]] constexpr Storage& Get() noexcept;

  /**
   * @brief Gets Storage for type `T` (const).
   * @warning Triggers assertion if storage for type `T` doesn't exist.
   * @tparam T The type key to get storage for
   * @return Const reference to the Storage for type `T`
   */
  template <typename T>
  [[nodiscard]] constexpr const Storage& Get() const noexcept;

  /**
   * @brief Gets Storage for the given type index.
   * @warning Triggers assertion if storage for the given type index doesn't
   * exist.
   * @param index The type index to get storage for
   * @return Reference to the Storage for the given type index
   */
  [[nodiscard]] constexpr Storage& Get(TypeIndex index) noexcept;

  /**
   * @brief Gets Storage for the given type index (const).
   * @warning Triggers assertion if storage for the given type index doesn't
   * exist.
   * @param index The type index to get storage for
   * @return Const reference to the Storage for the given type index
   */
  [[nodiscard]] constexpr const Storage& Get(TypeIndex index) const noexcept;

  /**
   * @brief Tries to get Storage for type `T`.
   * @tparam T The type key to get storage for
   * @return Pointer to Storage if exists, `nullptr` otherwise
   */
  template <typename T>
  [[nodiscard]] constexpr Storage* TryGet() noexcept {
    return TryGet(TypeIndexOf<T>());
  }

  /**
   * @brief Tries to get Storage for type `T` (const).
   * @tparam T The type key to get storage for
   * @return Const pointer to Storage if exists, `nullptr` otherwise
   */
  template <typename T>
  [[nodiscard]] constexpr const Storage* TryGet() const noexcept {
    return TryGet(TypeIndexOf<T>());
  }

  /**
   * @brief Tries to get Storage for the given type index.
   * @param index The type index to get storage for
   * @return Pointer to Storage if exists, `nullptr` otherwise
   */
  [[nodiscard]] constexpr Storage* TryGet(TypeIndex index) noexcept;

  /**
   * @brief Tries to get Storage for the given type index (const).
   * @param index The type index to get storage for
   * @return Const pointer to Storage if exists, `nullptr` otherwise
   */
  [[nodiscard]] constexpr const Storage* TryGet(TypeIndex index) const noexcept;

  /**
   * @brief Merges all entries from another `MultiTypeMap` into this one.
   * @details For each entry in `other`:
   * - If this map already contains the same type key, calls `Storage::Merge` or
   * `Storage::merge` on the existing entry if such a method is available.
   * - Otherwise, the entry from `other` is inserted into this map. For
   * differing Storage types, a new default Storage is created and
   * `Merge`/`merge` is attempted on it.
   * @tparam OtherStorage Storage type of the other map (may differ from
   * Storage)
   * @param other The map to merge from
   */
  template <typename OtherStorage>
  constexpr void Merge(const MultiTypeMap<OtherStorage>& other);

  /**
   * @brief Merges all entries from another `MultiTypeMap` into this one by
   * moving them.
   * @details For each entry in `other`:
   * - If this map already contains the same type key, calls `Storage::Merge` or
   * `Storage::merge` on the existing entry if such a method is available.
   * - Otherwise, the entry from `other` is inserted into this map. For
   * differing Storage types, a new default Storage is created and
   * `Merge`/`merge` is attempted on it.
   *
   * After merging, `other` is fully cleared (`TypeCount() == 0`).
   *
   * @tparam OtherStorage Storage type of the other map (may differ from
   * Storage)
   * @param other The map to merge from
   */
  template <typename OtherStorage>
  constexpr void Merge(MultiTypeMap<OtherStorage>&& other);

  /**
   * @brief Releases unused capacity in the map and, when available, in each
   * stored `Storage`.
   * @details If `Storage` provides `ShrinkToFit()` or `shrink_to_fit()`, it is
   * invoked on every entry before the map's own storage is compacted.
   */
  constexpr void ShrinkToFit();

  /**
   * @brief Swaps contents with another MultiTypeMap.
   * @param other Map to swap with
   */
  constexpr void Swap(MultiTypeMap& other) noexcept;
  friend constexpr void swap(MultiTypeMap& lhs, MultiTypeMap& rhs) noexcept {
    lhs.Swap(rhs);
  }

  /**
   * @brief Checks if a Storage entry exists for type `T`.
   * @tparam T The type key to check
   * @return true if an entry exists
   */
  template <typename T>
  [[nodiscard]] constexpr bool Contains() const noexcept {
    return Contains(TypeIndexOf<T>());
  }

  /**
   * @brief Checks if a Storage entry exists for the given type index.
   * @param index The type index to check
   * @return true if an entry exists
   */
  [[nodiscard]] constexpr bool Contains(TypeIndex index) const noexcept {
    return storage_.Contains(index);
  }

  /**
   * @brief Checks if the Storage for type `T` is empty (no elements / no
   * value).
   * @details Returns true if the entry doesn't exist, or if Storage supports
   * `Empty()`/`empty()` and it returns true.
   * @tparam T The type key to check
   * @return true if storage is empty or doesn't exist
   */
  template <typename T>
  [[nodiscard]] constexpr bool Empty() const noexcept {
    return Empty(TypeIndexOf<T>());
  }

  /**
   * @brief Checks if the Storage for the given type index is empty.
   * @param index The type index to check
   * @return true if storage is empty or doesn't exist
   */
  [[nodiscard]] constexpr bool Empty(TypeIndex index) const noexcept;

  /**
   * @brief Returns true if all per-type storages are empty or if the map has no
   * entries.
   * @return true if all storages are empty
   */
  [[nodiscard]] constexpr bool EmptyAll() const noexcept;

  /**
   * @brief Gets the compile-time type index for `T`.
   * @tparam T The type to get index for
   * @return Type index
   */
  template <typename T>
  [[nodiscard]] static constexpr TypeIndex TypeIndexOf() noexcept {
    return utils::TypeIndex::From<T>();
  }

  /**
   * @brief Returns the total number of elements across all entries.
   * @details Only meaningful if Storage supports `Size()` or `size()`.
   * Returns number of map entries otherwise.
   * @return Total element count (or entry count if Storage has no size method)
   */
  [[nodiscard]] constexpr size_type Size() const noexcept;

  /**
   * @brief Returns the number of elements stored for type `T`.
   * @details Returns `Storage::Size()` or `Storage::size()` if available, else
   * 0 if entry exists.
   * @tparam T The type key to query
   * @return Element count, 0 if storage doesn't exist
   */
  template <typename T>
  [[nodiscard]] constexpr size_type Size() const noexcept {
    return Size(TypeIndexOf<T>());
  }

  /**
   * @brief Returns the number of elements stored for the given type index.
   * @param index The type index to query
   * @return Element count, 0 if storage doesn't exist
   */
  [[nodiscard]] constexpr size_type Size(TypeIndex index) const noexcept;

  /**
   * @brief Returns the number of registered type entries.
   * @return Number of registered type entries
   */
  [[nodiscard]] constexpr size_type TypeCount() const noexcept {
    return storage_.Size();
  }

  /**
   * @brief Returns a view of the underlying map (non-const).
   * @return Reference to the underlying flat_map
   */
  [[nodiscard]] constexpr MapType& Data() noexcept { return storage_; }

  /**
   * @brief Returns a view of the underlying map (const).
   * @return Const reference to the underlying flat_map
   */
  [[nodiscard]] constexpr const MapType& Data() const noexcept {
    return storage_;
  }

  /**
   * @brief Returns the memory resource used for internal storage.
   * @return Memory resource passed to the constructor, or the default resource
   */
  [[nodiscard]] constexpr std::pmr::memory_resource* GetMemoryResource()
      const noexcept {
    return resource_;
  }

  /**
   * @brief Returns an iterator to the beginning of the map entries.
   * @return Iterator to the beginning of the map entries
   */
  [[nodiscard]] constexpr iterator begin() noexcept { return storage_.begin(); }

  /**
   * @brief Returns a const iterator to the beginning of the map entries.
   * @return Const iterator to the beginning of the map entries
   */
  [[nodiscard]] constexpr const_iterator begin() const noexcept {
    return storage_.begin();
  }

  /**
   * @brief Returns a const iterator to the beginning of the map entries.
   * @return Const iterator to the beginning of the map entries
   */
  [[nodiscard]] constexpr const_iterator cbegin() const noexcept {
    return storage_.cbegin();
  }

  /**
   * @brief Returns an iterator to the end of the map entries.
   * @return Iterator to the end of the map entries
   */
  [[nodiscard]] constexpr iterator end() noexcept { return storage_.end(); }

  /**
   * @brief Returns a const iterator to the end of the map entries.
   * @return Const iterator to the end of the map entries
   */
  [[nodiscard]] constexpr const_iterator end() const noexcept {
    return storage_.end();
  }

  /**
   * @brief Returns a const iterator to the end of the map entries.
   * @return Const iterator to the end of the map entries
   */
  [[nodiscard]] constexpr const_iterator cend() const noexcept {
    return storage_.cend();
  }

  /**
   * @brief Returns a reverse iterator to the last map entry.
   * @return Reverse iterator to the last map entry
   */
  [[nodiscard]] constexpr reverse_iterator rbegin() noexcept {
    return storage_.rbegin();
  }

  /**
   * @brief Returns a const reverse iterator to the last map entry.
   * @return Const reverse iterator to the last map entry
   */
  [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept {
    return storage_.rbegin();
  }

  /**
   * @brief Returns a const reverse iterator to the last map entry.
   * @return Const reverse iterator to the last map entry
   */
  [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept {
    return storage_.crbegin();
  }

  /**
   * @brief Returns a reverse iterator before the first map entry.
   * @return Reverse iterator before the first map entry
   */
  [[nodiscard]] constexpr reverse_iterator rend() noexcept {
    return storage_.rend();
  }

  /**
   * @brief Returns a const reverse iterator before the first map entry.
   * @return Const reverse iterator before the first map entry
   */
  [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept {
    return storage_.rend();
  }

  /**
   * @brief Returns a const reverse iterator before the first map entry.
   * @return Const reverse iterator before the first map entry
   */
  [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept {
    return storage_.crend();
  }

private:
  template <typename OtherStorage>
  friend class MultiTypeMap;

  /// @brief Creates new Storage initialized with the map resource if
  /// constructible from it, else default.
  [[nodiscard]] constexpr Storage MakeStorage() const;

  MapType storage_;
  std::pmr::memory_resource* resource_ = std::pmr::get_default_resource();
};

template <typename Storage>
constexpr auto MultiTypeMap<Storage>::operator=(const MultiTypeMap& other)
    -> MultiTypeMap& {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  storage_ = other.storage_;
  return *this;
}

template <typename Storage>
constexpr auto MultiTypeMap<Storage>::operator=(MultiTypeMap&& other) noexcept
    -> MultiTypeMap& {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  ResetAll();
  if (resource_ == other.resource_) {
    using std::swap;
    swap(storage_, other.storage_);
  } else {
    Merge(std::move(other));
  }
  return *this;
}

template <typename Storage>
constexpr void MultiTypeMap<Storage>::Clear(TypeIndex index) noexcept {
  if (auto* ptr = TryGet(index)) [[likely]] {
    if constexpr (requires { ptr->Clear(); }) {
      ptr->Clear();
    } else if constexpr (requires { ptr->clear(); }) {
      ptr->clear();
    }
  }
}

template <typename Storage>
constexpr void MultiTypeMap<Storage>::ClearAll() noexcept {
  for (auto& [_, storage] : storage_) {
    if constexpr (requires { storage.Clear(); }) {
      storage.Clear();
    } else if constexpr (requires { storage.clear(); }) {
      storage.clear();
    }
  }
}

template <typename Storage>
template <typename T>
constexpr Storage& MultiTypeMap<Storage>::Ensure() {
  constexpr auto type_index = TypeIndexOf<T>();
  const auto it = storage_.Find(type_index);
  if (it == storage_.end()) {
    return storage_.Emplace(type_index, MakeStorage())->second;
  }
  return it->second;
}

template <typename Storage>
constexpr Storage& MultiTypeMap<Storage>::Ensure(TypeIndex index) {
  const auto it = storage_.Find(index);
  if (it == storage_.end()) {
    return storage_.Emplace(index, MakeStorage())->second;
  }
  return it->second;
}

template <typename Storage>
template <typename T>
constexpr Storage& MultiTypeMap<Storage>::Get() noexcept {
  constexpr auto type_index = TypeIndexOf<T>();
  const auto it = storage_.Find(type_index);
  HELIOS_ASSERT(it != storage_.end(), "Storage for type '{}' does not exist!",
                utils::TypeNameOf<T>());
  return it->second;
}

template <typename Storage>
template <typename T>
constexpr const Storage& MultiTypeMap<Storage>::Get() const noexcept {
  constexpr auto type_index = TypeIndexOf<T>();
  const auto it = storage_.Find(type_index);
  HELIOS_ASSERT(it != storage_.end(), "Storage for type '{}' does not exist!",
                utils::TypeNameOf<T>());
  return it->second;
}

template <typename Storage>
constexpr Storage& MultiTypeMap<Storage>::Get(TypeIndex index) noexcept {
  const auto it = storage_.Find(index);
  HELIOS_ASSERT(it != storage_.end(),
                "Storage for type index '{}' does not exist!", index.Hash());
  return it->second;
}

template <typename Storage>
constexpr const Storage& MultiTypeMap<Storage>::Get(
    TypeIndex index) const noexcept {
  const auto it = storage_.Find(index);
  HELIOS_ASSERT(it != storage_.end(),
                "Storage for type index '{}' does not exist!", index.Hash());
  return it->second;
}

template <typename Storage>
constexpr Storage* MultiTypeMap<Storage>::TryGet(TypeIndex index) noexcept {
  const auto it = storage_.Find(index);
  return it != storage_.end() ? &it->second : nullptr;
}

template <typename Storage>
constexpr const Storage* MultiTypeMap<Storage>::TryGet(
    TypeIndex index) const noexcept {
  const auto it = storage_.Find(index);
  return it != storage_.end() ? &it->second : nullptr;
}

template <typename Storage>
template <typename OtherStorage>
constexpr void MultiTypeMap<Storage>::Merge(
    const MultiTypeMap<OtherStorage>& other) {
  for (const auto& [index, other_storage] : other.storage_) {
    if (const auto it = storage_.Find(index); it != storage_.end()) {
      // Existing key: prefer const overloads, then fall back to copy+move.
      if constexpr (requires { it->second.Merge(other_storage); }) {
        it->second.Merge(other_storage);
      } else if constexpr (requires { it->second.merge(other_storage); }) {
        it->second.merge(other_storage);
      } else if constexpr (std::copy_constructible<OtherStorage> &&
                           requires(OtherStorage copy) {
                             it->second.Merge(std::move(copy));
                           }) {
        auto copy = other_storage;
        it->second.Merge(std::move(copy));
      } else if constexpr (std::copy_constructible<OtherStorage> &&
                           requires(OtherStorage copy) {
                             it->second.merge(std::move(copy));
                           }) {
        auto copy = other_storage;
        it->second.merge(std::move(copy));
      }
      // else: no merge method available — existing entry is left unchanged.
    } else {
      // New key: insert storage.
      if constexpr (std::same_as<Storage, OtherStorage> &&
                    std::copy_constructible<Storage>) {
        // Same storage type — copy directly.
        storage_.Emplace(index, other_storage);
      } else {
        // Different storage type — create a new default Storage and attempt to
        // merge into it.
        auto new_storage = MakeStorage();
        if constexpr (requires { new_storage.Merge(other_storage); }) {
          new_storage.Merge(other_storage);
        } else if constexpr (requires { new_storage.merge(other_storage); }) {
          new_storage.merge(other_storage);
        } else if constexpr (std::copy_constructible<OtherStorage> &&
                             requires(OtherStorage copy) {
                               new_storage.Merge(std::move(copy));
                             }) {
          auto copy = other_storage;
          new_storage.Merge(std::move(copy));
        } else if constexpr (std::copy_constructible<OtherStorage> &&
                             requires(OtherStorage copy) {
                               new_storage.merge(std::move(copy));
                             }) {
          auto copy = other_storage;
          new_storage.merge(std::move(copy));
        } else if constexpr (std::constructible_from<Storage,
                                                     const OtherStorage&>) {
          new_storage = Storage(other_storage);
        }
        storage_.Emplace(index, std::move(new_storage));
      }
    }
  }
}

template <typename Storage>
template <typename OtherStorage>
constexpr void MultiTypeMap<Storage>::Merge(
    MultiTypeMap<OtherStorage>&& other) {
  for (auto& [index, other_storage] : other.storage_) {
    if (const auto it = storage_.Find(index); it != storage_.end()) {
      // Existing key: call Merge or merge on Storage if available.
      if constexpr (requires { it->second.Merge(std::move(other_storage)); }) {
        it->second.Merge(std::move(other_storage));
      } else if constexpr (requires {
                             it->second.merge(std::move(other_storage));
                           }) {
        it->second.merge(std::move(other_storage));
      }
      // else: no merge method available — existing entry is left unchanged.
    } else {
      // New key: insert storage.
      if constexpr (std::same_as<Storage, OtherStorage>) {
        // Same storage type — move directly.
        storage_.Emplace(index, std::move(other_storage));
      } else {
        // Different storage type — create a new default Storage and attempt to
        // merge into it.
        auto new_storage = MakeStorage();
        if constexpr (requires {
                        new_storage.Merge(std::move(other_storage));
                      }) {
          new_storage.Merge(std::move(other_storage));
        } else if constexpr (requires {
                               new_storage.merge(std::move(other_storage));
                             }) {
          new_storage.merge(std::move(other_storage));
        } else if constexpr (std::constructible_from<Storage, OtherStorage&&>) {
          new_storage = Storage(std::move(other_storage));
        }
        storage_.Emplace(index, std::move(new_storage));
      }
    }
  }
  other.storage_.Clear();
}

template <typename Storage>
constexpr void MultiTypeMap<Storage>::ShrinkToFit() {
  for (auto& [_, storage] : storage_) {
    if constexpr (requires { storage.ShrinkToFit(); }) {
      storage.ShrinkToFit();
    } else if constexpr (requires { storage.shrink_to_fit(); }) {
      storage.shrink_to_fit();
    }
  }

  storage_.ShrinkToFit();
}

template <typename Storage>
constexpr void MultiTypeMap<Storage>::Swap(MultiTypeMap& other) noexcept {
  if (resource_ == other.resource_) {
    using std::swap;
    swap(storage_, other.storage_);
    return;
  }

  MultiTypeMap tmp_this(GetMemoryResource());
  tmp_this.Merge(std::move(*this));
  MultiTypeMap tmp_other(other.GetMemoryResource());
  tmp_other.Merge(std::move(other));
  Merge(std::move(tmp_other));
  other.Merge(std::move(tmp_this));
}

template <typename Storage>
constexpr bool MultiTypeMap<Storage>::Empty(TypeIndex index) const noexcept {
  const auto* ptr = TryGet(index);
  if (ptr == nullptr) {
    return true;
  }
  if constexpr (requires { ptr->Empty(); }) {
    return ptr->Empty();
  } else if constexpr (requires { ptr->empty(); }) {
    return ptr->empty();
  } else {
    return false;
  }
}

template <typename Storage>
constexpr bool MultiTypeMap<Storage>::EmptyAll() const noexcept {
  return std::ranges::all_of(storage_, [](const auto& entry) {
    if constexpr (requires { entry.second.Empty(); }) {
      return entry.second.Empty();
    } else if constexpr (requires { entry.second.empty(); }) {
      return entry.second.empty();
    } else {
      return true;
    }
  });
}

template <typename Storage>
constexpr auto MultiTypeMap<Storage>::Size() const noexcept -> size_type {
  size_type total = 0;
  for (const auto& [_, storage] : storage_) {
    if constexpr (requires { storage.Size(); }) {
      total += storage.Size();
    } else if constexpr (requires { storage.size(); }) {
      total += storage.size();
    } else {
      ++total;
    }
  }
  return total;
}

template <typename Storage>
constexpr auto MultiTypeMap<Storage>::Size(TypeIndex index) const noexcept
    -> size_type {
  const auto* ptr = TryGet(index);
  if (ptr == nullptr) {
    return 0;
  }
  if constexpr (requires { ptr->Size(); }) {
    return ptr->Size();
  } else if constexpr (requires { ptr->size(); }) {
    return ptr->size();
  } else {
    return 0;
  }
}

template <typename Storage>
constexpr auto MultiTypeMap<Storage>::MakeStorage() const -> Storage {
  if constexpr (std::constructible_from<Storage, std::pmr::memory_resource*>) {
    return Storage(resource_);
  } else {
    return Storage{};
  }
}

}  // namespace helios::container
#endif  // HELIOS_MODULE_CONSUMER_SHIM
