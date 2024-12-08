#pragma once

#include "Core.h"

#include <source_location>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

namespace Helios {
	class HELIOSENGINE_API Log {
	public:
		static void Init();

		template <typename... Args>
		static void LogMessage(const std::shared_ptr<spdlog::logger>& logger, spdlog::level::level_enum level,
													 const std::source_location& loc, spdlog::fmt_lib::format_string<Args...> fmt, Args&&... args) {
			std::string_view filename(loc.file_name());
			uint64_t lastSlash = filename.find_last_of("/\\");
			if (lastSlash != std::string_view::npos) {
				filename = filename.substr(lastSlash + 1);
			}

			logger->log(spdlog::source_loc{ filename.data(), static_cast<int>(loc.line()), nullptr },
									level, spdlog::fmt_lib::format(fmt, std::forward<Args>(args)...));
		}

		static void LogMessage(const std::shared_ptr<spdlog::logger>& logger, spdlog::level::level_enum level,
													 const std::source_location& loc, std::string_view string) {
			std::string_view filename(loc.file_name());
			uint64_t lastSlash = filename.find_last_of("/\\");
			if (lastSlash != std::string_view::npos) {
				filename = filename.substr(lastSlash + 1);
			}

			logger->log(spdlog::source_loc{ filename.data(), static_cast<int>(loc.line()), nullptr },
									level, string);
		}

		static inline std::shared_ptr<spdlog::logger>& GetCoreLogger() { return m_CoreLogger; }
		static inline std::shared_ptr<spdlog::logger>& GetClientLogger() { return m_ClientLogger; }

	private:
		static inline std::shared_ptr<spdlog::logger> m_CoreLogger = nullptr;
		static inline std::shared_ptr<spdlog::logger> m_ClientLogger = nullptr;
	};
}

#define CORE_TRACE(...) 	 Helios::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define CORE_INFO(...) 		 Helios::Log::GetCoreLogger()->info(__VA_ARGS__)
#define CORE_WARN(...) 		 Helios::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define CORE_ERROR(...) 	 Helios::Log::LogMessage(Helios::Log::GetCoreLogger(), spdlog::level::err, std::source_location::current(), __VA_ARGS__)
#define CORE_CRITICAL(...) Helios::Log::LogMessage(Helios::Log::GetCoreLogger(), spdlog::level::critical, std::source_location::current(), __VA_ARGS__)

#define APP_TRACE(...) 		 Helios::Log::GetClientLogger()->trace(__VA_ARGS__)
#define APP_INFO(...) 		 Helios::Log::GetClientLogger()->info(__VA_ARGS__)
#define APP_WARN(...) 		 Helios::Log::GetClientLogger()->warn(__VA_ARGS__)
#define APP_ERROR(...) 		 Helios::Log::LogMessage(Helios::Log::GetClientLogger(), spdlog::level::err, std::source_location::current(), __VA_ARGS__)
#define APP_CRITICAL(...)	 Helios::Log::LogMessage(Helios::Log::GetClientLogger(), spdlog::level::critical, std::source_location::current(), __VA_ARGS__)