#pragma once

#include <helios/app/application.hpp>
#include <helios/app/plugin.hpp>
#include <helios/async/executor.hpp>

#include <functional>
#include <string_view>

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

  void Build(App& app) override {
    app.TryInsertResources(Executor{app.GetExecutor()});
  }
};

}  // namespace helios::app
