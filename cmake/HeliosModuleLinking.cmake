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
  if(TARGET helios::module::${MODULE})
    target_link_libraries(${TARGET} ${VISIBILITY} helios::module::${MODULE})
  elseif(REQUIRED)
    message(FATAL_ERROR
        "helios_link_modules: installed module target 'helios::module::${MODULE}' not found")
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
        helios_get_module_target(core core_target)
]]
function(helios_get_module_target NAME OUTPUT_VAR)
  set(${OUTPUT_VAR} "helios::module::${NAME}" PARENT_SCOPE)
endfunction()

#[[
    helios_get_module_alias(<name> <out-var>)

    Returns the canonical alias for an installed Helios module. Installed
    exports use the same helios::module::<name> spelling as the build tree.

    Example:
        helios_get_module_alias(core core_alias)
]]
function(helios_get_module_alias NAME OUTPUT_VAR)
  set(${OUTPUT_VAR} "helios::module::${NAME}" PARENT_SCOPE)
endfunction()
