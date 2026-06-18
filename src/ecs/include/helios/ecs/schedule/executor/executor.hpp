#pragma once

#include <cstdint>

namespace helios::ecs {

class Schedule;
class World;

/// @brief Kind of executor to use for schedule execution.
enum class ExecutorKind : uint8_t {
  /// Systems run sequentially on the calling (main) thread.
  /// Required for APIs with main-thread affinity (rendering, platform).
  kMainThread,
  /// Systems run sequentially on a single thread.
  kSingleThreaded,
  /// Systems run in parallel using work-stealing.
  kMultiThreaded,
};

/**
 * @brief Abstract interface for schedule executors.
 * @details Receives a compiled schedule and world, running all systems
 * according to the execution plan. Implementations determine threading
 * strategy.
 */
class Executor {
public:
  virtual ~Executor() = default;

  /**
   * @brief Submits all systems in the schedule for execution.
   * @details May return before execution completes. Call Wait() to block until
   * completion.
   * @param schedule Compiled schedule to execute
   * @param world World to pass to each system
   */
  virtual void Execute(Schedule& schedule, World& world) = 0;

  /**
   * @brief Executes all systems in the schedule and blocks until completion.
   * @details Default implementation calls Execute() then Wait().
   * @param schedule Compiled schedule to execute
   * @param world World to pass to each system
   */
  virtual void ExecuteAndWait(Schedule& schedule, World& world);

  /**
   * @brief Blocks until the most recent Execute() completes.
   * @details Does nothing if no execution is pending.
   */
  virtual void Wait() {}
};

inline void Executor::ExecuteAndWait(Schedule& schedule, World& world) {
  Execute(schedule, world);
  Wait();
}

}  // namespace helios::ecs
