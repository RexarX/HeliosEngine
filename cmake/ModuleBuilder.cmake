# Helios Engine Module Builder
#
# Unified module creation entry point: helios_module().
# Handles both registration pass (dry-run) and build pass (full target creation).
# Implements DEPENDS, OPTIONAL_DEPENDS, USES, TESTS, SUPPRESS_WARNINGS,
# CONFIGURE_HOOK, and compiler-condition shorthands.

include_guard(GLOBAL)

include(TargetUtils)
include(Sanitizers)
include(TestUtils)

# ============================================================================
# Internal Helpers
# ============================================================================

function(_helios_parse_visibility_args)
  set(options "")
  set(oneValueArgs PUBLIC_VAR PRIVATE_VAR INTERFACE_VAR DEFAULT_VISIBILITY)
  set(multiValueArgs INPUT)

  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT ARG_DEFAULT_VISIBILITY)
    set(ARG_DEFAULT_VISIBILITY "PUBLIC")
  endif()

  set(_public_items)
  set(_private_items)
  set(_interface_items)

  set(_current_visibility "${ARG_DEFAULT_VISIBILITY}")

  foreach(_item ${ARG_INPUT})
    if("${_item}" STREQUAL "PUBLIC")
      set(_current_visibility "PUBLIC")
    elseif("${_item}" STREQUAL "PRIVATE")
      set(_current_visibility "PRIVATE")
    elseif("${_item}" STREQUAL "INTERFACE")
      set(_current_visibility "INTERFACE")
    else()
      if("${_current_visibility}" STREQUAL "PUBLIC")
        list(APPEND _public_items "${_item}")
      elseif("${_current_visibility}" STREQUAL "PRIVATE")
        list(APPEND _private_items "${_item}")
      elseif("${_current_visibility}" STREQUAL "INTERFACE")
        list(APPEND _interface_items "${_item}")
      endif()
    endif()
  endforeach()

  if(ARG_PUBLIC_VAR)
    set(${ARG_PUBLIC_VAR} "${_public_items}" PARENT_SCOPE)
  endif()
  if(ARG_PRIVATE_VAR)
    set(${ARG_PRIVATE_VAR} "${_private_items}" PARENT_SCOPE)
  endif()
  if(ARG_INTERFACE_VAR)
    set(${ARG_INTERFACE_VAR} "${_interface_items}" PARENT_SCOPE)
  endif()
endfunction()

function(_helios_is_source_file FILENAME OUTPUT_VAR)
  get_filename_component(_ext "${FILENAME}" EXT)
  string(TOLOWER "${_ext}" _ext_lower)
  set(_source_extensions ".cpp" ".cxx" ".cc" ".c" ".c++" ".cu" ".mm" ".m")

  if("${_ext_lower}" IN_LIST _source_extensions)
    set(${OUTPUT_VAR} TRUE PARENT_SCOPE)
  else()
    set(${OUTPUT_VAR} FALSE PARENT_SCOPE)
  endif()
endfunction()

function(_helios_module_has_source_files SOURCES_LIST OUTPUT_VAR)
  foreach(_file ${SOURCES_LIST})
    _helios_is_source_file("${_file}" _is_source)
    if(_is_source)
      set(${OUTPUT_VAR} TRUE PARENT_SCOPE)
      return()
    endif()
  endforeach()
  set(${OUTPUT_VAR} FALSE PARENT_SCOPE)
endfunction()

function(_helios_validate_module_params)
  set(options STATIC SHARED)
  set(oneValueArgs NAME)
  set(multiValueArgs SOURCES HEADERS)

  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT ARG_NAME)
    message(FATAL_ERROR "Module validation: NAME is required")
  endif()

  if(ARG_STATIC AND ARG_SHARED)
    message(FATAL_ERROR
            "helios_module(${ARG_NAME}): Conflicting library type specifications: STATIC, SHARED. "
            "Only one of STATIC or SHARED can be specified.")
  endif()
endfunction()

# ============================================================================
# Expanded compile-options shorthands
# ============================================================================

function(_helios_expand_compiler_shorthands RAW_OPTS OUT_VAR)
  set(_expanded "")
  set(_pending_shorthand FALSE)
  set(_shorthand_body "")

  foreach(_opt ${RAW_OPTS})
    # Check for shorthand patterns like CLANG(...), GCC(...), MSVC(...), DEBUG(...), RELEASE(...), NOT_WINDOWS(...)
    if(_opt MATCHES "^(CLANG|GCC|MSVC|DEBUG|RELEASE|NOT_WINDOWS|WINDOWS|LINUX|MACOS)\\((.+)\\)$")
      set(_keyword "${CMAKE_MATCH_1}")
      set(_body "${CMAKE_MATCH_2}")
      if(_keyword STREQUAL "CLANG")
        list(APPEND _expanded "$<$<CXX_COMPILER_ID:Clang>:${_body}>")
      elseif(_keyword STREQUAL "GCC")
        list(APPEND _expanded "$<$<CXX_COMPILER_ID:GNU>:${_body}>")
      elseif(_keyword STREQUAL "MSVC")
        list(APPEND _expanded "$<$<CXX_COMPILER_ID:MSVC>:${_body}>")
      elseif(_keyword STREQUAL "DEBUG")
        list(APPEND _expanded "$<$<CONFIG:Debug>:${_body}>")
      elseif(_keyword STREQUAL "RELEASE")
        list(APPEND _expanded "$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:${_body}>")
      elseif(_keyword STREQUAL "NOT_WINDOWS")
        list(APPEND _expanded "$<$<NOT:$<PLATFORM_ID:Windows>>:${_body}>")
      elseif(_keyword STREQUAL "WINDOWS")
        list(APPEND _expanded "$<$<PLATFORM_ID:Windows>:${_body}>")
      elseif(_keyword STREQUAL "LINUX")
        list(APPEND _expanded "$<$<PLATFORM_ID:Linux>:${_body}>")
      elseif(_keyword STREQUAL "MACOS")
        list(APPEND _expanded "$<$<PLATFORM_ID:Darwin>:${_body}>")
      endif()
    else()
      list(APPEND _expanded "${_opt}")
    endif()
  endforeach()

  set(${OUT_VAR} "${_expanded}" PARENT_SCOPE)
endfunction()

# ============================================================================
# Internal: Process USES entries
# ============================================================================

function(_helios_process_uses TARGET DEF_VISIBILITY USES_LIST)
  set(_visibility "${DEF_VISIBILITY}")
  set(_state "expect_dep_file")
  set(_dep_file "")
  set(_link_target "")

  get_target_property(_target_type ${TARGET} TYPE)
  if(_target_type STREQUAL "INTERFACE_LIBRARY")
    set(_forced_visibility INTERFACE)
  else()
    set(_forced_visibility "")
  endif()

  foreach(_item ${USES_LIST})
    if(_state STREQUAL "expect_dep_file")
      set(_dep_file "${_item}")
      set(_state "expect_visibility_or_target")
    elseif(_state STREQUAL "expect_visibility_or_target")
      if("${_item}" STREQUAL "PUBLIC" OR "${_item}" STREQUAL "PRIVATE" OR "${_item}" STREQUAL "INTERFACE")
        set(_visibility "${_item}")
        set(_state "expect_link_target")
      else()
        set(_link_target "${_item}")
        set(_state "apply")
      endif()
    elseif(_state STREQUAL "expect_link_target")
      set(_link_target "${_item}")
      set(_state "apply")
    endif()

    if(_state STREQUAL "apply")
      helios_require_dependency(${_dep_file})

      if(_forced_visibility)
        set(_link_visibility ${_forced_visibility})
      else()
        set(_link_visibility ${_visibility})
      endif()

      if(TARGET ${_link_target})
        target_link_libraries(${TARGET} ${_link_visibility} ${_link_target})
      else()
        message(WARNING "USES target '${_link_target}' not found for dependency '${_dep_file}'")
      endif()

      set(_dep_file "")
      set(_link_target "")
      set(_state "expect_dep_file")
    endif()
  endforeach()
endfunction()

# ============================================================================
# Internal: Parse DEPENDS with visibility
# ============================================================================

function(_helios_parse_module_deps DEPENDS_LIST OUT_PUBLIC OUT_PRIVATE OUT_INTERFACE)
  set(_pub)
  set(_priv)
  set(_iface)
  set(_cur "PUBLIC")

  foreach(_item ${DEPENDS_LIST})
    if("${_item}" STREQUAL "PUBLIC")
      set(_cur "PUBLIC")
    elseif("${_item}" STREQUAL "PRIVATE")
      set(_cur "PRIVATE")
    elseif("${_item}" STREQUAL "INTERFACE")
      set(_cur "INTERFACE")
    else()
      if(_cur STREQUAL "PUBLIC")
        list(APPEND _pub "${_item}")
      elseif(_cur STREQUAL "PRIVATE")
        list(APPEND _priv "${_item}")
      else()
        list(APPEND _iface "${_item}")
      endif()
    endif()
  endforeach()

  set(${OUT_PUBLIC} "${_pub}" PARENT_SCOPE)
  set(${OUT_PRIVATE} "${_priv}" PARENT_SCOPE)
  set(${OUT_INTERFACE} "${_iface}" PARENT_SCOPE)
endfunction()

# ============================================================================
# Internal: Link a Helios module dependency with availability propagation
# ============================================================================

function(_helios_link_helios_module TARGET MODULE_NAME LINK_VISIBILITY)
  if(NOT TARGET helios::module::${MODULE_NAME})
    return()
  endif()

  string(TOUPPER "${MODULE_NAME}" _module_upper)

  get_target_property(_target_type ${TARGET} TYPE)
  if(_target_type STREQUAL "INTERFACE_LIBRARY")
    set(LINK_VISIBILITY "INTERFACE")
  endif()

  target_link_libraries(${TARGET} ${LINK_VISIBILITY} helios::module::${MODULE_NAME})
  target_compile_definitions(${TARGET} ${LINK_VISIBILITY}
        HELIOS_MODULE_${_module_upper}_AVAILABLE
    )
endfunction()

# ============================================================================
# Internal: Handle SUPPRESS_WARNINGS
# ============================================================================

function(_helios_apply_suppress_warnings TARGET WARNING_LIST)
  foreach(_warn ${WARNING_LIST})
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
      target_compile_options(${TARGET} PRIVATE "/wd${_warn}")
    else()
      target_compile_options(${TARGET} PRIVATE "-Wno-${_warn}")
    endif()
  endforeach()
endfunction()

# ============================================================================
# helios_module — Unified Module Entry Point
# ============================================================================

#[[
    helios_module(
        NAME        <name>
        VERSION     <version>
        DESCRIPTION <description>

        SOURCES <files...>
        HEADERS <files...>

        # Helios module deps with visibility
        DEPENDS
            [PUBLIC    <module> ...]
            [PRIVATE   <module> ...]

        OPTIONAL_DEPENDS
            [PRIVATE  <module> ...]

        # External deps: load + link in one declaration
        USES
            <dep_file> <visibility> <target> ...

        # Already-loaded external targets
        DEPENDENCIES
            [PUBLIC    <target> ...]
            [PRIVATE   <target> ...]
            [INTERFACE <target> ...]

        COMPILE_DEFINITIONS
            [PUBLIC    <def> ...]
            [PRIVATE   <def> ...]

        COMPILE_OPTIONS
            [PRIVATE   CLANG(...) GCC(...) MSVC(...) DEBUG(...) RELEASE(...) ...]

        INCLUDE_DIRECTORIES
            [PUBLIC    <dir> ...]
            [PRIVATE   <dir> ...]

        PCH <file>
        STATIC
        SHARED

        # Per-module warning suppression
        SUPPRESS_WARNINGS
            <warning-name> ...

        # Escape hatch
        CONFIGURE_HOOK <function_name>

        # Tests
        TEST_SOURCES <files...>
        TEST_DEPENDENCIES <targets...>

        TESTS
            SUITE <name>
                SOURCES      <files...>
                DEPENDENCIES <targets...>
                DEFINITIONS  PRIVATE <defs...>
                LABELS       <label...>
    )
]]
function(helios_module)
  set(options STATIC SHARED)
  set(oneValueArgs NAME VERSION DESCRIPTION DEFAULT PCH FOLDER OUTPUT_NAME TARGET_NAME CONFIGURE_HOOK)
  set(multiValueArgs
        SOURCES
        HEADERS
        DEPENDS
        OPTIONAL_DEPENDS
        DEPENDENCIES
        USES
        COMPILE_DEFINITIONS
        COMPILE_OPTIONS
        INCLUDE_DIRECTORIES
        SUPPRESS_WARNINGS
        TEST_SOURCES
        TEST_DEPENDENCIES
        TESTS
    )

  cmake_parse_arguments(MODULE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT MODULE_NAME)
    message(FATAL_ERROR "helios_module: NAME is required")
  endif()

  string(TOUPPER "${MODULE_NAME}" _upper_name)

  _helios_module_has_source_files("${MODULE_SOURCES}" _module_has_sources)
  if(_module_has_sources)
    set(_module_header_only FALSE)
  else()
    set(_module_header_only TRUE)
  endif()

  # ========================================================================
  # Registration Pass — dry-run: extract registration info only
  # ========================================================================
  if(HELIOS_REGISTRATION_PASS)
    set(_prereg_args NAME ${MODULE_NAME})

    if(MODULE_DESCRIPTION)
      list(APPEND _prereg_args DESCRIPTION "${MODULE_DESCRIPTION}")
    endif()

    if(DEFINED MODULE_DEFAULT)
      list(APPEND _prereg_args DEFAULT ${MODULE_DEFAULT})
    endif()

    if(_module_header_only)
      list(APPEND _prereg_args HEADER_ONLY)
    endif()

    if(MODULE_VERSION)
      list(APPEND _prereg_args VERSION ${MODULE_VERSION})
    endif()

    if(MODULE_DEPENDS)
      list(APPEND _prereg_args DEPENDS ${MODULE_DEPENDS})
    endif()

    if(MODULE_OPTIONAL_DEPENDS)
      list(APPEND _prereg_args OPTIONAL_DEPENDS ${MODULE_OPTIONAL_DEPENDS})
    endif()

    helios_preregister_module(${_prereg_args})
    return()
  endif()

  # ========================================================================
  # Build Pass — full module creation
  # ========================================================================

  # Check enabled status
  if(DEFINED HELIOS_BUILD_${_upper_name})
    if(NOT HELIOS_BUILD_${_upper_name})
      message(STATUS "Module ${MODULE_NAME} is disabled, skipping")
      return()
    endif()
  endif()

  # Validate registration vs build definition
  if(DEFINED HELIOS_MODULE_${_upper_name}_HEADER_ONLY AND HELIOS_MODULE_${_upper_name}_HEADER_ONLY)
    if(NOT _module_header_only)
      set(_source_files_found)
      foreach(_file ${MODULE_SOURCES})
        _helios_is_source_file("${_file}" _is_source)
        if(_is_source)
          list(APPEND _source_files_found "${_file}")
        endif()
      endforeach()
      string(REPLACE ";" "\n  - " _files_str "${_source_files_found}")
      message(FATAL_ERROR
                "helios_module(${MODULE_NAME}): registered as HEADER_ONLY in Module.cmake "
                "but source files were provided.\n"
                "Found source files:\n  - ${_files_str}")
    endif()
  endif()

  set(HELIOS_MODULE_${_upper_name}_HEADER_ONLY ${_module_header_only}
      CACHE INTERNAL "Module ${MODULE_NAME} is header-only")

  # Validate
  set(_validate_args NAME ${MODULE_NAME})
  if(MODULE_SOURCES)
    list(APPEND _validate_args SOURCES ${MODULE_SOURCES})
  endif()
  if(MODULE_HEADERS)
    list(APPEND _validate_args HEADERS ${MODULE_HEADERS})
  endif()
  if(MODULE_STATIC)
    list(APPEND _validate_args STATIC)
  endif()
  if(MODULE_SHARED)
    list(APPEND _validate_args SHARED)
  endif()
  _helios_validate_module_params(${_validate_args})

  # Defaults
  if(NOT MODULE_VERSION)
    set(MODULE_VERSION "0.1.0")
  endif()
  if(NOT MODULE_TARGET_NAME)
    set(MODULE_TARGET_NAME "helios_module_${MODULE_NAME}")
  endif()
  if(NOT MODULE_FOLDER)
    set(MODULE_FOLDER "Helios/Modules")
  endif()

  # Determine library type
  if(_module_header_only)
    set(_library_type INTERFACE)
    set(_target_scope INTERFACE)
    set(_default_dep_visibility INTERFACE)
  else()
    set(_build_option_name "HELIOS_BUILD_OPTION_${_upper_name}")
    if(MODULE_STATIC)
      set(_default_build_type "STATIC")
    elseif(MODULE_SHARED)
      set(_default_build_type "SHARED")
    else()
      set(_default_build_type "AUTO")
    endif()

    set(${_build_option_name} "${_default_build_type}" CACHE STRING
            "Library type for ${MODULE_NAME} module (AUTO, STATIC, SHARED)")
    set_property(CACHE ${_build_option_name} PROPERTY STRINGS "AUTO" "STATIC" "SHARED")

    set(HELIOS_MODULE_${_upper_name}_SPECIFICATION "${_default_build_type}" CACHE INTERNAL
            "Original specification for ${MODULE_NAME} module")

    set(_build_type_value "${${_build_option_name}}")
    if("${_build_type_value}" STREQUAL "STATIC")
      set(_library_type STATIC)
    elseif("${_build_type_value}" STREQUAL "SHARED")
      set(_library_type SHARED)
    else()
      set(_library_type)
    endif()

    set(_target_scope PUBLIC)
    set(_default_dep_visibility PUBLIC)
  endif()

  # Create library target
  if(_module_header_only)
    add_library(${MODULE_TARGET_NAME} INTERFACE)
  elseif(_library_type)
    add_library(${MODULE_TARGET_NAME} ${_library_type} ${MODULE_SOURCES} ${MODULE_HEADERS})
  else()
    add_library(${MODULE_TARGET_NAME} ${MODULE_SOURCES} ${MODULE_HEADERS})
  endif()

  # Create alias
  add_library(helios::module::${MODULE_NAME} ALIAS ${MODULE_TARGET_NAME})

  # Module availability define
  target_compile_definitions(${MODULE_TARGET_NAME} ${_target_scope}
        HELIOS_MODULE_${_upper_name}_AVAILABLE
    )

  # Standard Helios configurations (non-INTERFACE only)
  if(NOT _module_header_only)
    helios_target_set_cxx_standard(${MODULE_TARGET_NAME} STANDARD 23)
    helios_target_set_platform(${MODULE_TARGET_NAME})
    get_target_property(_actual_type ${MODULE_TARGET_NAME} TYPE)
    if(_actual_type STREQUAL "SHARED_LIBRARY")
      helios_target_set_shared_lib_platform(${MODULE_TARGET_NAME})
    endif()
    helios_target_set_optimization(${MODULE_TARGET_NAME})
    helios_target_set_warnings(${MODULE_TARGET_NAME})
    helios_target_enable_sanitizers(${MODULE_TARGET_NAME})
    helios_target_set_output_dirs(${MODULE_TARGET_NAME})
    helios_target_set_folder(${MODULE_TARGET_NAME} "${MODULE_FOLDER}")

    if(MODULE_OUTPUT_NAME)
      set_target_properties(${MODULE_TARGET_NAME} PROPERTIES OUTPUT_NAME ${MODULE_OUTPUT_NAME})
    endif()

    if(HELIOS_ENABLE_LTO)
      helios_target_enable_lto(${MODULE_TARGET_NAME})
    endif()

    if(MODULE_PCH)
      if(NOT IS_ABSOLUTE ${MODULE_PCH})
        set(MODULE_PCH "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE_PCH}")
      endif()
      if(EXISTS ${MODULE_PCH})
        helios_target_add_pch(${MODULE_TARGET_NAME} ${MODULE_PCH})
      else()
        message(WARNING "helios_module(${MODULE_NAME}): PCH file not found: ${MODULE_PCH}")
      endif()
    endif()

    if(HELIOS_ENABLE_UNITY_BUILD)
      helios_target_enable_unity_build(${MODULE_TARGET_NAME}
                BATCH_SIZE ${CMAKE_UNITY_BUILD_BATCH_SIZE}
            )
      if(MODULE_PCH)
        get_filename_component(_pch_name ${MODULE_PCH} NAME)
        get_filename_component(_pch_base ${MODULE_PCH} NAME_WE)
        set_source_files_properties(
                    ${MODULE_PCH}
                    "src/${_pch_base}.cpp"
                    PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
                )
      endif()
    endif()
  endif()

  # ========================================================================
  # DEPENDS — Helios module-to-module deps with visibility
  # ========================================================================

  set(_all_module_deps ${MODULE_DEPENDS})
  if(_all_module_deps)
    _helios_parse_module_deps("${_all_module_deps}" _pub_deps _priv_deps _iface_deps)

    foreach(_module_dep ${_pub_deps})
      _helios_link_helios_module(${MODULE_TARGET_NAME} ${_module_dep} PUBLIC)
    endforeach()
    foreach(_module_dep ${_priv_deps})
      _helios_link_helios_module(${MODULE_TARGET_NAME} ${_module_dep} PRIVATE)
    endforeach()
    foreach(_module_dep ${_iface_deps})
      _helios_link_helios_module(${MODULE_TARGET_NAME} ${_module_dep} INTERFACE)
    endforeach()
  endif()

  # OPTIONAL_DEPENDS — deferred links completed in Pass 2B (ModuleDiscovery.cmake)
  if(MODULE_OPTIONAL_DEPENDS)
    _helios_parse_module_deps("${MODULE_OPTIONAL_DEPENDS}" _opt_pub _opt_priv _opt_iface)
    set_target_properties(${MODULE_TARGET_NAME} PROPERTIES
        HELIOS_OPTIONAL_MODULE_DEPS_PUBLIC "${_opt_pub}"
        HELIOS_OPTIONAL_MODULE_DEPS_PRIVATE "${_opt_priv}"
        HELIOS_OPTIONAL_MODULE_DEPS_INTERFACE "${_opt_iface}"
    )
    foreach(_module_dep ${_opt_pub})
      _helios_link_helios_module(${MODULE_TARGET_NAME} ${_module_dep} PUBLIC)
    endforeach()
    foreach(_module_dep ${_opt_priv})
      _helios_link_helios_module(${MODULE_TARGET_NAME} ${_module_dep} PRIVATE)
    endforeach()
    foreach(_module_dep ${_opt_iface})
      _helios_link_helios_module(${MODULE_TARGET_NAME} ${_module_dep} INTERFACE)
    endforeach()
  endif()

  # ========================================================================
  # USES — Load external deps + link in one declaration
  # ========================================================================
  if(MODULE_USES)
    _helios_process_uses(${MODULE_TARGET_NAME} ${_default_dep_visibility} "${MODULE_USES}")
  endif()

  # ========================================================================
  # DEPENDENCIES — Already-loaded external targets
  # ========================================================================
  if(MODULE_DEPENDENCIES)
    _helios_parse_visibility_args(
            INPUT ${MODULE_DEPENDENCIES}
            PUBLIC_VAR _public_deps
            PRIVATE_VAR _private_deps
            INTERFACE_VAR _interface_deps
            DEFAULT_VISIBILITY ${_default_dep_visibility}
        )

    if(_module_header_only)
      set(_all_interface_deps ${_public_deps} ${_private_deps} ${_interface_deps})
      if(_all_interface_deps)
        target_link_libraries(${MODULE_TARGET_NAME} INTERFACE ${_all_interface_deps})
      endif()
    else()
      if(_public_deps)
        target_link_libraries(${MODULE_TARGET_NAME} PUBLIC ${_public_deps})
      endif()
      if(_private_deps)
        target_link_libraries(${MODULE_TARGET_NAME} PRIVATE ${_private_deps})
      endif()
      if(_interface_deps)
        target_link_libraries(${MODULE_TARGET_NAME} INTERFACE ${_interface_deps})
      endif()
    endif()
  endif()

  # ========================================================================
  # Include Directories
  # ========================================================================
  target_include_directories(${MODULE_TARGET_NAME}
        ${_target_scope}
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )

  if(NOT _module_header_only)
    target_include_directories(${MODULE_TARGET_NAME}
            PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
        )
  endif()

  if(MODULE_INCLUDE_DIRECTORIES)
    _helios_parse_visibility_args(
            INPUT ${MODULE_INCLUDE_DIRECTORIES}
            PUBLIC_VAR _public_includes
            PRIVATE_VAR _private_includes
            INTERFACE_VAR _interface_includes
            DEFAULT_VISIBILITY ${_default_dep_visibility}
        )
    if(_module_header_only)
      set(_all_iface_includes ${_public_includes} ${_private_includes} ${_interface_includes})
      if(_all_iface_includes)
        target_include_directories(${MODULE_TARGET_NAME} INTERFACE ${_all_iface_includes})
      endif()
    else()
      if(_public_includes)
        target_include_directories(${MODULE_TARGET_NAME} PUBLIC ${_public_includes})
      endif()
      if(_private_includes)
        target_include_directories(${MODULE_TARGET_NAME} PRIVATE ${_private_includes})
      endif()
      if(_interface_includes)
        target_include_directories(${MODULE_TARGET_NAME} INTERFACE ${_interface_includes})
      endif()
    endif()
  endif()

  # Third-party as SYSTEM
  if(PROJECT_IS_TOP_LEVEL)
    target_include_directories(${MODULE_TARGET_NAME}
            SYSTEM ${_target_scope} ${PROJECT_SOURCE_DIR}/third-party
        )
  endif()

  # ========================================================================
  # Compile Definitions
  # ========================================================================
  if(MODULE_COMPILE_DEFINITIONS)
    _helios_parse_visibility_args(
            INPUT ${MODULE_COMPILE_DEFINITIONS}
            PUBLIC_VAR _public_defs
            PRIVATE_VAR _private_defs
            INTERFACE_VAR _interface_defs
            DEFAULT_VISIBILITY ${_default_dep_visibility}
        )
    if(_module_header_only)
      set(_all_iface_defs ${_public_defs} ${_private_defs} ${_interface_defs})
      if(_all_iface_defs)
        target_compile_definitions(${MODULE_TARGET_NAME} INTERFACE ${_all_iface_defs})
      endif()
    else()
      if(_public_defs)
        target_compile_definitions(${MODULE_TARGET_NAME} PUBLIC ${_public_defs})
      endif()
      if(_private_defs)
        target_compile_definitions(${MODULE_TARGET_NAME} PRIVATE ${_private_defs})
      endif()
      if(_interface_defs)
        target_compile_definitions(${MODULE_TARGET_NAME} INTERFACE ${_interface_defs})
      endif()
    endif()
  endif()

  # ========================================================================
  # Compile Options (with shorthand expansion)
  # ========================================================================
  if(MODULE_COMPILE_OPTIONS)
    _helios_expand_compiler_shorthands("${MODULE_COMPILE_OPTIONS}" _expanded_opts)
    _helios_parse_visibility_args(
            INPUT ${_expanded_opts}
            PUBLIC_VAR _public_opts
            PRIVATE_VAR _private_opts
            INTERFACE_VAR _interface_opts
            DEFAULT_VISIBILITY ${_default_dep_visibility}
        )
    if(_module_header_only)
      set(_all_iface_opts ${_public_opts} ${_private_opts} ${_interface_opts})
      if(_all_iface_opts)
        target_compile_options(${MODULE_TARGET_NAME} INTERFACE ${_all_iface_opts})
      endif()
    else()
      if(_public_opts)
        target_compile_options(${MODULE_TARGET_NAME} PUBLIC ${_public_opts})
      endif()
      if(_private_opts)
        target_compile_options(${MODULE_TARGET_NAME} PRIVATE ${_private_opts})
      endif()
      if(_interface_opts)
        target_compile_options(${MODULE_TARGET_NAME} INTERFACE ${_interface_opts})
      endif()
    endif()
  endif()

  # ========================================================================
  # SUPPRESS_WARNINGS
  # ========================================================================
  if(MODULE_SUPPRESS_WARNINGS AND NOT _module_header_only)
    _helios_apply_suppress_warnings(${MODULE_TARGET_NAME} "${MODULE_SUPPRESS_WARNINGS}")
  endif()

  # ========================================================================
  # CONFIGURE_HOOK
  # ========================================================================
  if(MODULE_CONFIGURE_HOOK)
    if(COMMAND ${MODULE_CONFIGURE_HOOK})
      cmake_language(CALL ${MODULE_CONFIGURE_HOOK} ${MODULE_TARGET_NAME})
      set_target_properties(${MODULE_TARGET_NAME} PROPERTIES
                HELIOS_CONFIGURE_HOOK "${MODULE_CONFIGURE_HOOK}"
            )
    else()
      message(WARNING "helios_module(${MODULE_NAME}): CONFIGURE_HOOK function '${MODULE_CONFIGURE_HOOK}' not found")
    endif()
  endif()

  # ========================================================================
  # Shared library tracking
  # ========================================================================
  if(NOT _module_header_only)
    get_target_property(_target_type ${MODULE_TARGET_NAME} TYPE)
    if(_target_type STREQUAL "SHARED_LIBRARY")
      set_target_properties(${MODULE_TARGET_NAME} PROPERTIES HELIOS_IS_SHARED_MODULE TRUE)
    endif()
  endif()

  # ========================================================================
  # Install rules
  # ========================================================================
  if(HELIOS_ENABLE_INSTALL)
    install(TARGETS ${MODULE_TARGET_NAME}
            EXPORT HeliosTargets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
        )
  endif()

  # Store metadata
  set_target_properties(${MODULE_TARGET_NAME} PROPERTIES
        HELIOS_MODULE_NAME "${MODULE_NAME}"
        HELIOS_MODULE_VERSION "${MODULE_VERSION}"
    )

  set_property(GLOBAL APPEND PROPERTY HELIOS_MODULES ${MODULE_TARGET_NAME})

  # Status message
  if(_module_header_only)
    message(STATUS "Helios Module: ${MODULE_NAME} v${MODULE_VERSION} (${MODULE_TARGET_NAME}) [HEADER_ONLY]")
  else()
    get_target_property(_actual_type ${MODULE_TARGET_NAME} TYPE)
    message(STATUS "Helios Module: ${MODULE_NAME} v${MODULE_VERSION} (${MODULE_TARGET_NAME}) [${_actual_type}]")
  endif()

  # ========================================================================
  # Unit Tests
  # ========================================================================

  set(_test_option_name "HELIOS_MODULE_${_upper_name}_BUILD_TESTS")
  option(${_test_option_name} "Build tests for ${MODULE_NAME} module" ${HELIOS_BUILD_TESTS})

  # TESTS block (rich form)
  if(MODULE_TESTS AND ${_test_option_name} AND HELIOS_BUILD_TESTS)
    set(_suite_state "searching")
    set(_suite_name "")
    set(_suite_sources)
    set(_suite_deps)
    set(_suite_defs)
    set(_suite_labels)

    set(_suite_count 0)
    foreach(_item ${MODULE_TESTS})
      if("${_item}" STREQUAL "SUITE")
        # Commit previous suite if we have one
        if(_suite_count GREATER 0 AND _suite_sources)
          _helios_create_test_suite(
                        ${MODULE_NAME} "${_suite_name}" "${_suite_sources}"
                        "${_suite_deps}" "${_suite_defs}" "${_suite_labels}"
                    )
        endif()
        set(_suite_state "expect_name")
        set(_suite_name "")
        set(_suite_sources)
        set(_suite_deps)
        set(_suite_defs)
        set(_suite_labels)
        math(EXPR _suite_count "${_suite_count} + 1")
      elseif(_suite_state STREQUAL "expect_name")
        set(_suite_name "${_item}")
        set(_suite_state "collect")
      elseif(_item STREQUAL "SOURCES")
        set(_suite_state "sources")
      elseif(_item STREQUAL "DEPENDENCIES")
        set(_suite_state "deps")
      elseif(_item STREQUAL "DEFINITIONS")
        set(_suite_state "defs")
      elseif(_item STREQUAL "LABELS")
        set(_suite_state "labels")
      elseif(_suite_state STREQUAL "sources")
        list(APPEND _suite_sources "${_item}")
      elseif(_suite_state STREQUAL "deps")
        list(APPEND _suite_deps "${_item}")
      elseif(_suite_state STREQUAL "defs")
        list(APPEND _suite_defs "${_item}")
      elseif(_suite_state STREQUAL "labels")
        list(APPEND _suite_labels "${_item}")
      endif()
    endforeach()
    # Commit last suite
    if(_suite_count GREATER 0 AND _suite_sources)
      _helios_create_test_suite(
                ${MODULE_NAME} "${_suite_name}" "${_suite_sources}"
                "${_suite_deps}" "${_suite_defs}" "${_suite_labels}"
            )
    endif()
  endif()

  # TEST_SOURCES (shorthand — treated as a single "unit" suite)
  if(NOT MODULE_TESTS AND MODULE_TEST_SOURCES AND ${_test_option_name} AND HELIOS_BUILD_TESTS)
    set(_test_target_name "helios_${MODULE_NAME}_tests")

    add_executable(${_test_target_name} ${MODULE_TEST_SOURCES})

    helios_target_set_cxx_standard(${_test_target_name} STANDARD 23)
    helios_target_set_optimization(${_test_target_name})
    helios_target_set_output_dirs(${_test_target_name} CUSTOM_FOLDER tests)
    helios_target_set_folder(${_test_target_name} "Helios/Tests/${MODULE_NAME}")

    if(HELIOS_ENABLE_LTO)
      helios_target_enable_lto(${_test_target_name})
    endif()

    helios_target_enable_sanitizers(${_test_target_name})

    _helios_link_test_framework(${_test_target_name})

    target_link_libraries(${_test_target_name} PRIVATE helios::module::${MODULE_NAME})

    if(MODULE_TEST_DEPENDENCIES)
      target_link_libraries(${_test_target_name} PRIVATE ${MODULE_TEST_DEPENDENCIES})
    endif()

    target_compile_definitions(${_test_target_name} PRIVATE
            $<$<PLATFORM_ID:Windows>:NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE>
        )

    if(MODULE_PCH AND EXISTS "${MODULE_PCH}")
      target_precompile_headers(${_test_target_name} PRIVATE "${MODULE_PCH}")
    endif()

    add_test(NAME ${_test_target_name} COMMAND ${_test_target_name})
    set_tests_properties(${_test_target_name} PROPERTIES
            LABELS "${MODULE_NAME};unit"
        )

    _helios_register_test_target(${_test_target_name} ${MODULE_NAME} "unit")
    message(STATUS "  → Unit tests: ${_test_target_name}")
  endif()
endfunction()

# ============================================================================
# Internal: Create a test suite
# ============================================================================

function(_helios_create_test_suite MODULE_NAME SUITE_NAME SOURCES DEPENDENCIES DEFINITIONS LABELS)
  if(NOT SOURCES)
    return()
  endif()

  set(_target "helios_${MODULE_NAME}_${SUITE_NAME}_tests")

  add_executable(${_target} ${SOURCES})

  helios_target_set_cxx_standard(${_target} STANDARD 23)
  helios_target_set_optimization(${_target})
  helios_target_set_output_dirs(${_target} CUSTOM_FOLDER tests)
  helios_target_set_folder(${_target} "Helios/Tests/${MODULE_NAME}")

  if(HELIOS_ENABLE_LTO)
    helios_target_enable_lto(${_target})
  endif()

  helios_target_enable_sanitizers(${_target})

  _helios_link_test_framework(${_target})

  if(TARGET helios::module::${MODULE_NAME})
    target_link_libraries(${_target} PRIVATE helios::module::${MODULE_NAME})
  endif()

  if(DEPENDENCIES)
    target_link_libraries(${_target} PRIVATE ${DEPENDENCIES})
  endif()

  if(DEFINITIONS)
    target_compile_definitions(${_target} PRIVATE ${DEFINITIONS})
  endif()

  target_compile_definitions(${_target} PRIVATE
        $<$<PLATFORM_ID:Windows>:NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE>
    )

  add_test(NAME ${_target} COMMAND ${_target})

  set(_ctest_labels "${MODULE_NAME};${SUITE_NAME}")
  if(LABELS)
    list(APPEND _ctest_labels ${LABELS})
  endif()
  set_tests_properties(${_target} PROPERTIES LABELS "${_ctest_labels}")

  _helios_register_test_target(${_target} ${MODULE_NAME} "${SUITE_NAME}")

  message(STATUS "  → Test suite '${SUITE_NAME}': ${_target}")
endfunction()
