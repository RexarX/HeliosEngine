#pragma once

#include <helios/assert.hpp>

#include <algorithm>
#include <cstddef>
#include <vector>

namespace helios::ecs {

inline thread_local std::vector<size_t> g_active_schedule_stack;

/**
 * @brief RAII guard that records an active schedule on the current thread.
 * @details Used by `Scheduler` to detect nested `RunStage` calls and skip
 * schedules that are already executing when requested.
 */
class ScheduleRunScope {
public:
  explicit ScheduleRunScope(size_t schedule_hash) noexcept;
  ScheduleRunScope(const ScheduleRunScope&) = delete;
  ScheduleRunScope(ScheduleRunScope&&) = delete;
  ~ScheduleRunScope();

  ScheduleRunScope& operator=(const ScheduleRunScope&) = delete;
  ScheduleRunScope& operator=(ScheduleRunScope&&) = delete;

  /**
   * @brief Returns whether `schedule_hash` is currently executing on this
   * thread.
   * @param schedule_hash Schedule type hash
   */
  [[nodiscard]] static bool IsActive(size_t schedule_hash) noexcept;

private:
  size_t schedule_hash_ = 0;
};

inline ScheduleRunScope::ScheduleRunScope(size_t schedule_hash) noexcept
    : schedule_hash_(schedule_hash) {
  g_active_schedule_stack.push_back(schedule_hash);
}

inline ScheduleRunScope::~ScheduleRunScope() {
  HELIOS_ASSERT(!g_active_schedule_stack.empty() &&
                    g_active_schedule_stack.back() == schedule_hash_,
                "ScheduleRunScope stack mismatch!");
  g_active_schedule_stack.pop_back();
}

inline bool ScheduleRunScope::IsActive(size_t schedule_hash) noexcept {
  return std::ranges::any_of(g_active_schedule_stack,
                             [schedule_hash](size_t active_hash) {
                               return active_hash == schedule_hash;
                             });
}

}  // namespace helios::ecs
