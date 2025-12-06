#include <helios/core/logger.hpp>

#include <helios/core/utils/filesystem.hpp>

#include <spdlog/logger.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#ifdef HELIOS_ENABLE_STACKTRACE
#include <boost/stacktrace.hpp>
#endif

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <expected>
#include <filesystem>
#include <format>
#include <iterator>
#include <memory>
#include <source_location>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace {

constexpr size_t kFormatBufferReserveSize = 256;
constexpr size_t kMaxStackTraceFrames = 10;
constexpr size_t kStackTraceReserveSize = 512;

[[nodiscard]] auto GenerateTimestamp() noexcept -> std::expected<std::string, std::string_view> {
  try {
    const auto now = std::chrono::system_clock::now();
    const auto time_t_now = std::chrono::system_clock::to_time_t(now);

    std::tm local_time{};

    // Needed because of compatibility issues on Windows
    // For more info: https://en.cppreference.com/w/c/chrono/localtime.html
#ifdef _WIN32
    const errno_t err = localtime_s(&local_time, &time_t_now);
    if (err != 0) [[unlikely]] {
      return std::unexpected("Failed to get local time");
    }
#else
    const std::tm* const local_time_ptr = localtime_r(&time_t_now, &local_time);
    if (local_time_ptr == nullptr) [[unlikely]] {
      return std::unexpected("Failed to get local time");
    }
#endif

    const int year = local_time.tm_year + 1900;
    const int month = local_time.tm_mon + 1;
    const int day = local_time.tm_mday;
    const int hour = local_time.tm_hour;
    const int min = local_time.tm_min;
    const int sec = local_time.tm_sec;
    return std::format("{:04d}-{:02d}-{:02d}_{:02d}-{:02d}-{:02d}", year, month, day, hour, min, sec);
  } catch (...) {
    return std::unexpected("Exception during timestamp generation");
  }
}

class SourceLocationFormatterFlag final : public spdlog::custom_flag_formatter {
public:
  explicit SourceLocationFormatterFlag(spdlog::level::level_enum min_level) noexcept : min_level_(min_level) {}

  void format(const spdlog::details::log_msg& msg, const std::tm& tm, spdlog::memory_buf_t& dest) override;

  [[nodiscard]] auto clone() const -> std::unique_ptr<spdlog::custom_flag_formatter> override {
    return std::make_unique<SourceLocationFormatterFlag>(min_level_);
  }

private:
  spdlog::level::level_enum min_level_;
};

void SourceLocationFormatterFlag::format(const spdlog::details::log_msg& msg, [[maybe_unused]] const std::tm& tm,
                                         spdlog::memory_buf_t& dest) {
  // Only add source location if level is at or above minimum
  if (msg.level < min_level_) [[likely]] {
    return;
  }

  std::array<char, kFormatBufferReserveSize> buffer = {};

  char* ptr = buffer.data();
  char* const buffer_end = buffer.data() + buffer.size();

  *ptr++ = ' ';
  *ptr++ = '[';

  if (msg.source.filename != nullptr) [[likely]] {
    const std::string_view filename(msg.source.filename);
    const auto remaining_space = static_cast<size_t>(std::distance(ptr, buffer_end));
    const size_t copy_len = std::min(filename.size(), remaining_space - 20);
    ptr = std::copy_n(filename.begin(), copy_len, ptr);
  }

  *ptr++ = ':';

  // Use to_chars for safe formatting
  const auto result = std::to_chars(ptr, buffer_end - 1, msg.source.line);
  if (result.ec == std::errc{}) [[likely]] {
    ptr = result.ptr;
  } else {
    *ptr++ = '?';
  }

  *ptr++ = ']';
  dest.append(buffer.data(), ptr);
}

#ifdef HELIOS_ENABLE_STACKTRACE
class StackTraceFormatterFlag final : public spdlog::custom_flag_formatter {
public:
  explicit StackTraceFormatterFlag(spdlog::level::level_enum min_level) : min_level_(min_level) {}

  void format(const spdlog::details::log_msg& msg, const std::tm& tm, spdlog::memory_buf_t& dest) override;

  [[nodiscard]] auto clone() const -> std::unique_ptr<spdlog::custom_flag_formatter> override {
    return std::make_unique<StackTraceFormatterFlag>(min_level_);
  }

private:
  [[nodiscard]] static size_t FindStartingFrame(const boost::stacktrace::stacktrace& stack_trace,
                                                const spdlog::source_loc& source_loc) noexcept;

  [[nodiscard]] static bool IsExactSourceMatch(const std::string& stacktrace_entry,
                                               const std::string& target_pattern) noexcept;

  [[nodiscard]] static constexpr bool IsValidPrecedingChar(char ch) noexcept {
    return ch == ' ' || ch == '\t' || ch == '/' || ch == '\\' || ch == ':';
  }

  [[nodiscard]] static constexpr bool IsValidFollowingChar(char ch) noexcept {
    return ch == ' ' || ch == '\t' || ch == ')' || ch == '\n' || ch == '\r';
  }

  spdlog::level::level_enum min_level_;
};

void StackTraceFormatterFlag::format(const spdlog::details::log_msg& msg, [[maybe_unused]] const std::tm& tm,
                                     spdlog::memory_buf_t& dest) {
  // Only add stack trace if level is at or above minimum
  if (msg.level < min_level_) [[likely]] {
    return;
  }

  using namespace std::literals::string_view_literals;

  try {
    const boost::stacktrace::stacktrace stack_trace;
    if (stack_trace.size() <= 1) [[unlikely]] {
      dest.append("\nStack trace: <empty>"sv);
      return;
    }

    dest.append("\nStack trace:"sv);

    // Find the starting frame based on source location
    const size_t start_frame = FindStartingFrame(stack_trace, msg.source);

    const size_t frame_count = std::min(stack_trace.size(), start_frame + kMaxStackTraceFrames);

    constexpr size_t kEntimatedFrameSize = 64;
    dest.reserve(dest.size() + (frame_count * kEntimatedFrameSize));

    // Print up to kMaxStackTraceFrames frames starting from start_frame
    for (size_t i = start_frame, out_idx = 1; i < frame_count; ++i, ++out_idx) {
      const auto& entry = stack_trace[i];

      std::string entry_str = boost::stacktrace::to_string(entry);
      std::format_to(std::back_inserter(dest), "\n  {}: {}", out_idx, std::move(entry_str));
    }
  } catch (...) {
    dest.append("\nStack trace: <error>"sv);
  }
}

size_t StackTraceFormatterFlag::FindStartingFrame(const boost::stacktrace::stacktrace& stack_trace,
                                                  const spdlog::source_loc& source_loc) noexcept {
  if (source_loc.filename == nullptr || source_loc.line <= 0) [[unlikely]] {
    return 1;  // Default: skip frame 0 (current function)
  }

  // Extract just the filename from the full path
  const std::string_view filename = helios::utils::GetFileName(std::string_view(source_loc.filename));

  std::string target_pattern;
  std::string entry_str;

  try {
    target_pattern = std::format("{}:{}", filename, source_loc.line);
    entry_str.reserve(kStackTraceReserveSize);
  } catch (...) {
    return 1;  // Fallback if formatting fails
  }

  // Look for exact filename:line matches
  for (size_t i = 1; i < stack_trace.size(); ++i) {
    try {
      entry_str.clear();
      entry_str = boost::stacktrace::to_string(stack_trace[i]);
      if (IsExactSourceMatch(entry_str, target_pattern)) [[unlikely]] {
        return i;
      }
    } catch (...) {
      // Skip frames that can't be converted to string
      continue;
    }
  }

  return 1;  // Fallback: skip frame 0
}

bool StackTraceFormatterFlag::IsExactSourceMatch(const std::string& stacktrace_entry,
                                                 const std::string& target_pattern) noexcept {
  try {
    size_t pos = stacktrace_entry.find(target_pattern);
    if (pos == std::string::npos) [[likely]] {
      return false;
    }

    // Ensure we have an exact match by checking boundaries
    while (pos != std::string::npos) {
      const bool valid_start = (pos == 0) || IsValidPrecedingChar(stacktrace_entry[pos - 1]);
      const size_t end_pos = pos + target_pattern.length();
      const bool valid_end = (end_pos == stacktrace_entry.length()) || IsValidFollowingChar(stacktrace_entry[end_pos]);

      if (valid_start && valid_end) [[unlikely]] {
        return true;
      }

      // Continue searching for next occurrence
      pos = stacktrace_entry.find(target_pattern, pos + 1);
    }

    return false;
  } catch (...) {
    return false;
  }
}

#endif

[[nodiscard]] std::string FormatLogFileName(std::string_view logger_name, std::string_view pattern) {
  std::string result(pattern);

  size_t pos = result.find("{name}");
  if (pos != std::string::npos) {
    result.replace(pos, 6, logger_name);
  }

  pos = result.find("{timestamp}");
  if (pos != std::string::npos) {
    const std::string timestamp = GenerateTimestamp().value_or("unknown_time");
    result.replace(pos, 11, timestamp);
  }

  return result;
}

}  // namespace

namespace helios {

void Logger::FlushAll() noexcept {
  const std::shared_lock lock(loggers_mutex_);
  for (const auto& [_, logger] : loggers_) {
    if (logger) [[likely]] {
      try {
        logger->flush();
      } catch (...) {
        // Silently ignore flush errors
      }
    }
  }
}

void Logger::FlushImpl(LoggerId logger_id) noexcept {
  if (const auto logger = GetLogger(logger_id)) [[likely]] {
    try {
      logger->flush();
    } catch (...) {
      // Silently ignore flush errors
    }
  }
}

void Logger::SetLevel(LogLevel level) noexcept {
  if (const auto logger = GetDefaultLogger()) [[likely]] {
    logger->set_level(static_cast<spdlog::level::level_enum>(std::to_underlying(level)));
  }
}

bool Logger::ShouldLog(LogLevel level) const noexcept {
  if (const auto logger = GetDefaultLogger()) [[likely]] {
    return logger->should_log(static_cast<spdlog::level::level_enum>(std::to_underlying(level)));
  }
  return false;
}

LogLevel Logger::GetLevel() const noexcept {
  if (const auto logger = GetDefaultLogger()) [[likely]] {
    return static_cast<LogLevel>(logger->level());
  }
  return LogLevel::kTrace;
}

void Logger::SetLevelImpl(LoggerId logger_id, LogLevel level) noexcept {
  if (const auto logger = GetLogger(logger_id)) [[likely]] {
    logger->set_level(static_cast<spdlog::level::level_enum>(std::to_underlying(level)));

    const std::scoped_lock lock(loggers_mutex_);
    logger_levels_[logger_id] = level;
  }
}

bool Logger::ShouldLogImpl(LoggerId logger_id, LogLevel level) const noexcept {
  {
    const std::shared_lock lock(loggers_mutex_);
    if (const auto it = logger_levels_.find(logger_id); it != logger_levels_.end()) {
      return level >= it->second;
    }
  }

  // Fallback to checking spdlog if not in cache
  if (const auto logger = GetLogger(logger_id)) [[likely]] {
    return logger->should_log(static_cast<spdlog::level::level_enum>(std::to_underlying(level)));
  }
  return false;
}

LogLevel Logger::GetLevelImpl(LoggerId logger_id) const noexcept {
  {
    const std::shared_lock lock(loggers_mutex_);
    if (const auto it = logger_levels_.find(logger_id); it != logger_levels_.end()) {
      return it->second;
    }
  }

  // Fallback to checking spdlog if not in cache
  if (const auto logger = GetLogger(logger_id)) [[likely]] {
    return static_cast<LogLevel>(logger->level());
  }
  return LogLevel::kTrace;
}

std::shared_ptr<spdlog::logger> Logger::GetLogger(LoggerId logger_id) const noexcept {
  const std::shared_lock lock(loggers_mutex_);
  const auto it = loggers_.find(logger_id);
  return it == loggers_.end() ? nullptr : it->second;
}

void Logger::LogMessageImpl(const std::shared_ptr<spdlog::logger>& logger, LogLevel level,
                            const std::source_location& loc, std::string_view message) noexcept {
  if (!logger) [[unlikely]] {
    return;
  }

  try {
    const std::string_view filename = utils::GetFileName(std::string_view(loc.file_name()));
    logger->log(spdlog::source_loc{filename.data(), static_cast<int>(loc.line()), loc.function_name()},
                static_cast<spdlog::level::level_enum>(std::to_underlying(level)), message);
  } catch (...) {
    // Silently ignore logging errors
  }
}

void Logger::LogAssertionFailureImpl(const std::shared_ptr<spdlog::logger>& logger, std::string_view condition,
                                     const std::source_location& loc, std::string_view message) noexcept {
  if (!logger) [[unlikely]] {
    return;
  }

  try {
    const std::string assertion_msg = std::format("Assertion failed: {} | {}", condition, message);
    LogMessageImpl(logger, LogLevel::kCritical, loc, assertion_msg);
  } catch (...) {
    // Silently ignore logging errors
  }
}

auto Logger::CreateLogger(std::string_view logger_name, const LoggerConfig& config) noexcept
    -> std::shared_ptr<spdlog::logger> {
  try {
    std::vector<std::shared_ptr<spdlog::sinks::sink>> log_sinks;
    log_sinks.reserve(2);

    const auto source_loc_level =
        static_cast<spdlog::level::level_enum>(std::to_underlying(config.source_location_level));
    [[maybe_unused]] const auto stack_trace_level =
        static_cast<spdlog::level::level_enum>(std::to_underlying(config.stack_trace_level));

    // Create file sink if enabled
    if (config.enable_file) {
      // Create log directory if it doesn't exist
      std::error_code ec;
      std::filesystem::create_directories(config.log_directory, ec);
      if (ec) {
        // Failed to create directory, continue without file logging
      } else {
        // Format the log file name
        const std::string log_file_name = FormatLogFileName(logger_name, config.file_name_pattern);
        const std::filesystem::path log_file_path = config.log_directory / log_file_name;

        auto file_formatter = std::make_unique<spdlog::pattern_formatter>();
#ifdef HELIOS_ENABLE_STACKTRACE
        file_formatter->add_flag<SourceLocationFormatterFlag>('*', source_loc_level)
            .add_flag<StackTraceFormatterFlag>('#', stack_trace_level)
            .set_pattern(config.file_pattern);
#else
        file_formatter->add_flag<SourceLocationFormatterFlag>('*', source_loc_level).set_pattern(config.file_pattern);
#endif
        auto file_sink =
            std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file_path.string(), config.truncate_files);
        file_sink->set_formatter(std::move(file_formatter));
        log_sinks.push_back(std::move(file_sink));
      }
    }

    // Create console sink if enabled
    if (config.enable_console) {
      auto console_formatter = std::make_unique<spdlog::pattern_formatter>();
#ifdef HELIOS_ENABLE_STACKTRACE
      console_formatter->add_flag<SourceLocationFormatterFlag>('*', source_loc_level)
          .add_flag<StackTraceFormatterFlag>('#', stack_trace_level)
          .set_pattern(config.console_pattern);
#else
      console_formatter->add_flag<SourceLocationFormatterFlag>('*', source_loc_level)
          .set_pattern(config.console_pattern);
#endif
      auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
      console_sink->set_formatter(std::move(console_formatter));
      log_sinks.emplace_back(std::move(console_sink));
    }

    if (log_sinks.empty()) [[unlikely]] {
      return nullptr;
    }

    auto logger = std::make_shared<spdlog::logger>(std::string(logger_name), std::make_move_iterator(log_sinks.begin()),
                                                   std::make_move_iterator(log_sinks.end()));
    spdlog::register_logger(logger);
    logger->set_level(spdlog::level::trace);
    logger->flush_on(static_cast<spdlog::level::level_enum>(std::to_underlying(config.auto_flush_level)));
    return logger;
  } catch (...) {
    return nullptr;
  }
}

void Logger::DropLoggerFromSpdlog(const std::shared_ptr<spdlog::logger>& logger) noexcept {
  if (!logger) [[unlikely]] {
    return;
  }

  try {
    spdlog::drop(logger->name());
  } catch (...) {
    // Silently ignore drop errors
  }
}

}  // namespace helios

// MSVC-specific definition for assertion logging integration
// On GCC/Clang, the inline definition in logger.hpp takes precedence over the weak symbol in assert.cpp
#if defined(_MSC_VER)
namespace helios::details {

void LogAssertionFailureViaLogger(std::string_view condition, const std::source_location& loc,
                                  std::string_view message) noexcept {
  Logger::GetInstance().LogAssertionFailure(condition, loc, message);
}

}  // namespace helios::details
#endif
