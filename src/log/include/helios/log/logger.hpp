#pragma once

#include <helios/log/config.hpp>

#include <format>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <source_location>
#include <string>
#include <string_view>
#include <unordered_map>

namespace spdlog {

// Forward declaration for spdlog::logger
class logger;

}  // namespace spdlog

namespace helios::log {

/// @brief Default logger type.
struct DefaultLogger {
  static constexpr std::string_view kName = "HELIOS";
#ifdef HELIOS_RELEASE_MODE
  static constexpr auto kConfig = Config::Release();
#else
  static constexpr auto kConfig = Config::Debug();
#endif
};

/// @brief Constexpr instance of the default logger for easier user interface.
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
   * @brief Gets the singleton instance.
   * @return Reference to the Logger instance
   */
  [[nodiscard]] static Logger& Instance() noexcept {
    static Logger instance;
    return instance;
  }

  /**
   * @brief Adds a logger with the specified type and configuration.
   * @tparam T Logger type
   * @param logger Logger type instance
   * @param config Configuration for the logger
   */
  template <LoggerTrait T>
  void AddLogger(T logger, Config config = LoggerConfigOf<T>()) noexcept;

  /**
   * @brief Removes a logger with the given type.
   * @note Cannot remove the default logger
   * @tparam T Logger type
   * @param logger Logger type instance
   */
  template <LoggerTrait T>
  void RemoveLogger(T logger = {}) noexcept;

  /// @brief Flushes all registered loggers.
  void FlushAll() noexcept;

  /**
   * @brief Flushes a specific logger.
   * @tparam T Logger type
   * @param logger Logger type instance
   */
  template <LoggerTrait T>
  void Flush(T /*logger*/ = {}) noexcept {
    FlushImpl(LoggerTypeIndex::From<T>());
  }

  /**
   * @brief Logs a string message with typed logger.
   * @tparam T Logger type
   * @param logger Logger type instance
   * @param level Log severity level
   * @param message Message to log
   */
  template <LoggerTrait T>
  void Log(T logger, Level level, std::string_view message) noexcept;

  /**
   * @brief Logs a formatted message with typed logger.
   * @tparam T Logger type
   * @tparam Args Types of the format arguments
   * @param logger Logger type instance
   * @param level Log severity level
   * @param fmt Format string
   * @param args Arguments for the format string
   */
  template <LoggerTrait T, typename... Args>
    requires(sizeof...(Args) > 0)
  void Log(T logger, Level level, std::format_string<Args...> fmt,
           Args&&... args) noexcept;

  /**
   * @brief Logs a string message with default logger.
   * @param level Log severity level
   * @param message Message to log
   */
  void Log(Level level, std::string_view message) noexcept;

  /**
   * @brief Logs a formatted message with default logger.
   * @tparam Args Types of the format arguments
   * @param level Log severity level
   * @param fmt Format string
   * @param args Arguments for the format string
   */
  template <typename... Args>
    requires(sizeof...(Args) > 0)
  void Log(Level level, std::format_string<Args...> fmt,
           Args&&... args) noexcept;

  /**
   * @brief Sets the global default configuration for new loggers.
   * @param config The configuration to use as default
   */
  void SetDefaultConfig(const Config& config) noexcept {
    default_config_ = config;
  }

  /**
   * @brief Sets the minimum log level for a typed logger.
   * @tparam T Logger type
   * @param logger Logger type instance
   * @param level Minimum log level to set
   */
  template <LoggerTrait T>
  void SetLevel(T /*logger*/, Level level) noexcept {
    SetLevelImpl(LoggerTypeIndex::From<T>(), level);
  }

  /**
   * @brief Sets the minimum log level for the default logger.
   * @param level Minimum log level to set
   */
  void SetLevel(Level level) noexcept;

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
  [[nodiscard]] bool ShouldLog(Level level) const noexcept;

  /**
   * @brief Checks if a typed logger should log messages at the given level.
   * @tparam T Logger type
   * @param logger Logger type instance
   * @param level The log level to check
   * @return True if messages at this level should be logged
   */
  template <LoggerTrait T>
  [[nodiscard]] bool ShouldLog(T /*logger*/, Level level) const noexcept {
    return ShouldLogImpl(LoggerTypeIndex::From<T>(), level);
  }

  /**
   * @brief Gets the current log level for a typed logger.
   * @tparam T Logger type
   * @param logger Logger type instance
   * @return The current log level, or `Level::kTrace` if logger doesn't exist
   */
  template <LoggerTrait T>
  [[nodiscard]] Level GetLevel(T /*logger*/ = {}) const noexcept {
    return GetLevelImpl(LoggerTypeIndex::From<T>());
  }

  /**
   * @brief Gets the current log level for the default logger.
   * @return The current log level
   */
  [[nodiscard]] Level GetLevel() const noexcept;

  /**
   * @brief Gets the current default configuration.
   * @return The default logger configuration
   */
  [[nodiscard]] const Config& GetDefaultConfig() const noexcept {
    return default_config_;
  }

private:
  Logger() noexcept;

  [[nodiscard]] auto GetLogger(LoggerTypeIndex logger_index) const noexcept
      -> std::shared_ptr<spdlog::logger>;
  [[nodiscard]] auto GetDefaultLogger() const noexcept
      -> std::shared_ptr<spdlog::logger> {
    return GetLogger(LoggerTypeIndex::From<DefaultLogger>());
  }

  /**
   * @brief Flushes a specific logger by index (internal).
   * @param logger_index Logger index
   */
  void FlushImpl(LoggerTypeIndex logger_index) noexcept;

  /**
   * @brief Sets the minimum log level for a logger by index (internal).
   * @param logger_index Logger index
   * @param level Minimum log level to set
   */
  void SetLevelImpl(LoggerTypeIndex logger_index, Level level) noexcept;

  /**
   * @brief Checks if a logger by index should log messages at the given level
   * (internal).
   * @param logger_index Logger index
   * @param level The log level to check
   * @return True if messages at this level should be logged
   */
  [[nodiscard]] bool ShouldLogImpl(LoggerTypeIndex logger_index,
                                   Level level) const noexcept;

  /**
   * @brief Gets the current log level for a logger by index (internal).
   * @param logger_index Logger index
   * @return The current log level, or `Level::kTrace` if logger doesn't exist
   */
  [[nodiscard]] Level GetLevelImpl(LoggerTypeIndex logger_index) const noexcept;

  static void LogMessageImpl(const std::shared_ptr<spdlog::logger>& logger,
                             Level level, std::string_view message) noexcept;

  [[nodiscard]] auto CreateLogger(std::string_view logger_name,
                                  const Config& config) noexcept
      -> std::shared_ptr<spdlog::logger>;

  static void DropLoggerFromSpdlog(
      const std::shared_ptr<spdlog::logger>& logger) noexcept;

  std::unordered_map<LoggerTypeIndex, std::shared_ptr<spdlog::logger>> loggers_;
  std::unordered_map<LoggerTypeIndex, Config> logger_configs_;
  std::unordered_map<LoggerTypeIndex, Level> logger_levels_;
  mutable std::shared_mutex loggers_mutex_;

  Config default_config_;
};

inline Logger::Logger() noexcept {
  default_config_ = LoggerConfigOf<DefaultLogger>();

  constexpr LoggerTypeIndex default_id = LoggerTypeIndex::From<DefaultLogger>();
  constexpr std::string_view default_name = LoggerNameOf<DefaultLogger>();

  if (auto logger = CreateLogger(default_name, default_config_)) [[likely]] {
    loggers_.emplace(default_id, std::move(logger));
    logger_configs_.emplace(default_id, default_config_);
    logger_levels_.emplace(default_id, Level::kTrace);
  }
}

template <LoggerTrait T>
inline void Logger::AddLogger(T /*logger*/, Config config) noexcept {
  constexpr LoggerTypeIndex logger_index = LoggerTypeIndex::From<T>();
  constexpr std::string_view logger_name = LoggerNameOf<T>();

  const std::scoped_lock lock(loggers_mutex_);
  if (loggers_.contains(logger_index)) [[unlikely]] {
    return;
  }

  if (auto spdlog_logger = CreateLogger(logger_name, config)) [[likely]] {
    loggers_.emplace(logger_index, std::move(spdlog_logger));
    logger_configs_.emplace(logger_index, std::move(config));
    logger_levels_.emplace(logger_index, Level::kTrace);
  }
}

template <LoggerTrait T>
inline void Logger::RemoveLogger(T /*logger*/) noexcept {
  constexpr LoggerTypeIndex logger_index = LoggerTypeIndex::From<T>();

  // Cannot remove the default logger
  if (logger_index == LoggerTypeIndex::From<DefaultLogger>()) [[unlikely]] {
    return;
  }

  const std::scoped_lock lock(loggers_mutex_);
  if (const auto it = loggers_.find(logger_index); it != loggers_.end()) {
    // Unregister from spdlog before erasing
    DropLoggerFromSpdlog(it->second);
    loggers_.erase(it);
    logger_configs_.erase(logger_index);
    logger_levels_.erase(logger_index);
  }
}

template <LoggerTrait T>
inline void Logger::Log(T /*logger*/, Level level,
                        std::string_view message) noexcept {
  if (auto spdlog_logger = GetLogger(LoggerTypeIndex::From<T>())) [[likely]] {
    LogMessageImpl(spdlog_logger, level, message);
  }
}

template <LoggerTrait T, typename... Args>
  requires(sizeof...(Args) > 0)
inline void Logger::Log(T logger, Level level, std::format_string<Args...> fmt,
                        Args&&... args) noexcept {
  try {
    const std::string message = std::format(fmt, std::forward<Args>(args)...);
    Log(logger, level, message);
  } catch (...) {
    // Silently ignore formatting errors
  }
}

inline void Logger::Log(Level level, std::string_view message) noexcept {
  if (auto logger = GetDefaultLogger()) [[likely]] {
    LogMessageImpl(logger, level, message);
  }
}

template <typename... Args>
  requires(sizeof...(Args) > 0)
inline void Logger::Log(Level level, std::format_string<Args...> fmt,
                        Args&&... args) noexcept {
  try {
    const std::string message = std::format(fmt, std::forward<Args>(args)...);
    Log(level, message);
  } catch (...) {
    // Silently ignore formatting errors
  }
}

template <LoggerTrait T>
inline bool Logger::HasLogger(T /*logger*/) const noexcept {
  const std::shared_lock lock(loggers_mutex_);
  return loggers_.contains(LoggerTypeIndex::From<T>());
}

#if defined(HELIOS_DEBUG_MODE) || defined(HELIOS_RELEASE_WITH_DEBUG_INFO_MODE)
/**
 * @brief Logs a debug message with the default logger.
 * @param message The message to log
 */
inline void Debug(std::string_view message) noexcept {
  Logger::Instance().Log(Level::kDebug, message);
}

template <typename... Args>
  requires(sizeof...(Args) > 0)
/**
 * @brief Logs a formatted debug message with the default logger.
 * @tparam Args Types of the format arguments
 * @param fmt Format string
 * @param args Arguments for the format string
 */
inline void Debug(std::format_string<Args...> fmt, Args&&... args) noexcept {
  Logger::Instance().Log(Level::kDebug, fmt, std::forward<Args>(args)...);
}

/**
 * @brief Logs a debug message with the specified logger.
 * @tparam T Logger type
 * @param logger Logger type instance
 * @param message The message to log
 */
template <LoggerTrait T>
inline void Debug(T logger, std::string_view message) noexcept {
  Logger::Instance().Log(logger, Level::kDebug, message);
}

/**
 * @brief Logs a formatted debug message with the specified logger.
 * @tparam T Logger type
 * @tparam Args Types of the format arguments
 * @param logger Logger type instance
 * @param fmt Format string
 * @param args Arguments for the format string
 */
template <LoggerTrait T, typename... Args>
  requires(sizeof...(Args) > 0)
inline void Debug(T logger, std::format_string<Args...> fmt,
                  Args&&... args) noexcept {
  Logger::Instance().Log(logger, Level::kDebug, fmt,
                         std::forward<Args>(args)...);
}

/**
 * @brief Logs a trace message with the default logger.
 * @param message The message to log
 */
inline void Trace(std::string_view message) noexcept {
  Logger::Instance().Log(Level::kTrace, message);
}

/**
 * @brief Logs a formatted trace message with the default logger.
 * @tparam Args Types of the format arguments
 * @param fmt Format string
 * @param args Arguments for the format string
 */
template <typename... Args>
  requires(sizeof...(Args) > 0)
inline void Trace(std::format_string<Args...> fmt, Args&&... args) noexcept {
  Logger::Instance().Log(Level::kTrace, fmt, std::forward<Args>(args)...);
}

/**
 * @brief Logs a trace message with the specified logger.
 * @tparam T Logger type
 * @param logger Logger type instance
 * @param message The message to log
 */
template <LoggerTrait T>
inline void Trace(T logger, std::string_view message) noexcept {
  Logger::Instance().Log(logger, Level::kTrace, message);
}

/**
 * @brief Logs a formatted trace message with the specified logger.
 * @tparam T Logger type
 * @tparam Args Types of the format arguments
 * @param logger Logger type instance
 * @param fmt Format string
 * @param args Arguments for the format string
 */
template <LoggerTrait T, typename... Args>
  requires(sizeof...(Args) > 0)
inline void Trace(T logger, std::format_string<Args...> fmt,
                  Args&&... args) noexcept {
  Logger::Instance().Log(logger, Level::kTrace, fmt,
                         std::forward<Args>(args)...);
}
#else
inline void Trace(std::string_view /*message*/) noexcept {}

template <typename... Args>
inline void Trace(Args&&... /*args*/) noexcept {}

template <LoggerTrait T>
inline void Trace(T /*logger*/, std::string_view /*message*/) noexcept {}

template <LoggerTrait T, typename... Args>
inline void Trace(T /*logger*/, Args&&... /*args*/) noexcept {}

inline void Debug(std::string_view /*message*/) noexcept {}

template <typename... Args>
inline void Debug(Args&&... /*args*/) noexcept {}

template <LoggerTrait T>
inline void Debug(T /*logger*/, std::string_view /*message*/) noexcept {}

template <LoggerTrait T, typename... Args>
inline void Debug(T /*logger*/, Args&&... /*args*/) noexcept {}
#endif

/**
 * @brief Logs an info message with the default logger.
 * @param message The message to log
 */
inline void Info(std::string_view message) noexcept {
  Logger::Instance().Log(Level::kInfo, message);
}

/**
 * @brief Logs a formatted info message with the default logger.
 * @tparam Args Types of the format arguments
 * @param fmt Format string
 * @param args Arguments for the format string
 */
template <typename... Args>
  requires(sizeof...(Args) > 0)
inline void Info(std::format_string<Args...> fmt, Args&&... args) noexcept {
  Logger::Instance().Log(Level::kInfo, fmt, std::forward<Args>(args)...);
}

/**
 * @brief Logs an info message with the specified logger.
 * @tparam T Logger type
 * @param logger Logger type instance
 * @param message The message to log
 */
template <LoggerTrait T>
inline void Info(T logger, std::string_view message) noexcept {
  Logger::Instance().Log(logger, Level::kInfo, message);
}

/**
 * @brief Logs a formatted info message with the specified logger.
 * @tparam T Logger type
 * @tparam Args Types of the format arguments
 * @param logger Logger type instance
 * @param fmt Format string
 * @param args Arguments for the format string
 */
template <LoggerTrait T, typename... Args>
  requires(sizeof...(Args) > 0)
inline void Info(T logger, std::format_string<Args...> fmt,
                 Args&&... args) noexcept {
  Logger::Instance().Log(logger, Level::kInfo, fmt,
                         std::forward<Args>(args)...);
}

/**
 * @brief Logs a warning message with the default logger.
 * @param message The message to log
 */
inline void Warn(std::string_view message) noexcept {
  Logger::Instance().Log(Level::kWarn, message);
}

/**
 * @brief Logs a formatted warning message with the default logger.
 * @tparam Args Types of the format arguments
 * @param fmt Format string
 * @param args Arguments for the format string
 */
template <typename... Args>
  requires(sizeof...(Args) > 0)
inline void Warn(std::format_string<Args...> fmt, Args&&... args) noexcept {
  Logger::Instance().Log(Level::kWarn, fmt, std::forward<Args>(args)...);
}

/**
 * @brief Logs a warning message with the specified logger.
 * @tparam T Logger type
 * @param logger Logger type instance
 * @param message The message to log
 */
template <LoggerTrait T>
inline void Warn(T logger, std::string_view message) noexcept {
  Logger::Instance().Log(logger, Level::kWarn, message);
}

/**
 * @brief Logs a formatted warning message with the specified logger.
 * @tparam T Logger type
 * @tparam Args Types of the format arguments
 * @param logger Logger type instance
 * @param fmt Format string
 * @param args Arguments for the format string
 */
template <LoggerTrait T, typename... Args>
  requires(sizeof...(Args) > 0)
inline void Warn(T logger, std::format_string<Args...> fmt,
                 Args&&... args) noexcept {
  Logger::Instance().Log(logger, Level::kWarn, fmt,
                         std::forward<Args>(args)...);
}

/**
 * @brief Logs an error message with the default logger.
 * @param message The message to log
 */
inline void Error(std::string_view message) noexcept {
  Logger::Instance().Log(Level::kError, message);
}

/**
 * @brief Logs a formatted error message with the default logger.
 * @tparam Args Types of the format arguments
 * @param fmt Format string
 * @param args Arguments for the format string
 */
template <typename... Args>
  requires(sizeof...(Args) > 0)
inline void Error(std::format_string<Args...> fmt, Args&&... args) noexcept {
  Logger::Instance().Log(Level::kError, fmt, std::forward<Args>(args)...);
}

/**
 * @brief Logs an error message with the specified logger.
 * @tparam T Logger type
 * @param logger Logger type instance
 * @param message The message to log
 */
template <LoggerTrait T>
inline void Error(T logger, std::string_view message) noexcept {
  Logger::Instance().Log(logger, Level::kError, message);
}

/**
 * @brief Logs a formatted error message with the specified logger.
 * @tparam T Logger type
 * @tparam Args Types of the format arguments
 * @param logger Logger type instance
 * @param fmt Format string
 * @param args Arguments for the format string
 */
template <LoggerTrait T, typename... Args>
  requires(sizeof...(Args) > 0)
inline void Error(T logger, std::format_string<Args...> fmt,
                  Args&&... args) noexcept {
  Logger::Instance().Log(logger, Level::kError, fmt,
                         std::forward<Args>(args)...);
}

/**
 * @brief Logs a critical message with the default logger.
 * @param message The message to log
 */
inline void Critical(std::string_view message) noexcept {
  Logger::Instance().Log(Level::kCritical, message);
}

/**
 * @brief Logs a formatted critical message with the default logger.
 * @tparam Args Types of the format arguments
 * @param fmt Format string
 * @param args Arguments for the format string
 */
template <typename... Args>
  requires(sizeof...(Args) > 0)
inline void Critical(std::format_string<Args...> fmt, Args&&... args) noexcept {
  Logger::Instance().Log(Level::kCritical, fmt, std::forward<Args>(args)...);
}

/**
 * @brief Logs a critical message with the specified logger.
 * @tparam T Logger type
 * @param logger Logger type instance
 * @param message The message to log
 */
template <LoggerTrait T>
inline void Critical(T logger, std::string_view message) noexcept {
  Logger::Instance().Log(logger, Level::kCritical, message);
}

/**
 * @brief Logs a formatted critical message with the specified logger.
 * @tparam T Logger type
 * @tparam Args Types of the format arguments
 * @param logger Logger type instance
 * @param fmt Format string
 * @param args Arguments for the format string
 */
template <LoggerTrait T, typename... Args>
  requires(sizeof...(Args) > 0)
inline void Critical(T logger, std::format_string<Args...> fmt,
                     Args&&... args) noexcept {
  Logger::Instance().Log(logger, Level::kCritical, fmt,
                         std::forward<Args>(args)...);
}

}  // namespace helios::log

// This provides the assertion handler integration that the utils plugin's
// assert.hpp uses. Priority order for assertion handling:
//   1. Custom user handler (set via helios::SetAssertionHandler)
//   2. Log plugin handler (this - when log plugin is linked)
//   3. Default handler (printf/println to stderr)
//
// The actual symbol definitions are in logger.cpp to properly override
// the weak symbols from assert.cpp. This header only declares the
// implementation function used by logger.cpp.

namespace helios::log::details {

/**
 * @brief Log plugin assertion handler implementation.
 * @details Logs the assertion failure via the log system with critical level.
 * This is called by the weak symbol overrides in logger.cpp.
 */
void LogAssertionViaLogger(std::string_view condition,
                           const std::source_location& loc,
                           std::string_view message) noexcept;

}  // namespace helios::log::details
