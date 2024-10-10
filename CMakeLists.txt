cmake_minimum_required(VERSION 3.24.0)

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

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(Game)
add_subdirectory(HeliosEngine)
add_subdirectory(HeliosEngine/vendor/glfw)
add_subdirectory(HeliosEngine/vendor/glm)
add_subdirectory(HeliosEngine/vendor/imgui)
add_subdirectory(HeliosEngine/vendor/spdlog)
add_subdirectory(HeliosEngine/vendor/stb_image)
add_subdirectory(HeliosEngine/vendor/tiny_obj_loader)
add_subdirectory(HeliosEngine/vendor/entt)