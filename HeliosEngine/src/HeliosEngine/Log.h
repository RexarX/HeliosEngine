#pragma once

#include "Core.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

namespace Helios
{
	class HELIOSENGINE_API Log
	{
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
}

#define CORE_TRACE(...)		 ::Helios::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define CORE_INFO(...)		 ::Helios::Log::GetCoreLogger()->info(__VA_ARGS__)
#define CORE_WARN(...)		 ::Helios::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define CORE_ERROR(...)		 ::Helios::Log::GetCoreLogger()->error(__VA_ARGS__)
#define CORE_CRITICAL(...) ::Helios::Log::GetCoreLogger()->critical(__VA_ARGS__);

#define APP_TRACE(...)		:Helios::Log::GetClientLogger()->trace(__VA_ARGS__)
#define APP_INFO(...)			::Helios::Log::GetClientLogger()->info(__VA_ARGS__)
#define APP_WARN(...)			::Helios::Log::GetClientLogger()->warn(__VA_ARGS__)
#define APP_ERROR(...)		::Helios::Log::GetClientLogger()->error(__VA_ARGS__)
#define APP_CRITICAL(...) ::Helios::Log::GetClientLogger()->critical(__VA_ARGS__);