#include <pch.hpp>

#include <helios/ecs/schedule/dag.hpp>

#include <helios/ecs/system/system.hpp>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <format>
#include <queue>
#include <vector>

namespace helios::ecs {

size_t Dag::AddNode(SystemId id) {
  const size_t index = nodes_.size();
  nodes_.push_back(
      {.id = id, .kind = NodeKind::kSystem, .insertion_index = index});
  node_ids_.push_back(id);
  adjacency_.emplace_back();
  id_to_index_[id.id] = index;
  return index;
}

auto Dag::IndexOf(SystemId id) const -> DagResult<size_t> {
  const auto it = id_to_index_.find(id.id);
  if (it == id_to_index_.end()) [[unlikely]] {
    DagError error{
        .kind = DagErrorKind::kUnknownNode,
        .message =
            std::format("Node not found in DAG for system id '{}'", id.id),
        .involved_nodes = {id},
    };
    return std::unexpected(std::move(error));
  }
  return it->second;
}

auto Dag::AddEdge(SystemId from, SystemId to) -> DagResult<void> {
  if (from == to) [[unlikely]] {
    DagError error{
        .kind = DagErrorKind::kCycleDetected,
        .message =
            std::format("Self-loop detected for system id '{}'", from.id),
        .involved_nodes = {from},
    };
    return std::unexpected(std::move(error));
  }

  auto from_idx = IndexOf(from);
  if (!from_idx) [[unlikely]] {
    return std::unexpected(std::move(from_idx.error()));
  }

  auto to_idx = IndexOf(to);
  if (!to_idx) [[unlikely]] {
    return std::unexpected(std::move(to_idx.error()));
  }

  adjacency_[*from_idx].push_back(*to_idx);
  return {};
}

auto Dag::Sort() const -> DagResult<std::vector<SystemId>> {
  const size_t size = nodes_.size();
  if (size == 0) [[unlikely]] {
    return std::vector<SystemId>{};
  }

  std::vector<size_t> in_degree(size, 0);
  for (size_t i = 0; i < size; ++i) {
    for (size_t succ : adjacency_[i]) {
      ++in_degree[succ];
    }
  }

  using QueueEntry = std::pair<size_t, size_t>;
  const auto cmp = [](const QueueEntry& lhs, const QueueEntry& rhs) {
    return lhs.second > rhs.second;
  };
  std::priority_queue<QueueEntry, std::vector<QueueEntry>, decltype(cmp)> queue(
      cmp);

  for (size_t i = 0; i < size; ++i) {
    if (in_degree[i] == 0) {
      queue.emplace(i, nodes_[i].insertion_index);
    }
  }

  std::vector<SystemId> result;
  result.reserve(size);

  while (!queue.empty()) {
    const auto [current, _] = queue.top();
    queue.pop();

    result.push_back(nodes_[current].id);

    for (const size_t succ : adjacency_[current]) {
      if (--in_degree[succ] == 0) {
        queue.emplace(succ, nodes_[succ].insertion_index);
      }
    }
  }

  if (result.size() != size) {
    const auto cycle_path = ExtractCyclePath();
    std::vector<SystemId> involved;
    involved.reserve(cycle_path.size());
    for (const size_t idx : cycle_path) {
      involved.push_back(nodes_[idx].id);
    }

    DagError error{
        .kind = DagErrorKind::kCycleDetected,
        .message =
            std::format("Cycle detected in system ordering DAG with '{}' nodes",
                        cycle_path.size()),
        .involved_nodes = std::move(involved),
    };
    return std::unexpected(std::move(error));
  }

  return result;
}

auto Dag::ExtractCyclePath() const -> std::vector<size_t> {
  const size_t size = nodes_.size();

  std::vector<size_t> in_degree(size, 0);
  for (size_t i = 0; i < size; ++i) {
    for (const size_t succ : adjacency_[i]) {
      ++in_degree[succ];
    }
  }

  size_t start = 0;
  for (; start < size; ++start) {
    if (in_degree[start] > 0) {
      break;
    }
  }

  if (start == size) {
    return {};
  }

  std::vector<size_t> path;
  std::vector<bool> visited(size, false);
  std::vector<bool> on_stack(size, false);

  struct Frame {
    size_t node = 0;
    size_t next_child = 0;
  };

  std::vector<Frame> stack;
  stack.push_back({.node = start, .next_child = 0});
  on_stack[start] = true;
  path.push_back(start);

  while (!stack.empty()) {
    auto& frame = stack.back();

    if (frame.next_child >= adjacency_[frame.node].size()) {
      on_stack[frame.node] = false;
      path.pop_back();
      stack.pop_back();
      continue;
    }

    size_t succ = adjacency_[frame.node][frame.next_child];
    ++frame.next_child;

    if (on_stack[succ]) {
      const auto it = std::ranges::find(path, succ);
      std::vector<size_t> cycle(it, path.end());
      cycle.push_back(succ);
      return cycle;
    }

    if (!visited[succ]) {
      visited[succ] = true;
      on_stack[succ] = true;
      path.push_back(succ);
      stack.push_back({.node = succ, .next_child = 0});
    }
  }

  return {};
}

bool Dag::HasCycle() const {
  const size_t size = nodes_.size();
  if (size == 0) [[unlikely]] {
    return false;
  }

  std::vector<size_t> in_degree(size, 0);
  for (size_t i = 0; i < size; ++i) {
    for (size_t succ : adjacency_[i]) {
      ++in_degree[succ];
    }
  }

  std::queue<size_t> queue;
  for (size_t i = 0; i < size; ++i) {
    if (in_degree[i] == 0) {
      queue.push(i);
    }
  }

  size_t processed = 0;
  while (!queue.empty()) {
    size_t current = queue.front();
    queue.pop();
    ++processed;

    for (size_t succ : adjacency_[current]) {
      if (--in_degree[succ] == 0) {
        queue.push(succ);
      }
    }
  }

  return processed != size;
}

bool Dag::Reachable(SystemId from, SystemId to) const {
  if (from == to) {
    return true;
  }

  const auto from_idx = IndexOf(from);
  const auto to_idx = IndexOf(to);
  if (!from_idx || !to_idx) {
    return false;
  }

  std::vector<bool> visited(nodes_.size(), false);
  std::queue<size_t> queue;
  queue.push(*from_idx);
  visited[*from_idx] = true;

  while (!queue.empty()) {
    const size_t current = queue.front();
    queue.pop();

    for (const size_t succ : adjacency_[current]) {
      if (succ == *to_idx) {
        return true;
      }
      if (!visited[succ]) {
        visited[succ] = true;
        queue.push(succ);
      }
    }
  }

  return false;
}

}  // namespace helios::ecs
