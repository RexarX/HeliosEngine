#pragma once

#include <helios/ecs/message/message.hpp>

#include <cstdint>
#include <string_view>

namespace helios::app {

/// @brief Application exit codes.
enum class ExitCode : uint8_t {
  kSuccess = 0,  ///< Successful execution
  kFailure = 1,  ///< General failure
};

/**
 * @brief Message that requests the application to exit.
 * @details Written by systems; `RunDefault` stops when this message is present.
 * Uses manual clear policy so it survives per-stage message merge / advance
 * within a frame.
 */
struct AppExit {
  static constexpr std::string_view kName = "helios::app::AppExit";
  static constexpr auto kClearPolicy = ecs::MessageClearPolicy::kManual;
  static constexpr bool kAsync = false;
  static constexpr bool kConsumable = false;

  ExitCode code = ExitCode::kSuccess;

  /**
   * @brief Creates an `AppExit` message from an `ExitCode`.
   * @param exit_code The exit code to use
   * @return An `AppExit` message with the specified exit code
   */
  [[nodiscard]] static constexpr AppExit From(ExitCode exit_code) noexcept {
    return {.code = exit_code};
  }

  /**
   * @brief Creates an `AppExit` message indicating success.
   * @return An `AppExit` message with a success exit code
   */
  [[nodiscard]] static constexpr AppExit Success() noexcept {
    return AppExit::From(ExitCode::kSuccess);
  }

  /**
   * @brief Creates an `AppExit` message indicating failure.
   * @return An `AppExit` message with a failure exit code
   */
  [[nodiscard]] static constexpr AppExit Failure() noexcept {
    return AppExit::From(ExitCode::kFailure);
  }
};

/**
 * @brief Converts an `ExitCode` to a string representation.
 * @param code The exit code to convert
 * @return A string view representing the exit code
 */
[[nodiscard]] constexpr std::string_view ToString(ExitCode code) noexcept {
  switch (code) {
    using enum ExitCode;
    case kSuccess:
      return "Success";
    case kFailure:
      return "Failure";
  }

  return "Unknown";
}

/**
 * @brief Converts an `AppExit` message to a string representation.
 * @param app_exit The `AppExit` message to convert
 * @return A string view representing the exit code
 */
[[nodiscard]] constexpr std::string_view ToString(AppExit app_exit) noexcept {
  return ToString(app_exit.code);
}

}  // namespace helios::app
