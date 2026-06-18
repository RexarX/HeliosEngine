#pragma once

#include <helios/async/executor.hpp>
#include <helios/async/future.hpp>
#include <helios/async/task.hpp>
#include <helios/async/task_graph.hpp>
#include <helios/ecs/schedule/executor/executor.hpp>

#include <functional>
#include <optional>
#include <vector>

namespace helios::ecs {

/**
 * @brief Multi-threaded executor that runs systems in parallel using
 * work-stealing.
 * @details Uses an injected `async::Executor` reference to run a
 * `async::TaskGraph` that respects ordering constraints and access-policy
 * conflicts. The `async::TaskGraph` and task vector are cached across frames
 * and cleared/rebuilt each execution.
 */
class MultiThreadedExecutor final : public Executor {
public:
  /**
   * @brief Constructs a multi-threaded executor with an external async
   * executor.
   * @param executor Reference to the async executor for thread pool management
   */
  explicit MultiThreadedExecutor(async::Executor& executor) noexcept
      : executor_(executor), task_graph_("MultiThreadedExecutor") {}

  MultiThreadedExecutor(const MultiThreadedExecutor&) = delete;
  MultiThreadedExecutor(MultiThreadedExecutor&&) noexcept = default;
  ~MultiThreadedExecutor() override = default;

  MultiThreadedExecutor& operator=(const MultiThreadedExecutor&) = delete;
  MultiThreadedExecutor& operator=(MultiThreadedExecutor&&) noexcept = default;

  /**
   * @brief Executes the schedule using the multi-threaded executor.
   * @param schedule The schedule to execute
   * @param world The world context for system execution
   */
  void Execute(Schedule& schedule, World& world) override;

  void ExecuteAndWait(Schedule& schedule, World& world) override;

  /**
   * @brief Waits for the most recent Execute() to complete.
   * @details Blocks until the submitted task graph finishes.
   */
  void Wait() override;

private:
  void BuildGraph(Schedule& schedule, World& world);

  std::reference_wrapper<async::Executor> executor_;
  async::TaskGraph task_graph_;
  std::vector<async::Task> tasks_;
  std::optional<async::Future<void>> future_;
};

inline void MultiThreadedExecutor::Wait() {
  if (!future_.has_value()) [[unlikely]] {
    return;
  }
  future_->Wait();
  future_.reset();
}

}  // namespace helios::ecs
