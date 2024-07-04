#pragma once

#ifdef VE_PLATFORM_WINDOWS
	#ifdef VE_BUILD_DLL
		#define VOXELENGINE_API __declspec(dllexport)
	#else
		#define VOXELENGINE_API __declspec(dllimport)
	#endif
#endif

#ifdef VE_DEBUG
	#define ENABLE_ASSERTS
#endif

#ifdef VE_RELEASE
	#define VE_ENABLE_ASSERTS
#endif

#ifdef VE_DIST
	#define VOXELENGINE_DIR std::string("")
	#define VOXELCRAFT_DIR std::string("")
#else
	#define VOXELENGINE_DIR std::filesystem::current_path().parent_path().parent_path().string() + "/VoxelEngine/"
	#define VOXELCRAFT_DIR std::filesystem::current_path().parent_path().parent_path().string() + "/VoxelCraft/"
#endif

#ifdef ENABLE_ASSERTS
	#define ASSERT(x, ...) { if(!(x)) { ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
	#define CORE_ASSERT(x, ...) { if(!(x)) { CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
	#define ASSERT(x, ...)
	#define CORE_ASSERT(x, ...)
#endif

#define BIT(x) (1 << x)

#define BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)
