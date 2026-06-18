#pragma once

#if defined(__GNUC__) || defined(__clang__)
#define HELIOS_EXPECT_TRUE(x) __builtin_expect(!!(x), 1)
#define HELIOS_EXPECT_FALSE(x) __builtin_expect(!!(x), 0)
#else
#define HELIOS_EXPECT_TRUE(x) (x)
#define HELIOS_EXPECT_FALSE(x) (x)
#endif

#if defined(__cpp_lib_move_only_function) && \
    __cpp_lib_move_only_function >= 202110L
#define HELIOS_MOVEONLY_FUNCTION_AVAILABLE
#endif

#if defined(__cpp_lib_containers_ranges) && \
    __cpp_lib_containers_ranges >= 202202L
#define HELIOS_CONTAINERS_RANGES_AVAILABLE
#endif

#if defined(__cpp_lib_flat_map) && __cpp_lib_flat_map >= 202207L
#define HELIOS_STL_FLAT_MAP_AVAILABLE
#elif defined(HELIOS_USE_STL_FLAT_MAP)
#define HELIOS_STL_FLAT_MAP_AVAILABLE
#endif

#ifdef _MSC_VER
#define HELIOS_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define HELIOS_FORCE_INLINE __attribute__((always_inline)) inline
#else
#define HELIOS_FORCE_INLINE inline
#endif

#if defined(__clang__)
#define HELIOS_ALWAYS_INLINE [[clang::always_inline]]
#elif defined(__GNUC__)
#define HELIOS_ALWAYS_INLINE [[gnu::always_inline]]
#elif defined(_MSC_VER)
#define HELIOS_ALWAYS_INLINE [[msvc::forceinline]]
#else
#define HELIOS_ALWAYS_INLINE
#endif

#ifdef _MSC_VER
#define HELIOS_NO_INLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define HELIOS_NO_INLINE __attribute__((noinline))
#else
#define HELIOS_NO_INLINE
#endif
