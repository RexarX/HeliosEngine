cmake_minimum_required(VERSION 3.24.0)

project(VoxelEngine)

set(CMAKE_CONFIGURATION_TYPES
    "Debug"
    "Release"
    "Dist"
    CACHE STRING "" FORCE
)

if(WIN32)
	set(PLATFORM "Windows")
elseif(UNIX)
	set(PLATFORM "Linux")
endif()

if (NOT DEFINED ARCHITECTURE)
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(ARCHITECTURE "x86_64")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(ARCHITECTURE "x86")
    endif()
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(VoxelCraft)
add_subdirectory(VoxelEngine)
add_subdirectory(VoxelEngine/vendor/glad)
add_subdirectory(VoxelEngine/vendor/glfw)
add_subdirectory(VoxelEngine/vendor/glm)
add_subdirectory(VoxelEngine/vendor/imgui)
add_subdirectory(VoxelEngine/vendor/spdlog)
add_subdirectory(VoxelEngine/vendor/stb_image)
add_subdirectory(VoxelEngine/vendor/tiny_obj_loader)