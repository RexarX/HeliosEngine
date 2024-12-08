cmake_minimum_required(VERSION 3.25.0)

project(HeliosEngine)

set(CMAKE_CONFIGURATION_TYPES
    "Debug"
    "RelWithDebInfo"
    "Release"
    CACHE STRING "" FORCE
)

if (NOT DEFINED ARCHITECTURE)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(ARCHITECTURE "x86_64")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(ARCHITECTURE "x86")
    endif()
endif()

if (WIN32)
    set(PLATFORM "Windows")
elseif (UNIX)
    set(PLATFORM "Linux")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(ENABLE_SIMD_SSE2 "Enable SSE2 optimizations" OFF)
option(ENABLE_SIMD_SSE3 "Enable SSE3 optimizations" OFF)
option(ENABLE_SIMD_SSSE3 "Enable SSSE3 optimizations" OFF)
option(ENABLE_SIMD_SSE4_1 "Enable SSE 4.1 optimizations" OFF)
option(ENABLE_SIMD_SSE4_2 "Enable SSE 4.2 optimizations" OFF)
option(ENABLE_SIMD_AVX "Enable AVX optimizations" OFF)
option(ENABLE_SIMD_AVX2 "Enable AVX2 optimizations" OFF)

option(ENABLE_PROFILING "Enable profiling" OFF)

set(GLM_ENABLE_SIMD_SSE2 ENABLE_SIMD_SSE2)
set(GLM_ENABLE_SIMD_SSE3 ENABLE_SIMD_SSE3)
set(GLM_ENABLE_SIMD_SSSE3 ENABLE_SIMD_SSSE3)
set(GLM_ENABLE_SIMD_SSE4_1 ENABLE_SIMD_SSE4_1)
set(GLM_ENABLE_SIMD_SSE4_2 ENABLE_SIMD_SSE4_2)
set(GLM_ENABLE_SIMD_AVX ENABLE_SIMD_AVX)
set(GLM_ENABLE_SIMD_AVX2 ENABLE_SIMD_AVX2)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(Game)
add_subdirectory(HeliosEngine)
add_subdirectory(HeliosEngine/vendor/stb_image)