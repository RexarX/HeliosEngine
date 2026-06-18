#pragma once

#include <bit>

#include <taskflow/core/task.hpp>

#include <concepts>
#include <cstdint>
#include <expected>
#include <string_view>
#include <type_traits>

namespace helios::async {

class SubTaskGraph;

/// @brief Types of tasks in the async system.
enum class TaskType : uint8_t {
  kUndefined,
  kStatic,   ///< Static task with fixed callable
  kSubTask,  ///< Dynamic task that can spawn subtasks
  kAsync     ///< Asynchronous task executed independently
};

/// @brief Error codes for async operations.
enum class AsyncError : uint8_t {
  kInvalidTask,         ///< Task handle is invalid or empty
  kExecutorShutdown,    ///< Executor has been shut down
  kTaskNotFound,        ///< Specified task could not be found
  kInvalidDependency,   ///< Dependency relationship is invalid
  kCircularDependency,  ///< Circular dependency detected in task graph
  kSchedulingFailed,    ///< Task could not be scheduled for execution
  kThreadNotAvailable   ///< No worker thread available for execution
};

/**
 * @brief Converts `AsyncError` to string representation.
 * @param error AsyncError value to convert
 * @return String view describing the error
 */
[[nodiscard]] constexpr std::string_view ToString(AsyncError error) noexcept {
  switch (error) {
    case AsyncError::kInvalidTask:
      return "Invalid task";
    case AsyncError::kExecutorShutdown:
      return "Executor is shutdown";
    case AsyncError::kTaskNotFound:
      return "Task not found";
    case AsyncError::kInvalidDependency:
      return "Invalid dependency";
    case AsyncError::kCircularDependency:
      return "Circular dependency detected";
    case AsyncError::kSchedulingFailed:
      return "Task scheduling failed";
    case AsyncError::kThreadNotAvailable:
      return "Thread not available";
    default:
      return "Unknown async error";
  }
}

/// @brief Result type for async operations.
template <typename T = void>
using AsyncResult = std::expected<T, AsyncError>;

/// @brief Concept for static task callables.
template <typename C>
concept StaticTask =
    std::invocable<C> && std::same_as<std::invoke_result_t<C>, void>;

/// @brief Concept for sub-task callables that receive a SubTaskGraph reference.
template <typename C>
concept SubTask = std::invocable<C, SubTaskGraph&>;

/// @brief Concept for any valid task callable.
template <typename C>
concept AnyTask = StaticTask<C> || SubTask<C>;

namespace details {

/**
 * @brief Converts Taskflow task type to Helios task type.
 * @param type Taskflow task type
 * @return Corresponding Helios task type
 */
[[nodiscard]] static constexpr TaskType ConvertTaskType(
    tf::TaskType type) noexcept {
  switch (type) {
    case tf::TaskType::STATIC:
      return TaskType::kStatic;
    case tf::TaskType::SUBFLOW:
      return TaskType::kSubTask;
    case tf::TaskType::ASYNC:
      return TaskType::kAsync;
    default:
      return TaskType::kStatic;  // Fallback to Static if unknown
  }
}

}  // namespace details

}  // namespace helios::async
