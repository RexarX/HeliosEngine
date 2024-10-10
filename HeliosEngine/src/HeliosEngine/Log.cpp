#include "Log.h"

#include "pch.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace Helios
{
  std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
  std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

  void Log::Init()
  {
    std::vector<spdlog::sink_ptr> logSinks;

    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d_%H-%M-%S");

    std::string logFilePath = std::format("Logs/{}.log", ss.str());

    // Create the Logs directory if it doesn't exist
    std::filesystem::create_directories("Logs");

    logSinks.emplace_back(
      std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath, true)
    )->set_pattern("[%T] [%l] %n: %v");

#ifdef ENABLE_ASSERTS
    logSinks.emplace_back(
      std::make_shared<spdlog::sinks::stdout_color_sink_mt>()
    )->set_pattern("[%T] [%^%l%$] %n: %v");
#endif

    s_CoreLogger = std::make_shared<spdlog::logger>("HELIOSENGINE", logSinks.begin(), logSinks.end());
    spdlog::register_logger(s_CoreLogger);
    s_CoreLogger->set_level(spdlog::level::trace);
    s_CoreLogger->flush_on(spdlog::level::trace);

    s_ClientLogger = std::make_shared<spdlog::logger>("APP", logSinks.begin(), logSinks.end());
    spdlog::register_logger(s_ClientLogger);
    s_ClientLogger->set_level(spdlog::level::trace);
    s_ClientLogger->flush_on(spdlog::level::trace);
  }
}