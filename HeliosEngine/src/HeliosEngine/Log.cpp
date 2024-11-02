#include "Log.h"

#include "pch.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/pattern_formatter.h>

class star_formatter_flag final : public spdlog::custom_flag_formatter
{
public:
    void format(const spdlog::details::log_msg& msg, const std::tm&, spdlog::memory_buf_t& dest) override
    {
      if (msg.level == spdlog::level::err || msg.level == spdlog::level::critical) {
        std::source_location location = std::source_location::current();
        std::string some_txt = std::format("{}:{}", Helios::Utils::getFilename(location.file_name()),
                                           location.line());
        dest.append(some_txt.c_str(), some_txt.c_str() + some_txt.size());
      }
    }

    inline std::unique_ptr<custom_flag_formatter> clone() const override {
      return spdlog::details::make_unique<star_formatter_flag>();
    }
};

namespace Helios
{
  std::shared_ptr<spdlog::logger> Log::s_CoreLogger = nullptr;
  std::shared_ptr<spdlog::logger> Log::s_ClientLogger = nullptr;

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

    auto file_formatter = std::make_unique<spdlog::pattern_formatter>();
    file_formatter->add_flag<star_formatter_flag>('*').set_pattern("[%T] [%l] %n: %v %*");

    auto formatter = std::make_unique<spdlog::pattern_formatter>();
    formatter->add_flag<star_formatter_flag>('*').set_pattern("[%T] [%^%l%$] %n: %v %*");

    logSinks.emplace_back(
      std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath, true)
    )->set_formatter(std::move(file_formatter));

#ifdef ENABLE_ASSERTS
    logSinks.emplace_back(
      std::make_shared<spdlog::sinks::stdout_color_sink_mt>()
    )->set_formatter(std::move(formatter));
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