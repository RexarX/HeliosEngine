# Provides small, focused helper functions that do one thing well

include(CheckIPOSupported)
include(Primitives)

# ============================================================================
# Warning Configuration
# ============================================================================

#[[
    helios_target_set_warnings(<target>)

    Applies the standard Helios warning set to a non-interface target.
]]
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

#[[
    helios_target_set_test_warnings(<target>)

    Applies standard warnings plus doctest-friendly suppressions for tests.
]]
function(helios_target_set_test_warnings TARGET)
  helios_target_set_warnings(${TARGET})

  # doctest SUBCASE blocks routinely shadow outer locals; suppress in tests only.
  target_compile_options(${TARGET} PRIVATE
      $<$<OR:$<CXX_COMPILER_ID:GNU>,$<AND:$<CXX_COMPILER_ID:Clang>,$<NOT:$<PLATFORM_ID:Windows>>>>:-Wno-shadow>
      $<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<PLATFORM_ID:Windows>>>:/wd4456>
  )
endfunction()

# ============================================================================
# Optimization Configuration
# ============================================================================

#[[
    helios_target_set_optimization(<target>)

    Applies configuration-specific optimization and debug compile options.
]]
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

  # Enable incremental linking for Debug builds on MSVC
  # This significantly speeds up link times during development
  get_target_property(_target_type ${TARGET} TYPE)
  if(_target_type STREQUAL "EXECUTABLE" OR _target_type STREQUAL "SHARED_LIBRARY")
    target_link_options(${TARGET} PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<PLATFORM_ID:Windows>>>:
            $<$<CONFIG:Debug>:/INCREMENTAL>
            $<$<NOT:$<CONFIG:Debug>>:/INCREMENTAL:NO>
        >
    )
  endif()

  # Workaround for Clang < 21: std::forward_like builtin causes issues
  # See: https://github.com/llvm/llvm-project/issues/64029
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "21.0")
      target_compile_options(${TARGET} PRIVATE -fno-builtin-std-forward_like)
    endif()
  endif()
endfunction()

#[[
    helios_target_enable_lto(<target>)

    Enables IPO/LTO properties for Release and RelWithDebInfo when supported.
]]
function(helios_target_enable_lto TARGET)
  if(NOT HELIOS_ENABLE_LTO)
    return()
  endif()

  if(NOT DEFINED HELIOS_IPO_CACHED)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT HELIOS_IPO_SUPPORTED OUTPUT _ipo_error LANGUAGES CXX)
    set(HELIOS_IPO_SUPPORTED "${HELIOS_IPO_SUPPORTED}" CACHE INTERNAL "IPO/LTO support result")
    set(HELIOS_IPO_CACHED TRUE CACHE INTERNAL "IPO/LTO support has been checked globally")
  endif()

  if(HELIOS_IPO_SUPPORTED)
    set_target_properties(${TARGET} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON
        INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
    )
  endif()
endfunction()

# ============================================================================
# Platform Configuration
# ============================================================================

#[[
    helios_target_set_platform(<target>)

    Adds Helios platform/configuration compile definitions to a target.
]]
function(helios_target_set_platform TARGET)
  target_compile_definitions(${TARGET} PRIVATE
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

#[[
    helios_target_set_shared_lib_platform(<target>)

    Applies shared-library platform properties such as PIC and MSVC runtime.
]]
function(helios_target_set_shared_lib_platform TARGET)
  if(WIN32 AND MSVC)
    set_target_properties(${TARGET} PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )
  endif()

  # Use CMake PIC property instead of compiler-specific flags.
  set_target_properties(${TARGET} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endfunction()

# ============================================================================
# Output Directory Configuration
# ============================================================================

#[[
    helios_target_set_output_dirs(<target> [CUSTOM_FOLDER <folder>])

    Places target outputs under the standard Helios bin layout.
]]
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

#[[
    helios_target_set_cxx_standard(<target> [STANDARD <version>])

    Sets CXX_STANDARD, CXX_STANDARD_REQUIRED, and disables extensions.
]]
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

#[[
    helios_target_set_folder(<target> <folder>)

    Sets the IDE folder property for generated project views.
]]
function(helios_target_set_folder TARGET FOLDER_NAME)
  set_target_properties(${TARGET} PROPERTIES FOLDER ${FOLDER_NAME})
endfunction()

# ============================================================================
# Unity Build Configuration
# ============================================================================

#[[
    helios_target_enable_unity_build(<target> [BATCH_SIZE <n>] [EXCLUDE_FILES <files...>])

    Enables CMake unity builds and optionally excludes specific files.
]]
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

#[[
    helios_target_add_pch(<target> <pch-file> [PUBLIC|INTERFACE])

    Adds a C++ precompiled header to a target.
]]
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

# Reuse a module's precompiled header in a test (or other consumer) target.
function(_helios_share_module_pch_context CONSUMER_TARGET MODULE_TARGET)
  get_target_property(_defs ${MODULE_TARGET} COMPILE_DEFINITIONS)
  if(_defs AND NOT _defs STREQUAL "_defs-NOTFOUND")
    target_compile_definitions(${CONSUMER_TARGET} PRIVATE ${_defs})
  endif()

  get_target_property(_link_libs ${MODULE_TARGET} LINK_LIBRARIES)
  if(_link_libs AND NOT _link_libs STREQUAL "_link_libs-NOTFOUND")
    target_link_libraries(${CONSUMER_TARGET} PRIVATE ${_link_libs})
  endif()

  get_target_property(_include_dirs ${MODULE_TARGET} INCLUDE_DIRECTORIES)
  if(_include_dirs AND NOT _include_dirs STREQUAL "_include_dirs-NOTFOUND")
    target_include_directories(${CONSUMER_TARGET} PRIVATE ${_include_dirs})
  endif()
endfunction()

#[[
    helios_target_reuse_module_pch(<consumer-target> <module-name>)

    Reuses or mirrors a module target's PCH setup on a consumer target.
]]
function(helios_target_reuse_module_pch CONSUMER_TARGET MODULE_NAME)
  set(_module_target "helios_module_${MODULE_NAME}")
  if(NOT TARGET ${_module_target})
    return()
  endif()

  get_target_property(_module_type ${_module_target} TYPE)
  if(_module_type STREQUAL "INTERFACE_LIBRARY")
    return()
  endif()

  get_target_property(_pch ${_module_target} PRECOMPILE_HEADERS)
  if(NOT _pch OR _pch STREQUAL "_pch-NOTFOUND")
    return()
  endif()

  # REUSE_FROM is reliable on MSVC/clang-cl. On ELF, static-library PCHs use
  # -fPIC while executables default to -fPIE, so compile the PCH locally on the
  # test target and mirror the module's private link/include context instead.
  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"
      OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND WIN32))
    target_precompile_headers(${CONSUMER_TARGET} REUSE_FROM ${_module_target})
  else()
    target_precompile_headers(${CONSUMER_TARGET} PRIVATE ${_pch})
  endif()

  _helios_share_module_pch_context(${CONSUMER_TARGET} ${_module_target})
endfunction()

# ============================================================================
# External Dependency Configuration
# ============================================================================

#[[
    helios_target_mark_dependencies_as_system(<target>)

    Marks linked target dependencies' interface includes as SYSTEM.
]]
function(helios_target_mark_dependencies_as_system TARGET)
  get_target_property(_link_libs ${TARGET} LINK_LIBRARIES)
  if(_link_libs AND NOT _link_libs STREQUAL "_link_libs-NOTFOUND")
    helios_mark_system_includes(${_link_libs})
  endif()
endfunction()

#[[
    helios_target_link_system_libraries(<target> <visibility> <libs...>)

    Marks target libraries as SYSTEM where possible, then links them.
]]
function(helios_target_link_system_libraries TARGET VISIBILITY)
  cmake_parse_arguments(ARG "" "" "" ${ARGN})

  foreach(_lib ${ARG_UNPARSED_ARGUMENTS})
    if(TARGET ${_lib})
      helios_mark_system_includes(${_lib})
    endif()
    target_link_libraries(${TARGET} ${VISIBILITY} ${_lib})
  endforeach()
endfunction()

# ============================================================================
# Dynamic Library Management
# ============================================================================

#[[
    helios_copy_runtime_dependencies(<target> LIBRARIES <libs...>)

    Copies shared library targets or shared library file paths beside a target
    after build.
]]
function(helios_copy_runtime_dependencies TARGET)
  cmake_parse_arguments(ARG "" "" "LIBRARIES" ${ARGN})

  if(NOT ARG_LIBRARIES)
    return()
  endif()

  foreach(_lib ${ARG_LIBRARIES})
    if(TARGET ${_lib})
      helios_copy_shared_lib(CONSUMER ${TARGET} PROVIDER ${_lib})
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
