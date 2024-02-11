#pragma once

#include "Core.h"

#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

namespace VoxelEngine
{
	class VOXELENGINE_API Log
	{
	public:
		static void initialize();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
}

#define VE_CORE_TRACE(...)    ::VoxelEngine::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define VE_CORE_INFO(...)     ::VoxelEngine::Log::GetCoreLogger()->info(__VA_ARGS__)
#define VE_CORE_WARN(...)     ::VoxelEngine::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define VE_CORE_ERROR(...)    ::VoxelEngine::Log::GetCoreLogger()->error(__VA_ARGS__)
#define VE_CORE_FATAL(...)    ::VoxelEngine::Log::GetCoreLogger()->fatal(__VA_ARGS__)

#define VE_TRACE(...)					::VoxelEngine::Log::GetClientLogger()->trace(__VA_ARGS__)
#define VE_INFO(...)					::VoxelEngine::Log::GetClientLogger()->info(__VA_ARGS__)
#define VE_WARN(...)					::VoxelEngine::Log::GetClientLogger()->warn(__VA_ARGS__)
#define VE_ERROR(...)					::VoxelEngine::Log::GetClientLogger()->error(__VA_ARGS__)
#define VE_FATAL(...)					::VoxelEngine::Log::GetClientLogger()->fatal(__VA_ARGS__)