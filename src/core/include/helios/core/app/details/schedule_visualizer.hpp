#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/app/details/scheduler.hpp>
#include <helios/core/ecs/system.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace helios::app::details {

/**
 * @brief Visualizes schedule execution graphs for debugging and optimization.
 * @details Generates GraphViz DOT format output showing system dependencies,
 * data conflicts, and explicit ordering constraints.
 */
class ScheduleVisualizer {
public:
  /**
   * @brief Edge type in the schedule graph.
   */
  enum class EdgeType : uint8_t {
    DataDependency,   ///< Dependency due to data access conflict
    ExplicitBefore,   ///< Explicit "before" ordering constraint
    ExplicitAfter,    ///< Explicit "after" ordering constraint
    QueryConflict,    ///< Component query conflict
    ResourceConflict  ///< Resource access conflict
  };

  /**
   * @brief Information about an edge in the schedule graph.
   */
  struct EdgeInfo {
    size_t from_index = 0;
    size_t to_index = 0;
    EdgeType type;
    std::string description;
  };

  /**
   * @brief Exports a schedule to DOT format for visualization.
   * @param systems Vector of systems in the schedule
   * @param orderings Map of explicit ordering constraints
   * @param schedule_name Name of the schedule
   * @return DOT format string
   */
  [[nodiscard]] static std::string ExportToDot(std::span<const SystemNode> systems,
                                               const std::unordered_map<ecs::SystemTypeId, SystemOrdering>& orderings,
                                               std::string_view schedule_name);

  /**
   * @brief Exports a schedule with detailed conflict information.
   * @param systems Vector of systems in the schedule
   * @param orderings Map of explicit ordering constraints
   * @param schedule_name Name of the schedule
   * @return DOT format string with detailed annotations
   */
  [[nodiscard]] static std::string ExportDetailedToDot(
      std::span<const SystemNode> systems, const std::unordered_map<ecs::SystemTypeId, SystemOrdering>& orderings,
      std::string_view schedule_name);

  /**
   * @brief Generates a summary report of the schedule.
   * @param systems Vector of systems in the schedule
   * @param orderings Map of explicit ordering constraints
   * @return Human-readable text report
   */
  [[nodiscard]] static std::string GenerateReport(
      std::span<const SystemNode> systems, const std::unordered_map<ecs::SystemTypeId, SystemOrdering>& orderings);

private:
  /**
   * @brief Generates DOT node label for a system.
   * @param node System node
   * @param detailed Whether to include detailed info
   * @return DOT label string
   */
  [[nodiscard]] static std::string GenerateNodeLabel(const SystemNode& node, bool detailed);

  /**
   * @brief Generates DOT node attributes based on system properties.
   * @param node System node
   * @return DOT attributes string
   */
  [[nodiscard]] static std::string GenerateNodeAttributes(const SystemNode& node);

  /**
   * @brief Generates DOT edge representation.
   * @param edge Edge information
   * @return DOT edge string
   */
  [[nodiscard]] static std::string GenerateEdge(const EdgeInfo& edge);

  /**
   * @brief Collects all edges in the schedule graph.
   * @param systems Vector of systems in the schedule
   * @param orderings Map of explicit ordering constraints
   * @return Vector of edges
   */
  [[nodiscard]] static auto CollectEdges(std::span<const SystemNode> systems,
                                         const std::unordered_map<ecs::SystemTypeId, SystemOrdering>& orderings)
      -> std::vector<EdgeInfo>;

  /**
   * @brief Escapes special characters for DOT format.
   * @param str Input string
   * @return Escaped string
   */
  [[nodiscard]] static std::string EscapeDotString(std::string_view str);

  /**
   * @brief Gets color for edge type.
   * @param type Edge type
   * @return Color name or hex code
   */
  [[nodiscard]] static constexpr std::string_view GetEdgeColor(EdgeType type) noexcept;

  /**
   * @brief Gets label for edge type.
   * @param type Edge type
   * @return Label string
   */
  [[nodiscard]] static constexpr std::string_view GetEdgeLabel(EdgeType type) noexcept;
};

inline std::string ScheduleVisualizer::ExportToDot(
    std::span<const SystemNode> systems, const std::unordered_map<ecs::SystemTypeId, SystemOrdering>& orderings,
    std::string_view schedule_name) {
  std::string dot;
  dot.reserve(4096);  // Pre-allocate reasonable size

  // Header
  dot += "digraph ";
  dot += EscapeDotString(schedule_name);
  dot += " {\n";
  dot += "  rankdir=LR;\n";
  dot += "  node [shape=box, style=filled, fontname=\"Arial\"];\n";
  dot += "  edge [fontname=\"Arial\", fontsize=10];\n\n";

  // Add title
  dot += std::format("  labelloc=\"t\";\n");
  dot += std::format("  label=\"Schedule: {}\";\n", EscapeDotString(schedule_name));
  dot += std::format("  fontsize=16;\n\n");

  // Add nodes
  for (size_t i = 0; i < systems.size(); ++i) {
    const auto& node = systems[i];
    dot += std::format("  s{} [label=\"{}\", {}];\n", i, EscapeDotString(node.name), GenerateNodeAttributes(node));
  }

  dot += "\n";

  // Add edges
  const auto edges = CollectEdges(systems, orderings);
  for (const auto& edge : edges) {
    dot += "  ";
    dot += GenerateEdge(edge);
    dot += "\n";
  }

  // Legend
  dot += "\n  // Legend\n";
  dot += "  subgraph cluster_legend {\n";
  dot += "    label=\"Legend\";\n";
  dot += "    style=filled;\n";
  dot += "    color=lightgrey;\n";
  dot += "    node [shape=plaintext];\n";
  dot += "    legend [label=<\n";
  dot += "      <table border=\"0\" cellborder=\"1\" cellspacing=\"0\">\n";
  dot += "        <tr><td><font color=\"red\">Red</font></td><td>Data Dependency</td></tr>\n";
  dot += "        <tr><td><font color=\"blue\">Blue</font></td><td>Explicit Ordering</td></tr>\n";
  dot += "        <tr><td><font color=\"orange\">Orange</font></td><td>Query Conflict</td></tr>\n";
  dot += "        <tr><td><font color=\"purple\">Purple</font></td><td>Resource Conflict</td></tr>\n";
  dot += "      </table>\n";
  dot += "    >];\n";
  dot += "  }\n";

  dot += "}\n";
  return dot;
}

inline std::string ScheduleVisualizer::ExportDetailedToDot(
    std::span<const SystemNode> systems, const std::unordered_map<ecs::SystemTypeId, SystemOrdering>& orderings,
    std::string_view schedule_name) {
  std::string dot;
  dot.reserve(8192);

  // Header
  dot += "digraph ";
  dot += EscapeDotString(schedule_name);
  dot += " {\n";
  dot += "  rankdir=LR;\n";
  dot += "  node [shape=record, style=filled, fontname=\"Arial\"];\n";
  dot += "  edge [fontname=\"Arial\", fontsize=9];\n\n";

  // Add title
  dot += std::format("  labelloc=\"t\";\n");
  dot += std::format("  label=\"Schedule: {} (Detailed)\";\n", EscapeDotString(schedule_name));
  dot += std::format("  fontsize=16;\n\n");

  // Add nodes with detailed labels
  for (size_t i = 0; i < systems.size(); ++i) {
    const auto& node = systems[i];
    dot += std::format("  s{} [label=\"{}\", {}];\n", i, GenerateNodeLabel(node, true), GenerateNodeAttributes(node));
  }

  dot += "\n";

  // Add edges with detailed conflict information
  const auto edges = CollectEdges(systems, orderings);
  for (const auto& edge : edges) {
    dot += "  ";
    dot += GenerateEdge(edge);
    if (!edge.description.empty()) {
      dot += std::format(" [tooltip=\"{}\"]", EscapeDotString(edge.description));
    }
    dot += ";\n";
  }

  dot += "}\n";
  return dot;
}

inline std::string ScheduleVisualizer::GenerateReport(
    std::span<const SystemNode> systems, const std::unordered_map<ecs::SystemTypeId, SystemOrdering>& orderings) {
  std::string report;
  report.reserve(2048);

  report += "=== Schedule Analysis Report ===\n\n";
  report += std::format("Total Systems: {}\n\n", systems.size());

  // System list
  report += "Systems:\n";
  for (size_t i = 0; i < systems.size(); ++i) {
    const auto& node = systems[i];
    report += std::format("  {}. {}\n", i + 1, node.name);

    // Access policy summary
    const auto queries = node.access_policy.GetQueries();
    if (!queries.empty()) {
      report += std::format("     Queries: {}\n", queries.size());
    }

    const auto read_resources = node.access_policy.GetReadResources();
    const auto write_resources = node.access_policy.GetWriteResources();
    if (!read_resources.empty()) {
      report += std::format("     Read Resources: {}\n", read_resources.size());
    }
    if (!write_resources.empty()) {
      report += std::format("     Write Resources: {}\n", write_resources.size());
    }
  }

  // Dependency analysis
  report += "\nDependencies:\n";
  const auto edges = CollectEdges(systems, orderings);

  size_t data_deps = 0;
  size_t explicit_deps = 0;
  size_t query_conflicts = 0;
  size_t resource_conflicts = 0;

  for (const auto& edge : edges) {
    switch (edge.type) {
      case EdgeType::DataDependency:
        ++data_deps;
        break;
      case EdgeType::ExplicitBefore:
      case EdgeType::ExplicitAfter:
        ++explicit_deps;
        break;
      case EdgeType::QueryConflict:
        ++query_conflicts;
        break;
      case EdgeType::ResourceConflict:
        ++resource_conflicts;
        break;
    }
  }

  report += std::format("  Total Edges: {}\n", edges.size());
  report += std::format("  Data Dependencies: {}\n", data_deps);
  report += std::format("  Explicit Orderings: {}\n", explicit_deps);
  report += std::format("  Query Conflicts: {}\n", query_conflicts);
  report += std::format("  Resource Conflicts: {}\n", resource_conflicts);

  // Potential parallelism analysis
  report += "\nParallelism Analysis:\n";
  size_t independent_systems = 0;
  for (size_t i = 0; i < systems.size(); ++i) {
    bool has_conflict = false;
    for (const auto& edge : edges) {
      if (edge.from_index == i || edge.to_index == i) {
        has_conflict = true;
        break;
      }
    }
    if (!has_conflict) {
      ++independent_systems;
    }
  }
  report += std::format("  Independent Systems: {} (can run in parallel)\n", independent_systems);

  return report;
}

inline std::string ScheduleVisualizer::GenerateNodeLabel(const SystemNode& node, bool detailed) {
  if (!detailed) {
    return node.name;
  }

  // Detailed label with access policy info
  std::string label;
  label += node.name;
  label += "|";

  const auto queries = node.access_policy.GetQueries();
  if (!queries.empty()) {
    label += std::format("Queries: {}", queries.size());
  }

  const auto read_resources = node.access_policy.GetReadResources();
  const auto write_resources = node.access_policy.GetWriteResources();
  if (!read_resources.empty() || !write_resources.empty()) {
    if (!queries.empty()) {
      label += "\\n";
    }
    label += std::format("R:{} W:{}", read_resources.size(), write_resources.size());
  }

  return label;
}

inline std::string ScheduleVisualizer::GenerateNodeAttributes(const SystemNode& node) {
  // Color based on system characteristics
  std::string attrs = "fillcolor=\"lightblue\"";

  const auto queries = node.access_policy.GetQueries();
  const auto write_resources = node.access_policy.GetWriteResources();

  // Systems that write to resources get different color
  if (!write_resources.empty()) {
    attrs = "fillcolor=\"lightcoral\"";
  } else if (!queries.empty()) {
    attrs = "fillcolor=\"lightgreen\"";
  }

  return attrs;
}

inline std::string ScheduleVisualizer::GenerateEdge(const EdgeInfo& edge) {
  return std::format(R"(s{} -> s{} [color="{}", label="{}"])", edge.from_index, edge.to_index, GetEdgeColor(edge.type),
                     GetEdgeLabel(edge.type));
}

inline auto ScheduleVisualizer::CollectEdges(std::span<const SystemNode> systems,
                                             const std::unordered_map<ecs::SystemTypeId, SystemOrdering>& orderings)
    -> std::vector<EdgeInfo> {
  std::vector<EdgeInfo> edges;
  edges.reserve(systems.size() * 2);  // Rough estimate

  // Collect data dependency edges
  for (size_t i = 0; i < systems.size(); ++i) {
    for (size_t j = i + 1; j < systems.size(); ++j) {
      const auto& system_i = systems[i];
      const auto& system_j = systems[j];

      const bool query_conflict = system_i.access_policy.HasQueryConflict(system_j.access_policy);
      const bool resource_conflict = system_i.access_policy.HasResourceConflict(system_j.access_policy);

      if (query_conflict) {
        std::string desc = "Component access conflict";
        edges.push_back(
            EdgeInfo{.from_index = i, .to_index = j, .type = EdgeType::QueryConflict, .description = std::move(desc)});
      }

      if (resource_conflict) {
        std::string desc = "Resource access conflict";
        edges.push_back(EdgeInfo{
            .from_index = i, .to_index = j, .type = EdgeType::ResourceConflict, .description = std::move(desc)});
      }
    }
  }

  // Collect explicit ordering edges
  for (const auto& [system_id, ordering] : orderings) {
    // Find system index
    const auto it = std::ranges::find_if(systems, [system_id](const auto& node) { return node.type_id == system_id; });
    if (it == systems.end()) {
      continue;
    }

    const size_t from_idx = std::distance(systems.begin(), it);

    // Process "after" constraints
    for (auto after_id : ordering.after) {
      const auto after_it =
          std::ranges::find_if(systems, [after_id](const auto& node) { return node.type_id == after_id; });

      if (after_it != systems.end()) {
        const size_t to_idx = std::distance(systems.begin(), after_it);
        edges.push_back(EdgeInfo{.from_index = to_idx,
                                 .to_index = from_idx,
                                 .type = EdgeType::ExplicitAfter,
                                 .description = "Explicit: after"});
      }
    }

    // Process "before" constraints
    for (auto before_id : ordering.before) {
      const auto before_it =
          std::ranges::find_if(systems, [before_id](const auto& node) { return node.type_id == before_id; });

      if (before_it != systems.end()) {
        const size_t to_idx = std::distance(systems.begin(), before_it);
        edges.push_back(EdgeInfo{.from_index = from_idx,
                                 .to_index = to_idx,
                                 .type = EdgeType::ExplicitBefore,
                                 .description = "Explicit: before"});
      }
    }
  }

  return edges;
}

inline std::string ScheduleVisualizer::EscapeDotString(std::string_view str) {
  std::string escaped;
  escaped.reserve(str.size());

  for (const char ch : str) {
    switch (ch) {
      case '"':
        escaped += "\\\"";
        break;
      case '\\':
        escaped += "\\\\";
        break;
      case '\n':
        escaped += "\\n";
        break;
      default:
        escaped += ch;
        break;
    }
  }

  return escaped;
}

constexpr std::string_view ScheduleVisualizer::GetEdgeColor(EdgeType type) noexcept {
  switch (type) {
    case EdgeType::DataDependency:
      return "red";
    case EdgeType::ExplicitBefore:
    case EdgeType::ExplicitAfter:
      return "blue";
    case EdgeType::QueryConflict:
      return "orange";
    case EdgeType::ResourceConflict:
      return "purple";
    default:
      return "black";
  }
}

constexpr std::string_view ScheduleVisualizer::GetEdgeLabel(EdgeType type) noexcept {
  switch (type) {
    case EdgeType::DataDependency:
      return "data";
    case EdgeType::ExplicitBefore:
      return "before";
    case EdgeType::ExplicitAfter:
      return "after";
    case EdgeType::QueryConflict:
      return "query";
    case EdgeType::ResourceConflict:
      return "resource";
    default:
      return "";
  }
}

}  // namespace helios::app::details
