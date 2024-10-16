#pragma once

#ifdef PLATFORM_WINDOWS
	#ifdef BUILD_SHARED
			#define HELIOSENGINE_API __declspec(dllexport)
		#else
			#define HELIOSENGINE_API __declspec(dllimport)
	#endif
#elif defined(PLATFORM_LINUX)
	#ifdef BUILD_SHARED
		#define HELIOSENGINE_API __attribute__((visibility("default")))
	#else
    	#define HELIOSENGINE_API
	#endif
#endif

#ifndef RELEASE_MODE
	#define ENABLE_ASSERTS
#endif

#ifdef RELEASE_MODE
	#define HELIOSENGINE_DIR std::string("")
	#define GAME_DIR std::string("")
#else
	#define HELIOSENGINE_DIR std::format("{}/HeliosEngine/", std::filesystem::current_path().parent_path().parent_path().string())
	#define GAME_DIR std::format("{}/Game/", std::filesystem::current_path().parent_path().parent_path().string())
#endif

#ifdef ENABLE_ASSERTS
	#ifdef _MSC_VER
		#define DEBUG_BREAK __debugbreak()
	#else
		#define DEBUG_BREAK __builtin_trap()
	#endif

	#define APP_ASSERT(x, ...) if(!(x)) { APP_ERROR("Assertion Failed: {0}", __VA_ARGS__); DEBUG_BREAK; }
	#define APP_ASSERT_CRITICAL(x, ...) if(!(x)) { APP_CRITICAL("Assertion Failed: {0}", __VA_ARGS__); DEBUG_BREAK; }
	#define CORE_ASSERT(x, ...) if(!(x)) { CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); DEBUG_BREAK; }
	#define CORE_ASSERT_CRITICAL(x, ...) if(!(x)) { CORE_CRITICAL("Assertion Failed: {0}", __VA_ARGS__); DEBUG_BREAK; }
#else
	#define APP_ASSERT(x, ...) if(!(x)) { APP_ERROR("Assertion Failed: {0}", __VA_ARGS__); }
	#define APP_ASSERT_CRITICAL(x, ...) if(!(x)) { APP_CRITICAL("Assertion Failed: {0}", __VA_ARGS__); }
	#define CORE_ASSERT(x, ...) if(!(x)) { CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); }
	#define CORE_ASSERT_CRITICAL(x, ...) if(!(x)) { CORE_CRITICAL("Assertion Failed: {0}", __VA_ARGS__); }
#endif

#define BIT(x) (1 << x)

#define BIND_FN(fn) std::bind(&fn, this, std::placeholders::_1)
#define BIND_FN_WITH_REF(fn, reference) std::bind(&fn, this, std::ref(reference), std::placeholders::_1)