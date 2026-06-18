#include <pch.hpp>

#include <helios/log/logger.hpp>

#include <helios/assert.hpp>
#include <helios/container/static_string.hpp>
#include <helios/log/config.hpp>

#include <spdlog/async.h>
#include <spdlog/logger.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <ctime>
#include <expected>
#include <filesystem>
#include <format>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

using Timestamp = helios::container::StaticString<19>;  // YYYY-MM-DD_HH-MM-SS

namespace {

[[nodiscard]] auto GenerateTimestamp() noexcept
    -> std::expected<Timestamp, std::string_view> {
  try {
    const auto now = std::chrono::system_clock::now();
    const auto time_t_now = std::chrono::system_clock::to_time_t(now);

    std::tm local_time{};

    // Needed because of compatibility issues on Windows
    // For more info: https://en.cppreference.com/w/c/chrono/localtime.html
#ifdef HELIOS_PLATFORM_WINDOWS
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

    std::array<char, 20> time_buf = {};
    const auto result = std::format_to(
        time_buf.begin(), "{:04d}-{:02d}-{:02d}_{:02d}-{:02d}-{:02d}", year,
        month, day, hour, min, sec);
    const auto len =
        static_cast<size_t>(std::distance(time_buf.begin(), result));
    return Timestamp(std::string_view(time_buf.data(), len));
  } catch (...) {
    return std::unexpected("Exception during timestamp generation");
  }
}

[[nodiscard]] std::string FormatLogFileName(std::string_view logger_name,
                                            std::string_view pattern) {
  std::string result(pattern);

  size_t pos = result.find("{name}");
  if (pos != std::string::npos) {
    result.replace(pos, 6, logger_name);
  }

  pos = result.find("{timestamp}");
  if (pos != std::string::npos) {
    const Timestamp timestamp = GenerateTimestamp().value_or("unknown_time");
    result.replace(pos, 11, timestamp.View());
  }

  return result;
}

}  // namespace

namespace helios::log {

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

void Logger::FlushImpl(LoggerTypeIndex logger_index) noexcept {
  if (const auto logger = GetLogger(logger_index)) [[likely]] {
    try {
      logger->flush();
    } catch (...) {
      // Silently ignore flush errors
    }
  }
}

void Logger::SetLevel(Level level) noexcept {
  if (const auto logger = GetDefaultLogger()) [[likely]] {
    logger->set_level(
        static_cast<spdlog::level::level_enum>(std::to_underlying(level)));
  }
}

bool Logger::ShouldLog(Level level) const noexcept {
  if (const auto logger = GetDefaultLogger()) [[likely]] {
    return logger->should_log(
        static_cast<spdlog::level::level_enum>(std::to_underlying(level)));
  }
  return false;
}

Level Logger::GetLevel() const noexcept {
  if (const auto logger = GetDefaultLogger()) [[likely]] {
    return static_cast<Level>(logger->level());
  }
  return Level::kTrace;
}

void Logger::SetLevelImpl(LoggerTypeIndex logger_index, Level level) noexcept {
  if (const auto logger = GetLogger(logger_index)) [[likely]] {
    logger->set_level(
        static_cast<spdlog::level::level_enum>(std::to_underlying(level)));

    const std::scoped_lock lock(loggers_mutex_);
    logger_levels_[logger_index] = level;
  }
}

bool Logger::ShouldLogImpl(LoggerTypeIndex logger_index,
                           Level level) const noexcept {
  {
    const std::shared_lock lock(loggers_mutex_);
    if (const auto it = logger_levels_.find(logger_index);
        it != logger_levels_.end()) {
      return level >= it->second;
    }
  }

  // Fallback to checking spdlog if not in cache
  if (const auto logger = GetLogger(logger_index)) [[likely]] {
    return logger->should_log(
        static_cast<spdlog::level::level_enum>(std::to_underlying(level)));
  }
  return false;
}

Level Logger::GetLevelImpl(LoggerTypeIndex logger_index) const noexcept {
  {
    const std::shared_lock lock(loggers_mutex_);
    if (const auto it = logger_levels_.find(logger_index);
        it != logger_levels_.end()) {
      return it->second;
    }
  }

  // Fallback to checking spdlog if not in cache
  if (const auto logger = GetLogger(logger_index)) [[likely]] {
    return static_cast<Level>(logger->level());
  }
  return Level::kTrace;
}

auto Logger::GetLogger(LoggerTypeIndex logger_index) const noexcept
    -> std::shared_ptr<spdlog::logger> {
  const std::shared_lock lock(loggers_mutex_);
  const auto it = loggers_.find(logger_index);
  return it == loggers_.end() ? nullptr : it->second;
}

void Logger::LogMessageImpl(const std::shared_ptr<spdlog::logger>& logger,
                            Level level, std::string_view message) noexcept {
  if (!logger) [[unlikely]] {
    return;
  }

  try {
    logger->log(
        static_cast<spdlog::level::level_enum>(std::to_underlying(level)),
        message);
  } catch (...) {
    // Silently ignore logging errors
  }
}

auto Logger::CreateLogger(std::string_view logger_name,
                          const Config& config) noexcept
    -> std::shared_ptr<spdlog::logger> {
  try {
    std::vector<std::shared_ptr<spdlog::sinks::sink>> log_sinks;
    log_sinks.reserve(2);

    // Create file sink if enabled
    if (config.enable_file) {
      // Create log directory if it doesn't exist
      std::error_code ec;
      std::filesystem::create_directories(config.log_directory, ec);
      if (ec) {
        // Failed to create directory, continue without file logging
      } else {
        // Format the log file name
        const std::string log_file_name =
            FormatLogFileName(logger_name, config.file_name_pattern);
        std::filesystem::path log_file_path(config.log_directory);
        log_file_path /= log_file_name;

        auto file_formatter = std::make_unique<spdlog::pattern_formatter>();
        file_formatter->set_pattern(std::string(config.file_pattern));
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            log_file_path.string(), config.truncate_files);
        file_sink->set_formatter(std::move(file_formatter));
        log_sinks.push_back(std::move(file_sink));
      }
    }

    // Create console sink if enabled
    if (config.enable_console) {
      auto console_formatter = std::make_unique<spdlog::pattern_formatter>();
      console_formatter->set_pattern(std::string(config.console_pattern));
      auto console_sink =
          std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
      console_sink->set_formatter(std::move(console_formatter));
      log_sinks.emplace_back(std::move(console_sink));
    }

    if (log_sinks.empty()) [[unlikely]] {
      return nullptr;
    }

    std::shared_ptr<spdlog::logger> logger;
    if (config.async_logging) {
      // Initialize spdlog's global thread pool once per process
      static std::once_flag s_init_flag;
      std::call_once(s_init_flag, [&] {
        spdlog::init_thread_pool(config.async_queue_size,
                                 config.async_thread_count);
      });

      logger = std::make_shared<spdlog::async_logger>(
          std::string(logger_name), std::make_move_iterator(log_sinks.begin()),
          std::make_move_iterator(log_sinks.end()), spdlog::thread_pool(),
          spdlog::async_overflow_policy::block);
    } else {
      logger = std::make_shared<spdlog::logger>(
          std::string(logger_name), std::make_move_iterator(log_sinks.begin()),
          std::make_move_iterator(log_sinks.end()));
    }

    spdlog::register_logger(logger);
    logger->set_level(spdlog::level::trace);
    logger->flush_on(static_cast<spdlog::level::level_enum>(
        std::to_underlying(config.auto_flush_level)));
    return logger;
  } catch (...) {
    return nullptr;
  }
}

void Logger::DropLoggerFromSpdlog(
    const std::shared_ptr<spdlog::logger>& logger) noexcept {
  if (!logger) [[unlikely]] {
    return;
  }

  try {
    spdlog::drop(logger->name());
  } catch (...) {
    // Silently ignore drop errors
  }
}

namespace details {

void LogAssertionViaLogger(std::string_view condition,
                           const std::source_location& loc,
                           std::string_view message) noexcept {
  const std::string formatted =
      helios::details::FormatAssertionMessage(condition, loc, message);
  Logger::Instance().Log(Level::kCritical, formatted);
}

}  // namespace details

}  // namespace helios::log

// These functions override the weak symbols defined in assert.cpp to provide
// logging integration for assertions when the log plugin is linked.

namespace helios::details {

#ifdef _MSC_VER
// MSVC: Register the handler at static initialization time
namespace {

void MsvcLogPluginHandler(std::string_view condition,
                          const std::source_location& loc,
                          std::string_view message) noexcept {
  helios::log::details::LogAssertionViaLogger(condition, loc, message);
}

struct LogPluginHandlerRegistrar {
  LogPluginHandlerRegistrar() noexcept {
    // Register our handler with the assert system
    SetLogPluginHandler(&MsvcLogPluginHandler);
  }
};

// Static instance to register handler at startup
[[maybe_unused]] static LogPluginHandlerRegistrar g_log_plugin_registrar{};

}  // namespace

#else
// GCC/Clang: Override the weak symbols from assert.cpp

/// @brief Override weak symbol to indicate log plugin is available
bool HasLogPluginHandler() noexcept {
  return true;
}

/// @brief Override weak symbol to provide log plugin assertion handling.
void LogPluginAssertionHandler(std::string_view condition,
                               const std::source_location& loc,
                               std::string_view message) noexcept {
  helios::log::details::LogAssertionViaLogger(condition, loc, message);
}

#endif  // _MSC_VER

}  // namespace helios::details
