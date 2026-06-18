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
 * @brief Single-threaded executor that runs systems sequentially on a single
 * thread.
 * @details Uses an `async::Executor` with a `async::TaskGraph` where all tasks
 * are linearized (each depends on the previous), guaranteeing sequential
 * execution while still using the thread pool infrastructure.
 */
class SingleThreadedExecutor final : public Executor {
public:
  /**
   * @brief Constructs a single-threaded executor with an external async
   * executor.
   * @param executor Reference to the async executor for task execution
   */
  explicit SingleThreadedExecutor(async::Executor& executor) noexcept
      : executor_(executor), task_graph_("SingleThreadedExecutor") {}

  SingleThreadedExecutor(const SingleThreadedExecutor&) = delete;
  SingleThreadedExecutor(SingleThreadedExecutor&&) noexcept = default;
  ~SingleThreadedExecutor() override = default;

  SingleThreadedExecutor& operator=(const SingleThreadedExecutor&) = delete;
  SingleThreadedExecutor& operator=(SingleThreadedExecutor&&) noexcept =
      default;

  /**
   * @brief Executes the schedule using the single-threaded executor.
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

inline void SingleThreadedExecutor::Wait() {
  if (!future_.has_value()) [[unlikely]] {
    return;
  }
  future_->Wait();
  future_.reset();
}

}  // namespace helios::ecs
