# Helios Engine Module Linking
#
# Consumer API for linking Helios modules into external targets.

include_guard(GLOBAL)

# ============================================================================
# Deferred Module Linking (Pass 2C)
# ============================================================================

function(_helios_defer_module_link TARGET VISIBILITY MODULE)
  set_property(GLOBAL APPEND PROPERTY HELIOS_DEFERRED_MODULE_LINKS
               "${TARGET}::${VISIBILITY}::${MODULE}")
endfunction()

function(helios_flush_deferred_module_links)
  get_property(_links GLOBAL PROPERTY HELIOS_DEFERRED_MODULE_LINKS)
  if(NOT _links)
    return()
  endif()

  message(STATUS "--- Linking Deferred Module Dependencies (Pass 2C) ---")

  foreach(_entry ${_links})
    string(REPLACE "::" ";" _parts "${_entry}")
    list(GET _parts 0 _target)
    list(GET _parts 1 _visibility)
    list(GET _parts 2 _module)

    if(NOT TARGET helios::module::${_module})
      message(FATAL_ERROR
                  "helios_link_modules: Module target 'helios::module::${_module}' not found")
    endif()

    target_link_libraries(${_target} ${_visibility} helios::module::${_module})
    message(STATUS "  → Deferred link: ${_target} → ${_module} (${_visibility})")

    set(_module_target "helios_module_${_module}")
    if(TARGET ${_module_target})
      get_target_property(_is_shared ${_module_target} HELIOS_IS_SHARED_MODULE)
      if(_is_shared)
        add_custom_command(TARGET ${_target} POST_BUILD
                      COMMAND ${CMAKE_COMMAND} -E copy_if_different
                          $<TARGET_FILE:${_module_target}>
                          $<TARGET_FILE_DIR:${_target}>
                      COMMENT "Copying ${_module} shared library"
                  )
      endif()
    endif()
  endforeach()

  set_property(GLOBAL PROPERTY HELIOS_DEFERRED_MODULE_LINKS "")
  message(STATUS "")
endfunction()

# ============================================================================
# Unified Module Linking
# ============================================================================

#[[
    helios_link_modules(
        TARGET <target>
        MODULES  <visibility> <module1> <module2> ...
        OPTIONAL <visibility> <module1> <module2> ...
    )

    Links required and optional Helios modules to a consumer target.
    MODULES: FATAL_ERROR if the module is not available.
    OPTIONAL: silently skipped if disabled or unavailable.
]]
function(helios_link_modules)
  cmake_parse_arguments(ARG "" "TARGET" "MODULES;OPTIONAL" ${ARGN})

  if(NOT ARG_TARGET)
    message(FATAL_ERROR "helios_link_modules: TARGET is required")
  endif()

  if(ARG_MODULES)
    set(_visibility "PUBLIC")
    foreach(_item ${ARG_MODULES})
      if("${_item}" STREQUAL "PUBLIC" OR "${_item}" STREQUAL "PRIVATE" OR "${_item}" STREQUAL "INTERFACE")
        set(_visibility "${_item}")
        continue()
      endif()

      string(TOUPPER "${_item}" _upper)

      helios_module_enabled(${_item} _is_enabled)
      if(NOT _is_enabled)
        message(FATAL_ERROR
            "helios_link_modules: Required module '${_item}' is not enabled. "
            "Enable it with -DHELIOS_BUILD_${_upper}=ON")
      endif()

      if(TARGET helios::module::${_item})
        target_link_libraries(${ARG_TARGET} ${_visibility} helios::module::${_item})
      else()
        # Module enabled but not built yet (consumer runs earlier in Pass 2 topological order).
        _helios_defer_module_link(${ARG_TARGET} ${_visibility} ${_item})
      endif()

      set(_module_target "helios_module_${_item}")
      if(TARGET ${_module_target})
        get_target_property(_is_shared ${_module_target} HELIOS_IS_SHARED_MODULE)
        if(_is_shared)
          add_custom_command(TARGET ${ARG_TARGET} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            $<TARGET_FILE:${_module_target}>
                            $<TARGET_FILE_DIR:${ARG_TARGET}>
                        COMMENT "Copying ${_item} shared library"
                    )
        endif()
      endif()
    endforeach()
  endif()

  if(ARG_OPTIONAL)
    set(_visibility "PUBLIC")
    foreach(_item ${ARG_OPTIONAL})
      if("${_item}" STREQUAL "PUBLIC" OR "${_item}" STREQUAL "PRIVATE" OR "${_item}" STREQUAL "INTERFACE")
        set(_visibility "${_item}")
        continue()
      endif()

      helios_module_enabled(${_item} _is_enabled)
      if(_is_enabled AND TARGET helios::module::${_item})
        target_link_libraries(${ARG_TARGET} ${_visibility} helios::module::${_item})
        message(STATUS "Linked optional module '${_item}' to ${ARG_TARGET}")
      endif()
    endforeach()
  endif()
endfunction()

# ============================================================================
# Utility Functions
# ============================================================================

function(helios_get_module_target NAME OUTPUT_VAR)
  set(${OUTPUT_VAR} "helios_module_${NAME}" PARENT_SCOPE)
endfunction()

function(helios_get_module_alias NAME OUTPUT_VAR)
  set(${OUTPUT_VAR} "helios::module::${NAME}" PARENT_SCOPE)
endfunction()
