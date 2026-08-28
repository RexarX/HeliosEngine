#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.app;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <functional>
#include <string_view>
#endif
#include <helios/app/plugin.hpp>

HELIOS_MODULE_EXPORT
namespace helios::async {

class Executor;

}

HELIOS_MODULE_EXPORT
namespace helios::app {

/// @brief Resource wrapper for the application async executor.
struct Executor {
  static constexpr std::string_view kName = "helios::app::Executor";
  static constexpr bool kThreadSafe = true;

  std::reference_wrapper<async::Executor> executor;

  /**
   * @brief Dereferences the wrapped async executor.
   * @return Wrapped async executor
   */
  [[nodiscard]] async::Executor& operator*() const noexcept {
    return executor.get();
  }

  /**
   * @brief Accesses the wrapped async executor.
   * @return Pointer to the wrapped async executor
   */
  [[nodiscard]] async::Executor* operator->() const noexcept {
    return &executor.get();
  }
};

/// @brief Adds the `Executor` resource.
class ExecutorPlugin final : public Plugin {
public:
  static constexpr std::string_view kName = "helios::app::ExecutorPlugin";

  void Build(App& app) override;
};

}  // namespace helios::app
#endif  // HELIOS_MODULE_CONSUMER_SHIM
