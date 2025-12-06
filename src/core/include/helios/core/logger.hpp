#pragma once

#include <helios/core_pch.hpp>

#include <ctti/type_id.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace spdlog {

// Forward declaration for spdlog::logger
class logger;

}  // namespace spdlog

namespace helios {

/**
 * @brief Log severity levels.
 */
enum class LogLevel : uint8_t { kTrace = 0, kDebug = 1, kInfo = 2, kWarn = 3, kError = 4, kCritical = 5 };

/**
 * @brief Configuration for logger behavior and output.
 */
struct LoggerConfig {
  std::filesystem::path log_directory = "logs";  ///< Log output directory path.
  std::string file_name_pattern =
      "{name}_{timestamp}.log";  ///< Pattern for log file names (supports format placeholders: {name}, {timestamp}).
#ifdef HELIOS_ENABLE_STACKTRACE
  std::string console_pattern = "[%H:%M:%S.%e] [%t] [%^%l%$] %n: %v%*%#";    ///< Console log pattern.
  std::string file_pattern = "[%Y-%m-%d %H:%M:%S.%e] [%t] [%l] %n: %v%*%#";  ///< File log pattern.
#else
  std::string console_pattern = "[%H:%M:%S.%e] [%t] [%^%l%$] %n: %v%*";    ///< Console log pattern.
  std::string file_pattern = "[%Y-%m-%d %H:%M:%S.%e] [%t] [%l] %n: %v%*";  ///< File log pattern.
#endif

  size_t max_file_size = 0;                     ///< Maximum size of a single log file in bytes (0 = no limit).
  size_t max_files = 0;                         ///< Maximum number of log files to keep (0 = no limit).
  LogLevel auto_flush_level = LogLevel::kWarn;  ///< Minimum log level to flush automatically.

  bool enable_console = true;  ///< Enable console output.
  bool enable_file = true;     ///< Enable file output.
  bool truncate_files = true;  ///< Enable truncation of existing log files.
  bool async_logging = false;  ///< Enable async logging (better performance but may lose last logs on crash).

  LogLevel source_location_level = LogLevel::kError;  ///< Minimum level to include source location.
  LogLevel stack_trace_level = LogLevel::kCritical;   ///< Minimum level to include stack trace.

  /**
   * @brief Creates default configuration.
   * @return Default LoggerConfig instance
   */
  [[nodiscard]] static LoggerConfig Default() noexcept { return {}; }

  /**
   * @brief Creates configuration for console-only output.
   * @return LoggerConfig instance for console-only logging
   */
  [[nodiscard]] static LoggerConfig ConsoleOnly() noexcept {
    return LoggerConfig{.enable_console = true, .enable_file = false};
  }

  /**
   * @brief Creates configuration for file-only output.
   * @return LoggerConfig instance for file-only logging
   */
  [[nodiscard]] static LoggerConfig FileOnly() noexcept {
    return LoggerConfig{.enable_console = false, .enable_file = true};
  }

  /**
   * @brief Creates configuration optimized for debug builds.
   * @return LoggerConfig instance for debug logging
   */
  [[nodiscard]] static LoggerConfig Debug() noexcept {
    return LoggerConfig{.enable_console = true, .enable_file = true, .async_logging = false};
  }

  /**
   * @brief Creates configuration optimized for release builds.
   * @return LoggerConfig instance for release logging
   */
  [[nodiscard]] static LoggerConfig Release() noexcept {
    return LoggerConfig{.enable_console = false, .enable_file = true, .async_logging = true};
  }
};

/**
 * @brief Type alias for logger type IDs.
 * @details Used to uniquely identify logger types at runtime.
 */
using LoggerId = size_t;

/**
 * @brief Trait to identify valid logger types.
 * @details A valid logger type must be an empty struct or class with a Name() function.
 *
 * @example
 * @code
 * struct MyLogger {
 *   static constexpr std::string_view Name() noexcept { return "MyLogger"; }
 * };
 * static_assert(LoggerTrait<MyLogger>);
 * @endcode
 * @tparam T Type to check
 */
template <typename T>
concept LoggerTrait = std::is_empty_v<std::remove_cvref_t<T>> && requires {
  { T::Name() } -> std::same_as<std::string_view>;
};

/**
 * @brief Trait to identify loggers with custom configuration.
 * @details A logger with config trait must satisfy LoggerTrait and provide:
 * - `static constexpr LoggerConfig Config() noexcept`
 *
 * @example
 * @code
 * struct MyLogger {
 *   static constexpr std::string_view Name() noexcept { return "MyLogger"; }
 *   static constexpr LoggerConfig Config() noexcept { return LoggerConfig::Default(); }
 * };
 * static_assert(LoggerWithConfigTrait<MyLogger>);
 * @endcode
 * @tparam T Type to check
 */
template <typename T>
concept LoggerWithConfigTrait = LoggerTrait<T> && requires {
  { T::Config() } -> std::same_as<LoggerConfig>;
};

/**
 * @brief Gets unique type ID for a logger type.
 * @details Uses CTTI to generate a unique hash for the logger type.
 * The ID is consistent across compilation units.
 *
 * @example
 * @code
 * struct MyLogger { static constexpr std::string_view Name() noexcept { return "MyLogger"; } };
 * constexpr LoggerId id = LoggerIdOf<MyLogger>();
 * @endcode
 * @tparam T Logger type
 * @return Unique type ID for the logger
 */
template <LoggerTrait T>
constexpr LoggerId LoggerIdOf() noexcept {
  return ctti::type_index_of<T>().hash();
}

/**
 * @brief Gets the name of a logger type.
 * @details Returns the name provided by Name() function.
 *
 * @example
 * @code
 * struct MyLogger {
 *   static constexpr std::string_view Name() noexcept { return "MyLogger"; }
 * };
 * constexpr auto name = LoggerNameOf<MyLogger>();  // "MyLogger"
 * @endcode
 * @tparam T Logger type
 * @return Name of the logger
 */
template <LoggerTrait T>
constexpr std::string_view LoggerNameOf() noexcept {
  return T::Name();
}

/**
 * @brief Gets the configuration for a logger type.
 * @details Returns the config provided by Config() if available,
 * otherwise returns the default configuration.
 *
 * @example
 * @code
 * struct MyLogger {
 *   static constexpr std::string_view Name() noexcept { return "MyLogger"; }
 *   static constexpr LoggerConfig Config() noexcept { return LoggerConfig::ConsoleOnly(); }
 * };
 * constexpr auto config = LoggerConfigOf<MyLogger>();
 * @endcode
 * @tparam T Logger type
 * @return Configuration for the logger
 */
template <LoggerTrait T>
inline LoggerConfig LoggerConfigOf() noexcept {
  if constexpr (LoggerWithConfigTrait<T>) {
    return T::Config();
  } else {
    return LoggerConfig::Default();
  }
}

/**
 * @brief Default logger type.
 */
struct DefaultLogger {
  static constexpr std::string_view Name() noexcept { return "HELIOS"; }
  static LoggerConfig Config() noexcept {
#if defined(HELIOS_RELEASE_MODE)
    return LoggerConfig::Release();
#else
    return LoggerConfig::Debug();
#endif
  }
};

/**
 * @brief Constexpr instance of the default logger for easier user interface.
 */
inline constexpr DefaultLogger kDefaultLogger{};

/**
 * @brief Centralized logging system with configurable output and formatting.
 * @note Thread-safe.
 */
class Logger {
public:
  Logger(const Logger&) = delete;
  Logger(Logger&&) = delete;
  ~Logger() noexcept = default;

  Logger& operator=(const Logger&) = delete;
  Logger& operator=(Logger&&) = delete;

  /**
   * @brief Adds a logger with the specified type and configuration.
   * @tparam T Logger type
   * @param logger Logger type instance
   * @param config Configuration for the logger
   */
  template <LoggerTrait T>
  void AddLogger(T logger, LoggerConfig config = LoggerConfigOf<T>()) noexcept;

  /**
   * @brief Removes a logger with the given type.
   * @note Cannot remove the default logger
   * @tparam T Logger type
   * @param logger Logger type instance
   */
  template <LoggerTrait T>
  void RemoveLogger(T logger = {}) noexcept;

  /**
   * @brief Flushes all registered loggers.
   */
  void FlushAll() noexcept;

  /**
   * @brief Flushes a specific logger.
   * @tparam T Logger type
   * @param logger Logger type instance
   */
  template <LoggerTrait T>
  void Flush([[maybe_unused]] T logger = {}) noexcept {
    FlushImpl(LoggerIdOf<T>());
  }

  /**
   * @brief Logs a string message with typed logger.
   * @tparam T Logger type
   * @param logger Logger type instance
   * @param level Log severity level
   * @param loc Source location where the log was triggered
   * @param message Message to log
   */
  template <LoggerTrait T>
  void LogMessage(T logger, LogLevel level, const std::source_location& loc, std::string_view message) noexcept;

  /**
   * @brief Logs a formatted message with typed logger.
   * @tparam T Logger type
   * @tparam Args Types of the format arguments
   * @param logger Logger type instance
   * @param level Log severity level
   * @param loc Source location where the log was triggered
   * @param fmt Format string
   * @param args Arguments for the format string
   */
  template <LoggerTrait T, typename... Args>
    requires(sizeof...(Args) > 0)
  void LogMessage(T logger, LogLevel level, const std::source_location& loc, std::format_string<Args...> fmt,
                  Args&&... args) noexcept;

  /**
   * @brief Logs a string message with default logger.
   * @param level Log severity level
   * @param loc Source location where the log was triggered
   * @param message Message to log
   */
  void LogMessage(LogLevel level, const std::source_location& loc, std::string_view message) noexcept;

  /**
   * @brief Logs a formatted message with default logger.
   * @tparam Args Types of the format arguments
   * @param level Log severity level
   * @param loc Source location where the log was triggered
   * @param fmt Format string
   * @param args Arguments for the format string
   */
  template <typename... Args>
    requires(sizeof...(Args) > 0)
  void LogMessage(LogLevel level, const std::source_location& loc, std::format_string<Args...> fmt,
                  Args&&... args) noexcept;

  /**
   * @brief Logs assertion failure with typed logger.
   * @tparam T Logger type
   * @param logger Logger type instance
   * @param condition The failed condition as a string
   * @param loc Source location where the assertion failed
   * @param message Additional message describing the failure
   */
  template <LoggerTrait T>
  void LogAssertionFailure(T logger, std::string_view condition, const std::source_location& loc,
                           std::string_view message) noexcept;

  /**
   * @brief Logs assertion failure with typed logger (formatted).
   * @tparam T Logger type
   * @tparam Args Types of the format arguments
   * @param logger Logger type instance
   * @param condition The failed condition as a string
   * @param loc Source location where the assertion failed
   * @param fmt Format string for the failure message
   * @param args Arguments for the format string
   */
  template <LoggerTrait T, typename... Args>
    requires(sizeof...(Args) > 0)
  void LogAssertionFailure(T logger, std::string_view condition, const std::source_location& loc,
                           std::format_string<Args...> fmt, Args&&... args) noexcept;

  /**
   * @brief Logs assertion failure with default logger.
   * @param condition The failed condition as a string
   * @param loc Source location where the assertion failed
   * @param message Additional message describing the failure
   */
  void LogAssertionFailure(std::string_view condition, const std::source_location& loc,
                           std::string_view message) noexcept;

  /**
   * @brief Logs assertion failure with default logger (formatted).
   * @tparam Args Types of the format arguments
   * @param condition The failed condition as a string
   * @param loc Source location where the assertion failed
   * @param fmt Format string for the failure message
   * @param args Arguments for the format string
   */
  template <typename... Args>
    requires(sizeof...(Args) > 0)
  void LogAssertionFailure(std::string_view condition, const std::source_location& loc, std::format_string<Args...> fmt,
                           Args&&... args) noexcept;

  /**
   * @brief Sets the global default configuration for new loggers.
   * @param config The configuration to use as default
   */
  void SetDefaultConfig(const LoggerConfig& config) noexcept { default_config_ = config; }

  /**
   * @brief Sets the minimum log level for a typed logger.
   * @tparam T Logger type
   * @param logger Logger type instance
   * @param level Minimum log level to set
   */
  template <LoggerTrait T>
  void SetLevel([[maybe_unused]] T logger, LogLevel level) noexcept {
    SetLevelImpl(LoggerIdOf<T>(), level);
  }

  /**
   * @brief Sets the minimum log level for the default logger.
   * @param level Minimum log level to set
   */
  void SetLevel(LogLevel level) noexcept;

  /**
   * @brief Checks if a logger with the given type exists.
   * @tparam T Logger type
   * @param logger Logger type instance
   * @return True if the logger exists, false otherwise
   */
  template <LoggerTrait T>
  [[nodiscard]] bool HasLogger(T logger = {}) const noexcept;

  /**
   * @brief Checks if the default logger should log messages at the given level.
   * @param level The log level to check
   * @return True if messages at this level should be logged
   */
  [[nodiscard]] bool ShouldLog(LogLevel level) const noexcept;

  /**
   * @brief Checks if a typed logger should log messages at the given level.
   * @tparam T Logger type
   * @param logger Logger type instance
   * @param level The log level to check
   * @return True if messages at this level should be logged
   */
  template <LoggerTrait T>
  [[nodiscard]] bool ShouldLog([[maybe_unused]] T logger, LogLevel level) const noexcept {
    return ShouldLogImpl(LoggerIdOf<T>(), level);
  }

  /**
   * @brief Gets the current log level for a typed logger.
   * @tparam T Logger type
   * @param logger Logger type instance
   * @return The current log level, or LogLevel::kTrace if logger doesn't exist
   */
  template <LoggerTrait T>
  [[nodiscard]] LogLevel GetLevel([[maybe_unused]] T logger = {}) const noexcept {
    return GetLevelImpl(LoggerIdOf<T>());
  }

  /**
   * @brief Gets the current log level for the default logger.
   * @return The current log level
   */
  [[nodiscard]] LogLevel GetLevel() const noexcept;

  /**
   * @brief Gets the current default configuration.
   * @return The default logger configuration
   */
  [[nodiscard]] const LoggerConfig& GetDefaultConfig() const noexcept { return default_config_; }

  /**
   * @brief Gets the singleton instance.
   * @return Reference to the Logger instance
   */
  [[nodiscard]] static Logger& GetInstance() noexcept {
    static Logger instance;
    return instance;
  }

private:
  Logger() noexcept;

  [[nodiscard]] auto GetLogger(LoggerId logger_id) const noexcept -> std::shared_ptr<spdlog::logger>;
  [[nodiscard]] auto GetDefaultLogger() const noexcept -> std::shared_ptr<spdlog::logger> {
    return GetLogger(LoggerIdOf<DefaultLogger>());
  }

  /**
   * @brief Flushes a specific logger by ID (internal).
   * @param logger_id Logger ID
   */
  void FlushImpl(LoggerId logger_id) noexcept;

  /**
   * @brief Sets the minimum log level for a logger by ID (internal).
   * @param logger_id Logger ID
   * @param level Minimum log level to set
   */
  void SetLevelImpl(LoggerId logger_id, LogLevel level) noexcept;

  /**
   * @brief Checks if a logger by ID should log messages at the given level (internal).
   * @param logger_id Logger ID
   * @param level The log level to check
   * @return True if messages at this level should be logged
   */
  [[nodiscard]] bool ShouldLogImpl(LoggerId logger_id, LogLevel level) const noexcept;

  /**
   * @brief Gets the current log level for a logger by ID (internal).
   * @param logger_id Logger ID
   * @return The current log level, or LogLevel::kTrace if logger doesn't exist
   */
  [[nodiscard]] LogLevel GetLevelImpl(LoggerId logger_id) const noexcept;

  static void LogMessageImpl(const std::shared_ptr<spdlog::logger>& logger, LogLevel level,
                             const std::source_location& loc, std::string_view message) noexcept;

  static void LogAssertionFailureImpl(const std::shared_ptr<spdlog::logger>& logger, std::string_view condition,
                                      const std::source_location& loc, std::string_view message) noexcept;

  [[nodiscard]] static auto CreateLogger(std::string_view logger_name, const LoggerConfig& config) noexcept
      -> std::shared_ptr<spdlog::logger>;

  static void DropLoggerFromSpdlog(const std::shared_ptr<spdlog::logger>& logger) noexcept;

  std::unordered_map<LoggerId, std::shared_ptr<spdlog::logger>> loggers_;
  std::unordered_map<LoggerId, LoggerConfig> logger_configs_;
  std::unordered_map<LoggerId, LogLevel> logger_levels_;
  mutable std::shared_mutex loggers_mutex_;

  LoggerConfig default_config_;
};

inline Logger::Logger() noexcept {
  default_config_ = LoggerConfigOf<DefaultLogger>();

  constexpr LoggerId default_id = LoggerIdOf<DefaultLogger>();
  constexpr std::string_view default_name = LoggerNameOf<DefaultLogger>();

  if (auto logger = CreateLogger(default_name, default_config_)) [[likely]] {
    loggers_.emplace(default_id, std::move(logger));
    logger_configs_.emplace(default_id, default_config_);
    logger_levels_.emplace(default_id, LogLevel::kTrace);
  }
}

template <LoggerTrait T>
inline void Logger::AddLogger(T /*logger*/, LoggerConfig config) noexcept {
  constexpr LoggerId logger_id = LoggerIdOf<T>();
  constexpr std::string_view logger_name = LoggerNameOf<T>();

  const std::scoped_lock lock(loggers_mutex_);
  if (loggers_.contains(logger_id)) [[unlikely]] {
    return;
  }

  if (auto spdlog_logger = CreateLogger(logger_name, config)) [[likely]] {
    loggers_.emplace(logger_id, std::move(spdlog_logger));
    logger_configs_.emplace(logger_id, std::move(config));
    logger_levels_.emplace(logger_id, LogLevel::kTrace);
  }
}

template <LoggerTrait T>
inline void Logger::RemoveLogger(T /*logger*/) noexcept {
  constexpr LoggerId logger_id = LoggerIdOf<T>();

  // Cannot remove the default logger
  if (logger_id == LoggerIdOf<DefaultLogger>()) [[unlikely]] {
    return;
  }

  const std::scoped_lock lock(loggers_mutex_);
  if (const auto it = loggers_.find(logger_id); it != loggers_.end()) {
    // Unregister from spdlog before erasing
    DropLoggerFromSpdlog(it->second);
    loggers_.erase(it);
    logger_configs_.erase(logger_id);
    logger_levels_.erase(logger_id);
  }
}

template <LoggerTrait T>
inline void Logger::LogMessage(T /*logger*/, LogLevel level, const std::source_location& loc,
                               std::string_view message) noexcept {
  if (auto spdlog_logger = GetLogger(LoggerIdOf<T>())) [[likely]] {
    LogMessageImpl(spdlog_logger, level, loc, message);
  }
}

template <LoggerTrait T, typename... Args>
  requires(sizeof...(Args) > 0)
inline void Logger::LogMessage(T logger, LogLevel level, const std::source_location& loc,
                               std::format_string<Args...> fmt, Args&&... args) noexcept {
  try {
    const std::string message = std::format(fmt, std::forward<Args>(args)...);
    LogMessage(logger, level, loc, message);
  } catch (...) {
    // Silently ignore formatting errors
  }
}

inline void Logger::LogMessage(LogLevel level, const std::source_location& loc, std::string_view message) noexcept {
  if (auto logger = GetDefaultLogger()) [[likely]] {
    LogMessageImpl(logger, level, loc, message);
  }
}

template <typename... Args>
  requires(sizeof...(Args) > 0)
inline void Logger::LogMessage(LogLevel level, const std::source_location& loc, std::format_string<Args...> fmt,
                               Args&&... args) noexcept {
  try {
    const std::string message = std::format(fmt, std::forward<Args>(args)...);
    LogMessage(level, loc, message);
  } catch (...) {
    // Silently ignore formatting errors
  }
}

template <LoggerTrait T>
inline void Logger::LogAssertionFailure(T /*logger*/, std::string_view condition, const std::source_location& loc,
                                        std::string_view message) noexcept {
  if (auto spdlog_logger = GetLogger(LoggerIdOf<T>())) [[likely]] {
    LogAssertionFailureImpl(spdlog_logger, condition, loc, message);
  } else if (auto default_logger = GetDefaultLogger()) [[likely]] {
    LogAssertionFailureImpl(default_logger, condition, loc, message);
  }
}

template <LoggerTrait T, typename... Args>
  requires(sizeof...(Args) > 0)
inline void Logger::LogAssertionFailure(T logger, std::string_view condition, const std::source_location& loc,
                                        std::format_string<Args...> fmt, Args&&... args) noexcept {
  try {
    const std::string message = std::format(fmt, std::forward<Args>(args)...);
    LogAssertionFailure(logger, condition, loc, message);
  } catch (...) {
    LogAssertionFailure(logger, condition, loc, "Formatting error in assertion message");
  }
}

inline void Logger::LogAssertionFailure(std::string_view condition, const std::source_location& loc,
                                        std::string_view message) noexcept {
  if (auto logger = GetDefaultLogger()) [[likely]] {
    LogAssertionFailureImpl(logger, condition, loc, message);
  }
}

template <typename... Args>
  requires(sizeof...(Args) > 0)
inline void Logger::LogAssertionFailure(std::string_view condition, const std::source_location& loc,
                                        std::format_string<Args...> fmt, Args&&... args) noexcept {
  try {
    const std::string message = std::format(fmt, std::forward<Args>(args)...);
    LogAssertionFailure(condition, loc, message);
  } catch (...) {
    LogAssertionFailure(condition, loc, "Formatting error in assertion message");
  }
}

template <LoggerTrait T>
inline bool Logger::HasLogger(T /*logger*/) const noexcept {
  const std::shared_lock lock(loggers_mutex_);
  return loggers_.contains(LoggerIdOf<T>());
}

}  // namespace helios

// Provide logger integration for assert.hpp
// On MSVC, we define this in logger.cpp to avoid multiple definition errors
// On GCC/Clang, the inline definition here takes precedence over the weak symbol in assert.cpp
namespace helios::details {

#if !defined(_MSC_VER)
inline void LogAssertionFailureViaLogger(std::string_view condition, const std::source_location& loc,
                                         std::string_view message) noexcept {
  Logger::GetInstance().LogAssertionFailure(condition, loc, message);
}
#endif

}  // namespace helios::details

// Log level macros

#ifdef HELIOS_DEBUG_MODE
#define HELIOS_DEBUG(...) \
  ::helios::Logger::GetInstance().LogMessage(::helios::LogLevel::kDebug, std::source_location::current(), __VA_ARGS__)

#define HELIOS_DEBUG_LOGGER(logger, ...)                                                                          \
  ::helios::Logger::GetInstance().LogMessage(logger, ::helios::LogLevel::kDebug, std::source_location::current(), \
                                             __VA_ARGS__)
#else
#define HELIOS_DEBUG(...) [[maybe_unused]] static constexpr auto HELIOS_ANONYMOUS_VAR(unused_debug) = 0
#define HELIOS_DEBUG_LOGGER(logger, ...) \
  [[maybe_unused]] static constexpr auto HELIOS_ANONYMOUS_VAR(unused_debug_logger) = 0
#endif

#if defined(HELIOS_DEBUG_MODE) || defined(HELIOS_RELEASE_WITH_DEBUG_INFO_MODE)
#define HELIOS_TRACE(...) \
  ::helios::Logger::GetInstance().LogMessage(::helios::LogLevel::kTrace, std::source_location::current(), __VA_ARGS__)

#define HELIOS_TRACE_LOGGER(logger, ...)                                                                          \
  ::helios::Logger::GetInstance().LogMessage(logger, ::helios::LogLevel::kTrace, std::source_location::current(), \
                                             __VA_ARGS__)
#else
#define HELIOS_TRACE(...) [[maybe_unused]] static constexpr auto HELIOS_ANONYMOUS_VAR(unused_trace) = 0
#define HELIOS_TRACE_LOGGER(logger, ...) \
  [[maybe_unused]] static constexpr auto HELIOS_ANONYMOUS_VAR(unused_trace_logger) = 0
#endif

#define HELIOS_INFO(...) \
  ::helios::Logger::GetInstance().LogMessage(::helios::LogLevel::kInfo, std::source_location::current(), __VA_ARGS__)
#define HELIOS_WARN(...) \
  ::helios::Logger::GetInstance().LogMessage(::helios::LogLevel::kWarn, std::source_location::current(), __VA_ARGS__)
#define HELIOS_ERROR(...) \
  ::helios::Logger::GetInstance().LogMessage(::helios::LogLevel::kError, std::source_location::current(), __VA_ARGS__)
#define HELIOS_CRITICAL(...)                                                                                 \
  ::helios::Logger::GetInstance().LogMessage(::helios::LogLevel::kCritical, std::source_location::current(), \
                                             __VA_ARGS__)

#define HELIOS_INFO_LOGGER(logger, ...)                                                                          \
  ::helios::Logger::GetInstance().LogMessage(logger, ::helios::LogLevel::kInfo, std::source_location::current(), \
                                             __VA_ARGS__)
#define HELIOS_WARN_LOGGER(logger, ...)                                                                          \
  ::helios::Logger::GetInstance().LogMessage(logger, ::helios::LogLevel::kWarn, std::source_location::current(), \
                                             __VA_ARGS__)
#define HELIOS_ERROR_LOGGER(logger, ...)                                                                          \
  ::helios::Logger::GetInstance().LogMessage(logger, ::helios::LogLevel::kError, std::source_location::current(), \
                                             __VA_ARGS__)
#define HELIOS_CRITICAL_LOGGER(logger, ...)                                                                          \
  ::helios::Logger::GetInstance().LogMessage(logger, ::helios::LogLevel::kCritical, std::source_location::current(), \
                                             __VA_ARGS__)
