#pragma once

#include <helios/app/schedules.hpp>
#include <helios/async/future.hpp>
#include <helios/async/task_graph.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <optional>
#include <vector>

namespace helios::async {

class Executor;

}

namespace helios::app {

class App;
class SubApp;
class FrameOrder;

/**
 * @brief Orchestrates the main sub-app schedule loop and sub-app updates.
 * @details Each frame walks `MainFrameOrder` (default: `kUpdateStage`, then
 * `kExtractStage`). Nested pumps walk `FramePumpOrder`. Blocking sub-apps are
 * joined when extract runs. Overlapping sub-apps may skip extraction while an
 * update is in flight, up to `kMaxOverlappingUpdates` consecutive frames
 * (`0` = unlimited). Async sub-apps run updates on a background loop;
 * extraction runs every main frame that includes extract.
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
   * @details Joins only scheduler-owned async loops and overlapping updates,
   * then blocking frame updates. Does not drain unrelated executor tasks.
   * @param executor Executor that runs scheduler-owned tasks (unused for the
   * global drain; kept for API consistency with earlier versions)
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
   * @brief Runs one full application frame using `MainFrameOrder`.
   * @param app Owning application
   */
  void RunFrame(App& app);

  /**
   * @brief Runs the given ordered stage list on the main sub-app.
   * @details For each stage: `RunStage`, then optional stage
   * `apply_commands` / `merge_messages` via `ApplyStageDeferred`.
   * `MessageManager::Update()` runs only after the last present stage in the
   * order when that stage has `advance_messages` (so MainFrameOrder advances on
   * Extract while FramePumpOrder advances on Update without double-swapping).
   * When `kExtractStage` appears, extracts into sub-apps; after the full order,
   * launches and waits for sub-app updates if extract ran.
   * @param app Owning application
   * @param order Ordered stages to execute
   */
  void RunFrameOrder(App& app, const FrameOrder& order);

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
  static void RunMainShutdown(SubApp& main, async::Executor& executor);

  void LaunchSubAppUpdates(App& app);
  void StartAsyncUpdateLoops(App& app);
  void StopAsyncUpdateLoops();
  void WaitForOverlappingUpdates();
  void PruneCompletedOverlappingUpdates();

  static void ExtractSubApp(SubAppFrameState& state,
                            const ecs::World& main_world);

  [[nodiscard]] static SubAppMode ClassifySubApp(
      const SubApp& sub_app) noexcept;

  async::TaskGraph startup_graph_{"SubAppStartup"};
  async::TaskGraph blocking_update_graph_{"SubAppBlockingUpdate"};
  async::TaskGraph shutdown_graph_{"SubAppShutdown"};
  std::vector<SubAppFrameState> sub_app_states_;
  std::optional<async::Future<void>> blocking_update_future_;
  std::vector<std::future<void>> async_loop_futures_;
  std::vector<std::future<void>> overlapping_update_futures_;
  std::atomic<size_t> async_loops_running_{0};
};

}  // namespace helios::app
