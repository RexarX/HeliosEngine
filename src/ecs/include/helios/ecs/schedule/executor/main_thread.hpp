#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.ecs;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#include <helios/ecs/schedule/executor/executor.hpp>

HELIOS_MODULE_EXPORT
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
#endif  // HELIOS_MODULE_CONSUMER_SHIM
