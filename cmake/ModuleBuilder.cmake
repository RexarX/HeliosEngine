# Helios Engine Module Builder
#
# Composable module creation entry point: helios_module().

include_guard(GLOBAL)

include(Primitives)
include(TargetUtils)
include(Sanitizers)
include(TestUtils)

function(_helios_is_source_file FILENAME OUTPUT_VAR)
  get_filename_component(_ext "${FILENAME}" EXT)
  string(TOLOWER "${_ext}" _ext_lower)
  set(_source_extensions ".cpp" ".cxx" ".cc" ".c" ".c++" ".cu" ".mm" ".m")

  if(_ext_lower IN_LIST _source_extensions)
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

function(_helios_module_parse_args)
  set(options STATIC SHARED)
  set(oneValueArgs NAME VERSION DESCRIPTION DEFAULT PCH FOLDER OUTPUT_NAME TARGET_NAME)
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

  foreach(_var
      STATIC SHARED
      NAME VERSION DESCRIPTION DEFAULT PCH FOLDER OUTPUT_NAME TARGET_NAME
      SOURCES HEADERS DEPENDS OPTIONAL_DEPENDS DEPENDENCIES USES
      COMPILE_DEFINITIONS COMPILE_OPTIONS INCLUDE_DIRECTORIES SUPPRESS_WARNINGS
      TEST_SOURCES TEST_DEPENDENCIES TESTS)
    set(MODULE_${_var} "${MODULE_${_var}}" PARENT_SCOPE)
  endforeach()
endfunction()

function(_helios_module_detect_header_only OUTPUT_VAR)
  _helios_module_has_source_files("${MODULE_SOURCES}" _module_has_sources)
  if(_module_has_sources)
    set(${OUTPUT_VAR} FALSE PARENT_SCOPE)
  else()
    set(${OUTPUT_VAR} TRUE PARENT_SCOPE)
  endif()
endfunction()

function(_helios_module_validate)
  if(NOT MODULE_NAME)
    message(FATAL_ERROR "helios_module: NAME is required")
  endif()

  if(MODULE_STATIC AND MODULE_SHARED)
    message(FATAL_ERROR
        "helios_module(${MODULE_NAME}): Conflicting library type specifications: STATIC, SHARED. "
        "Only one of STATIC or SHARED can be specified.")
  endif()
endfunction()

function(_helios_module_register MODULE_HEADER_ONLY)
  set(_register_args NAME ${MODULE_NAME})

  if(MODULE_DESCRIPTION)
    list(APPEND _register_args DESCRIPTION "${MODULE_DESCRIPTION}")
  endif()
  if(DEFINED MODULE_DEFAULT AND NOT MODULE_DEFAULT STREQUAL "")
    list(APPEND _register_args DEFAULT ${MODULE_DEFAULT})
  endif()
  if(MODULE_HEADER_ONLY)
    list(APPEND _register_args HEADER_ONLY)
  endif()
  if(MODULE_VERSION)
    list(APPEND _register_args VERSION ${MODULE_VERSION})
  endif()
  if(MODULE_DEPENDS)
    list(APPEND _register_args DEPENDS ${MODULE_DEPENDS})
  endif()
  if(MODULE_OPTIONAL_DEPENDS)
    list(APPEND _register_args OPTIONAL_DEPENDS ${MODULE_OPTIONAL_DEPENDS})
  endif()

  helios_register_module(${_register_args})
endfunction()

function(_helios_expand_compiler_shorthands RAW_OPTS OUT_VAR)
  set(_expanded)

  foreach(_opt IN LISTS RAW_OPTS)
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

function(_helios_module_create_target MODULE_HEADER_ONLY OUT_TARGET OUT_SCOPE OUT_DEFAULT_VISIBILITY)
  string(TOUPPER "${MODULE_NAME}" _upper_name)

  if(NOT MODULE_VERSION)
    set(MODULE_VERSION "0.1.0")
  endif()
  if(NOT MODULE_TARGET_NAME)
    set(MODULE_TARGET_NAME "helios_module_${MODULE_NAME}")
  endif()

  if(MODULE_HEADER_ONLY)
    set(_library_type INTERFACE)
    set(_target_scope INTERFACE)
    set(_default_dep_visibility INTERFACE)
  else()
    set(_build_option_name "HELIOS_BUILD_OPTION_${_upper_name}")
    if(MODULE_STATIC)
      set(_default_build_type STATIC)
    elseif(MODULE_SHARED)
      set(_default_build_type SHARED)
    else()
      set(_default_build_type AUTO)
    endif()

    set(${_build_option_name} "${_default_build_type}" CACHE STRING
        "Library type for ${MODULE_NAME} module (AUTO, STATIC, SHARED)")
    set_property(CACHE ${_build_option_name} PROPERTY STRINGS "AUTO" "STATIC" "SHARED")
    set(HELIOS_MODULE_${_upper_name}_SPECIFICATION "${_default_build_type}" CACHE INTERNAL
        "Original specification for ${MODULE_NAME} module")

    set(_build_type_value "${${_build_option_name}}")
    if(_build_type_value STREQUAL "STATIC")
      set(_library_type STATIC)
    elseif(_build_type_value STREQUAL "SHARED")
      set(_library_type SHARED)
    else()
      set(_library_type)
    endif()

    set(_target_scope PUBLIC)
    set(_default_dep_visibility PUBLIC)
  endif()

  if(MODULE_HEADER_ONLY)
    add_library(${MODULE_TARGET_NAME} INTERFACE)
  elseif(_library_type)
    add_library(${MODULE_TARGET_NAME} ${_library_type} ${MODULE_SOURCES} ${MODULE_HEADERS})
  else()
    add_library(${MODULE_TARGET_NAME} ${MODULE_SOURCES} ${MODULE_HEADERS})
  endif()

  add_library(helios::module::${MODULE_NAME} ALIAS ${MODULE_TARGET_NAME})
  set_target_properties(${MODULE_TARGET_NAME} PROPERTIES
      EXPORT_NAME "module::${MODULE_NAME}"
      HELIOS_MODULE_NAME "${MODULE_NAME}"
      HELIOS_MODULE_VERSION "${MODULE_VERSION}"
  )

  target_compile_definitions(${MODULE_TARGET_NAME} ${_target_scope}
      HELIOS_MODULE_${_upper_name}_AVAILABLE)

  set(${OUT_TARGET} "${MODULE_TARGET_NAME}" PARENT_SCOPE)
  set(${OUT_SCOPE} "${_target_scope}" PARENT_SCOPE)
  set(${OUT_DEFAULT_VISIBILITY} "${_default_dep_visibility}" PARENT_SCOPE)
endfunction()

function(_helios_module_find_pch_source PCH_FILE OUT_VAR)
  get_filename_component(_pch_base "${PCH_FILE}" NAME_WE)
  set(_candidate "")

  foreach(_source IN LISTS MODULE_SOURCES)
    get_filename_component(_source_base "${_source}" NAME_WE)
    if(_source_base STREQUAL _pch_base)
      set(_candidate "${_source}")
      break()
    endif()
  endforeach()

  set(${OUT_VAR} "${_candidate}" PARENT_SCOPE)
endfunction()

function(_helios_module_apply_conventions TARGET MODULE_HEADER_ONLY)
  if(MODULE_HEADER_ONLY)
    if(MODULE_PCH)
      message(WARNING "helios_module(${MODULE_NAME}): PCH is ignored for header-only modules")
    endif()
    return()
  endif()

  if(NOT MODULE_FOLDER)
    set(MODULE_FOLDER "Helios/Modules")
  endif()

  helios_target_set_cxx_standard(${TARGET} STANDARD 23)
  helios_target_set_platform(${TARGET})
  get_target_property(_actual_type ${TARGET} TYPE)
  if(_actual_type STREQUAL "SHARED_LIBRARY")
    helios_target_set_shared_lib_platform(${TARGET})
    set_target_properties(${TARGET} PROPERTIES HELIOS_IS_SHARED_MODULE TRUE)
  endif()
  helios_target_set_optimization(${TARGET})
  helios_target_set_warnings(${TARGET})
  helios_target_enable_sanitizers(${TARGET})
  helios_target_set_output_dirs(${TARGET})
  helios_target_set_folder(${TARGET} "${MODULE_FOLDER}")

  if(MODULE_OUTPUT_NAME)
    set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME ${MODULE_OUTPUT_NAME})
  endif()

  if(HELIOS_ENABLE_LTO)
    helios_target_enable_lto(${TARGET})
  endif()

  set(_resolved_pch "")
  if(MODULE_PCH)
    set(_resolved_pch "${MODULE_PCH}")
    if(NOT IS_ABSOLUTE "${_resolved_pch}")
      set(_resolved_pch "${CMAKE_CURRENT_SOURCE_DIR}/${_resolved_pch}")
    endif()
    if(EXISTS "${_resolved_pch}")
      helios_target_add_pch(${TARGET} "${_resolved_pch}")
    else()
      message(WARNING "helios_module(${MODULE_NAME}): PCH file not found: ${_resolved_pch}")
    endif()
  endif()

  if(HELIOS_ENABLE_UNITY_BUILD)
    set(_unity_excludes)
    if(_resolved_pch)
      list(APPEND _unity_excludes "${_resolved_pch}")
      _helios_module_find_pch_source("${_resolved_pch}" _pch_source)
      if(_pch_source)
        list(APPEND _unity_excludes "${_pch_source}")
      endif()
    endif()
    helios_target_enable_unity_build(${TARGET}
        BATCH_SIZE ${CMAKE_UNITY_BUILD_BATCH_SIZE}
        EXCLUDE_FILES ${_unity_excludes}
    )
  endif()
endfunction()

function(_helios_link_helios_module TARGET MODULE_NAME LINK_VISIBILITY)
  if(NOT TARGET helios::module::${MODULE_NAME})
    return()
  endif()

  string(TOUPPER "${MODULE_NAME}" _module_upper)
  get_target_property(_target_type ${TARGET} TYPE)
  if(_target_type STREQUAL "INTERFACE_LIBRARY")
    set(LINK_VISIBILITY INTERFACE)
  endif()

  target_link_libraries(${TARGET} ${LINK_VISIBILITY} helios::module::${MODULE_NAME})
  target_compile_definitions(${TARGET} ${LINK_VISIBILITY}
      HELIOS_MODULE_${_module_upper}_AVAILABLE)
endfunction()

function(_helios_module_link_depends TARGET DEFAULT_VISIBILITY)
  if(MODULE_DEPENDS)
    helios_parse_visibility(
        INPUT ${MODULE_DEPENDS}
        PUBLIC_VAR _pub_deps
        PRIVATE_VAR _priv_deps
        INTERFACE_VAR _iface_deps
        DEFAULT ${DEFAULT_VISIBILITY}
    )
    foreach(_module_dep IN LISTS _pub_deps)
      _helios_link_helios_module(${TARGET} ${_module_dep} PUBLIC)
    endforeach()
    foreach(_module_dep IN LISTS _priv_deps)
      _helios_link_helios_module(${TARGET} ${_module_dep} PRIVATE)
    endforeach()
    foreach(_module_dep IN LISTS _iface_deps)
      _helios_link_helios_module(${TARGET} ${_module_dep} INTERFACE)
    endforeach()
  endif()

  if(MODULE_OPTIONAL_DEPENDS)
    helios_parse_visibility(
        INPUT ${MODULE_OPTIONAL_DEPENDS}
        PUBLIC_VAR _opt_pub
        PRIVATE_VAR _opt_priv
        INTERFACE_VAR _opt_iface
        DEFAULT ${DEFAULT_VISIBILITY}
    )
    set_target_properties(${TARGET} PROPERTIES
        HELIOS_OPTIONAL_MODULE_DEPS_PUBLIC "${_opt_pub}"
        HELIOS_OPTIONAL_MODULE_DEPS_PRIVATE "${_opt_priv}"
        HELIOS_OPTIONAL_MODULE_DEPS_INTERFACE "${_opt_iface}"
    )
    foreach(_module_dep IN LISTS _opt_pub)
      _helios_link_helios_module(${TARGET} ${_module_dep} PUBLIC)
    endforeach()
    foreach(_module_dep IN LISTS _opt_priv)
      _helios_link_helios_module(${TARGET} ${_module_dep} PRIVATE)
    endforeach()
    foreach(_module_dep IN LISTS _opt_iface)
      _helios_link_helios_module(${TARGET} ${_module_dep} INTERFACE)
    endforeach()
  endif()
endfunction()

function(_helios_module_apply_uses TARGET DEFAULT_VISIBILITY)
  if(NOT MODULE_USES)
    return()
  endif()

  set(_visibility "${DEFAULT_VISIBILITY}")
  set(_state "expect_dep_file")
  set(_dep_file "")
  set(_link_target "")

  get_target_property(_target_type ${TARGET} TYPE)
  if(_target_type STREQUAL "INTERFACE_LIBRARY")
    set(_forced_visibility INTERFACE)
  else()
    set(_forced_visibility "")
  endif()

  foreach(_item IN LISTS MODULE_USES)
    if(_state STREQUAL "expect_dep_file")
      set(_dep_file "${_item}")
      set(_state "expect_visibility_or_target")
    elseif(_state STREQUAL "expect_visibility_or_target")
      if(_item STREQUAL "PUBLIC" OR _item STREQUAL "PRIVATE" OR _item STREQUAL "INTERFACE")
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

function(_helios_module_apply_interface TARGET TARGET_SCOPE DEFAULT_VISIBILITY MODULE_HEADER_ONLY)
  target_include_directories(${TARGET}
      ${TARGET_SCOPE}
          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
          $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )

  if(NOT MODULE_HEADER_ONLY)
    target_include_directories(${TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  endif()

  helios_target_apply(TARGET ${TARGET} KIND LINK
      ITEMS ${MODULE_DEPENDENCIES}
      DEFAULT ${DEFAULT_VISIBILITY})
  helios_target_apply(TARGET ${TARGET} KIND INCLUDES
      ITEMS ${MODULE_INCLUDE_DIRECTORIES}
      DEFAULT ${DEFAULT_VISIBILITY})
  helios_target_apply(TARGET ${TARGET} KIND DEFINITIONS
      ITEMS ${MODULE_COMPILE_DEFINITIONS}
      DEFAULT ${DEFAULT_VISIBILITY})

  if(MODULE_COMPILE_OPTIONS)
    _helios_expand_compiler_shorthands(MODULE_COMPILE_OPTIONS _expanded_opts)
    helios_target_apply(TARGET ${TARGET} KIND OPTIONS
        ITEMS ${_expanded_opts}
        DEFAULT ${DEFAULT_VISIBILITY})
  endif()

  if(PROJECT_IS_TOP_LEVEL)
    target_include_directories(${TARGET}
        SYSTEM ${TARGET_SCOPE} ${PROJECT_SOURCE_DIR}/third-party)
  endif()

  if(MODULE_SUPPRESS_WARNINGS AND NOT MODULE_HEADER_ONLY)
    foreach(_warn IN LISTS MODULE_SUPPRESS_WARNINGS)
      target_compile_options(${TARGET} PRIVATE
          $<$<OR:$<CXX_COMPILER_ID:MSVC>,$<AND:$<CXX_COMPILER_ID:Clang>,$<PLATFORM_ID:Windows>>>:/wd${_warn}>
          $<$<AND:$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>,$<NOT:$<PLATFORM_ID:Windows>>>:-Wno-${_warn}>
      )
    endforeach()
  endif()
endfunction()

function(_helios_module_install TARGET)
  if(NOT HELIOS_ENABLE_INSTALL)
    return()
  endif()

  install(TARGETS ${TARGET}
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
endfunction()

function(_helios_module_add_test_suite MODULE_NAME SUITE_NAME SOURCES DEPENDENCIES DEFINITIONS LABELS REUSE_PCH)
  if(NOT SOURCES)
    return()
  endif()

  set(_target "helios_${MODULE_NAME}_${SUITE_NAME}_tests")
  set(_args
      NAME ${_target}
      MODULE ${MODULE_NAME}
      TYPE ${SUITE_NAME}
      SOURCES ${SOURCES}
      DEPENDENCIES ${DEPENDENCIES}
      DEFINITIONS ${DEFINITIONS}
      LABELS ${SUITE_NAME} ${LABELS}
  )
  if(REUSE_PCH)
    list(APPEND _args REUSE_PCH)
  endif()
  helios_add_test_executable(${_args})
endfunction()

function(_helios_module_add_rich_tests MODULE_NAME REUSE_PCH)
  set(_suite_name "")
  set(_suite_sources)
  set(_suite_deps)
  set(_suite_defs)
  set(_suite_labels)
  set(_suite_count 0)
  set(_suite_state "expect_suite")

  foreach(_item IN LISTS MODULE_TESTS)
    if(_item STREQUAL "SUITE")
      if(_suite_count GREATER 0)
        _helios_module_add_test_suite(
            ${MODULE_NAME}
            ${_suite_name}
            "${_suite_sources}"
            "${_suite_deps}"
            "${_suite_defs}"
            "${_suite_labels}"
            ${REUSE_PCH}
        )
      endif()
      set(_suite_name "")
      set(_suite_sources)
      set(_suite_deps)
      set(_suite_defs)
      set(_suite_labels)
      math(EXPR _suite_count "${_suite_count} + 1")
      set(_suite_state "expect_name")
    elseif(_suite_state STREQUAL "expect_name")
      set(_suite_name "${_item}")
      set(_suite_state "expect_key")
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
    else()
      message(FATAL_ERROR "helios_module(${MODULE_NAME}): unexpected TESTS token '${_item}'")
    endif()
  endforeach()

  if(_suite_count GREATER 0)
    _helios_module_add_test_suite(
        ${MODULE_NAME}
        ${_suite_name}
        "${_suite_sources}"
        "${_suite_deps}"
        "${_suite_defs}"
        "${_suite_labels}"
        ${REUSE_PCH}
    )
  endif()
endfunction()

function(_helios_module_add_tests TARGET MODULE_HEADER_ONLY)
  string(TOUPPER "${MODULE_NAME}" _upper_name)
  set(_test_option_name "HELIOS_MODULE_${_upper_name}_BUILD_TESTS")
  option(${_test_option_name} "Build tests for ${MODULE_NAME} module" ${HELIOS_BUILD_TESTS})

  if(NOT ${_test_option_name} OR NOT HELIOS_BUILD_TESTS)
    return()
  endif()

  if(MODULE_HEADER_ONLY)
    set(_reuse_pch FALSE)
  else()
    set(_reuse_pch TRUE)
  endif()

  if(MODULE_TESTS)
    _helios_module_add_rich_tests(${MODULE_NAME} ${_reuse_pch})
  elseif(MODULE_TEST_SOURCES)
    set(_args
        NAME helios_${MODULE_NAME}_tests
        MODULE ${MODULE_NAME}
        TYPE unit
        SOURCES ${MODULE_TEST_SOURCES}
        DEPENDENCIES ${MODULE_TEST_DEPENDENCIES}
        LABELS unit
    )
    if(_reuse_pch)
      list(APPEND _args REUSE_PCH)
    endif()
    helios_add_test_executable(${_args})
  endif()
endfunction()

#[[
    helios_module(
        NAME <name>
        [VERSION <version>]
        [DESCRIPTION <description>]
        [DEFAULT <ON|OFF>]
        [SOURCES <files...>]
        [HEADERS <files...>]
        [DEPENDS <visibility> <module>...]
        [OPTIONAL_DEPENDS <visibility> <module>...]
        [USES <dep-file> [visibility] <target>...]
        [DEPENDENCIES <visibility> <target>...]
        [COMPILE_DEFINITIONS <visibility> <defs...>]
        [COMPILE_OPTIONS <visibility> <opts...>]
        [INCLUDE_DIRECTORIES <visibility> <dirs...>]
        [PCH <file>]
        [STATIC|SHARED]
        [SUPPRESS_WARNINGS <warning-name>...]
        [TEST_SOURCES <files...>]
        [TEST_DEPENDENCIES <targets...>]
        [TESTS SUITE <name> SOURCES <files...> ...]
    )

    Defines a Helios module. During the registration pass it records only graph
    metadata and the public macro returns from the including CMakeLists.txt.
    During the build pass it creates the target, applies project conventions,
    links dependencies, configures install rules, and creates tests.

    Example:
        helios_module(
            NAME core
            DEPENDS PUBLIC compiler PUBLIC platform PUBLIC utils
            USES stduuid PUBLIC helios::lib::stduuid::stduuid
            TEST_SOURCES tests/main.cpp
        )
]]
function(_helios_module_impl)
  _helios_module_parse_args(${ARGN})
  _helios_module_validate()
  _helios_module_detect_header_only(_module_header_only)

  string(TOUPPER "${MODULE_NAME}" _upper_name)

  if(HELIOS_REGISTRATION_PASS)
    _helios_module_register(${_module_header_only})
    return()
  endif()

  if(DEFINED HELIOS_BUILD_${_upper_name} AND NOT HELIOS_BUILD_${_upper_name})
    message(STATUS "Module ${MODULE_NAME} is disabled, skipping")
    return()
  endif()

  set(HELIOS_MODULE_${_upper_name}_HEADER_ONLY ${_module_header_only}
      CACHE INTERNAL "Module ${MODULE_NAME} is header-only")

  _helios_module_create_target(${_module_header_only} _target _target_scope _default_dep_visibility)
  _helios_module_apply_conventions(${_target} ${_module_header_only})
  _helios_module_link_depends(${_target} ${_default_dep_visibility})
  _helios_module_apply_uses(${_target} ${_default_dep_visibility})
  _helios_module_apply_interface(${_target} ${_target_scope} ${_default_dep_visibility} ${_module_header_only})
  _helios_module_install(${_target})

  set_property(GLOBAL APPEND PROPERTY HELIOS_MODULES ${_target})

  if(_module_header_only)
    get_target_property(_module_version ${_target} HELIOS_MODULE_VERSION)
    message(STATUS "Helios Module: ${MODULE_NAME} v${_module_version} (${_target}) [HEADER_ONLY]")
  else()
    get_target_property(_actual_type ${_target} TYPE)
    get_target_property(_module_version ${_target} HELIOS_MODULE_VERSION)
    message(STATUS "Helios Module: ${MODULE_NAME} v${_module_version} (${_target}) [${_actual_type}]")
  endif()

  _helios_module_add_tests(${_target} ${_module_header_only})
endfunction()

#[[
    helios_module(
        NAME <name>
        [VERSION <version>]
        [DESCRIPTION <description>]
        [DEFAULT <ON|OFF>]
        [SOURCES <files...>]
        [HEADERS <files...>]
        [DEPENDS <visibility> <module>...]
        [OPTIONAL_DEPENDS <visibility> <module>...]
        [USES <dep-file> [visibility] <target>...]
        [DEPENDENCIES <visibility> <target>...]
        [COMPILE_DEFINITIONS <visibility> <defs...>]
        [COMPILE_OPTIONS <visibility> <opts...>]
        [INCLUDE_DIRECTORIES <visibility> <dirs...>]
        [PCH <file>]
        [STATIC|SHARED]
        [SUPPRESS_WARNINGS <warning-name>...]
        [TEST_SOURCES <files...>]
        [TEST_DEPENDENCIES <targets...>]
        [TESTS SUITE <name> SOURCES <files...> ...]
    )

    Public module declaration macro. The registration pass records dependency
    metadata and returns from the including CMakeLists.txt; the build pass
    creates and configures the actual target.

    Example:
        helios_module(
            NAME app
            DEPENDS PUBLIC async PUBLIC core PUBLIC ecs PUBLIC log PUBLIC utils
            OPTIONAL_DEPENDS PUBLIC profile
            TEST_SOURCES tests/main.cpp
        )
]]
macro(helios_module)
  _helios_module_impl(${ARGN})
  if(HELIOS_REGISTRATION_PASS)
    return()
  endif()
endmacro()
