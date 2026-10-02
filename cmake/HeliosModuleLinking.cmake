# Installed Helios module linking helpers.

include_guard(GLOBAL)

function(_helios_installed_parse_visibility INPUT_VAR OUT_PUBLIC OUT_PRIVATE OUT_INTERFACE)
  set(_public)
  set(_private)
  set(_interface)
  set(_visibility PUBLIC)

  foreach(_item IN LISTS ${INPUT_VAR})
    if(_item STREQUAL "PUBLIC" OR _item STREQUAL "PRIVATE" OR _item STREQUAL "INTERFACE")
      set(_visibility "${_item}")
      continue()
    endif()

    if(_visibility STREQUAL "PUBLIC")
      list(APPEND _public "${_item}")
    elseif(_visibility STREQUAL "PRIVATE")
      list(APPEND _private "${_item}")
    else()
      list(APPEND _interface "${_item}")
    endif()
  endforeach()

  set(${OUT_PUBLIC} "${_public}" PARENT_SCOPE)
  set(${OUT_PRIVATE} "${_private}" PARENT_SCOPE)
  set(${OUT_INTERFACE} "${_interface}" PARENT_SCOPE)
endfunction()

function(_helios_installed_link_one TARGET VISIBILITY MODULE REQUIRED)
  helios_get_module_alias(${MODULE} _alias)
  if(TARGET "${_alias}")
    target_link_libraries(${TARGET} ${VISIBILITY} ${_alias})
  elseif(REQUIRED)
    message(FATAL_ERROR
        "helios_link_modules: installed module target '${_alias}' not found")
  endif()
endfunction()

#[[
    helios_link_modules(
        TARGET <target>
        MODULES  [PUBLIC|PRIVATE|INTERFACE] <module>...
        OPTIONAL [PUBLIC|PRIVATE|INTERFACE] <module>...
    )

    Installed-package helper for linking exported Helios module targets.
]]
function(helios_link_modules)
  cmake_parse_arguments(ARG "" "TARGET" "MODULES;OPTIONAL" ${ARGN})

  if(NOT ARG_TARGET)
    message(FATAL_ERROR "helios_link_modules: TARGET is required")
  endif()

  if(ARG_MODULES)
    _helios_installed_parse_visibility(ARG_MODULES _pub _priv _iface)
    foreach(_module IN LISTS _pub)
      _helios_installed_link_one(${ARG_TARGET} PUBLIC ${_module} TRUE)
    endforeach()
    foreach(_module IN LISTS _priv)
      _helios_installed_link_one(${ARG_TARGET} PRIVATE ${_module} TRUE)
    endforeach()
    foreach(_module IN LISTS _iface)
      _helios_installed_link_one(${ARG_TARGET} INTERFACE ${_module} TRUE)
    endforeach()
  endif()

  if(ARG_OPTIONAL)
    _helios_installed_parse_visibility(ARG_OPTIONAL _pub _priv _iface)
    foreach(_module IN LISTS _pub)
      _helios_installed_link_one(${ARG_TARGET} PUBLIC ${_module} FALSE)
    endforeach()
    foreach(_module IN LISTS _priv)
      _helios_installed_link_one(${ARG_TARGET} PRIVATE ${_module} FALSE)
    endforeach()
    foreach(_module IN LISTS _iface)
      _helios_installed_link_one(${ARG_TARGET} INTERFACE ${_module} FALSE)
    endforeach()
  endif()
endfunction()

#[[
    helios_get_module_target(<name> <out-var>)

    Returns the exported target name for an installed Helios module.

    Example:
        helios_get_module_target(helios_core core_target)
]]
function(helios_get_module_target NAME OUTPUT_VAR)
  helios_get_module_alias(${NAME} _alias)
  set(${OUTPUT_VAR} "${_alias}" PARENT_SCOPE)
endfunction()

#[[
    helios_get_module_alias(<name> <out-var>)

    Returns the installed target name. That is the module alias: helios_app
    is helios::app and helios_sdl3_window is helios::sdl3::window.
    HeliosAliases.cmake records aliases from the installed graph; otherwise
    the name is split on the first underscore and further underscores become ::.

    Example:
        helios_get_module_alias(helios_app app_alias)
]]
function(helios_get_module_alias NAME OUTPUT_VAR)
  string(TOUPPER "${NAME}" _upper)
  if(DEFINED ${_upper}_ALIAS AND NOT "${${_upper}_ALIAS}" STREQUAL "")
    set(${OUTPUT_VAR} "${${_upper}_ALIAS}" PARENT_SCOPE)
    return()
  endif()
  if("${NAME}" MATCHES "^([^_]+)_(.+)$")
    string(REPLACE "_" "::" _rest "${CMAKE_MATCH_2}")
    set(${OUTPUT_VAR} "${CMAKE_MATCH_1}::${_rest}" PARENT_SCOPE)
    return()
  endif()
  set(${OUTPUT_VAR} "${NAME}" PARENT_SCOPE)
endfunction()
