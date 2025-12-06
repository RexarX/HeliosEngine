#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/assert.hpp>
#include <helios/core/async/async_task.hpp>
#include <helios/core/async/common.hpp>
#include <helios/core/async/future.hpp>
#include <helios/core/async/task.hpp>
#include <helios/core/async/task_graph.hpp>
#include <helios/core/logger.hpp>

#include <taskflow/algorithm/for_each.hpp>
#include <taskflow/algorithm/reduce.hpp>
#include <taskflow/algorithm/sort.hpp>
#include <taskflow/algorithm/transform.hpp>
#include <taskflow/core/async_task.hpp>
#include <taskflow/taskflow.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <future>
#include <ranges>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace helios::async {

/**
 * @brief Dynamic task graph that can be created within the execution of a task.
 * @details Wraps tf::Subflow and provides methods to create tasks dynamically at runtime.
 * SubTaskGraphs are spawned from the execution of a SubTask and allow for runtime-dependent task creation and
 * dependency management.
 * @note Partially thread-safe.
 * @warning Only the worker thread that spawned the subflow should modify it.
 */
class SubTaskGraph {
public:
  SubTaskGraph(const SubTaskGraph&) = delete;
  SubTaskGraph(SubTaskGraph&&) = default;
  ~SubTaskGraph() = default;

  SubTaskGraph& operator=(const SubTaskGraph&) = delete;
  SubTaskGraph& operator=(SubTaskGraph&&) = delete;

  /**
   * @brief Joins the subflow with its parent task.
   * @details Called automatically when the SubTaskGraph goes out of scope, unless Join() has already been called.
   * @warning Must be called by the same worker thread that created this subflow.
   */
  void Join() { subflow_.join(); }

  /**
   * @brief Specifies whether to keep the sub task graph after it is joined.
   * @details By default, a sub task graph is destroyed after being joined. Retaining it allows it to
   * remain valid after being joined.
   * @warning Must be called by the same worker thread that created this subflow.
   * @param flag True to retain, false otherwise
   */
  void Retain(bool flag) noexcept { subflow_.retain(flag); }

  /**
   * @brief Creates a static task with the given callable.
   * @note Not thread-safe
   * @tparam C Callable type
   * @param callable Function to execute when the task runs
   * @return Task handle for the created task
   */
  template <StaticTask C>
  Task EmplaceTask(C&& callable) {
    return Task(subflow_.emplace(std::forward<C>(callable)));
  }

  /**
   * @brief Creates a dynamic task (nested subflow) with the given callable.
   * @note Not thread-safe
   * @tparam C Callable type that accepts a SubTaskGraph reference
   * @param callable Function to execute when the task runs
   * @return Task handle for the created task
   */
  template <SubTask C>
  Task EmplaceTask(C&& callable);

  /**
   * @brief Creates multiple tasks from a list of callables.
   * @note Not thread-safe
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
   * @note Not thread-safe
   * @return Task handle that can later be assigned work
   */
  Task CreatePlaceholder() { return Task(subflow_.placeholder()); }

  /**
   * @brief Creates linear dependencies between tasks in the given range.
   * @note Not thread-safe
   * @tparam R Range type containing Task objects
   * @param tasks Range of tasks to linearize (first->second->third->...)
   */
  template <std::ranges::range R>
    requires std::same_as<std::ranges::range_value_t<R>, Task>
  void Linearize(const R& tasks);

  /**
   * @brief Creates a parallel for-each task over the given range.
   * @note Not thread-safe
   * @tparam R Range type
   * @tparam C Callable type
   * @param range Input range to iterate over
   * @param callable Function to apply to each element
   * @return Task handle for the parallel operation
   */
  template <std::ranges::range R, std::invocable<std::ranges::range_reference_t<R>> C>
  Task ForEach(const R& range, C&& callable) {
    return Task(subflow_.for_each(std::ranges::begin(range), std::ranges::end(range), std::forward<C>(callable)));
  }

  /**
   * @brief Creates a parallel for-each task over an index range.
   * @note Not thread-safe
   * @tparam I Integral type
   * @tparam C Callable type
   * @param start Starting index (inclusive)
   * @param end Ending index (exclusive)
   * @param step Step size
   * @param callable Function to apply to each index
   * @return Task handle for the parallel operation
   */
  template <std::integral I, std::invocable<I> C>
  Task ForEachIndex(I start, I end, I step, C&& callable) {
    return Task(subflow_.for_each_index(start, end, step, std::forward<C>(callable)));
  }

  /**
   * @brief Creates a parallel transform task that applies a function to each element.
   * @note Not thread-safe
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
    return Task(subflow_.transform(std::ranges::begin(input_range), std::ranges::end(input_range),
                                   std::ranges::begin(output_range), std::forward<TransformFunc>(transform_func)));
  }

  /**
   * @brief Creates a parallel reduction task that combines elements using a binary operation.
   * @note Not thread-safe
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
        subflow_.reduce(std::ranges::begin(range), std::ranges::end(range), init, std::forward<BinaryOp>(binary_op)));
  }

  /**
   * @brief Creates a parallel sort task for the given range.
   * @note Not thread-safe
   * @tparam R Random access range type
   * @tparam Compare Comparator type
   * @param range Range of elements to sort
   * @param comparator Comparison function (default: std::less<>)
   * @return Task handle for the parallel operation
   */
  template <std::ranges::random_access_range R, typename Compare = std::less<>>
    requires std::predicate<Compare, std::ranges::range_reference_t<R>, std::ranges::range_reference_t<R>>
  Task Sort(R& range, Compare&& comparator = Compare());

  /**
   * @brief Removes a task from this subflow.
   * @note Not thread-safe
   * @param task Task to remove
   */
  void RemoveTask(const Task& task) { subflow_.erase(task.UnderlyingTask()); }

  /**
   * @brief Creates a module task that encapsulates another task graph.
   * @note Not thread-safe
   * @param other_graph Task graph to compose into this subflow
   * @return Task handle representing the composed graph
   */
  template <typename T>
  Task ComposedOf(T& other_graph) {
    return Task(subflow_.composed_of(other_graph.UnderlyingTaskflow()));
  }

  /**
   * @brief Checks if this subflow can be joined.
   * @details A subflow is joinable if it has not yet been joined with its parent task.
   * @return True if the subflow is joinable, false otherwise
   */
  [[nodiscard]] bool Joinable() const noexcept { return subflow_.joinable(); }

  /**
   * @brief Checks if this subflow will be retained.
   * @details A retained subflow remains valid after being joined.
   * @return True if the subflow will be retained, false otherwise
   */
  [[nodiscard]] bool WillBeRetained() const noexcept { return subflow_.retain(); }

  // Executor related methods

  /**
   * @brief Runs a task graph once.
   * @details Task graph is not owned - ensure it remains alive during execution.
   * @note Thread-safe.
   * @param graph Task graph to execute
   * @return Future that completes when execution finishes
   */
  auto Run(TaskGraph& graph) -> Future<void> {
    return Future<void>(subflow_.executor().run(graph.UnderlyingTaskflow()));
  }

  /**
   * @brief Runs a task graph once.
   * @note Thread-safe.
   * @param graph Task graph to execute (moved)
   * @return Future that completes when execution finishes
   */
  auto Run(TaskGraph&& graph) -> Future<void> {
    return Future<void>(subflow_.executor().run(std::move(std::move(graph).UnderlyingTaskflow())));
  }

  /**
   * @brief Runs a task graph once and invokes a callback upon completion.
   * @details Task graph is not owned - ensure it remains alive during execution.
   * @note Thread-safe.
   * @tparam C Callable type
   * @param graph Task graph to execute
   * @param callable Callback to invoke after execution completes
   * @return Future that completes when execution finishes
   */
  template <std::invocable C>
  auto Run(TaskGraph& graph, C&& callable) -> Future<void> {
    return Future<void>(subflow_.executor().run(graph.UnderlyingTaskflow(), std::forward<C>(callable)));
  }

  /**
   * @brief Runs a moved task graph once and invokes a callback upon completion.
   * @note Thread-safe.
   * @tparam C Callable type
   * @param graph Task graph to execute (moved)
   * @param callable Callback to invoke after execution completes
   * @return Future that completes when execution finishes
   */
  template <std::invocable C>
  auto Run(TaskGraph&& graph, C&& callable) -> Future<void> {
    return Future<void>(
        subflow_.executor().run(std::move(std::move(graph).UnderlyingTaskflow()), std::forward<C>(callable)));
  }

  /**
   * @brief Runs a task graph for the specified number of times.
   * @details Task graph is not owned - ensure it remains alive during execution.
   * @note Thread-safe.
   * @param graph Task graph to execute
   * @param count Number of times to run the graph
   * @return Future that completes when all executions finish
   */
  auto RunN(TaskGraph& graph, size_t count) -> Future<void> {
    return Future<void>(subflow_.executor().run_n(graph.UnderlyingTaskflow(), count));
  }

  /**
   * @brief Runs a moved task graph for the specified number of times.
   * @note Thread-safe.
   * @param graph Task graph to execute (moved)
   * @param count Number of times to run the graph
   * @return Future that completes when all executions finish
   */
  auto RunN(TaskGraph&& graph, size_t count) -> Future<void> {
    return Future<void>(subflow_.executor().run_n(std::move(std::move(graph).UnderlyingTaskflow()), count));
  }

  /**
   * @brief Runs a task graph for the specified number of times and invokes a callback.
   * @details Task graph is not owned - ensure it remains alive during execution.
   * @note Thread-safe.
   * @tparam C Callable type
   * @param graph Task graph to execute
   * @param count Number of times to run the graph
   * @param callable Callback to invoke after all executions complete
   * @return Future that completes when all executions finish
   */
  template <std::invocable C>
  auto RunN(TaskGraph& graph, size_t count, C&& callable) -> Future<void> {
    return Future<void>(subflow_.executor().run_n(graph.UnderlyingTaskflow(), count, std::forward<C>(callable)));
  }

  /**
   * @brief Runs a moved task graph for the specified number of times and invokes a callback.
   * @note Thread-safe.
   * @tparam C Callable type
   * @param graph Task graph to execute (moved)
   * @param count Number of times to run the graph
   * @param callable Callback to invoke after all executions complete
   * @return Future that completes when all executions finish
   */
  template <std::invocable C>
  auto RunN(TaskGraph&& graph, size_t count, C&& callable) -> Future<void> {
    return Future<void>(
        subflow_.executor().run_n(std::move(std::move(graph).UnderlyingTaskflow()), count, std::forward<C>(callable)));
  }

  /**
   * @brief Runs a task graph repeatedly until the predicate returns true.
   * @details Task graph is not owned - ensure it remains alive during execution.
   * @note Thread-safe.
   * @tparam Predicate Predicate type
   * @param graph Task graph to execute
   * @param predicate Boolean predicate to determine when to stop
   * @return Future that completes when predicate returns true
   */
  template <std::predicate Predicate>
  auto RunUntil(TaskGraph& graph, Predicate&& predicate) -> Future<void> {
    return Future<void>(subflow_.executor().run_until(graph.UnderlyingTaskflow(), std::forward<Predicate>(predicate)));
  }

  /**
   * @brief Runs a moved task graph repeatedly until the predicate returns true.
   * @note Thread-safe.
   * @tparam Predicate Predicate type
   * @param graph Task graph to execute (moved)
   * @param predicate Boolean predicate to determine when to stop
   * @return Future that completes when predicate returns true
   */
  template <std::predicate Predicate>
  auto RunUntil(TaskGraph&& graph, Predicate&& predicate) -> Future<void> {
    return Future<void>(subflow_.executor().run_until(std::move(std::move(graph).UnderlyingTaskflow()),
                                                      std::forward<Predicate>(predicate)));
  }

  /**
   * @brief Runs a task graph repeatedly until the predicate returns true, then invokes a callback.
   * @details Task graph is not owned - ensure it remains alive during execution.
   * @note Thread-safe.
   * @tparam Predicate Predicate type
   * @tparam C Callable type
   * @param graph Task graph to execute
   * @param predicate Boolean predicate to determine when to stop
   * @param callable Callback to invoke after execution completes
   * @return Future that completes when predicate returns true and callback finishes
   */
  template <std::predicate Predicate, std::invocable C>
  auto RunUntil(TaskGraph& graph, Predicate&& predicate, C&& callable) -> Future<void> {
    return Future<void>(subflow_.executor().run_until(graph.UnderlyingTaskflow(), std::forward<Predicate>(predicate),
                                                      std::forward<C>(callable)));
  }

  /**
   * @brief Runs a moved task graph repeatedly until the predicate returns true, then invokes a callback.
   * @note Thread-safe.
   * @tparam Predicate Predicate type
   * @tparam C Callable type
   * @param graph Task graph to execute (moved)
   * @param predicate Boolean predicate to determine when to stop
   * @param callable Callback to invoke after execution completes
   * @return Future that completes when predicate returns true and callback finishes
   */
  template <std::predicate Predicate, std::invocable C>
  auto RunUntil(TaskGraph&& graph, Predicate&& predicate, C&& callable) -> Future<void> {
    return Future<void>(subflow_.executor().run_until(std::move(std::move(graph).UnderlyingTaskflow()),
                                                      std::forward<Predicate>(predicate), std::forward<C>(callable)));
  }

  /**
   * @brief Creates an asynchronous task that runs the given callable.
   * @details The task is scheduled immediately and runs independently.
   * @note Thread-safe.
   * @tparam C Callable type
   * @param callable Function to execute asynchronously
   * @return Future that will hold the result of the execution
   */
  template <std::invocable C>
  auto Async(C&& callable) -> std::future<std::invoke_result_t<C>> {
    return subflow_.executor().async(std::forward<C>(callable));
  }

  /**
   * @brief Creates a named asynchronous task that runs the given callable.
   * @details The task is scheduled immediately and runs independently.
   * @note Thread-safe.
   * @tparam C Callable type
   * @param name Name for the task (useful for debugging/profiling)
   * @param callable Function to execute asynchronously
   * @return Future that will hold the result of the execution
   */
  template <std::invocable C>
  auto Async(std::string name, C&& callable) -> std::future<std::invoke_result_t<C>> {
    return subflow_.executor().async(std::move(name), std::forward<C>(callable));
  }

  /**
   * @brief Creates an asynchronous task without returning a future.
   * @details More efficient than Async when you don't need the result.
   * @note Thread-safe.
   * @tparam C Callable type
   * @param callable Function to execute asynchronously
   */
  template <std::invocable C>
  void SilentAsync(C&& callable) {
    subflow_.executor().silent_async(std::forward<C>(callable));
  }

  /**
   * @brief Creates a named asynchronous task without returning a future.
   * @details More efficient than Async when you don't need the result.
   * @note Thread-safe.
   * @tparam C Callable type
   * @param name Name for the task (useful for debugging/profiling)
   * @param callable Function to execute asynchronously
   */
  template <std::invocable C>
  void SilentAsync(std::string name, C&& callable) {
    subflow_.executor().silent_async(std::move(name), std::forward<C>(callable));
  }

  /**
   * @brief Creates an asynchronous task that runs after specified dependencies complete.
   * @details The task will only execute after all dependencies finish.
   * @note Thread-safe.
   * @tparam C Callable type
   * @tparam Dependencies Range type containing AsyncTask dependencies
   * @param callable Function to execute asynchronously
   * @param dependencies Tasks that must complete before this task runs
   * @return Pair containing AsyncTask handle and Future for the result
   */
  template <std::invocable C, std::ranges::range Dependencies>
    requires std::same_as<std::ranges::range_value_t<Dependencies>, AsyncTask>
  auto DependentAsync(C&& callable, const Dependencies& dependencies)
      -> std::pair<AsyncTask, std::future<std::invoke_result_t<C>>>;

  /**
   * @brief Creates an asynchronous task that runs after dependencies complete, without returning a future.
   * @details More efficient than DependentAsync when you don't need the result.
   * @note Thread-safe.
   * @tparam C Callable type
   * @tparam Dependencies Range type containing AsyncTask dependencies
   * @param callable Function to execute asynchronously
   * @param dependencies Tasks that must complete before this task runs
   * @return AsyncTask handle
   */
  template <std::invocable C, std::ranges::range Dependencies>
    requires std::same_as<std::ranges::range_value_t<Dependencies>, AsyncTask>
  AsyncTask SilentDependentAsync(C&& callable, const Dependencies& dependencies);

  /**
   * @brief Blocks until all submitted tasks complete.
   * @details Waits for all taskflows and async tasks to finish.
   * @note Thread-safe.
   */
  void WaitForAll() { subflow_.executor().wait_for_all(); }

  /**
   * @brief Runs a task graph cooperatively and waits until it completes using the current worker thread.
   * @warning Must be called from within a worker thread of this executor.
   * Triggers assertion if called from a non-worker thread.
   * @param graph Task graph to execute
   */
  void CoRun(TaskGraph& graph);

  /**
   * @brief Keeps the current worker thread running until the predicate returns true.
   * @warning Must be called from within a worker thread of this executor.
   * Triggers assertion if called from a non-worker thread.
   * @tparam Predicate Predicate type
   * @param predicate Boolean predicate to determine when to stop
   */
  template <std::predicate Predicate>
  void CoRunUntil(Predicate&& predicate);

  /**
   * @brief Checks if the current thread is a worker thread of this executor.
   * @note Thread safe.
   * @return True if current thread is a worker, false otherwise
   */
  [[nodiscard]] bool IsWorkerThread() const { return CurrentWorkerId() != -1; }

  /**
   * @brief Gets the ID of the current worker thread.
   * @note Thread-safe.
   * @return Worker ID (0 to N-1) or -1 if not a worker thread
   */
  [[nodiscard]] int CurrentWorkerId() const { return subflow_.executor().this_worker_id(); }

  /**
   * @brief Gets the total number of worker threads.
   * @note Thread safe.
   * @return Count of workers.
   */
  [[nodiscard]] size_t WorkerCount() const noexcept { return subflow_.executor().num_workers(); }

  /**
   * @brief Gets the number of worker threads currently waiting for work.
   * @note Thread safe.
   * @return Count of idle workers.
   */
  [[nodiscard]] size_t IdleWorkerCount() const noexcept { return subflow_.executor().num_waiters(); }

  /**
   * @brief Gets the number of task queues in the work-stealing scheduler.
   * @note Thread safe.
   * @return Count of queues.
   */
  [[nodiscard]] size_t QueueCount() const noexcept { return subflow_.executor().num_queues(); }

  /**
   * @brief Gets the number of task graphs currently being executed.
   * @note Thread safe.
   * @return Count of running topologies.
   */
  [[nodiscard]] size_t RunningTopologyCount() const { return subflow_.executor().num_topologies(); }

private:
  explicit SubTaskGraph(tf::Subflow& subflow) : subflow_(subflow) {}

  [[nodiscard]] tf::Subflow& UnderlyingSubflow() noexcept { return subflow_; }
  [[nodiscard]] const tf::Subflow& UnderlyingSubflow() const noexcept { return subflow_; }

  tf::Subflow& subflow_;

  friend class TaskGraph;
  friend class Executor;
};

template <SubTask C>
inline Task SubTaskGraph::EmplaceTask(C&& callable) {
  return Task(subflow_.emplace([callable = std::forward<C>(callable)](tf::Subflow& sf) mutable {
    SubTaskGraph sub_graph(sf);
    callable(sub_graph);
  }));
}

template <std::ranges::range R>
  requires std::same_as<std::ranges::range_value_t<R>, Task>
inline void SubTaskGraph::Linearize(const R& tasks) {
  std::vector<tf::Task> tf_tasks;
  tf_tasks.reserve(std::ranges::size(tasks));

  for (const auto& task : tasks) {
    tf_tasks.push_back(task.UnderlyingTask());
  }

  subflow_.linearize(tf_tasks);
}

template <std::ranges::random_access_range R, typename Compare>
  requires std::predicate<Compare, std::ranges::range_reference_t<R>, std::ranges::range_reference_t<R>>
inline Task SubTaskGraph::Sort(R& range, Compare&& comparator) {
  if constexpr (std::same_as<std::remove_cvref_t<Compare>, std::less<>>) {
    return Task(subflow_.sort(std::ranges::begin(range), std::ranges::end(range)));
  } else {
    return Task(subflow_.sort(std::ranges::begin(range), std::ranges::end(range), std::forward<Compare>(comparator)));
  }
}

template <std::invocable C, std::ranges::range Dependencies>
  requires std::same_as<std::ranges::range_value_t<Dependencies>, AsyncTask>
inline auto SubTaskGraph::DependentAsync(C&& callable, const Dependencies& dependencies)
    -> std::pair<AsyncTask, std::future<std::invoke_result_t<C>>> {
  std::vector<tf::AsyncTask> tf_deps;
  if constexpr (std::ranges::sized_range<Dependencies>) {
    tf_deps.reserve(std::ranges::size(dependencies));
  }

  for (const auto& dep : dependencies) {
    tf_deps.push_back(dep.UnderlyingTask());
  }

  auto [task, future] = subflow_.executor().dependent_async(std::forward<C>(callable), tf_deps.begin(), tf_deps.end());
  return std::make_pair(AsyncTask(std::move(task)), std::move(future));
}

template <std::invocable C, std::ranges::range Dependencies>
  requires std::same_as<std::ranges::range_value_t<Dependencies>, AsyncTask>
inline AsyncTask SubTaskGraph::SilentDependentAsync(C&& callable, const Dependencies& dependencies) {
  std::vector<tf::AsyncTask> tf_deps;
  if constexpr (std::ranges::sized_range<Dependencies>) {
    tf_deps.reserve(std::ranges::size(dependencies));
  }

  for (const auto& dep : dependencies) {
    tf_deps.push_back(dep.UnderlyingTask());
  }

  return AsyncTask(
      subflow_.executor().silent_dependent_async(std::forward<C>(callable), tf_deps.begin(), tf_deps.end()));
}

template <SubTask C>
inline Task TaskGraph::EmplaceTask(C&& callable) {
  return Task(taskflow_.emplace([callable = std::forward<C>(callable)](tf::Subflow& subflow) {
    SubTaskGraph sub_graph(subflow);
    std::invoke(callable, sub_graph);
  }));
}

inline void SubTaskGraph::CoRun(TaskGraph& graph) {
  HELIOS_ASSERT(IsWorkerThread(), "Failed to co-run: Must be called from a worker thread");
  subflow_.executor().corun(graph.UnderlyingTaskflow());
}

template <std::predicate Predicate>
inline void SubTaskGraph::CoRunUntil(Predicate&& predicate) {
  HELIOS_ASSERT(IsWorkerThread(), "Failed to co-run until: Must be called from a worker thread");
  subflow_.executor().corun_until(std::forward<Predicate>(predicate));
}

}  // namespace helios::async
