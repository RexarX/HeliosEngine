# Window Module CMakeLists.txt
#
# Provides cross-platform window management using GLFW as the backend.
# Supports multiple graphics APIs (Vulkan, OpenGL, etc.) for use with rendering backends like DiligentEngine.

# Register module with dependencies
helios_register_module(
    NAME window
    DESCRIPTION "Cross-platform window management module using GLFW"
    DEFAULT ON
    EXTERNAL_DEPENDS glfw
)

# Platform-specific options for Linux
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    option(HELIOS_WINDOW_USE_WAYLAND "Use Wayland display server (prioritized over X11)" ON)
    option(HELIOS_WINDOW_USE_X11 "Use X11 display server" ON)

    if(HELIOS_WINDOW_USE_WAYLAND)
        message(STATUS "Window module: Wayland support enabled (prioritized)")
    endif()
    if(HELIOS_WINDOW_USE_X11)
        message(STATUS "Window module: X11 support enabled")
    endif()

    if(NOT HELIOS_WINDOW_USE_WAYLAND AND NOT HELIOS_WINDOW_USE_X11)
        message(FATAL_ERROR "Window module: At least one of HELIOS_WINDOW_USE_WAYLAND or HELIOS_WINDOW_USE_X11 must be enabled on Linux")
    endif()
endif()

# Define the module headers (public API - no GLFW includes)
set(WINDOW_HEADERS
    include/helios/window/window.hpp
)

# Define the module sources (GLFW implementation is private)
set(WINDOW_SOURCES
)

# Create the module
helios_add_module(
    NAME window
    VERSION 0.1.0

    HEADERS ${WINDOW_HEADERS}
    SOURCES ${WINDOW_SOURCES}

    PRIVATE_DEPENDENCIES
        helios::glfw
)

# Get the actual target name
set(_window_target helios_module_window)

# Platform-specific compile definitions
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Check if GLFW was built with Wayland support by looking for the native header functions
    # We need to check the actual GLFW library capabilities, not just if wayland is installed
    include(CheckCXXSourceCompiles)
    set(CMAKE_REQUIRED_LIBRARIES glfw)

    # Check for X11 support in GLFW
    check_cxx_source_compiles("
        #define GLFW_EXPOSE_NATIVE_X11
        #include <GLFW/glfw3.h>
        #include <GLFW/glfw3native.h>
        int main() { return 0; }
    " GLFW_HAS_X11_SUPPORT)

    # Check for Wayland support in GLFW (GLFW 3.4+ or specially compiled 3.3)
    # Note: glfwGetWaylandDisplay is only available if GLFW was compiled with Wayland
    check_cxx_source_compiles("
        #define GLFW_EXPOSE_NATIVE_WAYLAND
        #include <GLFW/glfw3.h>
        #include <GLFW/glfw3native.h>
        int main() { void* d = (void*)glfwGetWaylandDisplay; return d != 0; }
    " GLFW_HAS_WAYLAND_SUPPORT)

    unset(CMAKE_REQUIRED_LIBRARIES)

    # Only enable X11 support if user wants it AND GLFW has it
    if(HELIOS_WINDOW_USE_X11 AND GLFW_HAS_X11_SUPPORT)
        target_compile_definitions(${_window_target} PRIVATE HELIOS_WINDOW_X11_SUPPORT)
        message(STATUS "Window module: X11 support enabled (GLFW has X11)")

        find_package(X11 QUIET)
        if(X11_FOUND)
            target_include_directories(${_window_target} PRIVATE ${X11_INCLUDE_DIR})
            target_link_libraries(${_window_target} PRIVATE ${X11_LIBRARIES})
        endif()
    elseif(HELIOS_WINDOW_USE_X11)
        message(STATUS "Window module: X11 support disabled (GLFW lacks X11 support)")
    endif()

    # Only enable Wayland support if user wants it AND GLFW has it
    if(HELIOS_WINDOW_USE_WAYLAND AND GLFW_HAS_WAYLAND_SUPPORT)
        target_compile_definitions(${_window_target} PRIVATE HELIOS_WINDOW_WAYLAND_SUPPORT)
        message(STATUS "Window module: Wayland support enabled (GLFW has Wayland)")

        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(WAYLAND wayland-client wayland-egl)
            if(WAYLAND_FOUND)
                target_include_directories(${_window_target} PRIVATE ${WAYLAND_INCLUDE_DIRS})
                target_link_libraries(${_window_target} PRIVATE ${WAYLAND_LIBRARIES})
            endif()
        endif()
    elseif(HELIOS_WINDOW_USE_WAYLAND)
        message(STATUS "Window module: Wayland support disabled (GLFW lacks Wayland support)")
    endif()

    # Ensure at least one display server is available
    if(NOT GLFW_HAS_X11_SUPPORT AND NOT GLFW_HAS_WAYLAND_SUPPORT)
        message(WARNING "Window module: GLFW has neither X11 nor Wayland support!")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(${_window_target} PRIVATE HELIOS_WINDOW_WIN32_SUPPORT)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_compile_definitions(${_window_target} PRIVATE HELIOS_WINDOW_COCOA_SUPPORT)
endif()

# Compile definitions for graphics API support
target_compile_definitions(${_window_target} PRIVATE
    HELIOS_WINDOW_VULKAN_SUPPORT
    HELIOS_WINDOW_OPENGL_SUPPORT
)

# Windows-specific: DirectX support
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(${_window_target} PRIVATE HELIOS_WINDOW_DIRECTX_SUPPORT)
endif()
