cmake_minimum_required(VERSION 3.16.0 FATAL_ERROR)

project(VoxelCraft)

if(NOT CMAKE_VS_PLATFORM_NAME)
    set(CMAKE_VS_PLATFORM_NAME "x64")
endif()

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(CMAKE_CONFIGURATION_TYPES
    "Debug"
    "Release"
    "Dist"
    CACHE STRING "" FORCE
)

if(MSVC)
    set(CMAKE_C_FLAGS "")
    set(CMAKE_C_FLAGS_DEBUG "")
    set(CMAKE_C_FLAGS_DIST "")
    set(CMAKE_C_FLAGS_RELEASE "")
    set(CMAKE_CXX_FLAGS "")
    set(CMAKE_CXX_FLAGS_DEBUG "")
    set(CMAKE_CXX_FLAGS_DIST "")
    set(CMAKE_CXX_FLAGS_RELEASE "")
endif()

if(MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "")
    set(CMAKE_MODULE_LINKER_FLAGS "")
    set(CMAKE_SHARED_LINKER_FLAGS "")
    set(CMAKE_STATIC_LINKER_FLAGS "")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_DEBUG "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_DEBUG "${CMAKE_STATIC_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_DIST "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_DIST "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_DIST "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_DIST "${CMAKE_STATIC_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS}")
endif()

include(CMake/Utils.cmake)
include(CMake/GlobalSettingsInclude.cmake OPTIONAL)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include_directories(VoxelEngine/src)
include_directories(VoxelEngine/src/VoxelEngine)
include_directories(VoxelEngine/src/Platform)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
set(BUILD_SHARED_LIBS OFF)

option(GLM_ENABLE_FAST_MATH "Enable fast math optimizations" ON)
set(GLM_ENABLE_FAST_MATH ON)

option(SPDLOG_BUILD_SHARED "Build shared library" ON)
set(SPDLOG_BUILD_SHARED ON)

add_subdirectory(VoxelCraft)
add_subdirectory(VoxelEngine)
add_subdirectory(VoxelEngine/vendor/glad)
add_subdirectory(VoxelEngine/vendor/stb_image)
add_subdirectory(VoxelEngine/vendor/glfw)
add_subdirectory(VoxelEngine/vendor/glm)