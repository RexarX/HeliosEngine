# Provides centralized SIMD flag selection and safe application to targets

if(DEFINED HELIOS_SIMD_UTILS_LOADED)
    return()
endif()
set(HELIOS_SIMD_UTILS_LOADED TRUE)

include(CheckCXXCompilerFlag)

# Cache variable for SIMD level selection
set(HELIOS_SIMD_LEVEL "native" CACHE STRING "SIMD level: none native sse2 sse3 ssse3 sse4.1 sse4.2 avx avx2 avx512")
set_property(CACHE HELIOS_SIMD_LEVEL PROPERTY STRINGS
    "none" "native" "sse2" "sse3" "ssse3" "sse4.1" "sse4.2" "avx" "avx2" "avx512"
)

# Internal: Map SIMD level to compiler flags
function(helios_simd_build_flag_lists LEVEL OUT_GNU_FLAGS OUT_MSVC_FLAGS)
    set(_gnu "")
    set(_msvc "")

    string(TOLOWER "${LEVEL}" _lvl)

    if(_lvl STREQUAL "none" OR _lvl STREQUAL "")
        # No SIMD flags
    elseif(_lvl STREQUAL "native")
        list(APPEND _gnu "-march=native")
        # MSVC: no direct equivalent to -march=native
    elseif(_lvl STREQUAL "sse2")
        list(APPEND _gnu "-msse2")
        list(APPEND _msvc "/arch:SSE2")
    elseif(_lvl STREQUAL "sse3")
        list(APPEND _gnu "-msse3")
        list(APPEND _msvc "/arch:SSE2")
    elseif(_lvl STREQUAL "ssse3")
        list(APPEND _gnu "-mssse3")
        list(APPEND _msvc "/arch:SSE2")
    elseif(_lvl STREQUAL "sse4.1" OR _lvl STREQUAL "sse4_1")
        list(APPEND _gnu "-msse4.1")
        list(APPEND _msvc "/arch:SSE2")
    elseif(_lvl STREQUAL "sse4.2" OR _lvl STREQUAL "sse4_2")
        list(APPEND _gnu "-msse4.2")
        list(APPEND _msvc "/arch:SSE2")
    elseif(_lvl STREQUAL "avx")
        list(APPEND _gnu "-mavx")
        list(APPEND _msvc "/arch:AVX")
    elseif(_lvl STREQUAL "avx2")
        list(APPEND _gnu "-mavx2")
        list(APPEND _msvc "/arch:AVX2")
    elseif(_lvl STREQUAL "avx512" OR _lvl STREQUAL "avx512f")
        list(APPEND _gnu "-mavx512f" "-mavx512dq")
        list(APPEND _msvc "/arch:AVX512")
    else()
        message(WARNING "Unknown SIMD level '${LEVEL}'; treating as 'none'")
    endif()

    set(${OUT_GNU_FLAGS} "${_gnu}" PARENT_SCOPE)
    set(${OUT_MSVC_FLAGS} "${_msvc}" PARENT_SCOPE)
endfunction()

# Internal: Filter flags to only those supported by the compiler
function(helios_simd_filter_supported_flags FLAGS_LIST_VAR OUT_VAR)
    set(_in ${${FLAGS_LIST_VAR}})
    set(_accepted "")

    foreach(_f IN LISTS _in)
        string(MAKE_C_IDENTIFIER "HELIOS_SIMD_${_f}" _var_name)
        check_cxx_compiler_flag("${_f}" ${_var_name})
        if(${_var_name})
            list(APPEND _accepted "${_f}")
        else()
            message(VERBOSE "SIMD flag '${_f}' is not supported by the current compiler; skipping.")
        endif()
    endforeach()

    set(${OUT_VAR} "${_accepted}" PARENT_SCOPE)
endfunction()

# Apply SIMD flags to a target
# Usage:
#   helios_simd_apply_to_target(TARGET mylib LEVEL avx2)
#   helios_simd_apply_to_target(TARGET mylib) # uses HELIOS_SIMD_LEVEL
function(helios_simd_apply_to_target)
    cmake_parse_arguments(SIMD "" "TARGET;LEVEL" "" ${ARGN})

    if(NOT SIMD_TARGET)
        message(FATAL_ERROR "helios_simd_apply_to_target: TARGET is required")
    endif()

    set(_level "${SIMD_LEVEL}")
    if(NOT SIMD_LEVEL)
        set(_level "${HELIOS_SIMD_LEVEL}")
    endif()

    if(NOT _level OR _level STREQUAL "none")
        return()
    endif()

    # Build flag lists
    helios_simd_build_flag_lists("${_level}" _gnu_flags _msvc_flags)

    # Filter to supported flags
    helios_simd_filter_supported_flags(_gnu_flags _gnu_accepted)
    helios_simd_filter_supported_flags(_msvc_flags _msvc_accepted)

    # Apply flags only for Release and RelWithDebInfo
    if(_gnu_accepted)
        string(REPLACE ";" " " _gnu_joined "${_gnu_accepted}")
        target_compile_options(${SIMD_TARGET} PRIVATE
            $<$<AND:$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>,$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>>:${_gnu_joined}>
        )
    endif()

    if(_msvc_accepted)
        string(REPLACE ";" " " _msvc_joined "${_msvc_accepted}")
        target_compile_options(${SIMD_TARGET} PRIVATE
            $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>>:${_msvc_joined}>
        )
    endif()
endfunction()

# Create INTERFACE library targets for each SIMD level
# Usage:
#   helios_simd_create_interface_libs()
#   target_link_libraries(mytarget PRIVATE simd::avx2)
function(helios_simd_create_interface_libs)
    set(_levels "none" "native" "sse2" "sse3" "ssse3" "sse4.1" "sse4.2" "avx" "avx2" "avx512")

    foreach(_lvl IN LISTS _levels)
        if(_lvl STREQUAL "none")
            if(NOT TARGET simd::none)
                add_library(simd::none INTERFACE)
            endif()
            continue()
        endif()

        # Create valid target name
        string(REPLACE "." "_" _tag "${_lvl}")
        set(_tgt "simd::${_tag}")

        if(TARGET ${_tgt})
            continue()
        endif()

        add_library(${_tgt} INTERFACE)

        # Get flags
        helios_simd_build_flag_lists("${_lvl}" _g _m)
        helios_simd_filter_supported_flags(_g _g_ok)
        helios_simd_filter_supported_flags(_m _m_ok)

        set(_opts "")
        if(_g_ok)
            string(REPLACE ";" " " _g_joined "${_g_ok}")
            list(APPEND _opts
                $<$<AND:$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>,$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>>:${_g_joined}>
            )
        endif()

        if(_m_ok)
            string(REPLACE ";" " " _m_joined "${_m_ok}")
            list(APPEND _opts
                $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>>:${_m_joined}>
            )
        endif()

        if(_opts)
            target_compile_options(${_tgt} INTERFACE ${_opts})
        endif()
    endforeach()
endfunction()

# Utility: Get list of known SIMD levels
function(helios_simd_known_levels OUT_VAR)
    set(_levels "none" "native" "sse2" "sse3" "ssse3" "sse4.1" "sse4.2" "avx" "avx2" "avx512")
    set(${OUT_VAR} "${_levels}" PARENT_SCOPE)
endfunction()
