helios_module(
    NAME window
    VERSION 0.1.0
    DESCRIPTION "Cross-platform window management module using GLFW"

    HEADERS
        include/helios/window/window.hpp

    PCH src/pch.hpp

    USES
        glfw PUBLIC helios::lib::glfw::glfw
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include(CMakeDependentOption)
    cmake_dependent_option(
        HELIOS_WINDOW_FORCE_WAYLAND
        "Force Wayland backend (disable X11 fallback)"
        OFF
        "CMAKE_SYSTEM_NAME STREQUAL Linux;NOT HELIOS_WINDOW_FORCE_X11"
        OFF
    )
    cmake_dependent_option(
        HELIOS_WINDOW_FORCE_X11
        "Force X11 backend (disable Wayland probing)"
        OFF
        "CMAKE_SYSTEM_NAME STREQUAL Linux;NOT HELIOS_WINDOW_FORCE_WAYLAND"
        OFF
    )

    if(HELIOS_WINDOW_FORCE_WAYLAND AND HELIOS_WINDOW_FORCE_X11)
        message(FATAL_ERROR "Window: HELIOS_WINDOW_FORCE_WAYLAND and HELIOS_WINDOW_FORCE_X11 are mutually exclusive")
    endif()

    if(HELIOS_WINDOW_FORCE_WAYLAND)
        target_compile_definitions(helios_module_window INTERFACE HELIOS_WINDOW_FORCE_WAYLAND)
    elseif(HELIOS_WINDOW_FORCE_X11)
        target_compile_definitions(helios_module_window INTERFACE HELIOS_WINDOW_FORCE_X11)
    else()
        target_compile_definitions(helios_module_window INTERFACE HELIOS_WINDOW_PREFER_WAYLAND)
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(helios_module_window INTERFACE HELIOS_WINDOW_WIN32_SUPPORT)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_compile_definitions(helios_module_window INTERFACE HELIOS_WINDOW_COCOA_SUPPORT)
endif()

target_compile_definitions(helios_module_window INTERFACE
    HELIOS_WINDOW_VULKAN_SUPPORT
    HELIOS_WINDOW_OPENGL_SUPPORT
)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(helios_module_window INTERFACE HELIOS_WINDOW_DIRECTX_SUPPORT)
endif()
