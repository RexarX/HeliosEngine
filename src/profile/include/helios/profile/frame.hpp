#pragma once

#include <helios/cstring_view.hpp>
#include <helios/profile/profiler.hpp>

namespace helios::profile {

/// @brief Marks the end of the current frame on all active backends.
inline void FrameMark() noexcept {
  Profiler::Instance().FrameMark();
}

/**
 * @brief Marks the end of a named frame.
 * @param name The name of the frame to mark
 */
inline void FrameMarkNamed(CStringView name) noexcept {
  Profiler::Instance().FrameMark(name);
}

/**
 * @brief Marks the start of a named frame region.
 * @param name The name of the frame region to mark
 */
inline void FrameMarkStart(CStringView name) noexcept {
  Profiler::Instance().FrameMarkStart(name);
}

/**
 * @brief Marks the end of a named frame region.
 * @param name The name of the frame region to mark
 */
inline void FrameMarkEnd(CStringView name) noexcept {
  Profiler::Instance().FrameMarkEnd(name);
}

}  // namespace helios::profile
