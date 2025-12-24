# This module handles finding Taskflow from multiple sources:
# 1. Conan (if using Conan)
# 2. System packages (pacman, apt, etc.)
# 3. CPM download (fallback)

include_guard(GLOBAL)

message(STATUS "Configuring Taskflow dependency...")

# Use helios_module system for standard package finding
helios_dep_begin(
    NAME Taskflow
    VERSION 3.10
    DEBIAN_NAMES libtaskflow-cpp-dev
    RPM_NAMES taskflow-devel
    PACMAN_NAMES taskflow
    BREW_NAMES taskflow
    PKG_CONFIG_NAMES taskflow
    CPM_NAME taskflow
    CPM_VERSION 3.10.0
    CPM_GITHUB_REPOSITORY taskflow/taskflow
    CPM_GIT_TAG v3.10.0
    CPM_OPTIONS
        "TF_BUILD_TESTS OFF"
        "TF_BUILD_EXAMPLES OFF"
)

helios_dep_end()

# Create helios::taskflow alias if Taskflow was found
if(NOT TARGET helios::taskflow)
    if(TARGET Taskflow::Taskflow)
        # Taskflow::Taskflow might be an alias itself, get the real target
        get_target_property(_taskflow_aliased Taskflow::Taskflow ALIASED_TARGET)
        if(_taskflow_aliased)
            add_library(helios::taskflow ALIAS ${_taskflow_aliased})
        else()
            add_library(helios::taskflow ALIAS Taskflow::Taskflow)
        endif()
        message(STATUS "  ✓ Taskflow configured (Taskflow::Taskflow)")

        # Mark Taskflow targets as SYSTEM to suppress warnings if downloaded via CPM
        if(taskflow_ADDED)
            if(TARGET Taskflow::Taskflow)
                helios_cpm_mark_as_system(Taskflow::Taskflow)
            endif()
            if(TARGET Taskflow)
                helios_cpm_mark_as_system(Taskflow)
            endif()
        endif()
    elseif(TARGET Taskflow)
        add_library(helios::taskflow ALIAS Taskflow)
        message(STATUS "  ✓ Taskflow configured (Taskflow)")

        # Mark Taskflow targets as SYSTEM to suppress warnings if downloaded via CPM
        if(taskflow_ADDED)
            helios_cpm_mark_as_system(Taskflow)
        endif()
    else()
        message(WARNING "  ✗ Taskflow not found")
    endif()
else()
    message(STATUS "  ✓ Taskflow configured (helios::taskflow)")
endif()
