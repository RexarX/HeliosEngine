#pragma once

#include <helios/ecs/schedule/executor/executor.hpp>

namespace helios::ecs {

/// @brief Main-thread executor that runs systems sequentially on the calling
/// thread.
class MainThreadExecutor final : public Executor {
public:
  void Execute(Schedule& schedule, World& world) override;

  /**
   * @brief Executes and blocks; for the main thread this is identical to
   * Execute.
   */
  void ExecuteAndWait(Schedule& schedule, World& world) override {
    Execute(schedule, world);
  }

  /// @brief No-op: main-thread execution is already synchronous.
  void Wait() override {}
};

}  // namespace helios::ecs
