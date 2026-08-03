#pragma once

#include <helios/app/schedules.hpp>
#include <helios/app/sub_app.hpp>
#include <helios/async/executor.hpp>
#include <helios/async/future.hpp>
#include <helios/async/task_graph.hpp>

#include <atomic>
#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

namespace helios::app {

class App;

/**
 * @brief Orchestrates the main sub-app schedule loop and sub-app updates.
 * @details Each frame runs the main sub-app @ref kUpdateStage, then @ref
 * kExtractStage, then sub-app updates. Blocking sub-apps are joined each frame.
 * Overlapping sub-apps may skip extraction while an update is in flight, up to
 * `kMaxOverlappingUpdates` consecutive frames (`0` = unlimited). Async sub-apps
 * run updates on a background loop; extraction runs every main frame.
 */
class Scheduler {
public:
  Scheduler() = default;
  Scheduler(const Scheduler&) = delete;
  Scheduler(Scheduler&& other) noexcept;
  ~Scheduler() = default;

  Scheduler& operator=(const Scheduler&) = delete;
  Scheduler& operator=(Scheduler&& other) noexcept;

  /**
   * @brief Clears cached task graphs and pending sub-app update state.
   * @details Asserts that no sub-app updates or async loops are in flight. Call
   * @ref Stop() or @ref Shutdown() first if work may still be running.
   */
  void Clear();

  /**
   * @brief Stops async update loops and waits for in-flight sub-app work.
   * @param executor Executor that runs all current async tasks
   */
  void Stop(async::Executor& executor);

  /**
   * @brief Builds schedulers and precomputes sub-app task graphs.
   * @param app Owning application
   */
  void Build(App& app);

  /**
   * @brief Runs startup on the main sub-app, then startup on all sub-apps in
   * parallel, then starts async sub-app update loops.
   * @param app Owning application
   */
  void RunStartup(App& app);

  /**
   * @brief Runs one full application frame.
   * @param app Owning application
   */
  void RunFrame(App& app);

  /**
   * @brief Stops async loops, waits for in-flight updates, shuts down
   * sub-apps, then the main sub-app.
   * @param app Owning application
   */
  void Shutdown(App& app);

  /**
   * @brief Requests async sub-app loops to exit and waits until they finish.
   * @param app Owning application
   */
  void StopAsyncLoops(App& app);

  /// @brief Waits until blocking sub-apps finish their frame update.
  void WaitForSubApps();

private:
  enum class SubAppMode : uint8_t {
    kBlocking = 0,
    kOverlapping = 1,
    kAsync = 2,
  };

  struct SubAppFrameState {
    std::reference_wrapper<SubApp> sub_app;
    SubAppMode mode = SubAppMode::kBlocking;
    size_t consecutive_extract_skips = 0;
    bool fresh_extract_this_frame = false;
  };

  static void RunMainStartup(SubApp& main, async::Executor& executor);
  static void RunUpdateStage(SubApp& main, async::Executor& executor);
  void RunExtractStage(SubApp& main, async::Executor& executor);
  static void RunMainShutdown(SubApp& main, async::Executor& executor);

  void LaunchSubAppUpdates(App& app);
  void StartAsyncUpdateLoops(App& app);
  void StopAsyncUpdateLoops(async::Executor& executor);

  static void ExtractSubApp(SubAppFrameState& state,
                            const ecs::World& main_world);

  [[nodiscard]] static SubAppMode ClassifySubApp(
      const SubApp& sub_app) noexcept;

  async::TaskGraph startup_graph_{"SubAppStartup"};
  async::TaskGraph blocking_update_graph_{"SubAppBlockingUpdate"};
  async::TaskGraph shutdown_graph_{"SubAppShutdown"};
  std::vector<SubAppFrameState> sub_app_states_;
  std::optional<async::Future<void>> blocking_update_future_;
  std::atomic<size_t> async_loops_running_{0};
};

inline Scheduler::Scheduler(Scheduler&& other) noexcept
    : startup_graph_(std::move(other.startup_graph_)),
      blocking_update_graph_(std::move(other.blocking_update_graph_)),
      shutdown_graph_(std::move(other.shutdown_graph_)),
      sub_app_states_(std::move(other.sub_app_states_)),
      blocking_update_future_(std::move(other.blocking_update_future_)),
      async_loops_running_(
          other.async_loops_running_.load(std::memory_order_relaxed)) {}

inline Scheduler& Scheduler::operator=(Scheduler&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  startup_graph_ = std::move(other.startup_graph_);
  blocking_update_graph_ = std::move(other.blocking_update_graph_);
  shutdown_graph_ = std::move(other.shutdown_graph_);
  sub_app_states_ = std::move(other.sub_app_states_);
  blocking_update_future_ = std::move(other.blocking_update_future_);
  async_loops_running_.store(
      other.async_loops_running_.load(std::memory_order_relaxed),
      std::memory_order_release);

  return *this;
}

inline void Scheduler::Stop(async::Executor& executor) {
  StopAsyncUpdateLoops(executor);
  WaitForSubApps();
}

}  // namespace helios::app
