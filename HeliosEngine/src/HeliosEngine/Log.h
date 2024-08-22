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

#define CORE_TRACE(...)    ::Helios::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define CORE_INFO(...)     ::Helios::Log::GetCoreLogger()->info(__VA_ARGS__)
#define CORE_WARN(...)     ::Helios::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define CORE_ERROR(...)    ::Helios::Log::GetCoreLogger()->error(__VA_ARGS__)
#define CORE_FATAL(...)    ::Helios::Log::GetCoreLogger()->fatal(__VA_ARGS__)

#define TRACE(...)				 ::Helios::Log::GetClientLogger()->trace(__VA_ARGS__)
#define INFO(...)					 ::Helios::Log::GetClientLogger()->info(__VA_ARGS__)
#define WARN(...)					 ::Helios::Log::GetClientLogger()->warn(__VA_ARGS__)
#define ERROR(...)				 ::Helios::Log::GetClientLogger()->error(__VA_ARGS__)
#define FATAL(...)				 ::Helios::Log::GetClientLogger()->fatal(__VA_ARGS__)