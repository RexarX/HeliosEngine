#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/async/common.hpp>

#include <taskflow/core/async_task.hpp>

#include <cstddef>
#include <utility>

namespace helios::async {

/**
 * @brief Handle to an asynchronous task managed by the Executor.
 * @details Wraps a tf::AsyncTask, providing methods to query task state and manage its lifecycle.
 * Instances are typically returned by Executor's asynchronous methods.
 * @note Thread-safe.
 */
class AsyncTask {
public:
  AsyncTask() = default;
  AsyncTask(const AsyncTask&) = default;
  AsyncTask(AsyncTask&&) noexcept = default;
  ~AsyncTask() = default;

  AsyncTask& operator=(const AsyncTask&) = default;
  AsyncTask& operator=(AsyncTask&&) noexcept = default;

  /**
   * @brief Resets the underlying asynchronous task handle.
   */
  void Reset() { task_.reset(); }

  bool operator==(const AsyncTask& other) const noexcept { return task_.hash_value() == other.task_.hash_value(); }
  bool operator!=(const AsyncTask& other) const noexcept { return !(*this == other); }

  /**
   * @brief Checks if the asynchronous task has completed.
   * @return True if the task is done, false otherwise.
   * @details Returns false if task is empty.
   */
  [[nodiscard]] bool Done() const;

  /**
   * @brief Checks if the task handle is empty (not associated with any task).
   * @return True if empty, false otherwise.
   */
  [[nodiscard]] bool Empty() const { return task_.empty(); }

  /**
   * @brief Returns a hash value for the task.
   * @return Hash value.
   * @details Returns '0' if task is empty.
   */
  [[nodiscard]] size_t Hash() const;

  /**
   * @brief Returns the number of references to the underlying task.
   * @return Reference count.
   * @details Returns '0' if task is empty.
   */
  [[nodiscard]] size_t UseCount() const { return task_.use_count(); }

  /**
   * @brief Gets the type of the task.
   * @return TaskType::Async
   */
  [[nodiscard]] static constexpr TaskType GetTaskType() noexcept { return TaskType::Async; }

private:
  explicit AsyncTask(tf::AsyncTask task) : task_(std::move(task)) {}

  [[nodiscard]] tf::AsyncTask& UnderlyingTask() noexcept { return task_; }
  [[nodiscard]] const tf::AsyncTask& UnderlyingTask() const noexcept { return task_; }

  tf::AsyncTask task_;

  friend class Executor;
  friend class SubTaskGraph;
};

inline bool AsyncTask::Done() const {
  if (Empty()) [[unlikely]] {
    return false;
  }

  return task_.is_done();
}

inline size_t AsyncTask::Hash() const {
  if (Empty()) [[unlikely]] {
    return 0;
  }

  return task_.hash_value();
}

}  // namespace helios::async
