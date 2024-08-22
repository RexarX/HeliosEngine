#pragma once

#ifdef PLATFORM_WINDOWS
	#ifdef BUILD_DLL
		#define HELIOSENGINE_API __declspec(dllexport)
	#else
		#define HELIOSENGINE_API __declspec(dllimport)
	#endif
#endif

#ifdef DEBUG_MODE
	#define ENABLE_ASSERTS
#endif

#ifdef RELEASE_MODE
	#define ENABLE_ASSERTS
#endif

#ifdef DIST_MODE
	#define HELIOSENGINE_DIR std::string("")
	#define GAME_DIR std::string("")
#else
	#define HELIOSENGINE_DIR std::filesystem::current_path().parent_path().parent_path().string() + "/HeliosEngine/"
	#define GAME_DIR std::filesystem::current_path().parent_path().parent_path().string() + "/Game/"
#endif

#ifdef ENABLE_ASSERTS
	#ifdef _MSC_VER
		#define DEBUG_BREAK __debugbreak()
	#else
		#define DEBUG_BREAK __builtin_trap()
	#endif

	#define ASSERT(x, ...) if(!(x)) { ERROR("Assertion Failed: {0}", __VA_ARGS__); DEBUG_BREAK; }
	#define CORE_ASSERT(x, ...) if(!(x)) { CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); DEBUG_BREAK; }
#else
	#define ASSERT(x, ...)
	#define CORE_ASSERT(x, ...)
#endif

#define BIT(x) (1 << x)

#define BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)
#define BIND_EVENT_FN_WITH_REF(fn, reference) std::bind(&fn, this, std::ref(reference), std::placeholders::_1)