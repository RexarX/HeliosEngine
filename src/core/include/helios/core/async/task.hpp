#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/assert.hpp>
#include <helios/core/async/common.hpp>
#include <helios/core/logger.hpp>

#include <taskflow/core/task.hpp>

#include <concepts>
#include <cstddef>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>

namespace helios::async {

/**
 * @brief Represents a single task within a task graph.
 * @details Task objects are lightweight handles - copying them is safe and efficient.
 * Wraps tf::Task and provides methods to manage task dependencies, work assignment, and metadata.
 * Tasks can be static (simple callable) or dynamic (subtasks).
 * The underlying task data is managed by the TaskGraph.
 * @note Not thread-safe.
 */
class Task {
public:
  Task() = default;
  Task(const Task&) = default;
  Task(Task&&) = default;
  ~Task() = default;

  Task& operator=(const Task&) = default;
  Task& operator=(Task&&) = default;

  /**
   * @brief Resets the task handle to an empty state.
   * @note Not thread-safe.
   */
  void Reset() { task_.reset(); }

  /**
   * @brief Removes the work callable from this task.
   * @note Not thread-safe.
   */
  void ResetWork() { task_.reset_work(); }

  /**
   * @brief Assigns work to this task.
   * @warning Triggers assertion if current task is empty.
   * @tparam C Callable type
   * @param callable Function to execute when this task runs
   * @return Reference to this task for method chaining
   */
  template <AnyTask C>
  Task& Work(C&& callable);

  /**
   * @brief Makes this task run before the specified tasks.
   * @warning Triggers assertion if current task is empty.
   * @tparam Tasks Task parameter pack
   * @param tasks Tasks that should run after this task
   * @return Reference to this task for method chaining
   */
  template <typename... Tasks>
    requires(std::same_as<std::remove_cvref_t<Tasks>, Task> && ...)
  Task& Precede(Tasks&&... tasks);

  /**
   * @brief Makes this task run before all tasks in the specified range.
   * @warning Triggers assertion in next cases:
   * - Current task is empty.
   * - Any task in the range is empty.
   *
   * @tparam R Range type containing Task objects
   * @param tasks Range of tasks that should run after this task
   * @return Reference to this task for method chaining
   */
  template <std::ranges::range R>
    requires std::same_as<std::ranges::range_value_t<R>, Task>
  Task& Precede(const R& tasks);

  /**
   * @brief Makes this task run after the specified tasks.
   * @warning Triggers assertion in next cases:
   * - Current task is empty.
   * - Any task in the parameter pack is empty.
   *
   * @tparam Tasks Task parameter pack
   * @param tasks Tasks that should run before this task
   * @return Reference to this task for method chaining
   */
  template <typename... Tasks>
    requires(std::same_as<std::remove_cvref_t<Tasks>, Task> && ...)
  Task& Succeed(Tasks&&... tasks);

  /**
   * @brief Makes this task run after all tasks in the specified range.
   * @warning Triggers assertion in next cases:
   * - Current task is empty.
   * - Any task in the range is empty.
   *
   * @tparam R Range type containing Task objects
   * @param tasks Range of tasks that should run before this task
   * @return Reference to this task for method chaining
   */
  template <std::ranges::range R>
    requires std::same_as<std::ranges::range_value_t<R>, Task>
  Task& Succeed(const R& tasks);

  /**
   * @brief Assigns a name to this task.
   * @warning Triggers assertion if current task is empty.
   * @param name Name for the task (must not be empty)
   * @return Reference to this task for method chaining
   */
  Task& Name(const std::string& name);

  [[nodiscard]] bool operator==(const Task& other) const { return task_ == other.task_; }
  [[nodiscard]] bool operator!=(const Task& other) const { return !(*this == other); }

  /**
   * @brief Checks if this task has assigned work.
   * @return True if task has work, false otherwise.
   */
  [[nodiscard]] bool HasWork() const { return task_.has_work(); }

  /**
   * @brief Checks if this task handle is empty (not associated with any task).
   * @return True if task id empty, false otherwise.
   */
  [[nodiscard]] bool Empty() const { return task_.empty(); }

  /**
   * @brief Returns a hash value for this task.
   * @return Hash of task or 0 if current task is empty.
   */
  [[nodiscard]] size_t Hash() const;

  /**
   * @brief Gets the number of tasks that depend on this task.
   * @return Count of successors or '0' if current task is empty.
   */
  [[nodiscard]] size_t SuccessorsCount() const;

  /**
   * @brief Gets the number of tasks this task depends on.
   * @return Count of predecessors or 0 if current task is empty.
   */
  [[nodiscard]] size_t PredecessorsCount() const;

  /**
   * @brief Gets the number of strong dependencies this task has.
   * @return Count of strong dependencies or 0 if current task is empty.
   */
  [[nodiscard]] size_t StrongDependenciesCount() const;

  /**
   * @brief Gets the number of weak dependencies this task has.
   * @return Count of weak dependencies or 0 if current task is empty.
   */
  [[nodiscard]] size_t WeakDependenciesCount() const;

  /**
   * @brief Gets the name of this task.
   * @return Returns empty `std::string_view` if current task is empty.
   */
  [[nodiscard]] std::string_view Name() const;

  /**
   * @brief Gets the type of this task.
   * @details Returns `TaskType::Undefined` if current task is empty.
   */
  [[nodiscard]] TaskType Type() const;

private:
  explicit Task(const tf::Task& task) : task_(task) {}

  [[nodiscard]] tf::Task& UnderlyingTask() noexcept { return task_; }
  [[nodiscard]] const tf::Task& UnderlyingTask() const noexcept { return task_; }

  tf::Task task_;

  friend class TaskGraph;
  friend class SubTaskGraph;
  friend class TaskGraphBuilder;
  friend class Executor;
};

template <AnyTask C>
inline Task& Task::Work(C&& callable) {
  HELIOS_ASSERT(!Empty(), "Failed to set task work: Cannot assign work to empty task!");
  task_.work(std::forward<C>(callable));
  return *this;
}

template <typename... Tasks>
  requires(std::same_as<std::remove_cvref_t<Tasks>, Task> && ...)
inline Task& Task::Precede(Tasks&&... tasks) {
  HELIOS_ASSERT(!Empty(), "Failed to precede task: Task cannot be empty!");
  return Precede(std::to_array({std::forward<Tasks>(tasks)...}));
}

template <std::ranges::range R>
  requires std::same_as<std::ranges::range_value_t<R>, Task>
inline Task& Task::Precede(const R& tasks) {
  HELIOS_ASSERT(!Empty(), "Failed to precede task: Task cannot be empty!");
  auto view = tasks | std::ranges::views::filter([](const auto& task) { return !task.Empty(); });
  for (const auto& task : view) {
    task_.precede(task.task_);
  }
  return *this;
}

template <typename... Tasks>
  requires(std::same_as<std::remove_cvref_t<Tasks>, Task> && ...)
inline Task& Task::Succeed(Tasks&&... tasks) {
  HELIOS_ASSERT(!Empty(), "Failed to succeed task: Task cannot be empty!");
  return Succeed(std::to_array({std::forward<Tasks>(tasks)...}));
}

template <std::ranges::range R>
  requires std::same_as<std::ranges::range_value_t<R>, Task>
inline Task& Task::Succeed(const R& tasks) {
  HELIOS_ASSERT(!Empty(), "Failed to succeed task: Task cannot be empty!");
  auto view = tasks | std::ranges::views::filter([](const auto& task) { return !task.Empty(); });
  for (const auto& task : view) {
    task_.succeed(task.task_);
  }
  return *this;
}

inline Task& Task::Name(const std::string& name) {
  HELIOS_ASSERT(!Empty(), "Failed to set task name: Cannot assign name to empty task!");
  HELIOS_ASSERT(!name.empty(), "Failed to set task name: 'name' cannot be empty!");
  task_.name(name);
  return *this;
}

inline size_t Task::Hash() const {
  if (Empty()) [[unlikely]] {
    return 0;
  }

  return task_.hash_value();
}

inline size_t Task::SuccessorsCount() const {
  if (Empty()) [[unlikely]] {
    return 0;
  }

  return task_.num_successors();
}

inline size_t Task::PredecessorsCount() const {
  if (Empty()) [[unlikely]] {
    return 0;
  }

  return task_.num_predecessors();
}

inline size_t Task::StrongDependenciesCount() const {
  if (Empty()) [[unlikely]] {
    return 0;
  }

  return task_.num_strong_dependencies();
}

inline size_t Task::WeakDependenciesCount() const {
  if (Empty()) [[unlikely]] {
    return 0;
  }

  return task_.num_weak_dependencies();
}

inline std::string_view Task::Name() const {
  if (Empty()) [[unlikely]] {
    return {};
  }

  return task_.name();
}

inline TaskType Task::Type() const {
  if (Empty()) [[unlikely]] {
    return TaskType::Undefined;
  }

  return details::ConvertTaskType(task_.type());
}

}  // namespace helios::async
