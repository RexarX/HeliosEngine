#pragma once

namespace helios::ecs {

/// @brief Settings that control how a stage behaves when run.
struct StageSettings {
  /**
   * @brief When true, flush reserved entities and apply deferred commands after
   * the stage finishes.
   * @details Runs `World::Flush()` and per-schedule `ExecuteCommands` for stage
   * members (via `ApplyDeferred` with explicit flags). Independent of
   * `merge_messages` and `advance_messages`.
   */
  bool apply_commands = false;

  /**
   * @brief When true, merge local message writes and apply consumed registries
   * after the stage finishes.
   * @details Runs per-schedule `MergeMessages` for stage members. Does **not**
   * swap previous/current message buffers — use `advance_messages` for that.
   */
  bool merge_messages = false;

  /**
   * @brief When true, call `MessageManager::Update()` after the stage finishes.
   * @details Swaps previous/current message queues. Does **not** call
   * `World::Update()` (which also flushes). Independent of `apply_commands` and
   * `merge_messages`.
   */
  bool advance_messages = false;
};

}  // namespace helios::ecs
