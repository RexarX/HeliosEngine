#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/app/access_policy.hpp>
#include <helios/core/ecs/component.hpp>
#include <helios/core/ecs/resource.hpp>

#include <cstddef>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace helios::app::details {

/**
 * @brief Provides diagnostic information about system conflicts and validation errors.
 * @details Helper class for generating detailed error messages about scheduling conflicts
 * and access policy violations.
 */
class SystemDiagnostics {
public:
  /**
   * @brief Information about a component conflict between two systems.
   */
  struct ComponentConflict {
    ecs::ComponentTypeId component_id = 0;
    std::string_view component_name;
    bool read_write_conflict = false;  ///< true if one reads and other writes, false if both write
    std::string_view system_a_access;  ///< "read" or "write" for system A
    std::string_view system_b_access;  ///< "read" or "write" for system B
  };

  /**
   * @brief Information about a resource conflict between two systems.
   */
  struct ResourceConflict {
    ecs::ResourceTypeId resource_id = 0;
    std::string_view resource_name;
    bool read_write_conflict = false;  ///< true if one reads and other writes, false if both write
    std::string_view system_a_access;  ///< "read" or "write" for system A
    std::string_view system_b_access;  ///< "read" or "write" for system B
  };

  /**
   * @brief Analyzes conflicts between two access policies.
   * @param policy_a First access policy
   * @param policy_b Second access policy
   * @return Vector of component conflicts found
   */
  [[nodiscard]] static auto AnalyzeComponentConflicts(const AccessPolicy& policy_a, const AccessPolicy& policy_b)
      -> std::vector<ComponentConflict>;

  /**
   * @brief Analyzes resource conflicts between two access policies.
   * @param policy_a First access policy
   * @param policy_b Second access policy
   * @return Vector of resource conflicts found
   */
  [[nodiscard]] static auto AnalyzeResourceConflicts(const AccessPolicy& policy_a, const AccessPolicy& policy_b)
      -> std::vector<ResourceConflict>;

  /**
   * @brief Formats component conflict information into human-readable string.
   * @param system_a_name Name of first system
   * @param system_b_name Name of second system
   * @param conflicts List of component conflicts
   * @return Formatted error message
   */
  [[nodiscard]] static std::string FormatComponentConflicts(std::string_view system_a_name,
                                                            std::string_view system_b_name,
                                                            std::span<const ComponentConflict> conflicts);

  /**
   * @brief Formats resource conflict information into human-readable string.
   * @param system_a_name Name of first system
   * @param system_b_name Name of second system
   * @param conflicts List of resource conflicts
   * @return Formatted error message
   */
  [[nodiscard]] static std::string FormatResourceConflicts(std::string_view system_a_name,
                                                           std::string_view system_b_name,
                                                           std::span<const ResourceConflict> conflicts);

  /**
   * @brief Generates a summary of access policy for debugging.
   * @param policy Access policy to summarize
   * @return Human-readable summary string
   */
  [[nodiscard]] static std::string SummarizeAccessPolicy(const AccessPolicy& policy);

private:
  /**
   * @brief Helper to check if sorted ranges intersect and collect intersecting elements.
   * @tparam T Type of elements (ComponentTypeInfo or ResourceTypeInfo)
   * @param lhs First sorted range
   * @param rhs Second sorted range
   * @return Vector of intersecting elements
   */
  template <typename T>
  [[nodiscard]] static auto FindIntersection(std::span<const T> lhs, std::span<const T> rhs) -> std::vector<T>;

  template <typename T>
  [[nodiscard]] static auto FindIntersection(const std::vector<T>& lhs, const std::vector<T>& rhs) -> std::vector<T> {
    return FindIntersection(std::span<const T>(lhs), std::span<const T>(rhs));
  }
};

template <typename T>
inline auto SystemDiagnostics::FindIntersection(std::span<const T> lhs, std::span<const T> rhs) -> std::vector<T> {
  std::vector<T> result;

  auto it1 = lhs.begin();
  auto it2 = rhs.begin();

  while (it1 != lhs.end() && it2 != rhs.end()) {
    if (it1->type_id < it2->type_id) {
      ++it1;
    } else if (it2->type_id < it1->type_id) {
      ++it2;
    } else {
      result.push_back(*it1);
      ++it1;
      ++it2;
    }
  }

  return result;
}

inline auto SystemDiagnostics::AnalyzeComponentConflicts(const AccessPolicy& policy_a, const AccessPolicy& policy_b)
    -> std::vector<ComponentConflict> {
  const auto queries_a = policy_a.GetQueries();
  const auto queries_b = policy_b.GetQueries();

  std::vector<ComponentConflict> conflicts;
  conflicts.reserve(queries_a.size() * queries_b.size());

  // Check all query pairs for conflicts
  for (const auto& query_a : queries_a) {
    for (const auto& query_b : queries_b) {
      // Check write-write conflicts
      const auto write_write = FindIntersection(query_a.write_components, query_b.write_components);
      for (const auto& component : write_write) {
        conflicts.emplace_back(component.type_id, component.name, false, "write", "write");
      }

      // Check write-read conflicts (a writes, b reads)
      const auto write_read_a = FindIntersection(query_a.write_components, query_b.read_components);
      for (const auto& component : write_read_a) {
        conflicts.emplace_back(component.type_id, component.name, true, "write", "read");
      }

      // Check read-write conflicts (a reads, b writes)
      const auto read_write_a = FindIntersection(query_a.read_components, query_b.write_components);
      for (const auto& component : read_write_a) {
        conflicts.emplace_back(component.type_id, component.name, true, "read", "write");
      }
    }
  }

  return conflicts;
}

inline std::vector<SystemDiagnostics::ResourceConflict> SystemDiagnostics::AnalyzeResourceConflicts(
    const AccessPolicy& policy_a, const AccessPolicy& policy_b) {
  const auto read_a = policy_a.GetReadResources();
  const auto write_a = policy_a.GetWriteResources();
  const auto read_b = policy_b.GetReadResources();
  const auto write_b = policy_b.GetWriteResources();

  // Write-write conflicts
  const auto write_write = FindIntersection(write_a, write_b);

  // Write-read conflicts (a writes, b reads)
  const auto write_read = FindIntersection(write_a, read_b);

  // Read-write conflicts (a reads, b writes)
  const auto read_write = FindIntersection(read_a, write_b);

  std::vector<ResourceConflict> conflicts;
  conflicts.reserve(write_write.size() + write_read.size() + read_write.size());

  for (const auto& resource : write_write) {
    conflicts.emplace_back(resource.type_id, resource.name, false, "write", "write");
  }

  for (const auto& resource : write_read) {
    conflicts.emplace_back(resource.type_id, resource.name, true, "write", "read");
  }

  for (const auto& resource : read_write) {
    conflicts.emplace_back(resource.type_id, resource.name, true, "read", "write");
  }

  return conflicts;
}

inline std::string SystemDiagnostics::FormatComponentConflicts(std::string_view system_a_name,
                                                               std::string_view system_b_name,
                                                               std::span<const ComponentConflict> conflicts) {
  if (conflicts.empty()) {
    return "";
  }

  std::string result = std::format("Component conflicts between '{}' and '{}':\n", system_a_name, system_b_name);
  for (const auto& conflict : conflicts) {
    result += std::format("  - {} ({}: {}, {}: {})\n", conflict.component_name, system_a_name, conflict.system_a_access,
                          system_b_name, conflict.system_b_access);
  }

  return result;
}

inline std::string SystemDiagnostics::FormatResourceConflicts(std::string_view system_a_name,
                                                              std::string_view system_b_name,
                                                              std::span<const ResourceConflict> conflicts) {
  if (conflicts.empty()) {
    return {};
  }

  std::string result = std::format("Resource conflicts between '{}' and '{}':\n", system_a_name, system_b_name);

  for (const auto& conflict : conflicts) {
    result += std::format("  - {} ({}: {}, {}: {})\n", conflict.resource_name, system_a_name, conflict.system_a_access,
                          system_b_name, conflict.system_b_access);
  }
  return result;
}

inline std::string SystemDiagnostics::SummarizeAccessPolicy(const AccessPolicy& policy) {
  std::string result = "Access Policy Summary:\n";

  // Summarize queries
  const auto queries = policy.GetQueries();
  if (!queries.empty()) {
    result += std::format("  Queries ({}):\n", queries.size());
    for (size_t i = 0; i < queries.size(); ++i) {
      const auto& query = queries[i];
      result += std::format("    Query {}:\n", i);

      if (!query.read_components.empty()) {
        result += "      Read: ";
        for (const auto& comp : query.read_components) {
          result += std::format("{}, ", comp.name);
        }
        result += "\n";
      }

      if (!query.write_components.empty()) {
        result += "      Write: ";
        for (const auto& comp : query.write_components) {
          result += std::format("{}, ", comp.name);
        }
        result += "\n";
      }
    }
  }

  // Summarize resources
  const auto read_resources = policy.GetReadResources();
  const auto write_resources = policy.GetWriteResources();

  if (!read_resources.empty()) {
    result += "  Read Resources: ";
    for (const auto& res : read_resources) {
      result += std::format("{}, ", res.name);
    }
    result += "\n";
  }

  if (!write_resources.empty()) {
    result += "  Write Resources: ";
    for (const auto& res : write_resources) {
      result += std::format("{}, ", res.name);
    }
    result += "\n";
  }

  if (queries.empty() && read_resources.empty() && write_resources.empty()) {
    result += "  (No data access declared)\n";
  }

  return result;
}

}  // namespace helios::app::details
