#pragma once

#include <helios/ecs/component/component.hpp>
#include <helios/ecs/query/details/traits.hpp>
#include <helios/ecs/resource/resource.hpp>
#include <helios/log/logger.hpp>
#include <helios/utils/common_traits.hpp>

#include <algorithm>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace helios::ecs {

namespace details {

/**
 * @brief Checks if two sorted ranges have any common elements.
 * @details Uses a merge-like algorithm for O(n + m) complexity.
 * @param lhs The first sorted range
 * @param rhs The second sorted range
 * @return True if there are any common elements, false otherwise
 */
[[nodiscard]] constexpr bool HasIntersection(
    std::span<const ComponentTypeId> lhs,
    std::span<const ComponentTypeId> rhs) noexcept {
  auto it1 = lhs.begin();
  auto it2 = rhs.begin();

  while (it1 != lhs.end() && it2 != rhs.end()) {
    if (*it1 < *it2) {
      ++it1;
    } else if (*it2 < *it1) {
      ++it2;
    } else {
      return true;
    }
  }

  return false;
}

/**
 * @brief Checks if any element from one range exists in another sorted range.
 * @details For each element in lhs, performs binary search in rhs.
 * @param lhs The first sorted range
 * @param rhs The second sorted range
 * @return True if there are any common elements, false otherwise
 */
[[nodiscard]] constexpr bool HasIntersectionBinarySearch(
    std::span<const ResourceTypeId> lhs,
    std::span<const ResourceTypeId> rhs) noexcept {
  if (lhs.size() > rhs.size()) {
    std::swap(lhs, rhs);
  }

  const auto binary_search = [rhs](const auto& item) {
    return std::ranges::binary_search(rhs, item);
  };
  return std::ranges::any_of(lhs, binary_search);
}

}  // namespace details

/**
 * @brief Describes a single component access conflict between two policies.
 * @details `lhs` is the component type as seen from this policy's side;
 * `rhs` is the same component type as seen from the other policy's side
 * (they always refer to the same component — the type that is in conflict).
 * `lhs_writes` / `rhs_writes` indicate which side holds write access,
 * allowing callers to determine the exact conflict kind:
 * - write–write : both true
 * - write–read  : lhs_writes=true,  rhs_writes=false
 * - read–write  : lhs_writes=false, rhs_writes=true
 */
struct ComponentConflictInfo {
  ComponentTypeId lhs;      ///< Component type from this policy
  ComponentTypeId rhs;      ///< Same component type from the other policy
  bool lhs_writes = false;  ///< True if this policy writes the component
  bool rhs_writes = false;  ///< True if the other policy writes the component

  [[nodiscard]] constexpr bool operator==(
      const ComponentConflictInfo&) const noexcept = default;
};

/**
 * @brief Describes a single resource access conflict between two policies.
 * @details `lhs` is the resource type as seen from this policy's side;
 * `rhs` is the same resource type from the other policy.
 * `lhs_writes` / `rhs_writes` indicate which side holds write access.
 * At least one of `lhs_writes` / `rhs_writes` is always true in a conflict
 * (read–read is never a conflict).
 */
struct ResourceConflictInfo {
  ResourceTypeId lhs;       ///< Resource type from this policy
  ResourceTypeId rhs;       ///< Same resource type from the other policy
  bool lhs_writes = false;  ///< True if this policy writes the resource
  bool rhs_writes = false;  ///< True if the other policy writes the resource

  [[nodiscard]] constexpr bool operator==(
      const ResourceConflictInfo&) const noexcept = default;
};

/**
 * @brief Stores data access requirements for a system at compile time.
 * @details Components and resources are each kept in two flat sorted sets
 * (read / write), deduped across all `Query` / `ReadResources` /
 * `WriteResources` declarations. This mirrors how resources have always been
 * stored and removes the per-query descriptor indirection.
 *
 * `AccessPolicy` is used to:
 * - Enable automatic scheduling and conflict detection
 *
 * It is compile-time scheduling metadata only. Runtime access validation is
 * not implemented yet.
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
   * @brief Merges data access declarations from another policy.
   * @details Performs set-union merge for component and resource read/write
   * sets while keeping them sorted and deduplicated.
   * @param other Source policy to merge from
   */
  constexpr void Merge(const AccessPolicy& other);

  /**
   * @brief Merges data access declarations from another policy.
   * @details Performs set-union merge for component and resource read/write
   * sets while keeping them sorted and deduplicated.
   * @param other Source policy to merge from
   */
  constexpr void Merge(AccessPolicy&& other);

  /**
   * @brief Returns all component conflicts between this policy and another.
   * @details Each entry in the returned vector describes one conflicting
   * component type and which side(s) write it. The same component type appears
   * at most once in the result.
   * @param other Other access policy to inspect
   * @return Vector of `ComponentConflictInfo`, empty when no conflicts exist
   */
  [[nodiscard]] constexpr auto GetQueryConflictsWith(
      const AccessPolicy& other) const -> std::vector<ComponentConflictInfo>;

  /**
   * @brief Returns all resource conflicts between this policy and another.
   * @details Each entry describes one conflicting resource type and which
   * side(s) write it. The same resource type appears at most once.
   * @param other Other access policy to inspect
   * @return Vector of `ResourceConflictInfo`, empty when no conflicts exist
   */
  [[nodiscard]] constexpr auto GetResourceConflictsWith(
      const AccessPolicy& other) const -> std::vector<ResourceConflictInfo>;

  /**
   * @brief Checks if this policy conflicts with another policy at all.
   * @details Shorthand for `HasQueryConflictWith || HasResourceConflictWith`.
   * @param other Other access policy to check against
   * @return True if any conflict exists, false otherwise
   */
  [[nodiscard]] constexpr bool ConflictsWith(
      const AccessPolicy& other) const noexcept {
    return HasQueryConflictWith(other) || HasResourceConflictWith(other);
  }

  /**
   * @brief Checks if this policy has a component access conflict with another.
   * @details Conflict exists when:
   * - Both policies write the same component (write–write), or
   * - One writes a component the other reads (write–read / read–write).
   * @param other Other access policy to check against
   * @return True if a component conflict exists, false otherwise
   */
  [[nodiscard]] constexpr bool HasQueryConflictWith(
      const AccessPolicy& other) const noexcept;

  /**
   * @brief Checks if this policy has a resource access conflict with another.
   * @details Conflict exists when both policies access the same resource and at
   * least one of them writes it.
   * @param other Other access policy to check against
   * @return True if a resource conflict exists, false otherwise
   */
  [[nodiscard]] constexpr bool HasResourceConflictWith(
      const AccessPolicy& other) const noexcept;

  /**
   * @brief Checks if this policy declares any component access.
   * @return True if any read or write component has been declared
   */
  [[nodiscard]] constexpr bool HasComponents() const noexcept {
    return !read_components_.empty() || !write_components_.empty();
  }

  /**
   * @brief Checks if this policy declares any resource access.
   * @return True if any read or write resource has been declared
   */
  [[nodiscard]] constexpr bool HasResources() const noexcept {
    return !read_resources_.empty() || !write_resources_.empty();
  }

  /**
   * @brief Checks if this policy declares read access to a component.
   * @param type_index Component type index to look up
   * @return True if the component is in the read set
   */
  [[nodiscard]] constexpr bool HasReadComponent(
      ComponentTypeIndex type_index) const noexcept;

  /**
   * @brief Checks if this policy declares write access to a component.
   * @param type_index Component type index to look up
   * @return True if the component is in the write set
   */
  [[nodiscard]] constexpr bool HasWriteComponent(
      ComponentTypeIndex type_index) const noexcept;

  /**
   * @brief Checks if this policy declares read access to a resource.
   * @param type_index Resource type index to look up
   * @return True if the resource is in the read set
   */
  [[nodiscard]] constexpr bool HasReadResource(
      ResourceTypeIndex type_index) const noexcept;

  /**
   * @brief Checks if this policy declares write access to a resource.
   * @param type_index Resource type index to look up
   * @return True if the resource is in the write set
   */
  [[nodiscard]] constexpr bool HasWriteResource(
      ResourceTypeIndex type_index) const noexcept;

  /**
   * @brief Returns all component types declared for reading (sorted).
   * @return A `std::span` view of the read component types
   */
  [[nodiscard]] constexpr auto GetReadComponents() const noexcept
      -> std::span<const ComponentTypeId> {
    return read_components_;
  }

  /**
   * @brief Returns all component types declared for writing (sorted).
   * @return A `std::span` view of the write component types
   */
  [[nodiscard]] constexpr auto GetWriteComponents() const noexcept
      -> std::span<const ComponentTypeId> {
    return write_components_;
  }

  /**
   * @brief Returns all resource types declared for reading (sorted).
   * @return A `std::span` view of the read resource types
   */
  [[nodiscard]] constexpr auto GetReadResources() const noexcept
      -> std::span<const ResourceTypeId> {
    return read_resources_;
  }

  /**
   * @brief Returns all resource types declared for writing (sorted).
   * @return A `std::span` view of the write resource types
   */
  [[nodiscard]] constexpr auto GetWriteResources() const noexcept
      -> std::span<const ResourceTypeId> {
    return write_resources_;
  }

private:
  constexpr AccessPolicy(std::vector<ComponentTypeId>&& read_components,
                         std::vector<ComponentTypeId>&& write_components,
                         std::vector<ResourceTypeId>&& read_resources,
                         std::vector<ResourceTypeId>&& write_resources) noexcept
      : read_components_(std::move(read_components)),
        write_components_(std::move(write_components)),
        read_resources_(std::move(read_resources)),
        write_resources_(std::move(write_resources)) {}

  std::vector<ComponentTypeId> read_components_;   // Sorted, deduped
  std::vector<ComponentTypeId> write_components_;  // Sorted, deduped

  std::vector<ResourceTypeId> read_resources_;   // Sorted, deduped
  std::vector<ResourceTypeId> write_resources_;  // Sorted, deduped

  friend class AccessPolicyBuilder;
};

constexpr void AccessPolicy::Merge(const AccessPolicy& other) {
  std::vector<ComponentTypeId> merged_read_components;
  std::vector<ComponentTypeId> merged_write_components;
  std::vector<ResourceTypeId> merged_read_resources;
  std::vector<ResourceTypeId> merged_write_resources;

  merged_read_components.reserve(read_components_.size() +
                                 other.read_components_.size());
  merged_write_components.reserve(write_components_.size() +
                                  other.write_components_.size());
  merged_read_resources.reserve(read_resources_.size() +
                                other.read_resources_.size());
  merged_write_resources.reserve(write_resources_.size() +
                                 other.write_resources_.size());

  std::ranges::set_union(read_components_, other.read_components_,
                         std::back_inserter(merged_read_components));
  std::ranges::set_union(write_components_, other.write_components_,
                         std::back_inserter(merged_write_components));
  std::ranges::set_union(read_resources_, other.read_resources_,
                         std::back_inserter(merged_read_resources));
  std::ranges::set_union(write_resources_, other.write_resources_,
                         std::back_inserter(merged_write_resources));

  read_components_ = std::move(merged_read_components);
  write_components_ = std::move(merged_write_components);
  read_resources_ = std::move(merged_read_resources);
  write_resources_ = std::move(merged_write_resources);
}

constexpr void AccessPolicy::Merge(AccessPolicy&& other) {
  Merge(static_cast<const AccessPolicy&>(other));
}

constexpr auto AccessPolicy::GetQueryConflictsWith(
    const AccessPolicy& other) const -> std::vector<ComponentConflictInfo> {
  std::vector<ComponentConflictInfo> result;

  // Helper: add a conflict entry for a matching component, avoiding duplicates.
  // `lhs_type_id` is from this policy's set; `rhs_type_id` from other's set.
  const auto emit = [&result](const ComponentTypeId& lhs_type_id,
                              const ComponentTypeId& rhs_type_id,
                              bool lhs_writes, bool rhs_writes) {
    // Same component always appears at most once in the result.
    const auto already_recorded =
        [&lhs_type_id](const ComponentConflictInfo& conflict) {
          return conflict.lhs == lhs_type_id;
        };
    if (!std::ranges::any_of(result, already_recorded)) {
      result.emplace_back(lhs_type_id, rhs_type_id, lhs_writes, rhs_writes);
    }
  };

  // Walk sorted pairs with a merge-like scan for O(n + m) per pass.
  const auto collect = [&emit](std::span<const ComponentTypeId> lhs_set,
                               std::span<const ComponentTypeId> rhs_set,
                               bool lhs_writes, bool rhs_writes) {
    auto it1 = lhs_set.begin();
    auto it2 = rhs_set.begin();
    while (it1 != lhs_set.end() && it2 != rhs_set.end()) {
      if (*it1 < *it2) {
        ++it1;
      } else if (*it2 < *it1) {
        ++it2;
      } else {
        emit(*it1, *it2, lhs_writes, rhs_writes);
        ++it1;
        ++it2;
      }
    }
  };

  // write–write
  collect(write_components_, other.write_components_, true, true);
  // my write vs other read
  collect(write_components_, other.read_components_, true, false);
  // my read vs other write
  collect(read_components_, other.write_components_, false, true);

  return result;
}

constexpr auto AccessPolicy::GetResourceConflictsWith(
    const AccessPolicy& other) const -> std::vector<ResourceConflictInfo> {
  std::vector<ResourceConflictInfo> result;

  // Helper: record a conflict, avoiding duplicate entries for the same
  // resource.
  const auto emit = [&result](const ResourceTypeId& lhs_type_id,
                              const ResourceTypeId& rhs_type_id,
                              bool lhs_writes, bool rhs_writes) {
    const auto already_recorded =
        [&lhs_type_id](const ResourceConflictInfo& conflict) {
          return conflict.lhs == lhs_type_id;
        };
    if (!std::ranges::any_of(result, already_recorded)) {
      result.emplace_back(lhs_type_id, rhs_type_id, lhs_writes, rhs_writes);
    }
  };

  // Sorted scan for the two component sets; binary-search scan for
  // resources (matching the existing HasIntersectionBinarySearch strategy).
  const auto collect = [&emit](std::span<const ResourceTypeId> lhs_set,
                               std::span<const ResourceTypeId> rhs_set,
                               bool lhs_writes, bool rhs_writes) {
    for (const auto& lhs_id : lhs_set) {
      if (std::ranges::binary_search(rhs_set, lhs_id)) {
        // Recover the matching rhs entry for symmetry in the descriptor.
        const auto it = std::ranges::lower_bound(rhs_set, lhs_id);
        emit(lhs_id, *it, lhs_writes, rhs_writes);
      }
    }
  };

  // write–write
  collect(write_resources_, other.write_resources_, true, true);
  // my write vs other read
  collect(write_resources_, other.read_resources_, true, false);
  // my read vs other write
  collect(read_resources_, other.write_resources_, false, true);

  return result;
}

constexpr bool AccessPolicy::HasQueryConflictWith(
    const AccessPolicy& other) const noexcept {
  if (!HasComponents() || !other.HasComponents()) {
    return false;
  }

  // write–write
  if (!write_components_.empty() && !other.write_components_.empty() &&
      details::HasIntersection(write_components_, other.write_components_)) {
    return true;
  }

  // my write vs other read
  if (!write_components_.empty() && !other.read_components_.empty() &&
      details::HasIntersection(write_components_, other.read_components_)) {
    return true;
  }

  // my read vs other write
  if (!read_components_.empty() && !other.write_components_.empty() &&
      details::HasIntersection(read_components_, other.write_components_)) {
    return true;
  }

  return false;
}

constexpr bool AccessPolicy::HasResourceConflictWith(
    const AccessPolicy& other) const noexcept {
  if (!HasResources() || !other.HasResources()) {
    return false;
  }

  // write–write
  if (!write_resources_.empty() && !other.write_resources_.empty() &&
      details::HasIntersectionBinarySearch(write_resources_,
                                           other.write_resources_)) {
    return true;
  }

  // my write vs other read
  if (!write_resources_.empty() && !other.read_resources_.empty() &&
      details::HasIntersectionBinarySearch(write_resources_,
                                           other.read_resources_)) {
    return true;
  }

  // my read vs other write
  if (!read_resources_.empty() && !other.write_resources_.empty() &&
      details::HasIntersectionBinarySearch(read_resources_,
                                           other.write_resources_)) {
    return true;
  }

  return false;
}

constexpr bool AccessPolicy::HasReadComponent(
    ComponentTypeIndex type_index) const noexcept {
  const auto matches = [type_index](const ComponentTypeId& id) {
    return id == type_index;
  };
  return std::ranges::any_of(read_components_, matches);
}

constexpr bool AccessPolicy::HasWriteComponent(
    ComponentTypeIndex type_index) const noexcept {
  const auto matches = [type_index](const ComponentTypeId& id) {
    return id == type_index;
  };
  return std::ranges::any_of(write_components_, matches);
}

constexpr bool AccessPolicy::HasReadResource(
    ResourceTypeIndex type_index) const noexcept {
  const auto matches = [type_index](const ResourceTypeId& id) {
    return id == type_index;
  };
  return std::ranges::any_of(read_resources_, matches);
}

constexpr bool AccessPolicy::HasWriteResource(
    ResourceTypeIndex type_index) const noexcept {
  const auto matches = [type_index](const ResourceTypeId& id) {
    return id == type_index;
  };
  return std::ranges::any_of(write_resources_, matches);
}

/**
 * @brief Fluent builder for constructing an `AccessPolicy`.
 * @details Accumulates component and resource access declarations across any
 * number of `Query`, `ReadResources`, and `WriteResources` calls.
 * Duplicate entries are silently ignored; all sets are kept sorted.
 *
 * @code
 * helios::ecs::AccessPolicyBuilder()
 *     .Query<const Transform&, const Velocity&>()
 *     .Query<Gravity, Health*>()
 *     .ReadResources<Camera, RenderSettings>()
 *     .WriteResources<RenderQueue>()
 *     .Build();
 * @endcode
 */
class AccessPolicyBuilder {
public:
  constexpr AccessPolicyBuilder() noexcept = default;
  constexpr AccessPolicyBuilder(const AccessPolicyBuilder&) = default;
  constexpr AccessPolicyBuilder(AccessPolicyBuilder&&) noexcept = default;
  constexpr ~AccessPolicyBuilder() = default;

  constexpr AccessPolicyBuilder& operator=(const AccessPolicyBuilder&) =
      default;
  constexpr AccessPolicyBuilder& operator=(AccessPolicyBuilder&&) noexcept =
      default;

  /**
   * @brief Builds and returns the accumulated `AccessPolicy`.
   * @return The accumulated `AccessPolicy`
   */
  constexpr AccessPolicy Build() noexcept {
    return {std::move(read_components_), std::move(write_components_),
            std::move(read_resources_), std::move(write_resources_)};
  }

  /**
   * @brief Declares a query over component types.
   * @details Component accesses are merged into the policy-wide flat read /
   * write sets. Duplicates are silently ignored (a component already declared
   * for writing is not re-added if the same type appears again as a write in a
   * later `Query` call).
   *
   * Access classification:
   * - `const T&`, `const T*`, `const T`, `T` (value copy) → read
   * - `T&`, `T&&`, `T*`                                   → write
   *
   * Tag components (empty types) are excluded from conflict tracking.
   *
   * @tparam Components Component access types
   * @return `*this` for method chaining
   */
  template <typename... Components>
    requires details::UniqueComponentAccess<Components...> &&
             (ComponentTrait<details::ComponentTypeExtractor_t<Components>> &&
              ...)
  constexpr auto Query(this auto&& self)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Declares read-only access to resource types.
   * @details Thread-safe resources are silently ignored.
   * @tparam Resources Resource types to read
   * @return `*this` for method chaining
   */
  template <ResourceTrait... Resources>
    requires utils::UniqueTypes<Resources...>
  constexpr auto ReadResources(this auto&& self)
      -> decltype(std::forward<decltype(self)>(self));

  /**
   * @brief Declares write access to resource types.
   * @details Thread-safe resources are silently ignored.
   * @tparam Resources Resource types to write
   * @return `*this` for method chaining
   */
  template <ResourceTrait... Resources>
    requires utils::UniqueTypes<Resources...>
  constexpr auto WriteResources(this auto&& self)
      -> decltype(std::forward<decltype(self)>(self));

private:
  static constexpr void InsertSorted(std::vector<utils::TypeId>& vec,
                                     const utils::TypeId& type_id);

  std::vector<ComponentTypeId> read_components_;   ///< Sorted, deduped
  std::vector<ComponentTypeId> write_components_;  ///< Sorted, deduped
  std::vector<ResourceTypeId> read_resources_;     ///< Sorted, deduped
  std::vector<ResourceTypeId> write_resources_;    ///< Sorted, deduped
};

template <typename... Components>
  requires details::UniqueComponentAccess<Components...> &&
           (ComponentTrait<details::ComponentTypeExtractor_t<Components>> &&
            ...)
constexpr auto AccessPolicyBuilder::Query(this auto&& self)
    -> decltype(std::forward<decltype(self)>(self)) {
  (
      [&self]() {
        using RawComponent = details::ComponentTypeExtractor_t<Components>;

        // Tag components have no data to conflict on.
        if constexpr (TagComponentTrait<RawComponent>) {
          return;
        } else if constexpr (details::kIsConstAccess<Components>) {
          self.InsertSorted(self.read_components_,
                            ComponentTypeId::From<RawComponent>());
        } else {
          self.InsertSorted(self.write_components_,
                            ComponentTypeId::From<RawComponent>());
        }
      }(),
      ...);

  return std::forward<decltype(self)>(self);
}

template <ResourceTrait... Resources>
  requires utils::UniqueTypes<Resources...>
constexpr auto AccessPolicyBuilder::ReadResources(this auto&& self)
    -> decltype(std::forward<decltype(self)>(self)) {
  (
      [&self]() {
        if constexpr (!IsResourceThreadSafe<Resources>()) {
          self.InsertSorted(self.read_resources_,
                            ResourceTypeId::From<Resources>());
        }
      }(),
      ...);

  return std::forward<decltype(self)>(self);
}

template <ResourceTrait... Resources>
  requires utils::UniqueTypes<Resources...>
constexpr auto AccessPolicyBuilder::WriteResources(this auto&& self)
    -> decltype(std::forward<decltype(self)>(self)) {
  (
      [&self]() {
        if constexpr (!IsResourceThreadSafe<Resources>()) {
          self.InsertSorted(self.write_resources_,
                            ResourceTypeId::From<Resources>());
        }
      }(),
      ...);

  return std::forward<decltype(self)>(self);
}

constexpr void AccessPolicyBuilder::InsertSorted(
    std::vector<utils::TypeId>& vec, const utils::TypeId& type_id) {
  const auto cmp = [](const utils::TypeId& lhs, const utils::TypeId& rhs) {
    return lhs < rhs;
  };
  const auto pos = std::ranges::lower_bound(vec, type_id, cmp);
  if (pos == vec.end() || *pos != type_id) {
    vec.insert(pos, type_id);
  }
}

}  // namespace helios::ecs
