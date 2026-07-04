#include <doctest/doctest.h>

#include <helios/log/logger.hpp>

#include <source_location>
#include <string>
#include <string_view>

using namespace helios::log;

namespace {

// Custom logger types for testing
struct TestLogger {
  static constexpr std::string_view kName = "TestLogger";
};

struct TestLoggerWithConfig {
  static constexpr std::string_view kName = "TestLoggerWithConfig";
  static constexpr auto kConfig = Config::ConsoleOnly();
};

struct AnotherTestLogger {
  static constexpr std::string_view kName = "AnotherTestLogger";
  static constexpr auto kConfig = Config::FileOnly();
};

struct DebugTestLogger {
  static constexpr std::string_view kName = "DebugTestLogger";
  static constexpr auto kConfig = Config::Debug();
};

struct ReleaseTestLogger {
  static constexpr std::string_view kName = "ReleaseTestLogger";
  static constexpr auto kConfig = Config::Release();
};

}  // namespace

TEST_SUITE("helios::log::Logger") {
  TEST_CASE("log::Logger::GetInstance: singleton pattern") {
    SUBCASE("Returns reference to same instance") {
      auto& instance1 = Logger::Instance();
      auto& instance2 = Logger::Instance();
      CHECK_EQ(&instance1, &instance2);
    }
  }

  TEST_CASE("log::Logger::HasLogger: logger existence check") {
    auto& logger = Logger::Instance();

    SUBCASE("Default logger always exists") {
      CHECK(logger.HasLogger<DefaultLogger>());
    }

    SUBCASE("Custom logger doesn't exist until added") {
      // Remove if exists from previous tests
      logger.RemoveLogger<TestLogger>();
      CHECK_FALSE(logger.HasLogger<TestLogger>());
    }
  }

  TEST_CASE("log::Logger::AddLogger: adding custom loggers") {
    auto& logger = Logger::Instance();

    SUBCASE("Add new logger") {
      logger.RemoveLogger<TestLogger>();  // Ensure clean state
      CHECK_FALSE(logger.HasLogger<TestLogger>());

      logger.AddLogger(TestLogger{});
      CHECK(logger.HasLogger<TestLogger>());

      // Cleanup
      logger.RemoveLogger<TestLogger>();
    }

    SUBCASE("Add logger with custom config") {
      logger.RemoveLogger<TestLoggerWithConfig>();
      CHECK_FALSE(logger.HasLogger<TestLoggerWithConfig>());

      logger.AddLogger(TestLoggerWithConfig{}, Config::Debug());
      CHECK(logger.HasLogger<TestLoggerWithConfig>());

      // Cleanup
      logger.RemoveLogger<TestLoggerWithConfig>();
    }

    SUBCASE("Adding same logger twice is no-op") {
      logger.RemoveLogger<TestLogger>();
      logger.AddLogger(TestLogger{});
      CHECK(logger.HasLogger<TestLogger>());

      // Adding again should not throw or cause issues
      logger.AddLogger(TestLogger{});
      CHECK(logger.HasLogger<TestLogger>());

      // Cleanup
      logger.RemoveLogger<TestLogger>();
    }
  }

  TEST_CASE("log::Logger::RemoveLogger: removing loggers") {
    auto& logger = Logger::Instance();

    SUBCASE("Remove existing logger") {
      logger.AddLogger(TestLogger{});
      CHECK(logger.HasLogger<TestLogger>());

      logger.RemoveLogger<TestLogger>();
      CHECK_FALSE(logger.HasLogger<TestLogger>());
    }

    SUBCASE("Removing non-existent logger is no-op") {
      logger.RemoveLogger<TestLogger>();  // Ensure it doesn't exist
      CHECK_NOTHROW(logger.RemoveLogger<TestLogger>());
      CHECK_FALSE(logger.HasLogger<TestLogger>());
    }

    SUBCASE("Cannot remove default logger") {
      CHECK(logger.HasLogger<DefaultLogger>());
      logger.RemoveLogger<DefaultLogger>();
      // Default logger should still exist
      CHECK(logger.HasLogger<DefaultLogger>());
    }
  }

  TEST_CASE("log::Logger::SetLevel: log level management") {
    auto& logger = Logger::Instance();

    SUBCASE("Set level for default logger") {
      const auto original_level = logger.GetLevel();

      logger.SetLevel(Level::kWarn);
      CHECK_EQ(logger.GetLevel(), Level::kWarn);

      logger.SetLevel(Level::kDebug);
      CHECK_EQ(logger.GetLevel(), Level::kDebug);

      // Restore original
      logger.SetLevel(original_level);
    }

    SUBCASE("Set level for custom logger") {
      logger.AddLogger(TestLogger{});

      logger.SetLevel(TestLogger{}, Level::kError);
      CHECK_EQ(logger.GetLevel<TestLogger>(), Level::kError);

      logger.SetLevel(TestLogger{}, Level::kTrace);
      CHECK_EQ(logger.GetLevel<TestLogger>(), Level::kTrace);

      // Cleanup
      logger.RemoveLogger<TestLogger>();
    }
  }

  TEST_CASE("log::Logger::ShouldLog: log level filtering") {
    auto& logger = Logger::Instance();

    SUBCASE("ShouldLog respects level for default logger") {
      const auto original_level = logger.GetLevel();

      logger.SetLevel(Level::kWarn);

      CHECK_FALSE(logger.ShouldLog(Level::kTrace));
      CHECK_FALSE(logger.ShouldLog(Level::kDebug));
      CHECK_FALSE(logger.ShouldLog(Level::kInfo));
      CHECK(logger.ShouldLog(Level::kWarn));
      CHECK(logger.ShouldLog(Level::kError));
      CHECK(logger.ShouldLog(Level::kCritical));

      // Restore original
      logger.SetLevel(original_level);
    }

    SUBCASE("ShouldLog respects level for custom logger") {
      logger.AddLogger(TestLogger{});
      logger.SetLevel(TestLogger{}, Level::kError);

      CHECK_FALSE(logger.ShouldLog(TestLogger{}, Level::kTrace));
      CHECK_FALSE(logger.ShouldLog(TestLogger{}, Level::kWarn));
      CHECK(logger.ShouldLog(TestLogger{}, Level::kError));
      CHECK(logger.ShouldLog(TestLogger{}, Level::kCritical));

      // Cleanup
      logger.RemoveLogger<TestLogger>();
    }
  }

  TEST_CASE("log::Logger::Log: logging messages") {
    auto& logger = Logger::Instance();

    SUBCASE("Log string message with default logger") {
      CHECK_NOTHROW(logger.Log(Level::kInfo, "Test message"));
    }

    SUBCASE("Log formatted message with default logger") {
      CHECK_NOTHROW(logger.Log(Level::kInfo, "Value: {}", 42));
    }

    SUBCASE("Log string message with typed logger") {
      logger.AddLogger(TestLogger{});
      CHECK_NOTHROW(logger.Log(TestLogger{}, Level::kInfo, "Test message"));

      // Cleanup
      logger.RemoveLogger<TestLogger>();
    }

    SUBCASE("Log formatted message with typed logger") {
      logger.AddLogger(TestLogger{});
      CHECK_NOTHROW(logger.Log(TestLogger{}, Level::kInfo,
                               "Value: {} String: {}", 123, "test"));

      // Cleanup
      logger.RemoveLogger<TestLogger>();
    }

    SUBCASE("Log at different levels") {
      CHECK_NOTHROW(logger.Log(Level::kTrace, "Trace message"));
      CHECK_NOTHROW(logger.Log(Level::kDebug, "Debug message"));
      CHECK_NOTHROW(logger.Log(Level::kInfo, "Info message"));
      CHECK_NOTHROW(logger.Log(Level::kWarn, "Warn message"));
      CHECK_NOTHROW(logger.Log(Level::kError, "Error message"));
      CHECK_NOTHROW(logger.Log(Level::kCritical, "Critical message"));
    }
  }

  TEST_CASE("log::Logger::Flush: flushing loggers") {
    auto& logger = Logger::Instance();

    SUBCASE("Flush default logger") {
      CHECK_NOTHROW(logger.Flush<DefaultLogger>());
    }

    SUBCASE("Flush custom logger") {
      logger.AddLogger(TestLogger{});
      CHECK_NOTHROW(logger.Flush<TestLogger>());

      // Cleanup
      logger.RemoveLogger<TestLogger>();
    }

    SUBCASE("FlushAll") {
      logger.AddLogger(TestLogger{});
      CHECK_NOTHROW(logger.FlushAll());

      // Cleanup
      logger.RemoveLogger<TestLogger>();
    }
  }

  TEST_CASE("log::Logger::SetDefaultConfig: configuration management") {
    auto& logger = Logger::Instance();

    SUBCASE("Set and get default config") {
      const auto original_config = logger.GetDefaultConfig();

      const auto new_config = Config::ConsoleOnly();
      logger.SetDefaultConfig(new_config);

      const auto& retrieved_config = logger.GetDefaultConfig();
      CHECK_EQ(retrieved_config.enable_console, new_config.enable_console);
      CHECK_EQ(retrieved_config.enable_file, new_config.enable_file);

      // Restore original
      logger.SetDefaultConfig(original_config);
    }
  }

  TEST_CASE("log::DefaultLogger: default logger instance") {
    SUBCASE("kDefaultLogger is constexpr") {
      constexpr auto logger = kDefaultLogger;
      CHECK_EQ(LoggerNameOf(logger), "HELIOS");
    }

    SUBCASE("Default logger satisfies concepts") {
      CHECK(LoggerTrait<DefaultLogger>);
      CHECK(LoggerWithConfigTrait<DefaultLogger>);
    }
  }

  TEST_CASE("log::LogAssertionViaLogger: integration") {
    SUBCASE("LogAssertionViaLogger doesn't crash") {
      CHECK_NOTHROW(details::LogAssertionViaLogger(
          "test_condition", std::source_location::current(), ""));
    }

    SUBCASE("LogAssertionViaLogger with message") {
      CHECK_NOTHROW(details::LogAssertionViaLogger(
          "test_condition", std::source_location::current(), "Test message"));
    }
  }

  TEST_CASE("log::Info: free function logging") {
    SUBCASE("Info doesn't crash") {
      CHECK_NOTHROW(Info("Test info message"));
      CHECK_NOTHROW(Info("Value: {}", 42));
    }
  }

}  // TEST_SUITE
