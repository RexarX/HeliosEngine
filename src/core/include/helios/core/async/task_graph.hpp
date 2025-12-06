#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/assert.hpp>
#include <helios/core/async/common.hpp>
#include <helios/core/async/task.hpp>
#include <helios/core/logger.hpp>

#include <taskflow/algorithm/for_each.hpp>
#include <taskflow/algorithm/reduce.hpp>
#include <taskflow/algorithm/sort.hpp>
#include <taskflow/algorithm/transform.hpp>
#include <taskflow/taskflow.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace helios::async {

/**
 * @brief Represents a task dependency graph that can be executed by an Executor.
 * @details Wraps tf::Taskflow and provides methods to create tasks, manage dependencies, and build
 * complex computational graphs. Tasks within the graph can have dependencies that determine execution order.
 * @note Not thread-safe.
 * Do not modify a graph while it's being executed.
 */
class TaskGraph {
public:
  /**
   * @brief Constructs a task graph with the given name.
   * @param name Name for the task graph (default: "TaskGraph")
   */
  explicit TaskGraph(std::string_view name = "TaskGraph") { Name(name); }
  TaskGraph(const TaskGraph&) = delete;
  TaskGraph(TaskGraph&&) noexcept = default;
  ~TaskGraph() = default;

  TaskGraph& operator=(const TaskGraph&) = delete;
  TaskGraph& operator=(TaskGraph&&) noexcept = default;

  /*
   * @brief Clears all tasks and dependencies from this graph.
   */
  void Clear() { taskflow_.clear(); }

  /**
   * @brief Applies a visitor function to each task in this graph.
   * @tparam Visitor Callable type that accepts a Task reference
   * @param visitor Function to call for each task
   */
  template <typename Visitor>
    requires std::invocable<Visitor, Task&>
  void ForEachTask(const Visitor& visitor) const;

  /**
   * @brief Creates a static task with the given callable.
   * @tparam C Callable type
   * @param callable Function to execute when the task runs
   * @return Task handle for the created task
   */
  template <StaticTask C>
  Task EmplaceTask(C&& callable) {
    return Task(taskflow_.emplace(std::forward<C>(callable)));
  }

  /**
   * @brief Creates a dynamic task (subflow) with the given callable.
   * @details Requires sub_task_graph.hpp to be included.
   * @tparam C Callable type that accepts a SubTaskGraph reference
   * @param callable Function to execute when the task runs
   * @return Task handle for the created task
   */
  template <SubTask C>
  Task EmplaceTask(C&& callable);

  /**
   * @brief Creates multiple tasks from a list of callables.
   * @tparam Cs Callable types
   * @param callables Functions to execute when the tasks run
   * @return Array of task handles for the created tasks
   */
  template <AnyTask... Cs>
    requires(sizeof...(Cs) > 1)
  auto EmplaceTasks(Cs&&... callables) -> std::array<Task, sizeof...(Cs)> {
    return {EmplaceTask(std::forward<Cs>(callables))...};
  }

  /**
   * @brief Creates a placeholder task with no assigned work.
   * @return Task handle that can later be assigned work
   */
  Task CreatePlaceholder() { return Task(taskflow_.placeholder()); }

  /**
   * @brief Creates linear dependencies between tasks in the given range.
   * @tparam R Range type containing Task objects
   * @param tasks Range of tasks to linearize (first->second->third->...)
   */
  template <std::ranges::range R>
    requires std::same_as<std::ranges::range_value_t<R>, Task>
  void Linearize(const R& tasks);

  /**
   * @brief Creates a parallel for-each task over the given range.
   * @tparam R Range type
   * @tparam C Callable type
   * @param range Input range to iterate over
   * @param callable Function to apply to each element
   * @return Task handle for the parallel operation
   */
  template <std::ranges::range R, typename C>
    requires std::invocable<C, std::ranges::range_reference_t<R>>
  Task ForEach(const R& range, C&& callable) {
    return Task(taskflow_.for_each(std::ranges::begin(range), std::ranges::end(range), std::forward<C>(callable)));
  }

  /**
   * @brief Creates a parallel for-each task over an index range.
   * @tparam I Integral type
   * @tparam C Callable type
   * @param start Starting index (inclusive)
   * @param end Ending index (exclusive)
   * @param step Step size
   * @param callable Function to apply to each index
   * @return Task handle for the parallel operation
   */
  template <std::integral I, typename C>
    requires std::invocable<C, I>
  Task ForEachIndex(I start, I end, I step, C&& callable) {
    return Task(taskflow_.for_each_index(start, end, step, std::forward<C>(callable)));
  }

  /**
   * @brief Creates a parallel transform task that applies a function to each element.
   * @tparam InputRange Input range type
   * @tparam OutputRange Output range type
   * @tparam TransformFunc Transform function type
   * @param input_range Range of input elements
   * @param output_range Range to store transformed results
   * @param transform_func Function to apply to each input element
   * @return Task handle for the parallel operation
   */
  template <std::ranges::range InputRange, std::ranges::range OutputRange,
            std::invocable<std::ranges::range_reference_t<InputRange>> TransformFunc>
  Task Transform(const InputRange& input_range, OutputRange& output_range, TransformFunc&& transform_func) {
    return Task(taskflow_.transform(std::ranges::begin(input_range), std::ranges::end(input_range),
                                    std::ranges::begin(output_range), std::forward<TransformFunc>(transform_func)));
  }

  /**
   * @brief Creates a parallel reduction task that combines elements using a binary operation.
   * @tparam R Range type
   * @tparam T Result type
   * @tparam BinaryOp Binary operation type
   * @param range Range of elements to reduce
   * @param init Initial value and storage for the result
   * @param binary_op Binary function to combine elements
   * @return Task handle for the parallel operation
   */
  template <std::ranges::range R, typename T, typename BinaryOp>
    requires std::invocable<BinaryOp, T, std::ranges::range_reference_t<R>>
  Task Reduce(const R& range, T& init, BinaryOp&& binary_op) {
    return Task(
        taskflow_.reduce(std::ranges::begin(range), std::ranges::end(range), init, std::forward<BinaryOp>(binary_op)));
  }

  /**
   * @brief Creates a parallel sort task for the given range.
   * @tparam R Random access range type
   * @tparam Compare Comparator type
   * @param range Range of elements to sort
   * @param comparator Comparison function (default: std::less<>)
   * @return Task handle for the parallel operation
   */
  template <std::ranges::random_access_range R, typename Compare = std::less<>>
    requires std::predicate<Compare, std::ranges::range_reference_t<R>, std::ranges::range_reference_t<R>>
  Task Sort(R& range, Compare&& comparator = Compare{});

  /**
   * @brief Removes a task from this graph.
   * @param task Task to remove
   */
  void RemoveTask(const Task& task) { taskflow_.erase(task.UnderlyingTask()); }

  /**
   * @brief Removes a dependency relationship between two tasks.
   * @param from Source task (dependent)
   * @param to Target task (successor)
   */
  void RemoveDependency(const Task& from, const Task& to) {
    taskflow_.remove_dependency(from.UnderlyingTask(), to.UnderlyingTask());
  }

  /**
   * @brief Creates a module task that encapsulates another task graph.
   * @param other_graph Task graph to compose into this graph
   * @return Task handle representing the composed graph
   */
  Task Compose(TaskGraph& other_graph) { return Task(taskflow_.composed_of(other_graph.UnderlyingTaskflow())); }

  /**
   * @brief Dumps the task graph to a DOT format string.
   * @return String representation of the graph in DOT format
   */
  [[nodiscard]] std::string Dump() const { return taskflow_.dump(); }

  /**
   * @brief Sets the name of this task graph.
   * @warning Triggers an assertion if the name is empty.
   * @param name Name for the graph (must not be empty)
   */
  void Name(std::string_view name);

  /**
   * @brief Checks if this graph has no tasks.
   * @return True if the graph is empty, false otherwise
   */
  [[nodiscard]] bool Empty() const { return taskflow_.empty(); }

  /**
   * @brief Gets the number of tasks in this graph.
   * @return Count of tasks
   */
  [[nodiscard]] size_t TaskCount() const { return taskflow_.num_tasks(); }

  /**
   * @brief Gets the name of task graph.
   * @return Name of the graph
   */
  [[nodiscard]] const std::string& Name() const { return taskflow_.name(); }

private:
  [[nodiscard]] tf::Taskflow& UnderlyingTaskflow() noexcept { return taskflow_; }
  [[nodiscard]] const tf::Taskflow& UnderlyingTaskflow() const noexcept { return taskflow_; }

  tf::Taskflow taskflow_;

  friend class SubTaskGraph;
  friend class Executor;
};

template <typename Visitor>
  requires std::invocable<Visitor, Task&>
inline void TaskGraph::ForEachTask(const Visitor& visitor) const {
  taskflow_.for_each_task([&visitor](const tf::Task& tf_task) {
    Task helios_task(tf_task);
    std::invoke(visitor, helios_task);
  });
}

template <std::ranges::range R>
  requires std::same_as<std::ranges::range_value_t<R>, Task>
inline void TaskGraph::Linearize(const R& tasks) {
  std::vector<tf::Task> tf_tasks;
  if constexpr (std::ranges::sized_range<R>) {
    tf_tasks.reserve(std::ranges::size(tasks));
  }

  for (const auto& task : tasks) {
    tf_tasks.push_back(task.UnderlyingTask());
  }

  taskflow_.linearize(tf_tasks);
}

template <std::ranges::random_access_range R, typename Compare>
  requires std::predicate<Compare, std::ranges::range_reference_t<R>, std::ranges::range_reference_t<R>>
inline Task TaskGraph::Sort(R& range, Compare&& comparator) {
  if constexpr (std::same_as<std::remove_cvref_t<Compare>, std::less<>>) {
    return Task(taskflow_.sort(std::ranges::begin(range), std::ranges::end(range)));
  } else {
    return Task(taskflow_.sort(std::ranges::begin(range), std::ranges::end(range), std::forward<Compare>(comparator)));
  }
}

inline void TaskGraph::Name(std::string_view name) {
  HELIOS_ASSERT(!name.empty(), "Failed to set task graph name: 'name' cannot be empty!");
  taskflow_.name(std::string(name));
}

}  // namespace helios::async
