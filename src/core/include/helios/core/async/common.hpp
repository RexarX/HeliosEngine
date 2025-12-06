#pragma once

#include <helios/core_pch.hpp>

#include <taskflow/core/task.hpp>

#include <concepts>
#include <cstdint>
#include <expected>
#include <string_view>
#include <type_traits>

namespace helios::async {

class SubTaskGraph;

/**
 * @brief Types of tasks in the async system.
 */
enum class TaskType : uint8_t {
  Undefined,
  Static,   ///< Static task with fixed callable
  SubTask,  ///< Dynamic task that can spawn subtasks
  Async     ///< Asynchronous task executed independently
};

/**
 * @brief Error codes for async operations.
 */
enum class AsyncError : uint8_t {
  InvalidTask,         ///< Task handle is invalid or empty
  ExecutorShutdown,    ///< Executor has been shut down
  TaskNotFound,        ///< Specified task could not be found
  InvalidDependency,   ///< Dependency relationship is invalid
  CircularDependency,  ///< Circular dependency detected in task graph
  SchedulingFailed,    ///< Task could not be scheduled for execution
  ThreadNotAvailable   ///< No worker thread available for execution
};

/**
 * @brief Converts AsyncError to string representation.
 */
[[nodiscard]] constexpr std::string_view ToString(AsyncError error) noexcept {
  switch (error) {
    case AsyncError::InvalidTask:
      return "Invalid task";
    case AsyncError::ExecutorShutdown:
      return "Executor is shutdown";
    case AsyncError::TaskNotFound:
      return "Task not found";
    case AsyncError::InvalidDependency:
      return "Invalid dependency";
    case AsyncError::CircularDependency:
      return "Circular dependency detected";
    case AsyncError::SchedulingFailed:
      return "Task scheduling failed";
    case AsyncError::ThreadNotAvailable:
      return "Thread not available";
    default:
      return "Unknown async error";
  }
}

/**
 * @brief Result type for async operations.
 */
template <typename T = void>
using AsyncResult = std::expected<T, AsyncError>;

/**
 * @brief Concept for static task callables.
 */
template <typename C>
concept StaticTask = std::invocable<C> && std::same_as<std::invoke_result_t<C>, void>;

/**
 * @brief Concept for sub-task callables that receive a SubTaskGraph reference.
 */
template <typename C>
concept SubTask = std::invocable<C, SubTaskGraph&>;

/**
 * @brief Concept for any valid task callable.
 */
template <typename C>
concept AnyTask = StaticTask<C> || SubTask<C>;

namespace details {

/**
 * @brief Converts Taskflow task type to Helios task type.
 * @param type Taskflow task type
 * @return Corresponding Helios task type
 */
[[nodiscard]] static constexpr TaskType ConvertTaskType(tf::TaskType type) noexcept {
  switch (type) {
    case tf::TaskType::STATIC:
      return TaskType::Static;
    case tf::TaskType::SUBFLOW:
      return TaskType::SubTask;
    case tf::TaskType::ASYNC:
      return TaskType::Async;
    default:
      return TaskType::Static;  // Fallback to Static if unknown
  }
}

}  // namespace details

}  // namespace helios::async
