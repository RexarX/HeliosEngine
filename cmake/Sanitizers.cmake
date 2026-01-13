# ============================================================================
# Sanitizer Configuration for Helios Engine
# ============================================================================
#
# This module provides support for various C++ sanitizers:
# - AddressSanitizer (ASan): Detects memory errors (buffer overflows, use-after-free, etc.)
# - UndefinedBehaviorSanitizer (UBSan): Detects undefined behavior
# - ThreadSanitizer (TSan): Detects data races (mutually exclusive with ASan)
# - MemorySanitizer (MSan): Detects uninitialized memory reads (Clang only, requires instrumented libc++)
#
# Usage:
#   include(Sanitizers)
#   helios_target_enable_sanitizers(my_target)
#
# Options:
#   HELIOS_ENABLE_SANITIZERS       - Master switch for sanitizers (default: ON for Debug)
#   HELIOS_SANITIZER_ADDRESS       - Enable AddressSanitizer (default: ON)
#   HELIOS_SANITIZER_UNDEFINED     - Enable UndefinedBehaviorSanitizer (default: ON)
#   HELIOS_SANITIZER_THREAD        - Enable ThreadSanitizer (default: OFF, mutually exclusive with ASan)
#   HELIOS_SANITIZER_MEMORY        - Enable MemorySanitizer (default: OFF, Clang only)
#
# Notes:
#   - ASan and TSan cannot be used together
#   - MSan requires the entire program (including libc++) to be built with MSan
#   - MSVC only supports ASan (/fsanitize=address)
#   - Sanitizers are typically only enabled for Debug builds
#
# ============================================================================

include_guard(GLOBAL)

# Detect compiler capabilities
set(HELIOS_COMPILER_IS_GNU OFF)
set(HELIOS_COMPILER_IS_CLANG OFF)
set(HELIOS_COMPILER_IS_MSVC OFF)
set(HELIOS_COMPILER_IS_CLANG_CL OFF)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(HELIOS_COMPILER_IS_GNU ON)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if(MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        set(HELIOS_COMPILER_IS_CLANG_CL ON)
    else()
        set(HELIOS_COMPILER_IS_CLANG ON)
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(HELIOS_COMPILER_IS_MSVC ON)
endif()

# ============================================================================
# Sanitizer Options
# ============================================================================

# Master switch - enabled by default for Debug builds
option(HELIOS_ENABLE_SANITIZERS "Enable sanitizers for Debug builds" ON)

# Individual sanitizer options
option(HELIOS_SANITIZER_ADDRESS "Enable AddressSanitizer" ON)
option(HELIOS_SANITIZER_UNDEFINED "Enable UndefinedBehaviorSanitizer" ON)
option(HELIOS_SANITIZER_THREAD "Enable ThreadSanitizer (mutually exclusive with ASan)" OFF)
option(HELIOS_SANITIZER_MEMORY "Enable MemorySanitizer (Clang only, requires instrumented libc++)" OFF)

# ============================================================================
# Validation
# ============================================================================

# Check for mutually exclusive sanitizers
if(HELIOS_SANITIZER_ADDRESS AND HELIOS_SANITIZER_THREAD)
    message(WARNING "AddressSanitizer and ThreadSanitizer cannot be used together. Disabling ThreadSanitizer.")
    set(HELIOS_SANITIZER_THREAD OFF CACHE BOOL "Enable ThreadSanitizer" FORCE)
endif()

if(HELIOS_SANITIZER_ADDRESS AND HELIOS_SANITIZER_MEMORY)
    message(WARNING "AddressSanitizer and MemorySanitizer cannot be used together. Disabling MemorySanitizer.")
    set(HELIOS_SANITIZER_MEMORY OFF CACHE BOOL "Enable MemorySanitizer" FORCE)
endif()

if(HELIOS_SANITIZER_THREAD AND HELIOS_SANITIZER_MEMORY)
    message(WARNING "ThreadSanitizer and MemorySanitizer cannot be used together. Disabling MemorySanitizer.")
    set(HELIOS_SANITIZER_MEMORY OFF CACHE BOOL "Enable MemorySanitizer" FORCE)
endif()

# MSan is only available on Clang
if(HELIOS_SANITIZER_MEMORY AND NOT HELIOS_COMPILER_IS_CLANG)
    message(WARNING "MemorySanitizer is only available with Clang. Disabling MemorySanitizer.")
    set(HELIOS_SANITIZER_MEMORY OFF CACHE BOOL "Enable MemorySanitizer" FORCE)
endif()

# MSVC only supports ASan
if(HELIOS_COMPILER_IS_MSVC OR HELIOS_COMPILER_IS_CLANG_CL)
    if(HELIOS_SANITIZER_UNDEFINED)
        message(STATUS "UndefinedBehaviorSanitizer is not supported on MSVC. Disabling.")
        set(HELIOS_SANITIZER_UNDEFINED OFF CACHE BOOL "Enable UndefinedBehaviorSanitizer" FORCE)
    endif()
    if(HELIOS_SANITIZER_THREAD)
        message(STATUS "ThreadSanitizer is not supported on MSVC. Disabling.")
        set(HELIOS_SANITIZER_THREAD OFF CACHE BOOL "Enable ThreadSanitizer" FORCE)
    endif()
    if(HELIOS_SANITIZER_MEMORY)
        message(STATUS "MemorySanitizer is not supported on MSVC. Disabling.")
        set(HELIOS_SANITIZER_MEMORY OFF CACHE BOOL "Enable MemorySanitizer" FORCE)
    endif()
endif()

# ============================================================================
# Internal Helper Functions
# ============================================================================

# Build the sanitizer flags string for GCC/Clang
function(_helios_get_sanitizer_flags OUT_COMPILE_FLAGS OUT_LINK_FLAGS)
    set(_compile_flags "")
    set(_link_flags "")

    if(HELIOS_COMPILER_IS_GNU OR HELIOS_COMPILER_IS_CLANG)
        set(_sanitizers "")

        if(HELIOS_SANITIZER_ADDRESS)
            list(APPEND _sanitizers "address")
        endif()

        if(HELIOS_SANITIZER_UNDEFINED)
            list(APPEND _sanitizers "undefined")
        endif()

        if(HELIOS_SANITIZER_THREAD)
            list(APPEND _sanitizers "thread")
        endif()

        if(HELIOS_SANITIZER_MEMORY)
            list(APPEND _sanitizers "memory")
        endif()

        if(_sanitizers)
            list(JOIN _sanitizers "," _sanitizer_list)
            set(_compile_flags "-fsanitize=${_sanitizer_list} -fno-omit-frame-pointer -fno-optimize-sibling-calls")
            set(_link_flags "-fsanitize=${_sanitizer_list}")

            # Add extra flags for better error reporting
            if(HELIOS_SANITIZER_ADDRESS)
                string(APPEND _compile_flags " -fsanitize-address-use-after-scope")
            endif()

            if(HELIOS_SANITIZER_UNDEFINED)
                # Print stack trace on UBSan error
                string(APPEND _compile_flags " -fno-sanitize-recover=undefined")
            endif()
        endif()
    endif()

    set(${OUT_COMPILE_FLAGS} "${_compile_flags}" PARENT_SCOPE)
    set(${OUT_LINK_FLAGS} "${_link_flags}" PARENT_SCOPE)
endfunction()

# Build the sanitizer flags for MSVC
function(_helios_get_msvc_sanitizer_flags OUT_COMPILE_FLAGS OUT_LINK_FLAGS)
    set(_compile_flags "")
    set(_link_flags "")

    if(HELIOS_COMPILER_IS_MSVC OR HELIOS_COMPILER_IS_CLANG_CL)
        if(HELIOS_SANITIZER_ADDRESS)
            set(_compile_flags "/fsanitize=address")
            # MSVC ASan doesn't require special linker flags
        endif()
    endif()

    set(${OUT_COMPILE_FLAGS} "${_compile_flags}" PARENT_SCOPE)
    set(${OUT_LINK_FLAGS} "${_link_flags}" PARENT_SCOPE)
endfunction()

# ============================================================================
# Public API
# ============================================================================

# Enable sanitizers for a specific target
# Usage: helios_target_enable_sanitizers(my_target)
function(helios_target_enable_sanitizers TARGET)
    if(NOT HELIOS_ENABLE_SANITIZERS)
        return()
    endif()

    # Only apply sanitizers for Debug builds
    set(_is_debug "$<CONFIG:Debug>")

    if(HELIOS_COMPILER_IS_MSVC OR HELIOS_COMPILER_IS_CLANG_CL)
        _helios_get_msvc_sanitizer_flags(_compile_flags _link_flags)

        if(_compile_flags)
            target_compile_options(${TARGET} PRIVATE
                $<${_is_debug}:${_compile_flags}>
            )
        endif()

        if(_link_flags)
            target_link_options(${TARGET} PRIVATE
                $<${_is_debug}:${_link_flags}>
            )
        endif()
    else()
        _helios_get_sanitizer_flags(_compile_flags _link_flags)

        if(_compile_flags)
            separate_arguments(_compile_flags_list UNIX_COMMAND "${_compile_flags}")
            target_compile_options(${TARGET} PRIVATE
                $<${_is_debug}:${_compile_flags_list}>
            )
        endif()

        if(_link_flags)
            separate_arguments(_link_flags_list UNIX_COMMAND "${_link_flags}")
            target_link_options(${TARGET} PRIVATE
                $<${_is_debug}:${_link_flags_list}>
            )
        endif()
    endif()
endfunction()

# Print sanitizer configuration status
function(helios_print_sanitizer_status)
    if(NOT HELIOS_ENABLE_SANITIZERS)
        message(STATUS "Sanitizers: DISABLED")
        return()
    endif()

    message(STATUS "")
    message(STATUS "========== Sanitizer Configuration ==========")
    message(STATUS "Sanitizers enabled for Debug builds")

    if(HELIOS_COMPILER_IS_MSVC OR HELIOS_COMPILER_IS_CLANG_CL)
        message(STATUS "  Compiler: MSVC/clang-cl (limited sanitizer support)")
        if(HELIOS_SANITIZER_ADDRESS)
            message(STATUS "  ✓ AddressSanitizer")
        endif()
    else()
        message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
        if(HELIOS_SANITIZER_ADDRESS)
            message(STATUS "  ✓ AddressSanitizer")
        endif()
        if(HELIOS_SANITIZER_UNDEFINED)
            message(STATUS "  ✓ UndefinedBehaviorSanitizer")
        endif()
        if(HELIOS_SANITIZER_THREAD)
            message(STATUS "  ✓ ThreadSanitizer")
        endif()
        if(HELIOS_SANITIZER_MEMORY)
            message(STATUS "  ✓ MemorySanitizer")
        endif()
    endif()

    message(STATUS "==============================================")
    message(STATUS "")
endfunction()
