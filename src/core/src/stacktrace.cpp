#include <pch.hpp>

#include <helios/stacktrace.hpp>
#include <helios/utils/filesystem.hpp>

#ifdef HELIOS_USE_STL_STACKTRACE
#include <stacktrace>
#else
#include <boost/stacktrace.hpp>
#endif

#include <algorithm>
#include <cctype>
#include <format>
#include <iterator>
#include <string>
#include <vector>

namespace {

constexpr size_t kEntryReserveSize = 512;

struct CapturedFramesResult {
  std::vector<std::string> frames;
  bool capture_error = false;
};

[[nodiscard]] constexpr bool IsValidPrecedingChar(char ch) noexcept {
  return ch == ' ' || ch == '\t' || ch == '/' || ch == '\\' || ch == ':';
}

[[nodiscard]] constexpr bool IsValidFollowingChar(char ch) noexcept {
  return ch == ' ' || ch == '\t' || ch == ')' || ch == '\n' || ch == '\r';
}

[[nodiscard]] bool IsExactSourceMatch(const std::string& stacktrace_entry,
                                      const std::string& target_pattern) {
  size_t pos = stacktrace_entry.find(target_pattern);
  if (pos == std::string::npos) [[likely]] {
    return false;
  }

  while (pos != std::string::npos) {
    const bool valid_start =
        (pos == 0) || IsValidPrecedingChar(stacktrace_entry[pos - 1]);
    const size_t end_pos = pos + target_pattern.length();
    const bool valid_end = (end_pos == stacktrace_entry.length()) ||
                           IsValidFollowingChar(stacktrace_entry[end_pos]);

    if (valid_start && valid_end) [[unlikely]] {
      return true;
    }

    pos = stacktrace_entry.find(target_pattern, pos + 1);
  }

  return false;
}

[[nodiscard]] std::string NormalizePathSeparators(std::string_view path) {
  std::string normalized(path);
  std::ranges::replace(normalized, '\\', '/');
  return normalized;
}

[[nodiscard]] bool ContainsLineNumberToken(const std::string& stacktrace_entry,
                                           uint_least32_t line) {
  if (line == 0) {
    return true;
  }

  const std::string line_token = std::to_string(line);
  size_t pos = stacktrace_entry.find(line_token);
  while (pos != std::string::npos) {
    const bool valid_start =
        (pos == 0) ||
        !std::isdigit(static_cast<unsigned char>(stacktrace_entry[pos - 1]));
    const size_t end_pos = pos + line_token.size();
    const bool valid_end =
        (end_pos == stacktrace_entry.size()) ||
        !std::isdigit(static_cast<unsigned char>(stacktrace_entry[end_pos]));

    if (valid_start && valid_end) {
      return true;
    }

    pos = stacktrace_entry.find(line_token, pos + 1);
  }

  return false;
}

[[nodiscard]] size_t ClampStartFrame(size_t start_frame,
                                     size_t stack_size) noexcept {
  if (stack_size == 0) [[unlikely]] {
    return 0;
  }
  return std::min(start_frame, stack_size - 1);
}

template <typename Trace>
[[nodiscard]] std::string ToEntryString(const Trace& stack_trace,
                                        size_t index) {
#ifdef HELIOS_USE_STL_STACKTRACE
  return std::to_string(stack_trace[index]);
#else
  const auto& frame = stack_trace[index];

  std::string function_name;
  std::string source_file;
  size_t source_line = 0;

  try {
    function_name = frame.name();
  } catch (...) {
  }

  try {
    source_file = frame.source_file();
    source_line = frame.source_line();
  } catch (...) {
  }

  if (!function_name.empty() && !source_file.empty() && source_line != 0) {
    return std::format("{} at {}:{}", function_name, source_file, source_line);
  }

  if (!function_name.empty() && !source_file.empty()) {
    return std::format("{} in {}", function_name, source_file);
  }

  if (!function_name.empty()) {
    return function_name;
  }

  return boost::stacktrace::to_string(frame);
#endif
}

template <typename Trace>
[[nodiscard]] size_t FindStartingFrame(const Trace& stack_trace,
                                       const helios::StacktraceConfig& config) {
  size_t start_frame = ClampStartFrame(config.start_frame, stack_trace.size());

  const bool has_source_file = !config.source_file.empty();
  const bool has_source_function = !config.source_function.empty();
  if (!has_source_file && !has_source_function) [[likely]] {
    return start_frame;
  }

  std::string entry_str;
  std::string normalized_source_file;
  std::string source_file_name;
  size_t first_file_only_match = stack_trace.size();

  try {
    if (has_source_file) {
      normalized_source_file = NormalizePathSeparators(config.source_file);
      source_file_name =
          std::string(helios::utils::GetFileName(config.source_file));
    }
    entry_str.reserve(kEntryReserveSize);
  } catch (...) {
    return start_frame;
  }

  for (size_t index = start_frame; index < stack_trace.size(); ++index) {
    try {
      entry_str.clear();
      entry_str = ToEntryString(stack_trace, index);

      if (has_source_function && entry_str.contains(config.source_function))
          [[unlikely]] {
        return index;
      }

      if (!has_source_file) {
        continue;
      }

      const std::string normalized_entry = NormalizePathSeparators(entry_str);
      const bool matches_full_path =
          !normalized_source_file.empty() &&
          normalized_entry.contains(normalized_source_file);
      const bool matches_file_name =
          !source_file_name.empty() &&
          IsExactSourceMatch(normalized_entry, source_file_name);
      const bool matches_file = matches_full_path || matches_file_name;
      if (!matches_file) {
        continue;
      }

      if (ContainsLineNumberToken(entry_str, config.source_line)) [[unlikely]] {
        return index;
      }

      if (first_file_only_match == stack_trace.size()) {
        first_file_only_match = index;
      }
    } catch (...) {
      continue;
    }
  }

  if (first_file_only_match != stack_trace.size()) {
    return first_file_only_match;
  }

  return start_frame;
}

template <typename Trace>
[[nodiscard]] size_t ComputeEndFrame(const Trace& stack_trace,
                                     const helios::StacktraceConfig& config,
                                     size_t start_frame) {
  const size_t stack_size = stack_trace.size();
  size_t end_frame = stack_size;

  if (config.max_depth > 0) {
    end_frame = std::min(end_frame, start_frame + config.max_depth);
  }

  if (config.max_frames > 0) {
    end_frame = std::min(end_frame, start_frame + config.max_frames);
  }

  if (config.stop_before.empty()) [[likely]] {
    return end_frame;
  }

  for (size_t index = start_frame; index < end_frame; ++index) {
    try {
      const std::string entry_str = ToEntryString(stack_trace, index);
      if (entry_str.contains(config.stop_before)) {
        return index;
      }
    } catch (...) {
      continue;
    }
  }

  return end_frame;
}

template <typename Trace>
[[nodiscard]] CapturedFramesResult CaptureFrames(
    const Trace& stack_trace, const helios::StacktraceConfig& config) {
  CapturedFramesResult result;
  if (stack_trace.size() <= 1 || config.max_frames == 0) {
    return result;
  }

  const size_t initial_start =
      FindStartingFrame(stack_trace, config) + config.skip_frames;
  const size_t start_frame = ClampStartFrame(initial_start, stack_trace.size());
  const size_t end_frame = ComputeEndFrame(stack_trace, config, start_frame);

  if (start_frame >= end_frame) {
    return result;
  }

  result.frames.reserve(end_frame - start_frame);

  for (size_t index = start_frame; index < end_frame; ++index) {
    try {
      const std::string entry = ToEntryString(stack_trace, index);
      if (!config.include_frames_containing.empty() &&
          !entry.contains(config.include_frames_containing)) {
        continue;
      }
      if (!config.exclude_frames_containing.empty() &&
          entry.contains(config.exclude_frames_containing)) {
        continue;
      }
      result.frames.push_back(entry);
    } catch (...) {
      result.capture_error = true;
    }
  }

  return result;
}

}  // namespace

namespace helios {

Stacktrace Stacktrace::Capture(const StacktraceConfig& config) {
  try {
#ifdef HELIOS_USE_STL_STACKTRACE
    const std::stacktrace stack_trace = std::stacktrace::current();
#else
    const boost::stacktrace::stacktrace stack_trace;
#endif

    auto capture_result = CaptureFrames(stack_trace, config);
    return {std::move(capture_result.frames), capture_result.capture_error};
  } catch (...) {
    return {{}, true};
  }
}

std::string Stacktrace::ToString(bool include_header) const {
  if (frames_.empty()) {
    if (capture_error_) {
      if (include_header) {
        return "Stack trace: <error>";
      }
      return "<error>";
    }
    if (include_header) {
      return "Stack trace: <empty>";
    }
    return "<empty>";
  }

  std::string result;
  constexpr size_t estimated_frame_size = 64;
  result.reserve((frames_.size() * estimated_frame_size) +
                 (include_header ? 16 : 0));

  if (include_header) {
    result.append("Stack trace:");
  }

  for (size_t index = 0; index < frames_.size(); ++index) {
    std::format_to(std::back_inserter(result), "\n  {}: {}", index + 1,
                   frames_[index]);
  }

  return result;
}

}  // namespace helios
