# Include this file before add_subdirectory(Helios) when registering user modules.
# It has no dependency on the rest of the Helios CMake modules.

include_guard(GLOBAL)

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

    Gets directories registered by helios_add_extra_module_dirs() and
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
