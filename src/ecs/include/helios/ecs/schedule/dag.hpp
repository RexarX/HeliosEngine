#pragma once

#include <helios/ecs/system/system.hpp>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace helios::ecs {

enum class DagErrorKind : uint8_t {
  kUnknownNode,
  kCycleDetected,
};

struct DagError {
  DagErrorKind kind;
  std::string message;
  std::vector<SystemId> involved_nodes;
};

template <typename T>
using DagResult = std::expected<T, DagError>;

/**
 * @brief Directed acyclic graph used to compute the topological execution order
 * of systems within a schedule.
 */
class Dag {
public:
  Dag() = default;
  Dag(const Dag&) = delete;
  Dag(Dag&&) noexcept = default;
  ~Dag() = default;

  Dag& operator=(const Dag&) = delete;
  Dag& operator=(Dag&&) noexcept = default;

  /**
   * @brief Adds a node for `id`.
   * @return Internal index of the added node
   */
  size_t AddNode(SystemId id);

  /**
   * @brief Adds a directed edge from `from` to `to` (from must run before to).
   * @details Does **not** perform per-insertion cycle detection - O(1).
   * Call `Sort()` after all edges have been added; it detects cycles
   * as a side effect of Kahn's topological sort at O(V+E) total cost.
   * @return Result of the operation
   */
  [[nodiscard]] auto AddEdge(SystemId from, SystemId to) -> DagResult<void>;

  /**
   * @brief Performs a topological sort and returns system ids in execution
   * order.
   * @return Sorted ids on success, `DagError` on failure
   */
  [[nodiscard]] auto Sort() const -> DagResult<std::vector<SystemId>>;

  /**
   * @brief Gets the internal index of `id`.
   * @return Internal index on success, `DagError` on failure
   */
  [[nodiscard]] auto IndexOf(SystemId id) const -> DagResult<size_t>;

  /**
   * @brief Checks if this DAG has a cycle.
   * @return `true` if a cycle is detected, `false` otherwise
   */
  [[nodiscard]] bool HasCycle() const;

  /**
   * @brief Checks whether `to` is reachable from `from` along directed edges.
   * @param from Source system id
   * @param to Target system id
   * @return True if a directed path exists from `from` to `to`
   */
  [[nodiscard]] bool Reachable(SystemId from, SystemId to) const;

  /**
   * @brief Gets a read-only view of all nodes (in insertion order).
   * @return Read-only span of system ids in insertion order
   */
  [[nodiscard]] auto Nodes() const noexcept -> std::span<const SystemId> {
    return node_ids_;
  }

private:
  enum class NodeKind : uint8_t {
    kSystem,
    kSyncPoint,
  };

  struct Node {
    SystemId id;
    NodeKind kind = NodeKind::kSystem;
    size_t insertion_index = 0;
  };

  [[nodiscard]] auto ExtractCyclePath() const -> std::vector<size_t>;

  std::vector<Node> nodes_;
  std::vector<SystemId> node_ids_;
  std::vector<std::vector<size_t>> adjacency_;
  std::unordered_map<size_t, size_t> id_to_index_;
};

}  // namespace helios::ecs
