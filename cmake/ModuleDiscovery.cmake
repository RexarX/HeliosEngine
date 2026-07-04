# Helios Engine Module Discovery
#
# Discovers modules with a registration dry-run, then builds enabled modules in
# topological order.

include_guard(GLOBAL)

function(_helios_is_module_container_directory DIR OUTPUT_VAR)
  set(_has_child_modules FALSE)
  if(EXISTS "${DIR}")
    file(GLOB _children RELATIVE "${DIR}" "${DIR}/*")
    foreach(_child IN LISTS _children)
      set(_child_path "${DIR}/${_child}")
      if(IS_DIRECTORY "${_child_path}" AND EXISTS "${_child_path}/CMakeLists.txt")
        set(_has_child_modules TRUE)
        break()
      endif()
    endforeach()
  endif()

  if(_has_child_modules AND NOT EXISTS "${DIR}/CMakeLists.txt")
    set(${OUTPUT_VAR} TRUE PARENT_SCOPE)
  else()
    set(${OUTPUT_VAR} FALSE PARENT_SCOPE)
  endif()
endfunction()

function(_helios_discover_module_at_path MODULE_PATH)
  if(NOT EXISTS "${MODULE_PATH}/CMakeLists.txt")
    return()
  endif()

  get_property(_registered_before GLOBAL PROPERTY HELIOS_REGISTERED_MODULES)
  if(NOT _registered_before)
    set(_registered_before "")
  endif()

  set(HELIOS_REGISTRATION_PASS ON)
  include("${MODULE_PATH}/CMakeLists.txt")
  set(HELIOS_REGISTRATION_PASS OFF)

  get_property(_registered_after GLOBAL PROPERTY HELIOS_REGISTERED_MODULES)
  if(NOT _registered_after)
    message(WARNING "Module discovery: no module registered from ${MODULE_PATH}")
    return()
  endif()

  set(_new_modules)
  foreach(_module IN LISTS _registered_after)
    if(NOT _module IN_LIST _registered_before)
      list(APPEND _new_modules ${_module})
    endif()
  endforeach()

  if(NOT _new_modules)
    message(WARNING "Module discovery: no new module registered from ${MODULE_PATH}")
    return()
  endif()

  list(LENGTH _new_modules _new_count)
  if(_new_count GREATER 1)
    message(WARNING
        "Module discovery: multiple modules registered from ${MODULE_PATH}; "
        "using the first entry")
  endif()

  list(GET _new_modules 0 _module_name)
  string(TOUPPER "${_module_name}" _upper_name)

  if(DEFINED HELIOS_MODULE_${_upper_name}_PATH AND NOT
     "${HELIOS_MODULE_${_upper_name}_PATH}" STREQUAL "${MODULE_PATH}")
    message(FATAL_ERROR
        "Module '${_module_name}' already registered from "
        "${HELIOS_MODULE_${_upper_name}_PATH}; duplicate at ${MODULE_PATH}")
  endif()

  set(HELIOS_MODULE_${_upper_name}_PATH "${MODULE_PATH}" CACHE INTERNAL
      "Path for ${_module_name}")

  get_property(_discovered GLOBAL PROPERTY HELIOS_DISCOVERED)
  if(NOT _discovered)
    set(_discovered "")
  endif()

  if(NOT _module_name IN_LIST _discovered)
    set_property(GLOBAL APPEND PROPERTY HELIOS_DISCOVERED ${_module_name})
    message(STATUS "Discovered module: ${_module_name} (${MODULE_PATH})")
  endif()
endfunction()

#[[
    helios_discover_modules(
        DIRECTORY <path>
        [EXCLUDE <names...>]
    )

    Runs Pass 1A module registration. DIRECTORY may be a module container such
    as src/ or a single module root such as examples/custom_module/.
]]
function(helios_discover_modules)
  set(options "")
  set(oneValueArgs DIRECTORY)
  set(multiValueArgs EXCLUDE)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT ARG_DIRECTORY)
    set(ARG_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  endif()

  cmake_path(ABSOLUTE_PATH ARG_DIRECTORY NORMALIZE OUTPUT_VARIABLE _abs_directory)

  message(STATUS "")
  message(STATUS "--- Module Registration Scan (Pass 1A): ${_abs_directory} ---")

  _helios_is_module_container_directory("${_abs_directory}" _is_container)
  if(_is_container)
    file(GLOB _children RELATIVE "${_abs_directory}" "${_abs_directory}/*")
    foreach(_child IN LISTS _children)
      set(_child_path "${_abs_directory}/${_child}")
      if(NOT IS_DIRECTORY "${_child_path}")
        continue()
      endif()
      if(_child IN_LIST ARG_EXCLUDE)
        message(STATUS "Skipping excluded module directory: ${_child}")
        continue()
      endif()
      _helios_discover_module_at_path("${_child_path}")
    endforeach()
  else()
    _helios_discover_module_at_path("${_abs_directory}")
  endif()

  get_property(_discovered GLOBAL PROPERTY HELIOS_DISCOVERED)
  if(_discovered)
    list(LENGTH _discovered _module_count)
  else()
    set(_module_count 0)
  endif()
  message(STATUS "Total discovered modules: ${_module_count}")
  message(STATUS "")

  if(HELIOS_BUILD_ALL_MODULES)
    message(STATUS "--- HELIOS_BUILD_ALL_MODULES: force-enabling all modules (Pass 1B) ---")
    foreach(_module IN LISTS _discovered)
      string(TOUPPER "${_module}" _upper)
      set(HELIOS_BUILD_${_upper} ON CACHE BOOL "Build the ${_module} module" FORCE)
    endforeach()
    message(STATUS "")
  endif()
endfunction()

function(_helios_topological_sort INPUT_LIST OUTPUT_VAR)
  set(_in_degree)
  set(_adj)

  foreach(_node IN LISTS ${INPUT_LIST})
    set(_in_degree_${_node} 0)
  endforeach()

  foreach(_node IN LISTS ${INPUT_LIST})
    string(TOUPPER "${_node}" _upper)
    foreach(_dep IN LISTS HELIOS_MODULE_${_upper}_DEPENDS)
      if(_dep IN_LIST ${INPUT_LIST})
        math(EXPR _new_count "${_in_degree_${_node}} + 1")
        set(_in_degree_${_node} ${_new_count})
        list(APPEND _adj_${_dep} ${_node})
      endif()
    endforeach()
  endforeach()

  set(_queue)
  foreach(_node IN LISTS ${INPUT_LIST})
    if(_in_degree_${_node} EQUAL 0)
      list(APPEND _queue ${_node})
    endif()
  endforeach()

  set(_sorted)
  while(_queue)
    list(POP_FRONT _queue _current)
    list(APPEND _sorted ${_current})

    foreach(_neighbor IN LISTS _adj_${_current})
      math(EXPR _new_deg "${_in_degree_${_neighbor}} - 1")
      set(_in_degree_${_neighbor} ${_new_deg})
      if(_new_deg EQUAL 0)
        list(APPEND _queue ${_neighbor})
      endif()
    endforeach()
  endwhile()

  list(LENGTH ${INPUT_LIST} _input_len)
  list(LENGTH _sorted _sorted_len)
  if(NOT _sorted_len EQUAL _input_len)
    set(_cycle_nodes)
    foreach(_node IN LISTS ${INPUT_LIST})
      if(NOT _node IN_LIST _sorted)
        list(APPEND _cycle_nodes ${_node})
      endif()
    endforeach()
    message(FATAL_ERROR "Module dependency cycle detected among: ${_cycle_nodes}")
  endif()

  set(${OUTPUT_VAR} "${_sorted}" PARENT_SCOPE)
endfunction()

#[[
    helios_build_discovered_modules()

    Builds enabled discovered modules in topological order, then resolves
    optional and deferred module links.
]]
function(helios_build_discovered_modules)
  get_property(_discovered GLOBAL PROPERTY HELIOS_DISCOVERED)
  if(NOT _discovered)
    message(STATUS "No modules discovered to build")
    return()
  endif()

  set(_enabled_modules)
  foreach(_module IN LISTS _discovered)
    helios_module_enabled(${_module} _is_enabled)
    if(_is_enabled)
      list(APPEND _enabled_modules ${_module})
    endif()
  endforeach()

  _helios_topological_sort(_enabled_modules _sorted_modules)

  message(STATUS "")
  message(STATUS "--- Building Modules in Topological Order (Pass 2) ---")
  foreach(_module IN LISTS _sorted_modules)
    helios_get_module_path(NAME ${_module} OUTPUT_VAR _module_path)
    if(NOT _module_path)
      message(FATAL_ERROR "Missing path for discovered module '${_module}'")
    endif()

    message(STATUS "Building module: ${_module} (${_module_path})")
    add_subdirectory("${_module_path}")
  endforeach()
  message(STATUS "")

  message(STATUS "--- Linking Optional Module Dependencies (Pass 2B) ---")
  foreach(_module IN LISTS _sorted_modules)
    set(_target "helios_module_${_module}")
    if(NOT TARGET ${_target})
      continue()
    endif()

    foreach(_visibility PUBLIC PRIVATE INTERFACE)
      get_target_property(_opt_deps ${_target} HELIOS_OPTIONAL_MODULE_DEPS_${_visibility})
      if(NOT _opt_deps OR _opt_deps STREQUAL "_opt_deps-NOTFOUND")
        continue()
      endif()

      foreach(_dep IN LISTS _opt_deps)
        if(TARGET helios::module::${_dep})
          _helios_link_helios_module(${_target} ${_dep} ${_visibility})
          message(STATUS "  -> Optional link: ${_module} -> ${_dep} (${_visibility})")
        endif()
      endforeach()
    endforeach()
  endforeach()
  message(STATUS "")

  helios_flush_deferred_module_links()

  foreach(_module IN LISTS _discovered)
    helios_module_enabled(${_module} _is_enabled)
    if(NOT _is_enabled)
      message(STATUS "Skipping disabled module: ${_module}")
    endif()
  endforeach()
endfunction()

#[[
    helios_discover_extra_module_dirs()

    Discovers each directory registered through helios_add_extra_module_dirs()
    and HELIOS_EXTRA_MODULE_DIRS.
]]
function(helios_discover_extra_module_dirs)
  helios_get_extra_module_dirs(_extra_dirs)
  if(NOT _extra_dirs)
    return()
  endif()

  foreach(_extra_dir IN LISTS _extra_dirs)
    if(IS_DIRECTORY "${_extra_dir}")
      helios_discover_modules(DIRECTORY "${_extra_dir}")
    else()
      message(WARNING "Helios extra module directory does not exist: ${_extra_dir}")
    endif()
  endforeach()
endfunction()
