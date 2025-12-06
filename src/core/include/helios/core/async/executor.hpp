#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/assert.hpp>
#include <helios/core/async/async_task.hpp>
#include <helios/core/async/future.hpp>
#include <helios/core/async/task_graph.hpp>
#include <helios/core/logger.hpp>

#include <taskflow/core/async_task.hpp>
#include <taskflow/core/executor.hpp>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <future>
#include <ranges>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace helios::async {

/**
 * @brief Manages worker threads and executes task graphs using work-stealing scheduling.
 * @details The Executor wraps tf::Executor and provides methods to run TaskGraphs and create
 * asynchronous tasks. It manages a pool of worker threads that efficiently execute
 * tasks using a work-stealing algorithm.
 * @note All member functions are thread-safe unless otherwise noted.
 */
class Executor {
public:
  /** Default constructs an executor with hardware concurrency worker threads. */
  Executor() = default;

  /**
   * @brief Constructs an executor with the specified number of worker threads.
   * @param worker_thread_count Number of worker threads to create
   */
  explicit Executor(size_t worker_thread_count) : executor_(worker_thread_count) {}
  Executor(const Executor&) = delete;
  Executor(Executor&&) = delete;
  ~Executor() = default;

  Executor& operator=(const Executor&) = delete;
  Executor& operator=(Executor&&) = delete;

  /**
   * @brief Runs a task graph once.
   * @details The executor does not own the graph - ensure it remains alive during execution.
   * @note Thread-safe.
   * @param graph Task graph to execute
   * @return Future that completes when execution finishes
   */
  auto Run(TaskGraph& graph) -> Future<void> { return Future<void>(executor_.run(graph.UnderlyingTaskflow())); }

  /**
   * @brief Runs a moved task graph once.
   * @details The executor takes ownership of the moved graph.
   * @note Thread-safe.
   * @param graph Task graph to execute (moved)
   * @return Future that completes when execution finishes
   */
  auto Run(TaskGraph&& graph) -> Future<void> {
    return Future<void>(executor_.run(std::move(std::move(graph).UnderlyingTaskflow())));
  }

  /**
   * @brief Runs a task graph once and invokes a callback upon completion.
   * @details The executor does not own the graph - ensure it remains alive during execution.
   * @note Thread-safe.
   * @tparam C Callable type
   * @param graph Task graph to execute
   * @param callable Callback to invoke after execution completes
   * @return Future that completes when execution finishes
   */
  template <std::invocable C>
  auto Run(TaskGraph& graph, C&& callable) -> Future<void> {
    return Future<void>(executor_.run(graph.UnderlyingTaskflow(), std::forward<C>(callable)));
  }

  /**
   * @brief Runs a moved task graph once and invokes a callback upon completion.
   * @details The executor takes ownership of the moved graph.
   * @note Thread-safe.
   * @tparam C Callable type
   * @param graph Task graph to execute (moved)
   * @param callable Callback to invoke after execution completes
   * @return Future that completes when execution finishes
   */
  template <std::invocable C>
  auto Run(TaskGraph&& graph, C&& callable) -> Future<void> {
    return Future<void>(executor_.run(std::move(std::move(graph).UnderlyingTaskflow()), std::forward<C>(callable)));
  }

  /**
   * @brief Runs a task graph for the specified number of times.
   * @details The executor does not own the graph - ensure it remains alive during execution.
   * @note Thread-safe.
   * @param graph Task graph to execute
   * @param count Number of times to run the graph
   * @return Future that completes when all executions finish
   */
  auto RunN(TaskGraph& graph, size_t count) -> Future<void> {
    return Future<void>(executor_.run_n(graph.UnderlyingTaskflow(), count));
  }

  /**
   * @brief Runs a moved task graph for the specified number of times.
   * @details The executor takes ownership of the moved graph.
   * @note Thread-safe.
   * @param graph Task graph to execute (moved)
   * @param count Number of times to run the graph
   * @return Future that completes when all executions finish
   */
  auto RunN(TaskGraph&& graph, size_t count) -> Future<void> {
    return Future<void>(executor_.run_n(std::move(std::move(graph).UnderlyingTaskflow()), count));
  }

  /**
   * @brief Runs a task graph for the specified number of times and invokes a callback.
   * @details The executor does not own the graph - ensure it remains alive during execution.
   * @note Thread-safe.
   * @tparam C Callable type
   * @param graph Task graph to execute
   * @param count Number of times to run the graph
   * @param callable Callback to invoke after all executions complete
   * @return Future that completes when all executions finish
   */
  template <std::invocable C>
  auto RunN(TaskGraph& graph, size_t count, C&& callable) -> Future<void> {
    return Future<void>(executor_.run_n(graph.UnderlyingTaskflow(), count, std::forward<C>(callable)));
  }

  /**
   * @brief Runs a moved task graph for the specified number of times and invokes a callback.
   * @details The executor takes ownership of the moved graph.
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
        executor_.run_n(std::move(std::move(graph).UnderlyingTaskflow()), count, std::forward<C>(callable)));
  }

  /**
   * @brief Runs a task graph repeatedly until the predicate returns true.
   * @details The executor does not own the graph - ensure it remains alive during execution.
   * @note Thread-safe.
   * @tparam Predicate Predicate type
   * @param graph Task graph to execute
   * @param predicate Boolean predicate to determine when to stop
   * @return Future that completes when predicate returns true
   */
  template <std::predicate Predicate>
  auto RunUntil(TaskGraph& graph, Predicate&& predicate) -> Future<void> {
    return Future<void>(executor_.run_until(graph.UnderlyingTaskflow(), std::forward<Predicate>(predicate)));
  }

  /**
   * @brief Runs a moved task graph repeatedly until the predicate returns true.
   * @details The executor takes ownership of the moved graph.
   * @note Thread-safe.
   * @tparam Predicate Predicate type
   * @param graph Task graph to execute (moved)
   * @param predicate Boolean predicate to determine when to stop
   * @return Future that completes when predicate returns true
   */
  template <std::predicate Predicate>
  auto RunUntil(TaskGraph&& graph, Predicate&& predicate) -> Future<void> {
    return Future<void>(
        executor_.run_until(std::move(std::move(graph).UnderlyingTaskflow()), std::forward<Predicate>(predicate)));
  }

  /**
   * @brief Runs a task graph repeatedly until the predicate returns true, then invokes a callback.
   * @details The executor does not own the graph - ensure it remains alive during execution.
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
    return Future<void>(
        executor_.run_until(graph.UnderlyingTaskflow(), std::forward<Predicate>(predicate), std::forward<C>(callable)));
  }

  /**
   * @brief Runs a moved task graph repeatedly until the predicate returns true, then invokes a callback.
   * @details The executor takes ownership of the moved graph.
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
    return Future<void>(executor_.run_until(std::move(std::move(graph).UnderlyingTaskflow()),
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
    return executor_.async(std::forward<C>(callable));
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
  auto Async(std::string name, C&& callable) -> std::future<std::invoke_result_t<C>>;

  /**
   * @brief Creates an asynchronous task without returning a future.
   * @details More efficient than Async when you don't need the result.
   * @note Thread-safe.
   * @tparam C Callable type
   * @param callable Function to execute asynchronously
   */
  template <std::invocable C>
  void SilentAsync(C&& callable) {
    executor_.silent_async(std::forward<C>(callable));
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
  void SilentAsync(std::string name, C&& callable);

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
  void WaitForAll() { executor_.wait_for_all(); }

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
  [[nodiscard]] int CurrentWorkerId() const { return executor_.this_worker_id(); }

  /**
   * @brief Gets the total number of worker threads.
   * @note Thread safe.
   * @return Count of worker threads
   */
  [[nodiscard]] size_t WorkerCount() const noexcept { return executor_.num_workers(); }

  /**
   * @brief Gets the number of worker threads currently waiting for work.
   * @note Thread safe.
   * @return Count of idle worker threads
   */
  [[nodiscard]] size_t IdleWorkerCount() const noexcept { return executor_.num_waiters(); }

  /**
   * @brief Gets the number of task queues in the work-stealing scheduler.
   * @note Thread safe.
   * @return Count of task queues
   */
  [[nodiscard]] size_t QueueCount() const noexcept { return executor_.num_queues(); }

  /**
   * @brief Gets the number of task graphs currently being executed.
   * @note Thread safe.
   * @return Count of running task graphs
   */
  [[nodiscard]] size_t RunningTopologyCount() const { return executor_.num_topologies(); }

private:
  tf::Executor executor_;
};

template <std::invocable C>
inline auto Executor::Async(std::string name, C&& callable) -> std::future<std::invoke_result_t<C>> {
  tf::TaskParams params;
  params.name = std::move(name);
  return executor_.async(params, std::forward<C>(callable));
}

template <std::invocable C>
inline void Executor::SilentAsync(std::string name, C&& callable) {
  tf::TaskParams params;
  params.name = std::move(name);
  executor_.silent_async(params, std::forward<C>(callable));
}

template <std::invocable C, std::ranges::range Dependencies>
  requires std::same_as<std::ranges::range_value_t<Dependencies>, AsyncTask>
inline auto Executor::DependentAsync(C&& callable, const Dependencies& dependencies)
    -> std::pair<AsyncTask, std::future<std::invoke_result_t<C>>> {
  std::vector<tf::AsyncTask> tf_deps;
  if constexpr (std::ranges::sized_range<Dependencies>) {
    tf_deps.reserve(std::ranges::size(dependencies));
  }

  for (const auto& dep : dependencies) {
    tf_deps.push_back(dep.UnderlyingTask());
  }

  auto [task, future] = executor_.dependent_async(std::forward<C>(callable), tf_deps.begin(), tf_deps.end());
  return std::make_pair(AsyncTask(std::move(task)), std::move(future));
}

template <std::invocable C, std::ranges::range Dependencies>
  requires std::same_as<std::ranges::range_value_t<Dependencies>, AsyncTask>
inline AsyncTask Executor::SilentDependentAsync(C&& callable, const Dependencies& dependencies) {
  std::vector<tf::AsyncTask> tf_deps;
  if constexpr (std::ranges::sized_range<Dependencies>) {
    tf_deps.reserve(std::ranges::size(dependencies));
  }

  for (const auto& dep : dependencies) {
    tf_deps.push_back(dep.UnderlyingTask());
  }

  return AsyncTask(executor_.silent_dependent_async(std::forward<C>(callable), tf_deps.begin(), tf_deps.end()));
}

inline void Executor::CoRun(TaskGraph& graph) {
  HELIOS_ASSERT(IsWorkerThread(), "Failed to co-run: Must be called from a worker thread");
  executor_.corun(graph.UnderlyingTaskflow());
}

template <std::predicate Predicate>
inline void Executor::CoRunUntil(Predicate&& predicate) {
  HELIOS_ASSERT(IsWorkerThread(), "Failed to co-run until: Must be called from a worker thread");
  executor_.corun_until(std::forward<Predicate>(predicate));
}

}  // namespace helios::async
