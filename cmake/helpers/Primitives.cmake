# Helios Engine CMake primitives
#
# Small, dependency-free building blocks used by higher-level CMake helpers.

include_guard(GLOBAL)

#[[
    helios_parse_visibility(
        INPUT <items...>
        PUBLIC_VAR <out>
        PRIVATE_VAR <out>
        INTERFACE_VAR <out>
        [DEFAULT <PUBLIC|PRIVATE|INTERFACE>]
        [STRIP_VAR <out>]
    )

    Splits a visibility-tagged list into PUBLIC, PRIVATE, and INTERFACE lists.
    Untagged items use DEFAULT, which defaults to PUBLIC. STRIP_VAR receives all
    non-visibility items in their original order.

    Example:
        helios_parse_visibility(
            INPUT PUBLIC core PRIVATE platform
            PUBLIC_VAR public_deps
            PRIVATE_VAR private_deps
            INTERFACE_VAR interface_deps
            STRIP_VAR graph_deps
        )
]]
function(helios_parse_visibility)
  set(options "")
  set(oneValueArgs PUBLIC_VAR PRIVATE_VAR INTERFACE_VAR DEFAULT STRIP_VAR)
  set(multiValueArgs INPUT)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(ARG_DEFAULT)
    set(_current_visibility "${ARG_DEFAULT}")
  else()
    set(_current_visibility PUBLIC)
  endif()

  set(_public_items)
  set(_private_items)
  set(_interface_items)
  set(_stripped_items)

  foreach(_item IN LISTS ARG_INPUT)
    if(_item STREQUAL "PUBLIC" OR _item STREQUAL "PRIVATE" OR _item STREQUAL "INTERFACE")
      set(_current_visibility "${_item}")
      continue()
    endif()

    list(APPEND _stripped_items "${_item}")
    if(_current_visibility STREQUAL "PUBLIC")
      list(APPEND _public_items "${_item}")
    elseif(_current_visibility STREQUAL "PRIVATE")
      list(APPEND _private_items "${_item}")
    elseif(_current_visibility STREQUAL "INTERFACE")
      list(APPEND _interface_items "${_item}")
    else()
      message(FATAL_ERROR "helios_parse_visibility: invalid visibility '${_current_visibility}'")
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
  if(ARG_STRIP_VAR)
    set(${ARG_STRIP_VAR} "${_stripped_items}" PARENT_SCOPE)
  endif()
endfunction()

#[[
    helios_target_apply(
        TARGET <target>
        KIND LINK|DEFINITIONS|OPTIONS|INCLUDES
        ITEMS <visibility-tagged-items...>
        [DEFAULT <visibility>]
    )

    Applies visibility-tagged items to a target. INTERFACE libraries fold every
    visibility bucket to INTERFACE because CMake disallows PRIVATE/PUBLIC usage
    requirements on interface-only targets.

    Example:
        helios_target_apply(
            TARGET helios_module_core
            KIND DEFINITIONS
            ITEMS PUBLIC HELIOS_CORE PRIVATE HELIOS_CORE_IMPL
            DEFAULT PUBLIC
        )
]]
function(helios_target_apply)
  set(options "")
  set(oneValueArgs TARGET KIND DEFAULT)
  set(multiValueArgs ITEMS)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT ARG_TARGET)
    message(FATAL_ERROR "helios_target_apply: TARGET is required")
  endif()
  if(NOT ARG_KIND)
    message(FATAL_ERROR "helios_target_apply: KIND is required")
  endif()
  if(NOT TARGET ${ARG_TARGET})
    message(FATAL_ERROR "helios_target_apply: target '${ARG_TARGET}' does not exist")
  endif()
  if(NOT ARG_ITEMS)
    return()
  endif()

  if(ARG_DEFAULT)
    set(_default "${ARG_DEFAULT}")
  else()
    set(_default PUBLIC)
  endif()

  helios_parse_visibility(
      INPUT ${ARG_ITEMS}
      PUBLIC_VAR _public_items
      PRIVATE_VAR _private_items
      INTERFACE_VAR _interface_items
      DEFAULT ${_default}
  )

  get_target_property(_target_type ${ARG_TARGET} TYPE)
  if(_target_type STREQUAL "INTERFACE_LIBRARY")
    set(_interface_items ${_public_items} ${_private_items} ${_interface_items})
    set(_public_items)
    set(_private_items)
  endif()

  if(ARG_KIND STREQUAL "LINK")
    set(_command target_link_libraries)
  elseif(ARG_KIND STREQUAL "DEFINITIONS")
    set(_command target_compile_definitions)
  elseif(ARG_KIND STREQUAL "OPTIONS")
    set(_command target_compile_options)
  elseif(ARG_KIND STREQUAL "INCLUDES")
    set(_command target_include_directories)
  else()
    message(FATAL_ERROR "helios_target_apply: unknown KIND '${ARG_KIND}'")
  endif()

  if(_public_items)
    cmake_language(CALL ${_command} ${ARG_TARGET} PUBLIC ${_public_items})
  endif()
  if(_private_items)
    cmake_language(CALL ${_command} ${ARG_TARGET} PRIVATE ${_private_items})
  endif()
  if(_interface_items)
    cmake_language(CALL ${_command} ${ARG_TARGET} INTERFACE ${_interface_items})
  endif()
endfunction()

function(_helios_resolve_alias_target TARGET OUT_VAR)
  set(_actual "${TARGET}")
  if(TARGET ${TARGET})
    get_target_property(_aliased ${TARGET} ALIASED_TARGET)
    if(_aliased AND NOT _aliased MATCHES "-NOTFOUND$")
      set(_actual "${_aliased}")
    endif()
  endif()
  set(${OUT_VAR} "${_actual}" PARENT_SCOPE)
endfunction()

#[[
    helios_mark_system_includes(<target>...)

    Copies INTERFACE_INCLUDE_DIRECTORIES to INTERFACE_SYSTEM_INCLUDE_DIRECTORIES
    for each existing target. ALIAS targets are resolved before properties are
    read or written.

    Example:
        helios_mark_system_includes(spdlog::spdlog doctest::doctest)
]]
function(helios_mark_system_includes)
  foreach(_target IN LISTS ARGN)
    if(NOT TARGET ${_target})
      continue()
    endif()

    _helios_resolve_alias_target(${_target} _actual_target)
    get_target_property(_includes ${_actual_target} INTERFACE_INCLUDE_DIRECTORIES)
    if(_includes AND NOT _includes MATCHES "-NOTFOUND$")
      set_target_properties(${_actual_target} PROPERTIES
          INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_includes}"
      )
    endif()
  endforeach()
endfunction()

#[[
    helios_copy_shared_lib(CONSUMER <target> PROVIDER <target>)

    Adds a POST_BUILD copy_if_different command when PROVIDER is a shared
    library target. Non-target providers and non-shared targets are ignored.

    Example:
        helios_copy_shared_lib(CONSUMER game PROVIDER helios_module_window)
]]
function(helios_copy_shared_lib)
  set(options "")
  set(oneValueArgs CONSUMER PROVIDER)
  set(multiValueArgs "")
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT ARG_CONSUMER)
    message(FATAL_ERROR "helios_copy_shared_lib: CONSUMER is required")
  endif()
  if(NOT ARG_PROVIDER)
    message(FATAL_ERROR "helios_copy_shared_lib: PROVIDER is required")
  endif()
  if(NOT TARGET ${ARG_CONSUMER} OR NOT TARGET ${ARG_PROVIDER})
    return()
  endif()

  _helios_resolve_alias_target(${ARG_PROVIDER} _provider_target)
  get_target_property(_provider_type ${_provider_target} TYPE)
  if(NOT _provider_type STREQUAL "SHARED_LIBRARY")
    return()
  endif()

  add_custom_command(TARGET ${ARG_CONSUMER} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
          $<TARGET_FILE:${_provider_target}>
          $<TARGET_FILE_DIR:${ARG_CONSUMER}>
      COMMENT "Copying ${_provider_target} shared library"
      VERBATIM
  )
endfunction()
