#include <doctest/doctest.h>

#include <helios/log/config.hpp>

#include <string_view>
#include <type_traits>

using namespace helios::log;

namespace {

// Test logger types
struct TestLogger {
  static constexpr std::string_view kName = "TestLogger";
};

struct AnotherTestLogger {
  static constexpr std::string_view kName = "AnotherLogger";
};

struct LoggerWithConfig {
  static constexpr std::string_view kName = "ConfiguredLogger";
  static constexpr auto kConfig = Config::ConsoleOnly();
};

struct LoggerWithCustomConfig {
  static constexpr std::string_view kName = "CustomLogger";
  static constexpr auto kConfig = Config{.log_directory = "custom_logs",
                                         .max_file_size = 1024 * 1024,
                                         .max_files = 5,
                                         .enable_console = true,
                                         .enable_file = true,
                                         .async_logging = true};
};

// Non-logger type for negative testing
struct NotALogger {
  int value = 42;
};

struct LoggerWithoutName {
  // Missing kName
};

}  // namespace

TEST_SUITE("helios::log::Config") {
  TEST_CASE("log::Config::ctor") {
    SUBCASE("Default constructor") {
      constexpr Config config;

      CHECK_EQ(config.log_directory, "logs");
      CHECK_EQ(config.file_name_pattern, "{name}_{timestamp}.log");
      CHECK_FALSE(config.console_pattern.empty());
      CHECK_FALSE(config.file_pattern.empty());
    }

    SUBCASE("Copy constructor") {
      constexpr Config original = {.log_directory = "test_logs",
                                   .enable_console = false};
      constexpr Config copy = original;

      CHECK_EQ(copy.log_directory, "test_logs");
      CHECK_FALSE(copy.enable_console);
    }

    SUBCASE("Move constructor") {
      Config original = {.log_directory = "test_logs", .enable_console = false};
      const Config moved = std::move(original);

      CHECK_EQ(moved.log_directory, "test_logs");
      CHECK_FALSE(moved.enable_console);
    }
  }

  TEST_CASE("log::Config::operator=") {
    SUBCASE("Copy assignment") {
      constexpr Config original = {.log_directory = "test_logs",
                                   .enable_console = false};
      Config copy = {.log_directory = "logs", .enable_console = true};
      copy = original;

      CHECK_EQ(copy.log_directory, "test_logs");
      CHECK_FALSE(copy.enable_console);
    }

    SUBCASE("Move assignment") {
      Config original = {.log_directory = "test_logs", .enable_console = false};
      Config moved = {.log_directory = "logs", .enable_console = true};
      moved = std::move(original);

      CHECK_EQ(moved.log_directory, "test_logs");
      CHECK_FALSE(moved.enable_console);
    }
  }

  TEST_CASE("log::Config::Default: factory method") {
    constexpr auto config = Config::Default();

    SUBCASE("Default log directory") {
      CHECK_EQ(config.log_directory, "logs");
    }

    SUBCASE("Default file name pattern") {
      CHECK_EQ(config.file_name_pattern, "{name}_{timestamp}.log");
    }

    SUBCASE("Console patterns are non-empty") {
      CHECK_FALSE(config.console_pattern.empty());
    }

    SUBCASE("File patterns are non-empty") {
      CHECK_FALSE(config.file_pattern.empty());
    }

    SUBCASE("Default file limits are zero (unlimited)") {
      CHECK_EQ(config.max_file_size, 0);
      CHECK_EQ(config.max_files, 0);
    }

    SUBCASE("Default auto flush level is Warn") {
      CHECK_EQ(config.auto_flush_level, Level::kWarn);
    }

    SUBCASE("Console is enabled by default") {
      CHECK(config.enable_console);
    }

    SUBCASE("File is enabled by default") {
      CHECK(config.enable_file);
    }

    SUBCASE("Truncate files is enabled by default") {
      CHECK(config.truncate_files);
    }

    SUBCASE("Async logging is disabled by default") {
      CHECK_FALSE(config.async_logging);
    }
  }

  TEST_CASE("log::Config::ConsoleOnly: factory method") {
    constexpr auto config = Config::ConsoleOnly();

    SUBCASE("Console is enabled") {
      CHECK(config.enable_console);
    }

    SUBCASE("File is disabled") {
      CHECK_FALSE(config.enable_file);
    }
  }

  TEST_CASE("log::Config::FileOnly: factory method") {
    constexpr auto config = Config::FileOnly();

    SUBCASE("Console is disabled") {
      CHECK_FALSE(config.enable_console);
    }

    SUBCASE("File is enabled") {
      CHECK(config.enable_file);
    }
  }

  TEST_CASE("log::Config::Debug: factory method") {
    constexpr auto config = Config::Debug();

    SUBCASE("Console is enabled") {
      CHECK(config.enable_console);
    }

    SUBCASE("File is enabled") {
      CHECK(config.enable_file);
    }

    SUBCASE("Async logging is disabled for debugging") {
      CHECK_FALSE(config.async_logging);
    }
  }

  TEST_CASE("log::Config::Release: factory method") {
    constexpr auto config = Config::Release();

    SUBCASE("Console is disabled") {
      CHECK_FALSE(config.enable_console);
    }

    SUBCASE("File is enabled") {
      CHECK(config.enable_file);
    }

    SUBCASE("Async logging is enabled for performance") {
      CHECK(config.async_logging);
    }
  }
}

TEST_SUITE("helios::log::LoggerTrait") {
  TEST_CASE("log::LoggerTrait::concept") {
    SUBCASE("Valid logger type satisfies LoggerTrait") {
      CHECK(LoggerTrait<TestLogger>);
      CHECK(LoggerTrait<AnotherTestLogger>);
      CHECK(LoggerTrait<LoggerWithConfig>);
      CHECK(LoggerTrait<LoggerWithCustomConfig>);
    }

    SUBCASE("Non-logger types do not satisfy LoggerTrait") {
      CHECK_FALSE(LoggerTrait<NotALogger>);
      CHECK_FALSE(LoggerTrait<LoggerWithoutName>);
      CHECK_FALSE(LoggerTrait<int>);
      CHECK_FALSE(LoggerTrait<std::string_view>);
    }
  }
}

TEST_SUITE("helios::log::LoggerWithConfigTrait") {
  TEST_CASE("log::LoggerWithConfigTrait::concept") {
    SUBCASE("Logger with GetConfig() satisfies LoggerWithConfigTrait") {
      CHECK(LoggerWithConfigTrait<LoggerWithConfig>);
      CHECK(LoggerWithConfigTrait<LoggerWithCustomConfig>);
    }

    SUBCASE(
        "Logger without GetConfig() doesn't satisfy LoggerWithConfigTrait") {
      CHECK_FALSE(LoggerWithConfigTrait<TestLogger>);
      CHECK_FALSE(LoggerWithConfigTrait<AnotherTestLogger>);
    }
  }
}

TEST_SUITE("helios::log::LoggerNameOf") {
  TEST_CASE("log::LoggerNameOf: name retrieval") {
    SUBCASE("Returns correct name for TestLogger") {
      constexpr std::string_view name = LoggerNameOf<TestLogger>();
      CHECK_EQ(name, "TestLogger");
    }

    SUBCASE("Returns correct name for AnotherTestLogger") {
      constexpr std::string_view name = LoggerNameOf<AnotherTestLogger>();
      CHECK_EQ(name, "AnotherLogger");
    }

    SUBCASE("Name works with instance parameter") {
      constexpr TestLogger logger;
      const std::string_view name = LoggerNameOf(logger);
      CHECK_EQ(name, "TestLogger");
    }

    SUBCASE("Name is constexpr") {
      constexpr std::string_view name = LoggerNameOf<TestLogger>();
      static_assert(name == "TestLogger", "Logger name should match");
      CHECK_EQ(name, "TestLogger");
    }
  }
}

TEST_SUITE("helios::log::LoggerConfigOf") {
  TEST_CASE("log::LoggerConfigOf: config retrieval") {
    SUBCASE("Logger without Config() returns Default") {
      constexpr Config config = LoggerConfigOf<TestLogger>();
      constexpr auto default_config = Config::Default();

      CHECK_EQ(config.enable_console, default_config.enable_console);
      CHECK_EQ(config.enable_file, default_config.enable_file);
      CHECK_EQ(config.async_logging, default_config.async_logging);
    }

    SUBCASE("Logger with Config() returns custom config") {
      constexpr Config config = LoggerConfigOf<LoggerWithConfig>();
      constexpr auto console_only = Config::ConsoleOnly();

      CHECK_EQ(config.enable_console, console_only.enable_console);
      CHECK_EQ(config.enable_file, console_only.enable_file);
    }

    SUBCASE("Logger with custom Config() returns correct values") {
      constexpr Config config = LoggerConfigOf<LoggerWithCustomConfig>();

      CHECK_EQ(config.log_directory, "custom_logs");
      CHECK_EQ(config.max_file_size, 1024 * 1024);
      CHECK_EQ(config.max_files, 5);
      CHECK(config.enable_console);
      CHECK(config.enable_file);
      CHECK(config.async_logging);
    }

    SUBCASE("Config works with instance parameter") {
      constexpr TestLogger logger;

      constexpr Config config = LoggerConfigOf(logger);
      constexpr auto default_config = Config::Default();

      CHECK_EQ(config.enable_console, default_config.enable_console);
    }
  }
}
