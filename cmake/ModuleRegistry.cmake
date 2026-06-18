# Helios Engine Module Registry
#
# Handles module registration, dependency resolution, validation,
# and circular dependency checking.

include_guard(GLOBAL)

# ============================================================================
# Public Registration API
# ============================================================================

# Function: helios_preregister_module
#
# Lightweight pre-registration for when a module must appear in the
# dependency graph before its CMakeLists.txt is processed. Called from
# Module.cmake or during the dry-run registration pass of helios_module.
#
function(helios_preregister_module)
  set(options HEADER_ONLY)
  set(oneValueArgs NAME DESCRIPTION DEFAULT VERSION)
  set(multiValueArgs DEPENDS OPTIONAL_DEPENDS)

  cmake_parse_arguments(MODULE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT MODULE_NAME)
    message(FATAL_ERROR "helios_preregister_module: NAME is required")
  endif()

  string(TOUPPER "${MODULE_NAME}" _upper_name)
  set(_option_name "HELIOS_BUILD_${_upper_name}_MODULE")

  if(NOT DEFINED MODULE_DEFAULT)
    set(MODULE_DEFAULT ON)
  endif()

  if(NOT MODULE_DESCRIPTION)
    set(MODULE_DESCRIPTION "Build the ${MODULE_NAME} module")
  endif()

  # Create the build option only if not already defined
  if(NOT DEFINED ${_option_name})
    option(${_option_name} "${MODULE_DESCRIPTION}" ${MODULE_DEFAULT})
  endif()

  set(HELIOS_MODULE_${_upper_name}_REGISTERED TRUE CACHE INTERNAL "Module ${MODULE_NAME} is registered")

  if(MODULE_DEPENDS)
    set(HELIOS_MODULE_${_upper_name}_DEPENDS "${MODULE_DEPENDS}" CACHE INTERNAL "Dependencies for ${MODULE_NAME}")
  else()
    set(HELIOS_MODULE_${_upper_name}_DEPENDS "" CACHE INTERNAL "Dependencies for ${MODULE_NAME}")
  endif()

  if(MODULE_OPTIONAL_DEPENDS)
    set(HELIOS_MODULE_${_upper_name}_OPTIONAL_DEPENDS "${MODULE_OPTIONAL_DEPENDS}" CACHE INTERNAL "Optional dependencies for ${MODULE_NAME}")
  else()
    set(HELIOS_MODULE_${_upper_name}_OPTIONAL_DEPENDS "" CACHE INTERNAL "Optional dependencies for ${MODULE_NAME}")
  endif()

  if(MODULE_VERSION)
    set(HELIOS_MODULE_${_upper_name}_VERSION "${MODULE_VERSION}" CACHE INTERNAL "Version of ${MODULE_NAME}")
  else()
    set(HELIOS_MODULE_${_upper_name}_VERSION "0.1.0" CACHE INTERNAL "Version of ${MODULE_NAME}")
  endif()

  if(MODULE_HEADER_ONLY)
    set(HELIOS_MODULE_${_upper_name}_HEADER_ONLY TRUE CACHE INTERNAL "Module ${MODULE_NAME} is header-only")
  else()
    set(HELIOS_MODULE_${_upper_name}_HEADER_ONLY FALSE CACHE INTERNAL "Module ${MODULE_NAME} is header-only")
  endif()

  get_property(_registered GLOBAL PROPERTY HELIOS_REGISTERED_MODULES)
  if(NOT "${MODULE_NAME}" IN_LIST _registered)
    set_property(GLOBAL APPEND PROPERTY HELIOS_REGISTERED_MODULES ${MODULE_NAME})
  endif()

  message(STATUS "Registered module: ${MODULE_NAME} v${HELIOS_MODULE_${_upper_name}_VERSION} (${_option_name}=${${_option_name}})")
endfunction()

# Function: helios_register_module
#
# Registers a module in the dependency graph before its CMakeLists.txt is
# processed. Called from Module.cmake or during the dry-run registration pass
# of helios_module (when no Module.cmake exists).
#
function(helios_register_module)
  helios_preregister_module(${ARGN})
endfunction()

# ============================================================================
# Module Status Queries
# ============================================================================

function(helios_module_enabled NAME OUTPUT_VAR)
  string(TOUPPER "${NAME}" _upper_name)
  set(_option_name "HELIOS_BUILD_${_upper_name}_MODULE")

  if(DEFINED ${_option_name} AND ${_option_name})
    set(${OUTPUT_VAR} TRUE PARENT_SCOPE)
  else()
    set(${OUTPUT_VAR} FALSE PARENT_SCOPE)
  endif()
endfunction()

function(helios_module_is_header_only NAME OUTPUT_VAR)
  string(TOUPPER "${NAME}" _upper_name)

  if(DEFINED HELIOS_MODULE_${_upper_name}_HEADER_ONLY AND HELIOS_MODULE_${_upper_name}_HEADER_ONLY)
    set(${OUTPUT_VAR} TRUE PARENT_SCOPE)
  else()
    set(${OUTPUT_VAR} FALSE PARENT_SCOPE)
  endif()
endfunction()

# ============================================================================
# Dependency Resolution
# ============================================================================

function(helios_resolve_module_dependencies)
  get_property(_modules GLOBAL PROPERTY HELIOS_REGISTERED_MODULES)

  if(NOT _modules)
    return()
  endif()

  set(_changed TRUE)
  set(_iterations 0)
  while(_changed AND _iterations LESS 100)
    set(_changed FALSE)
    math(EXPR _iterations "${_iterations} + 1")

    foreach(_module ${_modules})
      helios_module_enabled(${_module} _is_enabled)

      if(_is_enabled)
        string(TOUPPER "${_module}" _upper_name)
        set(_deps ${HELIOS_MODULE_${_upper_name}_DEPENDS})

        foreach(_dep ${_deps})
          helios_module_enabled(${_dep} _dep_enabled)

          if(NOT _dep_enabled)
            string(TOUPPER "${_dep}" _dep_upper)
            set(_dep_option "HELIOS_BUILD_${_dep_upper}_MODULE")
            message(STATUS "Enabling module '${_dep}' (required by '${_module}')")
            set(${_dep_option} ON CACHE BOOL "Build the ${_dep} module" FORCE)
            set(_changed TRUE)
          endif()
        endforeach()
      endif()
    endforeach()
  endwhile()

  if(_iterations GREATER_EQUAL 100)
    message(WARNING "Module dependency resolution reached iteration limit. Possible circular dependency.")
  endif()
endfunction()

function(helios_validate_module_dependencies)
  get_property(_modules GLOBAL PROPERTY HELIOS_REGISTERED_MODULES)

  foreach(_module ${_modules})
    helios_module_enabled(${_module} _is_enabled)

    if(_is_enabled)
      string(TOUPPER "${_module}" _upper_name)

      set(_deps ${HELIOS_MODULE_${_upper_name}_DEPENDS})
      foreach(_dep ${_deps})
        helios_module_enabled(${_dep} _dep_enabled)
        if(NOT _dep_enabled)
          string(TOUPPER "${_dep}" _upper_dep)
          message(FATAL_ERROR
              "Module '${_module}' requires module '${_dep}', but it is disabled. "
              "Enable it with -DHELIOS_BUILD_${_upper_dep}_MODULE=ON")
        endif()
      endforeach()
    endif()
  endforeach()
endfunction()

function(helios_check_circular_dependencies)
  get_property(_modules GLOBAL PROPERTY HELIOS_REGISTERED_MODULES)

  if(NOT _modules)
    return()
  endif()

  foreach(_module ${_modules})
    helios_module_is_header_only(${_module} _is_ho)
    if(_is_ho)
      continue()
    endif()

    helios_module_enabled(${_module} _is_enabled)
    if(NOT _is_enabled)
      continue()
    endif()

    set(_visited)
    set(_queue)
    string(TOUPPER "${_module}" _upper_name)
    set(_deps ${HELIOS_MODULE_${_upper_name}_DEPENDS})

    foreach(_dep ${_deps})
      helios_module_is_header_only(${_dep} _dep_is_ho)
      if(NOT _dep_is_ho)
        list(APPEND _queue ${_dep})
      endif()
    endforeach()

    while(_queue)
      list(POP_FRONT _queue _current)

      if("${_current}" STREQUAL "${_module}")
        message(FATAL_ERROR
            "Circular dependency detected: module '${_module}' depends on itself "
            "(through non-header-only module chain). Consider refactoring.")
      endif()

      if("${_current}" IN_LIST _visited)
          continue()
      endif()
      list(APPEND _visited ${_current})

      string(TOUPPER "${_current}" _current_upper)
      set(_current_deps ${HELIOS_MODULE_${_current_upper}_DEPENDS})

      foreach(_dep ${_current_deps})
        helios_module_is_header_only(${_dep} _dep_is_ho)
        if(NOT _dep_is_ho)
          if(NOT "${_dep}" IN_LIST _visited)
            list(APPEND _queue ${_dep})
          endif()
        endif()
      endforeach()
    endwhile()
  endforeach()
endfunction()

# ============================================================================
# Informational Functions
# ============================================================================

function(helios_print_modules)
  get_property(_registered GLOBAL PROPERTY HELIOS_REGISTERED_MODULES)
  get_property(_built GLOBAL PROPERTY HELIOS_MODULES)

  message(STATUS "")
  message(STATUS "========== Helios Engine Modules ==========")

  if(_registered)
    list(REMOVE_DUPLICATES _registered)
    list(SORT _registered)
    message(STATUS "Registered modules:")
    foreach(_module ${_registered})
      helios_module_enabled(${_module} _is_enabled)
      helios_module_is_header_only(${_module} _is_header_only)
      string(TOUPPER "${_module}" _upper_name)
      if(_is_enabled)
        if(_is_header_only)
          message(STATUS "  ✓ ${_module} [ENABLED] [HEADER_ONLY]")
        else()
          message(STATUS "  ✓ ${_module} [ENABLED]")
        endif()
      else()
        message(STATUS "  ✗ ${_module} [DISABLED]")
      endif()
    endforeach()
  else()
    message(STATUS "No modules registered")
  endif()

  message(STATUS "")

  if(_built)
    list(REMOVE_DUPLICATES _built)
    list(SORT _built)
    message(STATUS "Built modules:")
    foreach(_target ${_built})
      get_target_property(_name ${_target} HELIOS_MODULE_NAME)
      get_target_property(_version ${_target} HELIOS_MODULE_VERSION)
      get_target_property(_type ${_target} TYPE)
      if(_name AND _version)
        message(STATUS "  ✓ ${_name} v${_version} (${_target}) [${_type}]")
      else()
        message(STATUS "  ✓ ${_target} [${_type}]")
      endif()
    endforeach()
  else()
    message(STATUS "No modules built")
  endif()

  message(STATUS "===========================================")
  message(STATUS "")
endfunction()

function(helios_print_module_build_options)
  get_property(_registered GLOBAL PROPERTY HELIOS_REGISTERED_MODULES)
  get_property(_built GLOBAL PROPERTY HELIOS_MODULES)

  message(STATUS "")
  message(STATUS "========== Module Build Options ==========")

  if(_registered)
    list(SORT _registered)
    message(STATUS "Enable/Disable modules:")
    foreach(_module ${_registered})
      string(TOUPPER "${_module}" _upper_name)
      set(_option "HELIOS_BUILD_${_upper_name}_MODULE")
      helios_module_is_header_only(${_module} _is_ho)
      if(_is_ho)
        message(STATUS "  -D${_option}=ON|OFF  (current: ${${_option}}) [HEADER_ONLY]")
      else()
        message(STATUS "  -D${_option}=ON|OFF  (current: ${${_option}})")
      endif()
    endforeach()
  else()
    message(STATUS "No modules registered")
  endif()

  message(STATUS "")

  if(_built)
    list(SORT _built)
    message(STATUS "Library type overrides (non-header-only modules):")
    set(_has_type_options FALSE)
    foreach(_target ${_built})
      get_target_property(_name ${_target} HELIOS_MODULE_NAME)
      if(_name)
        string(TOUPPER "${_name}" _upper_name)
        set(_type_option "HELIOS_BUILD_OPTION_${_upper_name}_MODULE")
        if(DEFINED ${_type_option})
          set(_has_type_options TRUE)
          set(_spec "${HELIOS_MODULE_${_upper_name}_SPECIFICATION}")
          if(NOT _spec)
            set(_spec "AUTO")
          endif()
          message(STATUS "  -D${_type_option}=AUTO|STATIC|SHARED  (current: ${${_type_option}}, default: ${_spec})")
        endif()
      endif()
    endforeach()
    if(NOT _has_type_options)
      message(STATUS "  (none - all modules are header-only)")
    endif()
  endif()

  message(STATUS "==========================================")
  message(STATUS "")
endfunction()

function(helios_get_module_list OUTPUT_VAR)
  get_property(_modules GLOBAL PROPERTY HELIOS_MODULES)
  if(_modules)
    list(REMOVE_DUPLICATES _modules)
    list(SORT _modules)
  endif()
  set(${OUTPUT_VAR} ${_modules} PARENT_SCOPE)
endfunction()

function(helios_get_enabled_modules OUTPUT_VAR)
  get_property(_registered GLOBAL PROPERTY HELIOS_REGISTERED_MODULES)
  set(_enabled)

  foreach(_module ${_registered})
    helios_module_enabled(${_module} _is_enabled)
    if(_is_enabled)
      list(APPEND _enabled ${_module})
    endif()
  endforeach()

  set(${OUTPUT_VAR} ${_enabled} PARENT_SCOPE)
endfunction()

function(helios_get_module_path)
  cmake_parse_arguments(ARG "" "NAME;OUTPUT_VAR" "" ${ARGN})

  if(NOT ARG_NAME)
    message(FATAL_ERROR "helios_get_module_path: NAME is required")
  endif()

  if(NOT ARG_OUTPUT_VAR)
    message(FATAL_ERROR "helios_get_module_path: OUTPUT_VAR is required")
  endif()

  string(TOUPPER "${ARG_NAME}" _upper_name)
  if(DEFINED HELIOS_MODULE_${_upper_name}_PATH)
    set(${ARG_OUTPUT_VAR} "${HELIOS_MODULE_${_upper_name}_PATH}" PARENT_SCOPE)
  else()
    set(${ARG_OUTPUT_VAR} "" PARENT_SCOPE)
  endif()
endfunction()

# ============================================================================
# Extra Module Search Paths
# ============================================================================

#[[
    helios_add_extra_module_dirs(<path>...)

    Appends directories to the extra module search list. Call before module
    discovery begins — either from a parent project (include ModuleRegistry.cmake
    before add_subdirectory(HeliosEngine)) or from Helios CMakeLists.txt prior to
    helios_discover_modules() / helios_discover_extra_module_dirs().

    Paths are normalized to absolute form relative to CMAKE_CURRENT_SOURCE_DIR
    at the call site. Merged at discovery time with HELIOS_EXTRA_MODULE_DIRS.
]]
function(helios_add_extra_module_dirs)
  if(ARGC EQUAL 0)
    message(FATAL_ERROR "helios_add_extra_module_dirs: at least one directory path is required")
  endif()

  get_property(_dirs GLOBAL PROPERTY HELIOS_EXTRA_MODULE_DIRS_ACCUM)
  if(NOT _dirs)
    set(_dirs "")
  endif()

  foreach(_raw_dir ${ARGN})
    cmake_path(ABSOLUTE_PATH _raw_dir BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
               NORMALIZE OUTPUT_VARIABLE _abs_dir)

    if(NOT _abs_dir IN_LIST _dirs)
      list(APPEND _dirs "${_abs_dir}")
      message(STATUS "Helios extra module search path: ${_abs_dir}")
    endif()
  endforeach()

  set_property(GLOBAL PROPERTY HELIOS_EXTRA_MODULE_DIRS_ACCUM "${_dirs}")
endfunction()

function(helios_get_extra_module_dirs OUTPUT_VAR)
  get_property(_accum GLOBAL PROPERTY HELIOS_EXTRA_MODULE_DIRS_ACCUM)
  if(NOT _accum)
    set(_accum "")
  endif()

  set(_dirs "${_accum}")
  if(DEFINED HELIOS_EXTRA_MODULE_DIRS AND HELIOS_EXTRA_MODULE_DIRS)
    list(APPEND _dirs ${HELIOS_EXTRA_MODULE_DIRS})
  endif()

  list(REMOVE_DUPLICATES _dirs)
  set(${OUTPUT_VAR} "${_dirs}" PARENT_SCOPE)
endfunction()
