#pragma once

#include <helios/utils/type_info.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace helios::log {

/// @brief Log severity levels.
enum class Level : uint8_t {
  kTrace = 0,
  kDebug = 1,
  kInfo = 2,
  kWarn = 3,
  kError = 4,
  kCritical = 5
};

/// @brief Configuration for logger behavior and output.
struct Config {
  std::string_view log_directory = "logs";  ///< Log output directory path.
  std::string_view file_name_pattern =
      "{name}_{timestamp}.log";  ///< Pattern for log file names (supports
                                 ///< format placeholders: {name}, {timestamp}).
  std::string_view console_pattern =
      "[%H:%M:%S.%e] [%t] [%^%l%$] %n: %v";  ///< Console log pattern.
  std::string_view file_pattern =
      "[%Y-%m-%d %H:%M:%S.%e] [%t] [%l] %n: %v";  ///< File log pattern.

  size_t max_file_size =
      0;  ///< Maximum size of a single log file in bytes (0 = no limit).
  size_t max_files =
      0;  ///< Maximum number of log files to keep (0 = no limit).
  Level auto_flush_level =
      Level::kWarn;  ///< Minimum log level to flush automatically.

  bool enable_console = true;  ///< Enable console output.
  bool enable_file = true;     ///< Enable file output.
  bool truncate_files = true;  ///< Enable truncation of existing log files.
  bool async_logging = false;  ///< Enable async logging (better performance but
                               ///< may lose last logs on crash).
  size_t async_queue_size =
      8192;  ///< Queue size for async logging (only used when async_logging).
  size_t async_thread_count =
      1;  ///< Background thread count for async logging.

  /**
   * @brief Creates default configuration.
   * @return Default `Config` instance
   */
  [[nodiscard]] static constexpr Config Default() noexcept { return {}; }

  /**
   * @brief Creates configuration for console-only output.
   * @return `Config` instance for console-only logging
   */
  [[nodiscard]] static constexpr Config ConsoleOnly() noexcept {
    return {.enable_console = true, .enable_file = false};
  }

  /**
   * @brief Creates configuration for file-only output.
   * @return `Config` instance for file-only logging
   */
  [[nodiscard]] static constexpr Config FileOnly() noexcept {
    return {.enable_console = false, .enable_file = true};
  }

  /**
   * @brief Creates configuration optimized for debug builds.
   * @return `Config` instance for debug logging
   */
  [[nodiscard]] static constexpr Config Debug() noexcept {
    return {.auto_flush_level = Level::kTrace,
            .enable_console = true,
            .enable_file = true,
            .async_logging = false};
  }

  /**
   * @brief Creates configuration optimized for release builds.
   * @return `Config` instance for release logging
   */
  [[nodiscard]] static constexpr Config Release() noexcept {
    return {
        .enable_console = false, .enable_file = true, .async_logging = true};
  }
};

/// @brief Type alias for logger type IDs.
using LoggerTypeId = utils::TypeId;

/// @brief Type alias for logger type indices.
using LoggerTypeIndex = utils::TypeIndex;

/**
 * @brief Trait to identify valid logger types.
 * @details A logger type must be an object, be empty and provide:
 * - `static constexpr std::string_view kName` variable
 * @tparam T Type to check
 *
 * @code
 * struct MyLogger {
 *   static constexpr std::string_view kName = return "MyLogger";
 * };
 *
 * static_assert(LoggerTrait<MyLogger>);
 * @endcode
 */
template <typename T>
concept LoggerTrait = std::is_object_v<std::remove_cvref_t<T>> &&
                      std::is_empty_v<std::remove_cvref_t<T>> && requires {
                        {
                          std::remove_cvref_t<T>::kName
                        } -> std::convertible_to<std::string_view>;
                      };

/**
 * @brief Trait to identify loggers with custom configuration.
 * @details A logger with config trait must satisfy `LoggerTrait` and provide:
 * - `static Config kConfig` variable
 * @tparam T Type to check
 *
 * @code
 * struct MyLogger {
 *   static constexpr std::string_view kName = "MyLogger";
 *   static constexpr auto kConfig = Config::Default();
 * };
 *
 * static_assert(LoggerWithConfigTrait<MyLogger>);
 * @endcode
 */
template <typename T>
concept LoggerWithConfigTrait = LoggerTrait<T> && requires {
  { std::remove_cvref_t<T>::kConfig } -> std::convertible_to<Config>;
};

/**
 * @brief Gets the name of a logger type.
 * @details Returns the name provided by `kName` variable.
 * @tparam T Logger type
 * @param logger logger instance
 * @return Name of the logger
 *
 * @code
 * struct MyLogger {
 *   static constexpr std::string_view kName = "MyLogger";
 * };
 *
 * constexpr auto name = LoggerNameOf<MyLogger>();  // "MyLogger"
 * @endcode
 */
template <LoggerTrait T>
[[nodiscard]] constexpr std::string_view LoggerNameOf(
    T /*logger*/ = {}) noexcept {
  return std::remove_cvref_t<T>::kName;
}

/**
 * @brief Gets the configuration for a logger type.
 * @details Returns the config provided by `kConfig` variable if available,
 * otherwise returns the default configuration.
 * @tparam T Logger type
 * @param logger logger instance
 * @return `Config` for the logger
 *
 * @code
 * struct MyLogger {
 *   static constexpr std::string_view kName = "MyLogger";
 *   static constexpr auto kConfig = Config::ConsoleOnly();
 * };
 *
 * constexpr auto config = LoggerConfigOf<MyLogger>();
 * @endcode
 */
template <LoggerTrait T>
[[nodiscard]] constexpr Config LoggerConfigOf(T /*logger*/ = {}) noexcept {
  if constexpr (LoggerWithConfigTrait<T>) {
    return std::remove_cvref_t<T>::kConfig;
  } else {
    return Config::Default();
  }
}

}  // namespace helios::log
