# Helios Engine Module Linking
#
# Consumer API for linking Helios modules into external targets.

include_guard(GLOBAL)

include(Primitives)

function(_helios_defer_module_link TARGET VISIBILITY MODULE)
  set_property(GLOBAL APPEND PROPERTY HELIOS_DEFERRED_MODULE_LINKS
               "${TARGET}|${VISIBILITY}|${MODULE}")
endfunction()

#[[
    helios_flush_deferred_module_links()

    Resolves required module links recorded before the requested module target
    existed. Called by module discovery after all enabled modules are built.
]]
function(helios_flush_deferred_module_links)
  get_property(_links GLOBAL PROPERTY HELIOS_DEFERRED_MODULE_LINKS)
  if(NOT _links)
    return()
  endif()

  message(STATUS "--- Linking Deferred Module Dependencies (Pass 2C) ---")

  foreach(_entry IN LISTS _links)
    string(REPLACE "|" ";" _parts "${_entry}")
    list(GET _parts 0 _target)
    list(GET _parts 1 _visibility)
    list(GET _parts 2 _module)

    if(NOT TARGET helios::module::${_module})
      message(FATAL_ERROR
          "helios_link_modules: Module target 'helios::module::${_module}' not found")
    endif()

    target_link_libraries(${_target} ${_visibility} helios::module::${_module})
    message(STATUS "  -> Deferred link: ${_target} -> ${_module} (${_visibility})")

    set(_module_target "helios_module_${_module}")
    helios_copy_shared_lib(CONSUMER ${_target} PROVIDER ${_module_target})
  endforeach()

  set_property(GLOBAL PROPERTY HELIOS_DEFERRED_MODULE_LINKS "")
  message(STATUS "")
endfunction()

function(_helios_link_module_to_consumer TARGET VISIBILITY MODULE REQUIRED)
  string(TOUPPER "${MODULE}" _upper)

  helios_module_enabled(${MODULE} _is_enabled)
  if(REQUIRED AND NOT _is_enabled)
    message(FATAL_ERROR
        "helios_link_modules: Required module '${MODULE}' is not enabled. "
        "Enable it with -DHELIOS_BUILD_${_upper}=ON")
  endif()
  if(NOT _is_enabled)
    return()
  endif()

  if(TARGET helios::module::${MODULE})
    target_link_libraries(${TARGET} ${VISIBILITY} helios::module::${MODULE})
    helios_copy_shared_lib(CONSUMER ${TARGET} PROVIDER helios_module_${MODULE})
  elseif(REQUIRED)
    _helios_defer_module_link(${TARGET} ${VISIBILITY} ${MODULE})
  endif()
endfunction()

#[[
    helios_link_modules(
        TARGET <target>
        MODULES  [PUBLIC|PRIVATE|INTERFACE] <module>...
        OPTIONAL [PUBLIC|PRIVATE|INTERFACE] <module>...
    )

    Links required and optional Helios modules to a consumer target. Required
    modules must be enabled; links are deferred when the module target has not
    been created yet. Optional modules are skipped when disabled or unavailable.

    Example:
        helios_link_modules(
            TARGET game
            MODULES PRIVATE app ecs
            OPTIONAL PRIVATE profile
        )
]]
function(helios_link_modules)
  cmake_parse_arguments(ARG "" "TARGET" "MODULES;OPTIONAL" ${ARGN})

  if(NOT ARG_TARGET)
    message(FATAL_ERROR "helios_link_modules: TARGET is required")
  endif()

  if(ARG_MODULES)
    helios_parse_visibility(
        INPUT ${ARG_MODULES}
        PUBLIC_VAR _required_public
        PRIVATE_VAR _required_private
        INTERFACE_VAR _required_interface
    )
    foreach(_module IN LISTS _required_public)
      _helios_link_module_to_consumer(${ARG_TARGET} PUBLIC ${_module} TRUE)
    endforeach()
    foreach(_module IN LISTS _required_private)
      _helios_link_module_to_consumer(${ARG_TARGET} PRIVATE ${_module} TRUE)
    endforeach()
    foreach(_module IN LISTS _required_interface)
      _helios_link_module_to_consumer(${ARG_TARGET} INTERFACE ${_module} TRUE)
    endforeach()
  endif()

  if(ARG_OPTIONAL)
    helios_parse_visibility(
        INPUT ${ARG_OPTIONAL}
        PUBLIC_VAR _optional_public
        PRIVATE_VAR _optional_private
        INTERFACE_VAR _optional_interface
    )
    foreach(_module IN LISTS _optional_public)
      _helios_link_module_to_consumer(${ARG_TARGET} PUBLIC ${_module} FALSE)
    endforeach()
    foreach(_module IN LISTS _optional_private)
      _helios_link_module_to_consumer(${ARG_TARGET} PRIVATE ${_module} FALSE)
    endforeach()
    foreach(_module IN LISTS _optional_interface)
      _helios_link_module_to_consumer(${ARG_TARGET} INTERFACE ${_module} FALSE)
    endforeach()
  endif()
endfunction()

#[[
    helios_get_module_target(<name> <output_var>)

    Returns the concrete build-tree target name for a module.
]]
function(helios_get_module_target NAME OUTPUT_VAR)
  set(${OUTPUT_VAR} "helios_module_${NAME}" PARENT_SCOPE)
endfunction()

#[[
    helios_get_module_alias(<name> <output_var>)

    Returns the public alias target name for a module.
]]
function(helios_get_module_alias NAME OUTPUT_VAR)
  set(${OUTPUT_VAR} "helios::module::${NAME}" PARENT_SCOPE)
endfunction()
