#pragma once

#include <helios/assert.hpp>
#include <helios/compiler/compiler.hpp>
#include <helios/utils/type_info.hpp>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <memory>
#include <memory_resource>
#include <type_traits>
#include <utility>

#ifdef HELIOS_STL_FLAT_MAP_AVAILABLE
#include <flat_map>
#else
#include <boost/container/flat_map.hpp>
#endif

namespace helios::container {

/**
 * @brief Generic type-indexed map that stores one `Storage` instance per
 * registered type key.
 * @details Uses a flat_map keyed by `TypeIndex` to store `Storage` instances.
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
 * @tparam Allocator The allocator type for the underlying flat_map (default:
 * `std::allocator<std::byte>`)
 */
template <typename Storage, typename Allocator = std::allocator<std::byte>>
class MultiTypeMap {
public:
  using TypeIndex = utils::TypeIndex;

  using size_type = size_t;
  using allocator_type = Allocator;

private:
#ifdef HELIOS_STL_FLAT_MAP_AVAILABLE
  using KeyContainer =
      std::vector<TypeIndex, typename std::allocator_traits<allocator_type>::
                                 template rebind_alloc<TypeIndex>>;
  using MappedContainer =
      std::vector<Storage, typename std::allocator_traits<
                               allocator_type>::template rebind_alloc<Storage>>;

  using MapType = std::flat_map<TypeIndex, Storage, std::less<TypeIndex>,
                                KeyContainer, MappedContainer>;
#else
  using MapValueType = std::pair<TypeIndex, Storage>;
  using MapAllocator = typename std::allocator_traits<
      allocator_type>::template rebind_alloc<MapValueType>;

  using MapType =
      boost::container::flat_map<TypeIndex, Storage, std::less<TypeIndex>,
                                 MapAllocator>;
#endif

public:
  constexpr MultiTypeMap() = default;

#ifdef HELIOS_STL_FLAT_MAP_AVAILABLE
  /**
   * @brief Constructs with a custom allocator.
   * @param alloc Allocator instance to use
   */
  explicit constexpr MultiTypeMap(const allocator_type& alloc)
      : storage_(alloc), allocator_(alloc) {}
#else
  /**
   * @brief Constructs with a custom allocator.
   * @param alloc Allocator instance to use
   */
  explicit constexpr MultiTypeMap(const allocator_type& alloc)
      : storage_(MapAllocator(alloc)), allocator_(alloc) {}
#endif

  /**
   * @brief Constructs with a PMR memory resource.
   * @details Enabled only when `allocator_type` is constructible from
   * `std::pmr::memory_resource*`.
   * @param resource Memory resource used to construct allocator
   */
  explicit constexpr MultiTypeMap(std::pmr::memory_resource* resource) noexcept(
      std::is_nothrow_constructible_v<allocator_type,
                                      std::pmr::memory_resource*>)
    requires std::constructible_from<allocator_type, std::pmr::memory_resource*>
      : MultiTypeMap(allocator_type{resource}) {}

  MultiTypeMap(std::nullptr_t) = delete;

  constexpr MultiTypeMap(const MultiTypeMap& other) = default;
  constexpr MultiTypeMap(MultiTypeMap&& other) noexcept = default;
  constexpr ~MultiTypeMap() = default;

  constexpr MultiTypeMap& operator=(const MultiTypeMap& other) = default;
  constexpr MultiTypeMap& operator=(MultiTypeMap&& other) noexcept = default;

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
    storage_.erase(TypeIndexOf<T>());
  }

  /**
   * @brief Resets (removes) the Storage entry for the given type index.
   * @param index The type index to reset
   */
  constexpr void Reset(TypeIndex index) noexcept { storage_.erase(index); }

  /// @brief Removes all Storage entries from the map.
  constexpr void ResetAll() noexcept { storage_.clear(); }

  /**
   * @brief Creates or replaces Storage for type `T` with the given value.
   * @details The type key is derived from `T`
   * @tparam T The type key — `TypeIndexOf<T>` is used as the map key
   * @tparam Args The argument types for constructing Storage
   * @param args The arguments to construct the Storage value
   * (perfect-forwarded)
   * @return Pair of iterator to the entry and bool — `true` if a new entry was
   * inserted
   */
  template <typename T, typename... Args>
    requires std::constructible_from<Storage, Args...>
  constexpr auto Emplace(Args&&... args) {
    return storage_.emplace(TypeIndexOf<T>(), std::forward<Args>(args)...);
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
    return storage_.try_emplace(TypeIndexOf<T>(), std::forward<Args>(args)...);
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
    return storage_.erase(index) > 0;
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
   * @brief Merges all entries from another MultiTypeMap into this one.
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
   * @tparam OtherAllocator The allocator template of the other map
   * @param other The map to merge from
   */
  template <typename OtherStorage, typename OtherAllocator>
  constexpr void Merge(const MultiTypeMap<OtherStorage, OtherAllocator>& other);

  template <typename OtherStorage, typename OtherAllocator>
  constexpr void Merge(MultiTypeMap<OtherStorage, OtherAllocator>&& other);

  /**
   * @brief Swaps contents with another MultiTypeMap.
   * @param other Map to swap with
   */
  constexpr void Swap(MultiTypeMap& other) noexcept(
      std::is_nothrow_swappable_v<MapType> &&
      std::is_nothrow_swappable_v<allocator_type>) {
    std::swap(storage_, other.storage_);
    std::swap(allocator_, other.allocator_);
  }

  friend constexpr void swap(MultiTypeMap& lhs, MultiTypeMap& rhs) noexcept(
      std::is_nothrow_swappable_v<MapType> &&
      std::is_nothrow_swappable_v<allocator_type>) {
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
    return storage_.contains(index);
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
    return storage_.size();
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
   * @brief Gets the allocator.
   * @return Copy of the allocator
   */
  [[nodiscard]] constexpr allocator_type GetAllocator() const noexcept {
    return allocator_;
  }

  /**
   * @brief Returns an iterator to the beginning of the map entries.
   * @return Iterator to the beginning of the map entries
   */
  [[nodiscard]] constexpr auto begin() noexcept { return storage_.begin(); }

  /**
   * @brief Returns a const iterator to the beginning of the map entries.
   * @return Const iterator to the beginning of the map entries
   */
  [[nodiscard]] constexpr auto begin() const noexcept {
    return storage_.begin();
  }

  /**
   * @brief Returns a const iterator to the beginning of the map entries.
   * @return Const iterator to the beginning of the map entries
   */
  [[nodiscard]] constexpr auto cbegin() const noexcept {
    return storage_.cbegin();
  }

  /**
   * @brief Returns an iterator to the end of the map entries.
   * @return Iterator to the end of the map entries
   */
  [[nodiscard]] constexpr auto end() noexcept { return storage_.end(); }

  /**
   * @brief Returns a const iterator to the end of the map entries.
   * @return Const iterator to the end of the map entries
   */
  [[nodiscard]] constexpr auto end() const noexcept { return storage_.end(); }

  /**
   * @brief Returns a const iterator to the end of the map entries.
   * @return Const iterator to the end of the map entries
   */
  [[nodiscard]] constexpr auto cend() const noexcept { return storage_.cend(); }

private:
  template <typename OtherStorage, typename OtherA>
  friend class MultiTypeMap;

  /// @brief Creates new Storage initialized with allocator if constructible
  /// from it, else default.
  [[nodiscard]] constexpr Storage MakeStorage() const;

  MapType storage_;
  [[no_unique_address]] allocator_type allocator_{};
};

template <typename Storage, typename Allocator>
constexpr void MultiTypeMap<Storage, Allocator>::Clear(
    TypeIndex index) noexcept {
  if (auto* ptr = TryGet(index)) [[likely]] {
    if constexpr (requires { ptr->Clear(); }) {
      ptr->Clear();
    } else if constexpr (requires { ptr->clear(); }) {
      ptr->clear();
    }
  }
}

template <typename Storage, typename Allocator>
constexpr void MultiTypeMap<Storage, Allocator>::ClearAll() noexcept {
  for (auto&& [_, storage] : storage_) {
    if constexpr (requires { storage.Clear(); }) {
      storage.Clear();
    } else if constexpr (requires { storage.clear(); }) {
      storage.clear();
    }
  }
}

template <typename Storage, typename Allocator>
template <typename T>
constexpr Storage& MultiTypeMap<Storage, Allocator>::Ensure() {
  constexpr auto type_index = TypeIndexOf<T>();
  const auto it = storage_.find(type_index);
  if (it == storage_.end()) {
    const auto [inserted_iter, success] =
        storage_.emplace(type_index, MakeStorage());
    HELIOS_ASSERT(success, "Failed to create storage for type '{}'!",
                  utils::TypeNameOf<T>());
    return inserted_iter->second;
  }
  return it->second;
}

template <typename Storage, typename Allocator>
constexpr Storage& MultiTypeMap<Storage, Allocator>::Ensure(TypeIndex index) {
  const auto it = storage_.find(index);
  if (it == storage_.end()) {
    const auto [inserted_iter, success] =
        storage_.emplace(index, MakeStorage());
    HELIOS_ASSERT(success, "Failed to create storage for type index '{}'!",
                  index.Hash());
    return inserted_iter->second;
  }
  return it->second;
}

template <typename Storage, typename Allocator>
template <typename T>
constexpr Storage& MultiTypeMap<Storage, Allocator>::Get() noexcept {
  constexpr auto type_index = TypeIndexOf<T>();
  const auto it = storage_.find(type_index);
  HELIOS_ASSERT(it != storage_.end(), "Storage for type '{}' does not exist!",
                utils::TypeNameOf<T>());
  return it->second;
}

template <typename Storage, typename Allocator>
template <typename T>
constexpr const Storage& MultiTypeMap<Storage, Allocator>::Get()
    const noexcept {
  constexpr auto type_index = TypeIndexOf<T>();
  const auto it = storage_.find(type_index);
  HELIOS_ASSERT(it != storage_.end(), "Storage for type '{}' does not exist!",
                utils::TypeNameOf<T>());
  return it->second;
}

template <typename Storage, typename Allocator>
constexpr Storage& MultiTypeMap<Storage, Allocator>::Get(
    TypeIndex index) noexcept {
  const auto it = storage_.find(index);
  HELIOS_ASSERT(it != storage_.end(),
                "Storage for type index '{}' does not exist!", index.Hash());
  return it->second;
}

template <typename Storage, typename Allocator>
constexpr const Storage& MultiTypeMap<Storage, Allocator>::Get(
    TypeIndex index) const noexcept {
  const auto it = storage_.find(index);
  HELIOS_ASSERT(it != storage_.end(),
                "Storage for type index '{}' does not exist!", index.Hash());
  return it->second;
}

template <typename Storage, typename Allocator>
constexpr Storage* MultiTypeMap<Storage, Allocator>::TryGet(
    TypeIndex index) noexcept {
  const auto it = storage_.find(index);
  return it != storage_.end() ? &it->second : nullptr;
}

template <typename Storage, typename Allocator>
constexpr const Storage* MultiTypeMap<Storage, Allocator>::TryGet(
    TypeIndex index) const noexcept {
  const auto it = storage_.find(index);
  return it != storage_.end() ? &it->second : nullptr;
}

template <typename Storage, typename Allocator>
template <typename OtherStorage, typename OtherAllocator>
constexpr void MultiTypeMap<Storage, Allocator>::Merge(
    const MultiTypeMap<OtherStorage, OtherAllocator>& other) {
  for (const auto& [index, other_storage] : other.storage_) {
    if (const auto it = storage_.find(index); it != storage_.end()) {
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
        storage_.emplace(index, other_storage);
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
        storage_.emplace(index, std::move(new_storage));
      }
    }
  }
}

template <typename Storage, typename Allocator>
template <typename OtherStorage, typename OtherAllocator>
constexpr void MultiTypeMap<Storage, Allocator>::Merge(
    MultiTypeMap<OtherStorage, OtherAllocator>&& other) {
  for (auto&& [index, other_storage] : other.storage_) {
    if (const auto it = storage_.find(index); it != storage_.end()) {
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
        storage_.emplace(index, std::move(other_storage));
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
        storage_.emplace(index, std::move(new_storage));
      }
    }
  }
  other.storage_.clear();
}

template <typename Storage, typename Allocator>
constexpr bool MultiTypeMap<Storage, Allocator>::Empty(
    TypeIndex index) const noexcept {
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

template <typename Storage, typename Allocator>
constexpr bool MultiTypeMap<Storage, Allocator>::EmptyAll() const noexcept {
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

template <typename Storage, typename Allocator>
constexpr auto MultiTypeMap<Storage, Allocator>::Size() const noexcept
    -> size_type {
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

template <typename Storage, typename Allocator>
constexpr auto MultiTypeMap<Storage, Allocator>::Size(
    TypeIndex index) const noexcept -> size_type {
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

template <typename Storage, typename Allocator>
constexpr auto MultiTypeMap<Storage, Allocator>::MakeStorage() const
    -> Storage {
  if constexpr (std::constructible_from<Storage, allocator_type>) {
    return Storage(allocator_);
  } else {
    return Storage{};
  }
}

template <typename Storage>
using PmrMultiTypeMap =
    MultiTypeMap<Storage, std::pmr::polymorphic_allocator<std::byte>>;

}  // namespace helios::container
