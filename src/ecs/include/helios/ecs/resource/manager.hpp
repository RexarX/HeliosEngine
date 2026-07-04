#pragma once

#include <helios/assert.hpp>
#include <helios/container/multi_type_map.hpp>
#include <helios/container/typed_buffer.hpp>
#include <helios/ecs/resource/resource.hpp>
#include <helios/utils/common_traits.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <type_traits>

namespace helios::ecs {

/**
 * @brief Resource manager for storing and managing resources of various types.
 * @details Stores resources inline using `MultiTypeMap<TypedBuffer>`.
 * @note Not thread-safe.
 */
class ResourceManager {
public:
  ResourceManager() = default;
  ResourceManager(const ResourceManager&) = default;
  ResourceManager(ResourceManager&&) noexcept = default;
  ~ResourceManager() = default;

  ResourceManager& operator=(const ResourceManager&) = default;
  ResourceManager& operator=(ResourceManager&&) noexcept = default;

  /// @brief Clears all resources, destroying stored values and removing all
  /// entries.
  void Clear() { resources_.ResetAll(); }

  /**
   * @brief Inserts a new resource.
   * @details Replaces existing resource if present.
   * @tparam T Resource type
   * @param resource Resource to insert
   */
  template <ResourceTrait T>
  void Insert(T&& resource);

  /**
   * @brief Inserts multiple resources.
   * @details Replaces existing resources if present.
   * @tparam Ts Resource types, must be unique and non-empty
   * @param resources Resources to insert
   */
  template <ResourceTrait... Ts>
    requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 1)
  void Insert(Ts&&... resources) {
    (Insert(std::forward<Ts>(resources)), ...);
  }

  /**
   * @brief Tries to insert resource if not present.
   * @tparam T Resource type
   * @param resource Resource to insert
   * @return True if inserted, false if resource already exists
   */
  template <ResourceTrait T>
  bool TryInsert(T&& resource);

  /**
   * @brief Tries to insert resources if not present.
   * @tparam Ts Resource types, must be unique and non-empty
   * @param resources Resources to insert
   * @return True if inserted, false if resource already exists
   */
  template <ResourceTrait... Ts>
    requires utils::UniqueTypes<Ts...> && (sizeof...(Ts) > 1)
  auto TryInsert(Ts&&... resources) -> std::array<bool, sizeof...(Ts)> {
    return {TryInsert(std::forward<Ts>(resources))...};
  }

  /**
   * @brief Emplaces a new resource in-place.
   * @details Constructs resource directly in storage.
   * @tparam T Resource type
   * @tparam Args Constructor argument types
   * @param args Arguments to forward to resource constructor
   */
  template <ResourceTrait T, typename... Args>
    requires std::constructible_from<T, Args...>
  void Emplace(Args&&... args);

  /**
   * @brief Tries to emplace a resource if not present.
   * @tparam T Resource type
   * @tparam Args Constructor argument types
   * @param args Arguments to forward to resource constructor
   * @return True if emplaced, false if resource already exists
   */
  template <ResourceTrait T, typename... Args>
    requires std::constructible_from<T, Args...>
  bool TryEmplace(Args&&... args);

  /**
   * @brief Removes a resource.
   * @warning Triggers assertion if resource doesn't exist.
   * @tparam T Resource type
   */
  template <ResourceTrait T>
  void Remove();

  /**
   * @brief Tries to remove a resource.
   * @tparam T Resource type
   * @return True if removed, false if resource didn't exist
   */
  template <ResourceTrait T>
  bool TryRemove() {
    return resources_.Remove<T>();
  }

  /**
   * @brief Gets mutable reference to a resource.
   * @warning Triggers assertion if resource doesn't exist.
   * @tparam T Resource type
   * @return Mutable reference to resource
   */
  template <ResourceTrait T>
  [[nodiscard]] T& Get() noexcept;

  /**
   * @brief Gets const reference to a resource.
   * @warning Triggers assertion if resource doesn't exist.
   * @tparam T Resource type
   * @return Const reference to resource
   */
  template <ResourceTrait T>
  [[nodiscard]] const T& Get() const noexcept;

  /**
   * @brief Tries to get mutable pointer to a resource.
   * @tparam T Resource type
   * @return Pointer to resource, or `nullptr` if not found
   */
  template <ResourceTrait T>
  [[nodiscard]] T* TryGet() noexcept;

  /**
   * @brief Tries to get const pointer to a resource.
   * @tparam T Resource type
   * @return Const pointer to resource, or `nullptr` if not found
   */
  template <ResourceTrait T>
  [[nodiscard]] const T* TryGet() const noexcept;

  /**
   * @brief Checks if a resource exists.
   * @tparam T Resource type
   * @return True if resource exists, false otherwise
   */
  template <ResourceTrait T>
  [[nodiscard]] bool Has() const noexcept {
    return resources_.Contains<T>();
  }

  /**
   * @brief Gets the number of stored resources.
   * @return Resource count
   */
  [[nodiscard]] size_t Count() const noexcept { return resources_.TypeCount(); }

private:
  using StorageType = container::TypedBuffer<>;
  using MapType = container::MultiTypeMap<StorageType>;

  MapType resources_;
};

template <ResourceTrait T>
inline void ResourceManager::Insert(T&& resource) {
  using DecayedT = std::remove_cvref_t<T>;
  auto& buf = resources_.Ensure<DecayedT>();
  buf.template Set<DecayedT>(std::forward<T>(resource));
}

template <ResourceTrait T>
inline bool ResourceManager::TryInsert(T&& resource) {
  using DecayedT = std::remove_cvref_t<T>;
  if (resources_.Contains<DecayedT>() && !resources_.Empty<DecayedT>()) {
    return false;
  }
  auto& buf = resources_.Ensure<DecayedT>();
  buf.template Set<DecayedT>(std::forward<T>(resource));
  return true;
}

template <ResourceTrait T, typename... Args>
  requires std::constructible_from<T, Args...>
inline void ResourceManager::Emplace(Args&&... args) {
  auto& buf = resources_.Ensure<T>();
  buf.template Set<T>(std::forward<Args>(args)...);
}

template <ResourceTrait T, typename... Args>
  requires std::constructible_from<T, Args...>
inline bool ResourceManager::TryEmplace(Args&&... args) {
  if (resources_.Contains<T>() && !resources_.Empty<T>()) {
    return false;
  }
  auto& buf = resources_.Ensure<T>();
  buf.template Set<T>(std::forward<Args>(args)...);
  return true;
}

template <ResourceTrait T>
inline void ResourceManager::Remove() {
  [[maybe_unused]] const bool removed = resources_.Remove<T>();
  HELIOS_ASSERT(removed, "Resource '{}' does not exist!", ResourceNameOf<T>());
}

template <ResourceTrait T>
inline T& ResourceManager::Get() noexcept {
  auto* buf = resources_.TryGet<T>();
  HELIOS_ASSERT(buf != nullptr && !buf->Empty(),
                "Resource '{}' does not exist!", ResourceNameOf<T>());
  return buf->template Value<T>();
}

template <ResourceTrait T>
inline const T& ResourceManager::Get() const noexcept {
  const auto* buf = resources_.TryGet<T>();
  HELIOS_ASSERT(buf != nullptr && !buf->Empty(),
                "Resource '{}' does not exist!", ResourceNameOf<T>());
  return buf->template Value<T>();
}

template <ResourceTrait T>
inline T* ResourceManager::TryGet() noexcept {
  auto* buf = resources_.TryGet<T>();
  if (buf == nullptr || buf->Empty()) {
    return nullptr;
  }
  return &buf->template Value<T>();
}

template <ResourceTrait T>
inline const T* ResourceManager::TryGet() const noexcept {
  const auto* buf = resources_.TryGet<T>();
  if (buf == nullptr || buf->Empty()) {
    return nullptr;
  }
  return &buf->template Value<T>();
}

}  // namespace helios::ecs
