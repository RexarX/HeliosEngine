# Helios Engine Module Registry
#
# Stores module metadata gathered during discovery and resolves enabled module
# dependencies before targets are created.

include_guard(GLOBAL)

include(Primitives)

#[[
    helios_register_module(
        NAME <name>
        [DESCRIPTION <text>]
        [VERSION <version>]
        [DEFAULT <ON|OFF>]
        [HEADER_ONLY]
        [DEPENDS <visibility> <module>...]
        [OPTIONAL_DEPENDS <visibility> <module>...]
    )

    Registers a module in the dependency graph. Visibility markers are accepted
    for authoring symmetry with helios_module(), but the cached graph stores
    bare module names.

    Example:
        helios_register_module(
            NAME app
            DEFAULT ON
            DEPENDS PUBLIC async PUBLIC core PUBLIC ecs
            OPTIONAL_DEPENDS PUBLIC profile
        )
]]
function(helios_register_module)
  set(options HEADER_ONLY)
  set(oneValueArgs NAME DESCRIPTION DEFAULT VERSION)
  set(multiValueArgs DEPENDS OPTIONAL_DEPENDS)
  cmake_parse_arguments(MODULE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT MODULE_NAME)
    message(FATAL_ERROR "helios_register_module: NAME is required")
  endif()

  string(TOUPPER "${MODULE_NAME}" _upper_name)
  set(_option_name "HELIOS_BUILD_${_upper_name}")

  if(NOT DEFINED MODULE_DEFAULT)
    set(MODULE_DEFAULT ON)
  endif()
  if(NOT MODULE_DESCRIPTION)
    set(MODULE_DESCRIPTION "Build the ${MODULE_NAME} module")
  endif()
  if(NOT MODULE_VERSION)
    set(MODULE_VERSION "0.1.0")
  endif()

  if(MODULE_DEPENDS)
    helios_parse_visibility(
        INPUT ${MODULE_DEPENDS}
        PUBLIC_VAR _unused_public
        PRIVATE_VAR _unused_private
        INTERFACE_VAR _unused_interface
        STRIP_VAR _depends_stripped
    )
  else()
    set(_depends_stripped)
  endif()

  if(MODULE_OPTIONAL_DEPENDS)
    helios_parse_visibility(
        INPUT ${MODULE_OPTIONAL_DEPENDS}
        PUBLIC_VAR _unused_public
        PRIVATE_VAR _unused_private
        INTERFACE_VAR _unused_interface
        STRIP_VAR _optional_depends_stripped
    )
  else()
    set(_optional_depends_stripped)
  endif()

  if(NOT DEFINED ${_option_name})
    option(${_option_name} "${MODULE_DESCRIPTION}" ${MODULE_DEFAULT})
  endif()

  set(HELIOS_MODULE_${_upper_name}_REGISTERED TRUE CACHE INTERNAL "Module ${MODULE_NAME} is registered")
  set(HELIOS_MODULE_${_upper_name}_DEPENDS "${_depends_stripped}" CACHE INTERNAL "Dependencies for ${MODULE_NAME}")
  set(HELIOS_MODULE_${_upper_name}_OPTIONAL_DEPENDS "${_optional_depends_stripped}" CACHE INTERNAL "Optional dependencies for ${MODULE_NAME}")
  set(HELIOS_MODULE_${_upper_name}_VERSION "${MODULE_VERSION}" CACHE INTERNAL "Version of ${MODULE_NAME}")

  if(MODULE_HEADER_ONLY)
    set(HELIOS_MODULE_${_upper_name}_HEADER_ONLY TRUE CACHE INTERNAL "Module ${MODULE_NAME} is header-only")
  else()
    set(HELIOS_MODULE_${_upper_name}_HEADER_ONLY FALSE CACHE INTERNAL "Module ${MODULE_NAME} is header-only")
  endif()

  get_property(_registered GLOBAL PROPERTY HELIOS_REGISTERED_MODULES)
  if(NOT _registered)
    set(_registered)
  endif()
  if(NOT MODULE_NAME IN_LIST _registered)
    set_property(GLOBAL APPEND PROPERTY HELIOS_REGISTERED_MODULES ${MODULE_NAME})
  endif()

  message(STATUS "Registered module: ${MODULE_NAME} v${MODULE_VERSION} (${_option_name}=${${_option_name}})")
endfunction()

#[[
    helios_module_enabled(<name> <output_var>)

    Sets output_var to TRUE when HELIOS_BUILD_<NAME> is enabled.
]]
function(helios_module_enabled NAME OUTPUT_VAR)
  string(TOUPPER "${NAME}" _upper_name)
  set(_option_name "HELIOS_BUILD_${_upper_name}")

  if(DEFINED ${_option_name} AND ${_option_name})
    set(${OUTPUT_VAR} TRUE PARENT_SCOPE)
  else()
    set(${OUTPUT_VAR} FALSE PARENT_SCOPE)
  endif()
endfunction()

#[[
    helios_module_is_header_only(<name> <output_var>)

    Sets output_var to TRUE when the registered module has no source files.
]]
function(helios_module_is_header_only NAME OUTPUT_VAR)
  string(TOUPPER "${NAME}" _upper_name)

  if(DEFINED HELIOS_MODULE_${_upper_name}_HEADER_ONLY AND HELIOS_MODULE_${_upper_name}_HEADER_ONLY)
    set(${OUTPUT_VAR} TRUE PARENT_SCOPE)
  else()
    set(${OUTPUT_VAR} FALSE PARENT_SCOPE)
  endif()
endfunction()

#[[
    helios_resolve_module_dependencies()

    Forces required dependencies of enabled modules on until the enabled module
    set reaches a fixed point.
]]
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

    foreach(_module IN LISTS _modules)
      helios_module_enabled(${_module} _is_enabled)
      if(NOT _is_enabled)
        continue()
      endif()

      string(TOUPPER "${_module}" _upper_name)
      foreach(_dep IN LISTS HELIOS_MODULE_${_upper_name}_DEPENDS)
        helios_module_enabled(${_dep} _dep_enabled)
        if(NOT _dep_enabled)
          string(TOUPPER "${_dep}" _dep_upper)
          set(_dep_option "HELIOS_BUILD_${_dep_upper}")
          message(STATUS "Enabling module '${_dep}' (required by '${_module}')")
          set(${_dep_option} ON CACHE BOOL "Build the ${_dep} module" FORCE)
          set(_changed TRUE)
        endif()
      endforeach()
    endforeach()
  endwhile()

  if(_iterations GREATER_EQUAL 100)
    message(FATAL_ERROR "Module dependency resolution reached iteration limit. Possible circular dependency.")
  endif()
endfunction()

#[[
    helios_validate_module_dependencies()

    Fails configure when an enabled module has a required dependency that remains
    disabled after dependency resolution.
]]
function(helios_validate_module_dependencies)
  get_property(_modules GLOBAL PROPERTY HELIOS_REGISTERED_MODULES)
  foreach(_module IN LISTS _modules)
    helios_module_enabled(${_module} _is_enabled)
    if(NOT _is_enabled)
      continue()
    endif()

    string(TOUPPER "${_module}" _upper_name)
    foreach(_dep IN LISTS HELIOS_MODULE_${_upper_name}_DEPENDS)
      helios_module_enabled(${_dep} _dep_enabled)
      if(NOT _dep_enabled)
        string(TOUPPER "${_dep}" _upper_dep)
        message(FATAL_ERROR
            "Module '${_module}' requires module '${_dep}', but it is disabled. "
            "Enable it with -DHELIOS_BUILD_${_upper_dep}=ON")
      endif()
    endforeach()
  endforeach()
endfunction()

#[[
    helios_print_modules()

    Prints registered and built module summaries.
]]
function(helios_print_modules)
  get_property(_registered GLOBAL PROPERTY HELIOS_REGISTERED_MODULES)
  get_property(_built GLOBAL PROPERTY HELIOS_MODULES)

  message(STATUS "")
  message(STATUS "========== Helios Engine Modules ==========")

  if(_registered)
    list(REMOVE_DUPLICATES _registered)
    list(SORT _registered)
    message(STATUS "Registered modules:")
    foreach(_module IN LISTS _registered)
      helios_module_enabled(${_module} _is_enabled)
      helios_module_is_header_only(${_module} _is_header_only)
      if(_is_enabled)
        if(_is_header_only)
          message(STATUS "  + ${_module} [ENABLED] [HEADER_ONLY]")
        else()
          message(STATUS "  + ${_module} [ENABLED]")
        endif()
      else()
        message(STATUS "  - ${_module} [DISABLED]")
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
    foreach(_target IN LISTS _built)
      get_target_property(_name ${_target} HELIOS_MODULE_NAME)
      get_target_property(_version ${_target} HELIOS_MODULE_VERSION)
      get_target_property(_type ${_target} TYPE)
      if(_name AND _version)
        message(STATUS "  + ${_name} v${_version} (${_target}) [${_type}]")
      else()
        message(STATUS "  + ${_target} [${_type}]")
      endif()
    endforeach()
  else()
    message(STATUS "No modules built")
  endif()

  message(STATUS "===========================================")
  message(STATUS "")
endfunction()

#[[
    helios_print_module_build_options()

    Prints cache options that control module enablement and library type.
]]
function(helios_print_module_build_options)
  get_property(_registered GLOBAL PROPERTY HELIOS_REGISTERED_MODULES)
  get_property(_built GLOBAL PROPERTY HELIOS_MODULES)

  message(STATUS "")
  message(STATUS "========== Module Build Options ==========")

  if(_registered)
    list(SORT _registered)
    message(STATUS "Enable/Disable modules:")
    foreach(_module IN LISTS _registered)
      string(TOUPPER "${_module}" _upper_name)
      set(_option "HELIOS_BUILD_${_upper_name}")
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
    foreach(_target IN LISTS _built)
      get_target_property(_name ${_target} HELIOS_MODULE_NAME)
      if(_name)
        string(TOUPPER "${_name}" _upper_name)
        set(_type_option "HELIOS_BUILD_OPTION_${_upper_name}")
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

#[[
    helios_get_module_list(<output_var>)

    Gets built module target names.
]]
function(helios_get_module_list OUTPUT_VAR)
  get_property(_modules GLOBAL PROPERTY HELIOS_MODULES)
  if(_modules)
    list(REMOVE_DUPLICATES _modules)
    list(SORT _modules)
  endif()
  set(${OUTPUT_VAR} ${_modules} PARENT_SCOPE)
endfunction()

#[[
    helios_get_enabled_modules(<output_var>)

    Gets registered module names whose HELIOS_BUILD_<NAME> option is enabled.
]]
function(helios_get_enabled_modules OUTPUT_VAR)
  get_property(_registered GLOBAL PROPERTY HELIOS_REGISTERED_MODULES)
  set(_enabled)

  foreach(_module IN LISTS _registered)
    helios_module_enabled(${_module} _is_enabled)
    if(_is_enabled)
      list(APPEND _enabled ${_module})
    endif()
  endforeach()

  set(${OUTPUT_VAR} ${_enabled} PARENT_SCOPE)
endfunction()

#[[
    helios_get_module_path(NAME <name> OUTPUT_VAR <out>)

    Gets the discovered filesystem path for a registered module.
]]
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

#[[
    helios_add_extra_module_dirs(<path>...)

    Appends directories to the extra module search list. Paths are normalized to
    absolute form relative to the call site.
]]
function(helios_add_extra_module_dirs)
  if(ARGC EQUAL 0)
    message(FATAL_ERROR "helios_add_extra_module_dirs: at least one directory path is required")
  endif()

  get_property(_dirs GLOBAL PROPERTY HELIOS_EXTRA_MODULE_DIRS_ACCUM)
  if(NOT _dirs)
    set(_dirs "")
  endif()

  foreach(_raw_dir IN LISTS ARGN)
    cmake_path(ABSOLUTE_PATH _raw_dir BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
               NORMALIZE OUTPUT_VARIABLE _abs_dir)

    if(NOT _abs_dir IN_LIST _dirs)
      list(APPEND _dirs "${_abs_dir}")
      message(STATUS "Helios extra module search path: ${_abs_dir}")
    endif()
  endforeach()

  set_property(GLOBAL PROPERTY HELIOS_EXTRA_MODULE_DIRS_ACCUM "${_dirs}")
endfunction()

#[[
    helios_get_extra_module_dirs(<output_var>)

    Gets all module search dirs registered by helios_add_extra_module_dirs() and
    HELIOS_EXTRA_MODULE_DIRS.
]]
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
