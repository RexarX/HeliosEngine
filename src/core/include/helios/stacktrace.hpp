#pragma once

#include <cstddef>
#include <cstdint>
#include <source_location>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace helios {

/// @brief Configuration for stacktrace capture and formatting.
struct StacktraceConfig {
  /// Maximum number of frames to emit after filtering.
  size_t max_frames = 10;

  /// Default frame index to start from when source matching is disabled or
  /// fails.
  size_t start_frame = 1;

  /// Additional number of frames to skip from the computed start.
  size_t skip_frames = 0;

  /**
   * @brief Maximum number of frames to scan from start frame.
   * @details `0` means no explicit depth limit.
   */
  size_t max_depth = 0;

  /// @brief Source file used to infer a relevant starting frame.
  std::string_view source_file;

  /// @brief Source line used to infer a relevant starting frame.
  uint_least32_t source_line = 0;

  /// @brief Source function used to infer a relevant starting frame.
  std::string_view source_function;

  /**
   * @brief Marker to stop capture before frames that contain this token.
   * @details Typical values are `"__libc_start_main"`, `"_start"` or `"main"`.
   */
  std::string_view stop_before = "__libc_start_main";

  /**
   * @brief Keep only frames that contain this token.
   * @details Empty token keeps all frames.
   */
  std::string_view include_frames_containing;

  /**
   * @brief Drop frames that contain this token.
   * @details Empty token drops no frames.
   */
  std::string_view exclude_frames_containing;

  [[nodiscard]] static constexpr StacktraceConfig FromSourceLocation(
      const std::source_location& loc) noexcept {
    StacktraceConfig config;
    config.source_file = loc.file_name();
    config.source_line = loc.line();
    config.source_function = loc.function_name();
    return config;
  }

  [[nodiscard]] static constexpr StacktraceConfig FromSource(
      std::string_view file, uint_least32_t line) noexcept {
    StacktraceConfig config;
    config.source_file = file;
    config.source_line = line;
    return config;
  }
};

/// @brief Captured and filtered stacktrace with string conversion support.
class Stacktrace {
public:
  constexpr Stacktrace() = default;
  constexpr Stacktrace(const Stacktrace&) = default;
  constexpr Stacktrace(Stacktrace&&) noexcept = default;
  constexpr ~Stacktrace() noexcept = default;

  constexpr Stacktrace& operator=(const Stacktrace&) = default;
  constexpr Stacktrace& operator=(Stacktrace&&) noexcept = default;

  /**
   * @brief Captures stacktrace using provided config.
   * @param config Capture and formatting configuration
   * @return Captured stacktrace object
   */
  [[nodiscard]] static Stacktrace Capture(const StacktraceConfig& config = {});

  /**
   * @brief Converts stacktrace to a formatted string.
   * @param include_header Whether to include `Stack trace:` header
   * @return Formatted stacktrace string
   */
  [[nodiscard]] std::string ToString(bool include_header = true) const;

  /**
   * @brief Checks if no frames were captured.
   * @return True if no frames were captured, false otherwise
   */
  [[nodiscard]] constexpr bool Empty() const noexcept {
    return frames_.empty();
  }

  /**
   * @brief Returns captured frames.
   * @return Read-only span of captured frames
   */
  [[nodiscard]] constexpr auto Frames() const noexcept
      -> std::span<const std::string> {
    return frames_;
  }

  /**
   * @brief Returns number of captured frames.
   * @return Number of captured frames
   */
  [[nodiscard]] constexpr size_t Size() const noexcept {
    return frames_.size();
  }

private:
  constexpr Stacktrace(std::vector<std::string> frames,
                       bool capture_error) noexcept
      : frames_(std::move(frames)), capture_error_(capture_error) {}

  std::vector<std::string> frames_;
  bool capture_error_ = false;
};

}  // namespace helios
