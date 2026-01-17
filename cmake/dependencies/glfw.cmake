# This module handles finding GLFW from multiple sources:
# 1. Conan (if using Conan)
# 2. System packages (pacman, apt, etc.)
# 3. CPM download (fallback)

include_guard(GLOBAL)

message(STATUS "Configuring GLFW dependency...")

# Use helios_dep system for standard package finding
helios_dep_begin(
    NAME glfw3
    VERSION 3.3
    DEBIAN_NAMES libglfw3-dev
    RPM_NAMES glfw-devel
    PACMAN_NAMES glfw
    BREW_NAMES glfw
    PKG_CONFIG_NAMES glfw3
    CPM_NAME glfw
    CPM_VERSION 3.4
    CPM_GITHUB_REPOSITORY glfw/glfw
    CPM_GIT_TAG 3.4
    CPM_OPTIONS
        "GLFW_BUILD_DOCS OFF"
        "GLFW_BUILD_EXAMPLES OFF"
        "GLFW_BUILD_TESTS OFF"
        "GLFW_INSTALL OFF"
)

helios_dep_end()

# Helper function to get the real (non-alias) target from a potentially aliased target
function(_helios_glfw_get_real_target target_name out_var)
    if(NOT TARGET ${target_name})
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    set(_current_target ${target_name})
    set(_max_iterations 10)
    set(_iteration 0)

    while(_iteration LESS _max_iterations)
        get_target_property(_aliased ${_current_target} ALIASED_TARGET)
        if(_aliased)
            set(_current_target ${_aliased})
        else()
            break()
        endif()
        math(EXPR _iteration "${_iteration} + 1")
    endwhile()

    set(${out_var} ${_current_target} PARENT_SCOPE)
endfunction()

# Create helios::glfw alias target
# GLFW can be found under different target names depending on how it was installed:
# - glfw (CPM/build from source)
# - glfw3 (pkg-config)
# - glfw::glfw (Conan)

set(_glfw_target_found FALSE)

if(TARGET glfw::glfw AND NOT TARGET helios::glfw::glfw)
    _helios_glfw_get_real_target(glfw::glfw _glfw_real)
    if(_glfw_real)
        add_library(helios::glfw::glfw ALIAS ${_glfw_real})
    else()
        add_library(helios::glfw::glfw ALIAS glfw::glfw)
    endif()
    set(_glfw_target_found TRUE)
    message(STATUS "  ✓ GLFW configured (glfw::glfw)")
endif()

if(TARGET glfw AND NOT TARGET helios::glfw::glfw)
    _helios_glfw_get_real_target(glfw _glfw_real)
    if(_glfw_real)
        add_library(helios::glfw::glfw ALIAS ${_glfw_real})
    else()
        add_library(helios::glfw::glfw ALIAS glfw)
    endif()
    set(_glfw_target_found TRUE)
    message(STATUS "  ✓ GLFW configured (glfw)")
endif()

if(TARGET glfw3 AND NOT TARGET helios::glfw::glfw)
    _helios_glfw_get_real_target(glfw3 _glfw_real)
    if(_glfw_real)
        add_library(helios::glfw::glfw ALIAS ${_glfw_real})
    else()
        add_library(helios::glfw::glfw ALIAS glfw3)
    endif()
    set(_glfw_target_found TRUE)
    message(STATUS "  ✓ GLFW configured (glfw3)")
endif()

# Create convenience target
if(TARGET helios::glfw::glfw AND NOT TARGET helios::glfw)
    add_library(_helios_glfw_all INTERFACE)
    target_link_libraries(_helios_glfw_all INTERFACE helios::glfw::glfw)
    add_library(helios::glfw ALIAS _helios_glfw_all)
endif()

# Warn if not found
if(NOT _glfw_target_found)
    message(WARNING "  ✗ GLFW not found")
endif()
