#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/ecs/component.hpp>
#include <helios/core/ecs/resource.hpp>
#include <helios/core/logger.hpp>
#include <helios/core/utils/common_traits.hpp>

#include <algorithm>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace helios::app {

namespace details {

/**
 * @brief Component type information with ID and name.
 * @details Stores both type ID and name for diagnostics.
 */
struct ComponentTypeInfo {
  ecs::ComponentTypeId type_id = 0;
  std::string_view name;

  constexpr bool operator<(const ComponentTypeInfo& other) const noexcept { return type_id < other.type_id; }
  constexpr bool operator==(const ComponentTypeInfo& other) const noexcept { return type_id == other.type_id; }
};

/**
 * @brief Resource type information with ID and name.
 * @details Stores both type ID and name for diagnostics.
 */
struct ResourceTypeInfo {
  ecs::ResourceTypeId type_id = 0;
  std::string_view name;

  constexpr bool operator<(const ResourceTypeInfo& other) const noexcept { return type_id < other.type_id; }
  constexpr bool operator==(const ResourceTypeInfo& other) const noexcept { return type_id == other.type_id; }
};

/**
 * @brief Query descriptor for AccessPolicy.
 * @details Stores component type IDs and names for a single query specification.
 * Component types are kept sorted for efficient conflict detection.
 */
struct QueryDescriptor {
  std::vector<ComponentTypeInfo> read_components;   // Kept sorted by type_id
  std::vector<ComponentTypeInfo> write_components;  // Kept sorted by type_id
};

/**
 * @brief Checks if two sorted ranges have any common elements.
 * @details Uses a merge-like algorithm for O(n + m) complexity.
 */
[[nodiscard]] constexpr bool HasIntersection(std::span<const ComponentTypeInfo> lhs,
                                             std::span<const ComponentTypeInfo> rhs) noexcept {
  auto it1 = lhs.begin();
  auto it2 = rhs.begin();

  while (it1 != lhs.end() && it2 != rhs.end()) {
    if (it1->type_id < it2->type_id) {
      ++it1;
    } else if (it2->type_id < it1->type_id) {
      ++it2;
    } else {
      return true;  // Found common element
    }
  }

  return false;
}

/**
 * @brief Checks if any element from one range exists in another sorted range.
 * @details For each element in lhs, performs binary search in rhs.
 */
[[nodiscard]] constexpr bool HasIntersectionBinarySearch(std::span<const ResourceTypeInfo> lhs,
                                                         std::span<const ResourceTypeInfo> rhs) noexcept {
  // Optimize by iterating over smaller range
  if (lhs.size() > rhs.size()) {
    std::swap(lhs, rhs);
  }

  return std::ranges::any_of(lhs, [rhs](const auto& item) {
    return std::ranges::binary_search(
        rhs, item, [](const auto& info1, const auto& info2) { return info1.type_id < info2.type_id; });
  });
}

}  // namespace details

/**
 * @brief Declares data access requirements for a system at compile time.
 * @details AccessPolicy is used to:
 * - Declare which components a system will query
 * - Declare which resources a system will read/write
 * - Enable automatic scheduling and conflict detection
 * - Validate runtime access through SystemContext
 * @note All methods are constexpr to enable compile-time policy construction.
 *
 * @example
 * @code
 * static constexpr auto GetAccessPolicy() {
 *   return AccessPolicy()
 *       .Query<const Transform&, const MeshRenderer&>()
 *       .Query<Transform&, const SpriteRenderer&>()
 *       .ReadResources<Camera, RenderSettings>()
 *       .WriteResources<RenderQueue>();  // Write implies read access
 * }
 * @endcode
 */
class AccessPolicy {
public:
  constexpr AccessPolicy() noexcept = default;
  constexpr AccessPolicy(const AccessPolicy&) = default;
  constexpr AccessPolicy(AccessPolicy&&) noexcept = default;
  constexpr ~AccessPolicy() = default;

  constexpr AccessPolicy& operator=(const AccessPolicy&) = default;
  constexpr AccessPolicy& operator=(AccessPolicy&&) noexcept = default;

  /**
   * @brief Declares a query over component types.
   * @details Adds a query specification to the policy. Multiple queries can be declared.
   * Component access is classified per-type into read-only or writable sets.
   * Component types are automatically sorted for efficient conflict detection.
   * @tparam Components Component access types (e.g., const Position&, Velocity&)
   * @return Updated AccessPolicy for method chaining
   */
  template <ecs::ComponentTrait... Components>
    requires((ecs::ComponentTrait<Components> && ...) && utils::UniqueTypes<Components...>)
  constexpr auto Query(this auto&& self) -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Declares read-only access to resource types.
   * @details Adds resource types that the system will read but not modify.
   * Resources are kept sorted for efficient conflict detection.
   * @note If resource is thread-safe it will be ignored.
   * @tparam Resources Resource types to read
   * @return Updated AccessPolicy for method chaining
   */
  template <ecs::ResourceTrait... Resources>
    requires utils::UniqueTypes<Resources...>
  constexpr auto ReadResources(this auto&& self) -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Declares write access to resource types.
   * @details Adds resource types that the system will modify.
   * Resources are kept sorted for efficient conflict detection.
   * @note If resource is thread-safe it will be ignored.
   * @tparam Resources Resource types to write
   * @return Updated AccessPolicy for method chaining
   */
  template <ecs::ResourceTrait... Resources>
    requires utils::UniqueTypes<Resources...>
  constexpr auto WriteResources(this auto&& self) -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Checks if this policy has query conflict with another policy.
   * @details Two policies conflict if:
   * - One reads from a component that the other writes to
   * - Both write to the same component
   * - Both access the same resource with at least one write access
   * @param other Other access policy to check against
   * @return True if policies have a conflict, false otherwise
   */
  [[nodiscard]] constexpr bool HasQueryConflict(const AccessPolicy& other) const noexcept;

  /**
   * @brief Checks if this policy has resource conflict with another policy.
   * @details Two policies conflict if both access the same resource with at least one write access.
   * @param other Other access policy to check against
   * @return True if policies have a conflict, false otherwise
   */
  [[nodiscard]] constexpr bool HasResourceConflict(const AccessPolicy& other) const noexcept;

  /**
   * @brief Checks if this policy conflicts with another policy.
   * @details Checks for query and resource conflict.
   * @param other Other access policy to check against
   * @return True if policies conflict, false otherwise
   */
  [[nodiscard]] constexpr bool ConflictsWith(const AccessPolicy& other) const noexcept {
    return HasQueryConflict(other) || HasResourceConflict(other);
  }

  /**
   * @brief Checks if this policy has any queries.
   * @return True if any queries are declared, false otherwise
   */
  [[nodiscard]] constexpr bool HasQueries() const noexcept { return !queries_.empty(); }

  /**
   * @brief Checks if this policy has any resource access.
   * @return True if any resources are declared, false otherwise
   */
  [[nodiscard]] constexpr bool HasResources() const noexcept {
    return !read_resources_.empty() || !write_resources_.empty();
  }

  /**
   * @brief Gets all declared query descriptors.
   * @return Span of query descriptors
   */
  [[nodiscard]] constexpr auto GetQueries() const noexcept -> std::span<const details::QueryDescriptor> {
    return queries_;
  }

  /**
   * @brief Gets all resource types declared for reading.
   * @return Span of resource type info (sorted)
   */
  [[nodiscard]] constexpr auto GetReadResources() const noexcept -> std::span<const details::ResourceTypeInfo> {
    return read_resources_;
  }

  /**
   * @brief Gets all resource types declared for writing.
   * @return Span of resource type info (sorted)
   */
  [[nodiscard]] constexpr auto GetWriteResources() const noexcept -> std::span<const details::ResourceTypeInfo> {
    return write_resources_;
  }

  /**
   * @brief Checks if this policy has the specified component type for reading.
   * @param type_id Component type ID to check
   * @return True if component is declared for reading in any query
   */
  [[nodiscard]] constexpr bool HasReadComponent(ecs::ComponentTypeId type_id) const noexcept {
    for (const auto& query : queries_) {
      auto matches_id = [type_id](const details::ComponentTypeInfo& info) { return info.type_id == type_id; };
      if (std::ranges::any_of(query.read_components, matches_id)) {
        return true;
      }
    }
    return false;
  }

  /**
   * @brief Checks if this policy has the specified component type for writing.
   * @param type_id Component type ID to check
   * @return True if component is declared for writing in any query
   */
  [[nodiscard]] constexpr bool HasWriteComponent(ecs::ComponentTypeId type_id) const noexcept {
    for (const auto& query : queries_) {
      auto matches_id = [type_id](const details::ComponentTypeInfo& info) { return info.type_id == type_id; };
      if (std::ranges::any_of(query.write_components, matches_id)) {
        return true;
      }
    }
    return false;
  }

  /**
   * @brief Checks if this policy has the specified resource type for reading.
   * @param type_id Resource type ID to check
   * @return True if resource is declared for reading
   */
  [[nodiscard]] constexpr bool HasReadResource(ecs::ResourceTypeId type_id) const noexcept {
    auto matches_id = [type_id](const details::ResourceTypeInfo& info) { return info.type_id == type_id; };
    return std::ranges::any_of(read_resources_, matches_id);
  }

  /**
   * @brief Checks if this policy has the specified resource type for writing.
   * @param type_id Resource type ID to check
   * @return True if resource is declared for writing
   */
  [[nodiscard]] constexpr bool HasWriteResource(ecs::ResourceTypeId type_id) const noexcept {
    auto matches_id = [type_id](const details::ResourceTypeInfo& info) { return info.type_id == type_id; };
    return std::ranges::any_of(write_resources_, matches_id);
  }

private:
  /**
   * @brief Helper to insert a resource while maintaining sorted order.
   * @details Checks for duplicates before inserting.
   */
  static constexpr void InsertSorted(std::vector<details::ResourceTypeInfo>& vec, details::ResourceTypeInfo info);

  std::vector<details::QueryDescriptor> queries_;
  std::vector<details::ResourceTypeInfo> read_resources_;   // Kept sorted
  std::vector<details::ResourceTypeInfo> write_resources_;  // Kept sorted
};

template <ecs::ComponentTrait... Components>
  requires((ecs::ComponentTrait<Components> && ...) && utils::UniqueTypes<Components...>)
constexpr auto AccessPolicy::Query(this auto&& self) -> decltype(std::forward<decltype(self)>(self)) {
  details::QueryDescriptor query;

  // Populate read vs write component lists based on constness of the parameter type.
  // For component parameter T:
  // - If T is const (after removing reference), treat as read-only.
  // - Otherwise treat as writable.
  (
      [&query]() {
        using Component = std::remove_cvref_t<Components>;
        constexpr ecs::ComponentTypeId id = ecs::ComponentTypeIdOf<Component>();
        constexpr std::string_view name = ecs::ComponentNameOf<Component>();
        if constexpr (ecs::TagComponentTrait<Component>) {
          return;
        }
        // Check if the original Components type (before removing cvref) is const
        if constexpr (std::is_const_v<std::remove_reference_t<Components>>) {
          query.read_components.push_back({id, name});
        } else {
          query.write_components.push_back({id, name});
        }
      }(),
      ...);

  // Sort component type lists for efficient conflict detection
  auto cmp = [](const details::ComponentTypeInfo& lhs, const details::ComponentTypeInfo& rhs) {
    return lhs.type_id < rhs.type_id;
  };
  std::ranges::sort(query.read_components, cmp);
  std::ranges::sort(query.write_components, cmp);

  self.queries_.push_back(std::move(query));
  return std::forward<decltype(self)>(self);
}

template <ecs::ResourceTrait... Resources>
  requires utils::UniqueTypes<Resources...>
constexpr auto AccessPolicy::ReadResources(this auto&& self) -> decltype(std::forward<decltype(self)>(self)) {
  // Insert each resource that is not thread-safe while maintaining sorted order
  (
      [&self]() {
        if constexpr (ecs::ResourceWithThreadSafetyTrait<Resources>) {
          if constexpr (Resources::ThreadSafe()) {
            HELIOS_INFO(
                "'{}' resource was declared in AccessPolicy::ReadResources, but will be ignored since it is "
                "thread-safe.",
                ecs::ResourceNameOf<Resources>());
          } else {
            self.InsertSorted(self.read_resources_,
                              {ecs::ResourceTypeIdOf<Resources>(), ecs::ResourceNameOf<Resources>()});
          }
        } else {
          self.InsertSorted(self.read_resources_,
                            {ecs::ResourceTypeIdOf<Resources>(), ecs::ResourceNameOf<Resources>()});
        }
      }(),
      ...);

  return std::forward<decltype(self)>(self);
}

template <ecs::ResourceTrait... Resources>
  requires utils::UniqueTypes<Resources...>
constexpr auto AccessPolicy::WriteResources(this auto&& self) -> decltype(std::forward<decltype(self)>(self)) {
  // Insert each resource that is not thread-safe while maintaining sorted order
  (
      [&self]() {
        if constexpr (ecs::ResourceWithThreadSafetyTrait<Resources>) {
          if constexpr (Resources::ThreadSafe()) {
            HELIOS_INFO(
                "'{}' resource was declared in AccessPolicy::WriteResources, but will be ignored since it is "
                "thread-safe.",
                ecs::ResourceNameOf<Resources>());
          } else {
            self.InsertSorted(self.write_resources_,
                              {ecs::ResourceTypeIdOf<Resources>(), ecs::ResourceNameOf<Resources>()});
          }
        } else {
          self.InsertSorted(self.write_resources_,
                            {ecs::ResourceTypeIdOf<Resources>(), ecs::ResourceNameOf<Resources>()});
        }
      }(),
      ...);

  return std::forward<decltype(self)>(self);
}

constexpr bool AccessPolicy::HasQueryConflict(const AccessPolicy& other) const noexcept {
  if (!HasQueries() || !other.HasQueries()) {
    return false;
  }

  // Check component conflicts using sorted-range intersection: O(n + m)
  // Conflict cases:
  // - my.write intersects other.write (write-write)
  // - my.write intersects other.read  (write-read)
  // - my.read intersects other.write  (read-write)
  for (const auto& my_query : queries_) {
    for (const auto& other_query : other.queries_) {
      // write-write
      if (details::HasIntersection(my_query.write_components, other_query.write_components)) {
        return true;
      }

      // my write vs other read
      if (details::HasIntersection(my_query.write_components, other_query.read_components)) {
        return true;
      }

      // my read vs other write
      if (details::HasIntersection(my_query.read_components, other_query.write_components)) {
        return true;
      }
    }
  }

  return false;
}
constexpr bool AccessPolicy::HasResourceConflict(const AccessPolicy& other) const noexcept {
  if (!HasResources() || !other.HasResources()) {
    return false;
  }

  // Write-write conflicts
  if (!write_resources_.empty() && !other.write_resources_.empty() &&
      details::HasIntersectionBinarySearch(write_resources_, other.write_resources_)) {
    return true;
  }

  // Write-read conflicts (my write vs other read)
  if (!write_resources_.empty() && !other.read_resources_.empty() &&
      details::HasIntersectionBinarySearch(write_resources_, other.read_resources_)) {
    return true;
  }

  // Read-write conflicts (my read vs other write)
  if (!read_resources_.empty() && !other.write_resources_.empty() &&
      details::HasIntersectionBinarySearch(read_resources_, other.write_resources_)) {
    return true;
  }

  return false;
}

constexpr void AccessPolicy::InsertSorted(std::vector<details::ResourceTypeInfo>& vec, details::ResourceTypeInfo info) {
  auto cmp = [](const details::ResourceTypeInfo& lhs, const details::ResourceTypeInfo& rhs) {
    return lhs.type_id < rhs.type_id;
  };
  const auto pos = std::ranges::lower_bound(vec, info, cmp);
  // Avoid duplicates
  if (pos == vec.end() || pos->type_id != info.type_id) {
    vec.insert(pos, info);
  }
}

}  // namespace helios::app
