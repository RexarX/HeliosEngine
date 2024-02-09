cmake_minimum_required(VERSION 3.20)

project(VoxelCraft)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG bin.dbg)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE bin.rel)

if (WIN32)
    add_compile_definitions(VE_PLATFORM_WINDOWS)
endif (WIN32)

add_compile_definitions(VE_BUILD_DLL)

string(TOLOWER ${CMAKE_BUILD_TYPE} build_type)
if (build_type STREQUAL debug)
  add_compile_definitions(VE_DEBUG)
endif()

add_compile_definitions(GLFW_INCLUDE_NONE)

add_subdirectory(VoxelCraft)

add_subdirectory(VoxelEngine)