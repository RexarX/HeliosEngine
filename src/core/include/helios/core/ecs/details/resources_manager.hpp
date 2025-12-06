#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/assert.hpp>
#include <helios/core/ecs/resource.hpp>
#include <helios/core/logger.hpp>

#include <concepts>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <unordered_map>

namespace helios::ecs::details {

/**
 * @brief Base class for type-erased resource storage.
 */
class ResourceStorageBase {
public:
  virtual ~ResourceStorageBase() = default;
};

/**
 * @brief Type-specific resource storage.
 * @tparam T Resource type
 */
template <ResourceTrait T>
class ResourceStorage final : public ResourceStorageBase {
public:
  template <typename... Args>
    requires std::constructible_from<T, Args...>
  explicit(sizeof...(Args) == 1) ResourceStorage(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
      : resource_(std::forward<Args>(args)...) {}

  ResourceStorage(const ResourceStorage&) noexcept(std::is_nothrow_copy_constructible_v<T>)
    requires std::copy_constructible<T>
  = default;

  ResourceStorage(ResourceStorage&&) noexcept(std::is_nothrow_move_constructible_v<T>)
    requires std::move_constructible<T>
  = default;

  ~ResourceStorage() override = default;

  ResourceStorage& operator=(const ResourceStorage&) noexcept(std::is_nothrow_copy_assignable_v<T>)
    requires std::assignable_from<T&, const T&>
  = default;

  ResourceStorage& operator=(ResourceStorage&&) noexcept(std::is_nothrow_move_assignable_v<T>)
    requires std::assignable_from<T&, T>
  = default;

  [[nodiscard]] T& Get() noexcept { return resource_; }
  [[nodiscard]] const T& Get() const noexcept { return resource_; }

private:
  T resource_;
};

/**
 * @brief Resource container for World.
 * @details Manages resources with thread-safe access using Boost's concurrent_node_map.
 * @note Not thread-safe.
 */
class Resources {
public:
  Resources() = default;
  Resources(const Resources&) = delete;
  Resources(Resources&&) noexcept = default;
  ~Resources() = default;

  Resources& operator=(const Resources&) = delete;
  Resources& operator=(Resources&&) noexcept = default;

  /**
   * @brief Clears all resources.
   */
  void Clear() { resources_.clear(); }

  /**
   * @brief Inserts a new resource.
   * @details Replaces existing resource if present.
   * @tparam T Resource type
   * @param resource Resource to insert
   */
  template <ResourceTrait T>
  void Insert(T&& resource);

  /**
   * @brief Tries to insert a resource if not present.
   * @tparam T Resource type
   * @param resource Resource to insert
   * @return True if inserted, false if resource already exists
   */
  template <ResourceTrait T>
  bool TryInsert(T&& resource);

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
  bool TryRemove();

  /**
   * @brief Gets mutable reference to a resource.
   * Triggers assertion if resource doesn't exist.
   * @tparam T Resource type
   * @return Mutable reference to resource
   */
  template <ResourceTrait T>
  [[nodiscard]] T& Get();

  /**
   * @brief Gets const reference to a resource.
   * Triggers assertion if resource doesn't exist.
   * @tparam T Resource type
   * @return Const reference to resource
   */
  template <ResourceTrait T>
  [[nodiscard]] const T& Get() const;

  /**
   * @brief Tries to get mutable pointer to a resource.
   * @tparam T Resource type
   * @return Pointer to resource, or nullptr if not found
   */
  template <ResourceTrait T>
  [[nodiscard]] T* TryGet();

  /**
   * @brief Tries to get const pointer to a resource.
   * @tparam T Resource type
   * @return Const pointer to resource, or nullptr if not found
   */
  template <ResourceTrait T>
  [[nodiscard]] const T* TryGet() const;

  /**
   * @brief Checks if a resource exists.
   * @tparam T Resource type
   * @return True if resource exists, false otherwise
   */
  template <ResourceTrait T>
  [[nodiscard]] bool Has() const;

  /**
   * @brief Gets the number of stored resources.
   * @return Resource count
   */
  [[nodiscard]] size_t Count() const { return resources_.size(); }

private:
  /// Concurrent map of resource type IDs to their storage
  std::unordered_map<ResourceTypeId, std::unique_ptr<ResourceStorageBase>> resources_;
};

template <ResourceTrait T>
void Resources::Insert(T&& resource) {
  constexpr ResourceTypeId type_id = ResourceTypeIdOf<T>();
  auto storage = std::make_unique<ResourceStorage<T>>(std::forward<T>(resource));
  resources_.insert_or_assign(type_id, std::move(storage));
}

template <ResourceTrait T>
inline bool Resources::TryInsert(T&& resource) {
  constexpr ResourceTypeId type_id = ResourceTypeIdOf<T>();
  auto storage = std::make_unique<ResourceStorage<T>>(std::forward<T>(resource));
  return resources_.try_emplace(type_id, std::move(storage)).second;
}

template <ResourceTrait T, typename... Args>
  requires std::constructible_from<T, Args...>
inline void Resources::Emplace(Args&&... args) {
  constexpr ResourceTypeId type_id = ResourceTypeIdOf<T>();
  auto storage = std::make_unique<ResourceStorage<T>>(std::forward<Args>(args)...);
  resources_.insert_or_assign(type_id, std::move(storage));
}

template <ResourceTrait T, typename... Args>
  requires std::constructible_from<T, Args...>
inline bool Resources::TryEmplace(Args&&... args) {
  constexpr ResourceTypeId type_id = ResourceTypeIdOf<T>();
  auto storage = std::make_unique<ResourceStorage<T>>(std::forward<Args>(args)...);
  return resources_.try_emplace(type_id, std::move(storage)).second;
}

template <ResourceTrait T>
inline void Resources::Remove() {
  constexpr ResourceTypeId type_id = ResourceTypeIdOf<T>();
  const size_t erased = resources_.erase(type_id);
  HELIOS_ASSERT(erased > 0, "Failed to remove resource '{}': Resource does not exist!", ResourceNameOf<T>());
}

template <ResourceTrait T>
inline bool Resources::TryRemove() {
  constexpr ResourceTypeId type_id = ResourceTypeIdOf<T>();
  return resources_.erase(type_id) > 0;
}

template <ResourceTrait T>
inline T& Resources::Get() {
  constexpr ResourceTypeId type_id = ResourceTypeIdOf<T>();
  const auto it = resources_.find(type_id);
  HELIOS_ASSERT(it != resources_.end(), "Failed to get resource '{}': Resource does not exist!", ResourceNameOf<T>());
  return static_cast<ResourceStorage<T>&>(*it->second).Get();
}

template <ResourceTrait T>
inline const T& Resources::Get() const {
  constexpr ResourceTypeId type_id = ResourceTypeIdOf<T>();
  const auto it = resources_.find(type_id);
  HELIOS_ASSERT(it != resources_.end(), "Failed to get resource '{}': Resource does not exist!", ResourceNameOf<T>());
  return static_cast<const ResourceStorage<T>&>(*it->second).Get();
}

template <ResourceTrait T>
inline T* Resources::TryGet() {
  constexpr ResourceTypeId type_id = ResourceTypeIdOf<T>();
  const auto it = resources_.find(type_id);
  if (it == resources_.end()) {
    return nullptr;
  }
  return &static_cast<ResourceStorage<T>&>(*it->second).Get();
}

template <ResourceTrait T>
inline const T* Resources::TryGet() const {
  constexpr ResourceTypeId type_id = ResourceTypeIdOf<T>();
  const auto it = resources_.find(type_id);
  if (it == resources_.end()) {
    return nullptr;
  }
  return &static_cast<const ResourceStorage<T>&>(*it->second).Get();
}

template <ResourceTrait T>
inline bool Resources::Has() const {
  constexpr ResourceTypeId type_id = ResourceTypeIdOf<T>();
  return resources_.contains(type_id);
}

}  // namespace helios::ecs::details
