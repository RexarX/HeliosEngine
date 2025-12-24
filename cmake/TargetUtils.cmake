# Provides small, focused helper functions that do one thing well

include(CheckIPOSupported)

# ============================================================================
# Warning Configuration
# ============================================================================

# Apply standard compiler warnings to a target
function(helios_target_set_warnings TARGET)
    set(MSVC_WARNINGS
        /W4              # High warning level
        /w14242          # Conversion warnings
        /w14254          # Operator conversion
        /w14263          # Member function doesn't override
        /w14265          # Virtual destructor warning
        /w14287          # Unsigned/negative constant mismatch
        /we4289          # Loop variable used outside scope
        /w14296          # Expression is always true/false
        /w14311          # Pointer truncation
        /w14545 /w14546 /w14547 /w14549 /w14555 # Suspicious operations
        /w14619          # Unknown warning number
        /w14640          # Thread-unsafe static initialization
        /w14826          # Wide character conversion
        /w14905 /w14906  # String literal casts
        /w14928          # Illegal copy initialization
    )

    set(CLANG_WARNINGS
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wconversion
        -Wsign-conversion
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
        -Wimplicit-fallthrough
    )

    set(GCC_WARNINGS
        ${CLANG_WARNINGS}
        -Wmisleading-indentation
        -Wduplicated-cond
        -Wduplicated-branches
        -Wlogical-op
        -Wuseless-cast
    )

    if(HELIOS_ENABLE_WARNINGS_AS_ERRORS)
        list(APPEND MSVC_WARNINGS /WX)
        list(APPEND CLANG_WARNINGS -Werror)
        list(APPEND GCC_WARNINGS -Werror)
    endif()

    target_compile_options(${TARGET} PRIVATE
        # MSVC and clang-cl (MSVC frontend) use MSVC-style warnings
        $<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<PLATFORM_ID:Windows>>>:${MSVC_WARNINGS}>
        # Clang on Unix-like systems
        $<$<AND:$<CXX_COMPILER_ID:Clang>,$<NOT:$<PLATFORM_ID:Windows>>>:${CLANG_WARNINGS}>
        # GCC
        $<$<CXX_COMPILER_ID:GNU>:${GCC_WARNINGS}>
    )
endfunction()

# ============================================================================
# Optimization Configuration
# ============================================================================

# Apply standard optimization flags to a target
function(helios_target_set_optimization TARGET)
    target_compile_options(${TARGET} PRIVATE
        # MSVC and clang-cl (MSVC frontend)
        $<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<PLATFORM_ID:Windows>>>:
            /Zc:preprocessor
            $<$<CONFIG:Debug>:/Od /Zi /RTC1>
            $<$<CONFIG:RelWithDebInfo>:/O2 /Zi /DNDEBUG>
            $<$<CONFIG:Release>:/O2 /Ob2 /DNDEBUG>
        >
        # GCC and Clang on Unix-like systems
        $<$<AND:$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>,$<NOT:$<PLATFORM_ID:Windows>>>:
            $<$<CONFIG:Debug>:-O0 -g3 -ggdb>
            $<$<CONFIG:RelWithDebInfo>:-O2 -g -DNDEBUG>
            $<$<CONFIG:Release>:-O3 -DNDEBUG>
        >
    )

    # Workaround for Clang < 21: std::forward_like builtin causes issues
    # See: https://github.com/llvm/llvm-project/issues/64029
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "21.0")
            target_compile_options(${TARGET} PRIVATE -fno-builtin-std-forward_like)
        endif()
    endif()
endfunction()

# Enable Link-Time Optimization (LTO) for a target
function(helios_target_enable_lto TARGET)
    # If global IPO is already enabled via CMAKE_INTERPROCEDURAL_OPTIMIZATION_*,
    # the target will inherit it automatically - no need to check again
    if(${HELIOS_ENABLE_LTO})
        # Global LTO already enabled, target will inherit
        return()
    endif()

    # Check if IPO is supported for this specific target
    check_ipo_supported(RESULT _ipo_supported OUTPUT _ipo_error LANGUAGES CXX)
    if(_ipo_supported)
        set_target_properties(${TARGET} PROPERTIES
            INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON
            INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
        )
    else()
        # Only warn in Debug mode - LTO failures in Release should be more visible
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            message(STATUS "LTO not supported for target ${TARGET} (not needed in Debug)")
        else()
            message(WARNING "LTO not supported for target ${TARGET}: ${_ipo_error}")
        endif()
    endif()
endfunction()

# ============================================================================
# Platform Configuration
# ============================================================================

# Apply platform-specific compile definitions to a target
function(helios_target_set_platform TARGET)
    target_compile_definitions(${TARGET} PUBLIC
        $<$<PLATFORM_ID:Windows>:HELIOS_PLATFORM_WINDOWS>
        $<$<PLATFORM_ID:Linux>:HELIOS_PLATFORM_LINUX>
        $<$<PLATFORM_ID:Darwin>:HELIOS_PLATFORM_MACOS>
        $<$<CONFIG:Debug>:HELIOS_DEBUG_MODE>
        $<$<CONFIG:RelWithDebInfo>:HELIOS_RELEASE_WITH_DEBUG_INFO_MODE>
        $<$<CONFIG:Release>:HELIOS_RELEASE_MODE NDEBUG>
    )

    if(WIN32)
        target_compile_definitions(${TARGET} PRIVATE
            UNICODE _UNICODE
            WIN32_LEAN_AND_MEAN
            NOMINMAX
        )
    endif()
endfunction()

function(helios_target_set_shared_lib_platform TARGET)
    if(WIN32 AND MSVC)
        set_target_properties(${TARGET} PROPERTIES
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
        )
    elseif(UNIX AND NOT APPLE)
        target_compile_options(${TARGET} PRIVATE -fPIC)
    endif()
endfunction()

# ============================================================================
# Output Directory Configuration
# ============================================================================

# Set standard output directories for a target
function(helios_target_set_output_dirs TARGET)
    cmake_parse_arguments(ARG "" "CUSTOM_FOLDER" "" ${ARGN})

    if(NOT ARG_CUSTOM_FOLDER)
        set(ARG_CUSTOM_FOLDER "")
    endif()

    if(ARG_CUSTOM_FOLDER)
        set(_output_dir "${HELIOS_ROOT_DIR}/bin/${ARG_CUSTOM_FOLDER}/$<LOWER_CASE:$<CONFIG>>-$<LOWER_CASE:${CMAKE_SYSTEM_NAME}>-$<LOWER_CASE:${CMAKE_SYSTEM_PROCESSOR}>")
    else()
        set(_output_dir "${HELIOS_ROOT_DIR}/bin/$<LOWER_CASE:$<CONFIG>>-$<LOWER_CASE:${CMAKE_SYSTEM_NAME}>-$<LOWER_CASE:${CMAKE_SYSTEM_PROCESSOR}>")
    endif()

    set_target_properties(${TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${_output_dir}"
        LIBRARY_OUTPUT_DIRECTORY "${_output_dir}"
        ARCHIVE_OUTPUT_DIRECTORY "${_output_dir}"
    )
endfunction()


# ============================================================================
# C++ Standard Configuration
# ============================================================================

# Set C++ standard for a target
function(helios_target_set_cxx_standard TARGET)
    cmake_parse_arguments(ARG "" "STANDARD" "" ${ARGN})

    if(NOT ARG_STANDARD)
        set(ARG_STANDARD 23)
    endif()

    set_target_properties(${TARGET} PROPERTIES
        CXX_STANDARD ${ARG_STANDARD}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
endfunction()

# ============================================================================
# IDE Organization
# ============================================================================

# Set IDE folder for a target
function(helios_target_set_folder TARGET FOLDER_NAME)
    set_target_properties(${TARGET} PROPERTIES FOLDER ${FOLDER_NAME})
endfunction()

# ============================================================================
# Unity Build Configuration
# ============================================================================

# Enable unity build for a target
function(helios_target_enable_unity_build TARGET)
    cmake_parse_arguments(ARG "" "BATCH_SIZE" "EXCLUDE_FILES" ${ARGN})

    if(NOT ARG_BATCH_SIZE)
        set(ARG_BATCH_SIZE 16)
    endif()

    set_target_properties(${TARGET} PROPERTIES
        UNITY_BUILD ON
        UNITY_BUILD_MODE BATCH
        UNITY_BUILD_BATCH_SIZE ${ARG_BATCH_SIZE}
    )

    # Exclude specified files from unity build
    if(ARG_EXCLUDE_FILES)
        set_source_files_properties(${ARG_EXCLUDE_FILES}
            PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
        )
    endif()
endfunction()

# ============================================================================
# Precompiled Headers
# ============================================================================

# Add precompiled header to a target
function(helios_target_add_pch TARGET PCH_FILE)
    cmake_parse_arguments(ARG "PUBLIC;INTERFACE" "" "" ${ARGN})

    set(VISIBILITY PRIVATE)
    if(ARG_PUBLIC)
        set(VISIBILITY PUBLIC)
    elseif(ARG_INTERFACE)
        set(VISIBILITY INTERFACE)
    endif()

    target_precompile_headers(${TARGET} ${VISIBILITY}
        $<$<COMPILE_LANGUAGE:CXX>:${PCH_FILE}>
    )
endfunction()

# ============================================================================
# External Dependency Configuration
# ============================================================================

# Mark external dependencies with SYSTEM includes to suppress warnings
# This prevents warnings from third-party code from cluttering build output
function(helios_target_mark_dependencies_as_system TARGET)
    # Get all link libraries for the target
    get_target_property(_link_libs ${TARGET} LINK_LIBRARIES)

    if(_link_libs AND NOT _link_libs STREQUAL "_link_libs-NOTFOUND")
        foreach(_lib ${_link_libs})
            if(TARGET ${_lib})
                # Get the include directories from the dependency
                get_target_property(_lib_includes ${_lib} INTERFACE_INCLUDE_DIRECTORIES)

                if(_lib_includes AND NOT _lib_includes STREQUAL "_lib_includes-NOTFOUND")
                    # Mark them as system includes
                    set_target_properties(${_lib} PROPERTIES
                        INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_lib_includes}"
                    )
                endif()
            endif()
        endforeach()
    endif()
endfunction()

# Link to external target with SYSTEM includes to suppress warnings
# Usage: helios_target_link_system_libraries(my_target PUBLIC external::lib1 external::lib2)
function(helios_target_link_system_libraries TARGET VISIBILITY)
    cmake_parse_arguments(ARG "" "" "" ${ARGN})

    foreach(_lib ${ARG_UNPARSED_ARGUMENTS})
        if(TARGET ${_lib})
            # Get the include directories
            get_target_property(_lib_includes ${_lib} INTERFACE_INCLUDE_DIRECTORIES)

            if(_lib_includes AND NOT _lib_includes STREQUAL "_lib_includes-NOTFOUND")
                # Mark as system before linking
                set_target_properties(${_lib} PROPERTIES
                    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_lib_includes}"
                )
            endif()

            # Now link the library
            target_link_libraries(${TARGET} ${VISIBILITY} ${_lib})
        else()
            # If not a target, just link normally
            target_link_libraries(${TARGET} ${VISIBILITY} ${_lib})
        endif()
    endforeach()
endfunction()

# ============================================================================
# Dynamic Library Management
# ============================================================================

# Copy dynamic library dependencies to target output directory
# This ensures that executables can find their runtime dependencies
# Usage: helios_copy_runtime_dependencies(TARGET my_target LIBRARIES lib1 lib2...)
function(helios_copy_runtime_dependencies TARGET)
    cmake_parse_arguments(ARG "" "" "LIBRARIES" ${ARGN})

    if(NOT ARG_LIBRARIES)
        return()
    endif()

    foreach(_lib ${ARG_LIBRARIES})
        if(TARGET ${_lib})
            # Get the library type
            get_target_property(_lib_type ${_lib} TYPE)

            if(_lib_type STREQUAL "SHARED_LIBRARY")
                # Add a post-build command to copy the shared library
                add_custom_command(TARGET ${TARGET} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        $<TARGET_FILE:${_lib}>
                        $<TARGET_FILE_DIR:${TARGET}>
                    COMMENT "Copying ${_lib} runtime dependency to ${TARGET} output directory"
                    VERBATIM
                )
            endif()
        else()
            # Handle non-target libraries (file paths)
            if(EXISTS "${_lib}" AND _lib MATCHES "\\.(so|dll|dylib)(\\..*)?$")
                add_custom_command(TARGET ${TARGET} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${_lib}"
                        $<TARGET_FILE_DIR:${TARGET}>
                    COMMENT "Copying ${_lib} runtime dependency to ${TARGET} output directory"
                    VERBATIM
                )
            endif()
        endif()
    endforeach()
endfunction()
