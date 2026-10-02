# C API support for helios_module().

include_guard(GLOBAL)

# Appends C API sources when HELIOS_BUILD_C_API is on. Must run as a macro so
# MODULE_* variables in _helios_module_impl stay updated.
macro(_helios_module_prepare_c_api)
  set(_helios_c_api_active FALSE)
  if(HELIOS_BUILD_C_API AND (MODULE_C_SOURCES OR MODULE_C_HEADERS OR MODULE_C_TEST_SOURCES))
    set(_helios_c_api_active TRUE)
    if(NOT MODULE_C_STANDARD)
      set(MODULE_C_STANDARD 11)
    endif()
    if(MODULE_C_SOURCES)
      list(APPEND MODULE_SOURCES ${MODULE_C_SOURCES})
    endif()
    if(MODULE_C_HEADERS)
      list(APPEND MODULE_HEADERS ${MODULE_C_HEADERS})
    endif()
  endif()
endmacro()

function(_helios_link_with_cxx_usage TARGET VISIBILITY LIB)
  if(NOT TARGET ${LIB})
    return()
  endif()

  get_target_property(_type ${TARGET} TYPE)
  set(_visibility "${VISIBILITY}")
  if(_type STREQUAL "INTERFACE_LIBRARY" AND NOT _visibility STREQUAL "INTERFACE")
    set(_visibility INTERFACE)
  endif()

  if(_visibility STREQUAL "PRIVATE")
    target_link_libraries(${TARGET} PRIVATE ${LIB})
    return()
  endif()

  if(_visibility STREQUAL "PUBLIC" AND NOT _type STREQUAL "INTERFACE_LIBRARY")
    target_link_libraries(${TARGET} PRIVATE ${LIB})
    target_link_libraries(${TARGET} PUBLIC $<LINK_ONLY:${LIB}>)
    set(_usage PUBLIC)
  else()
    target_link_libraries(${TARGET} ${_visibility} $<LINK_ONLY:${LIB}>)
    set(_usage ${_visibility})
  endif()

  # Usage requirements stay on C++ compilations. $<LINK_ONLY> still links.
  target_include_directories(${TARGET} ${_usage}
      "$<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:${LIB},INTERFACE_INCLUDE_DIRECTORIES>>")
  target_compile_definitions(${TARGET} ${_usage}
      "$<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:${LIB},INTERFACE_COMPILE_DEFINITIONS>>")
  target_compile_options(${TARGET} ${_usage}
      "$<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:${LIB},INTERFACE_COMPILE_OPTIONS>>")
endfunction()

# Macro so MODULE_* and _helios_c_api_active from _helios_module_impl are visible.
# Do not return(): that would leave the caller.
macro(_helios_module_apply_c_api TARGET)
  if(_helios_c_api_active)
    get_target_property(_lib_type ${TARGET} TYPE)
    if(_lib_type STREQUAL "INTERFACE_LIBRARY")
      set(_c_scope INTERFACE)
    else()
      set(_c_scope PUBLIC)
      set_target_properties(${TARGET} PROPERTIES
          C_STANDARD ${MODULE_C_STANDARD}
          C_STANDARD_REQUIRED ON
          C_EXTENSIONS OFF
          C_VISIBILITY_PRESET hidden
          CXX_VISIBILITY_PRESET hidden
          VISIBILITY_INLINES_HIDDEN ON
      )
    endif()
    set_target_properties(${TARGET} PROPERTIES HELIOS_HAS_C_API TRUE)

    # Consumers of a static or header-only module see an empty HELIOS_C_API.
    # Implementation TUs keep HELIOS_C_BUILDING so a SHARED umbrella can
    # whole-archive these objects and export them. The macro is per source,
    # not target-wide: the module PCH is C++ and tests REUSE_FROM it.
    if(_lib_type STREQUAL "STATIC_LIBRARY" OR _lib_type STREQUAL "INTERFACE_LIBRARY")
      target_compile_definitions(${TARGET} INTERFACE HELIOS_C_STATIC)
    endif()

    target_include_directories(${TARGET} ${_c_scope}
        $<BUILD_INTERFACE:$<$<COMPILE_LANGUAGE:C,CXX>:${CMAKE_CURRENT_SOURCE_DIR}/capi/include>>
        $<BUILD_INTERFACE:$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/capi/src>>
        $<INSTALL_INTERFACE:$<$<COMPILE_LANGUAGE:C,CXX>:${CMAKE_INSTALL_INCLUDEDIR}>>
    )

    # C headers that still live under include/ (shared with C++) are mirrored
    # here. C compilations must not receive the C++ include tree.
    set(_c_mirror "${CMAKE_CURRENT_BINARY_DIR}/c_api_include")
    set(_mirrored FALSE)
    foreach(_header IN LISTS MODULE_C_HEADERS)
      if(_header MATCHES "^capi/include/" OR NOT _header MATCHES "^include/(.+)$")
        continue()
      endif()
      set(_rel "${CMAKE_MATCH_1}")
      if(IS_ABSOLUTE "${_header}")
        set(_src "${_header}")
      else()
        set(_src "${CMAKE_CURRENT_SOURCE_DIR}/${_header}")
      endif()
      configure_file("${_src}" "${_c_mirror}/${_rel}" COPYONLY)
      set(_mirrored TRUE)
    endforeach()
    if(_mirrored)
      target_include_directories(${TARGET} ${_c_scope}
          $<BUILD_INTERFACE:$<$<COMPILE_LANGUAGE:C>:${_c_mirror}>>
      )
    endif()


    if(HELIOS_ENABLE_INSTALL AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/capi/include")
      install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/capi/include/"
          DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
          FILES_MATCHING PATTERN "*.h"
      )
    endif()

    get_target_property(_sources ${TARGET} SOURCES)
    if(_sources)
      foreach(_src IN LISTS _sources)
        get_filename_component(_ext "${_src}" LAST_EXT)
        string(TOLOWER "${_ext}" _ext)
        if(_ext STREQUAL ".c" OR _ext STREQUAL ".cc" OR _ext STREQUAL ".cpp" OR _ext STREQUAL ".cxx")
          string(REPLACE "\\" "/" _norm "${_src}")
          if(_norm MATCHES "(^|/)capi/")
            set_property(SOURCE "${_src}" APPEND PROPERTY
                COMPILE_DEFINITIONS HELIOS_C_BUILDING)
            set_source_files_properties("${_src}" PROPERTIES
                SKIP_UNITY_BUILD_INCLUSION ON
                SKIP_PRECOMPILE_HEADERS ON
            )
          endif()
        endif()
      endforeach()
    endif()
  endif()
endmacro()

macro(_helios_module_add_c_tests TARGET)
  if(_helios_c_api_active AND HELIOS_BUILD_TESTS)
    string(TOUPPER "${MODULE_NAME}" _upper)
    set(_test_option_name "${_upper}_BUILD_TESTS")
    if(NOT DEFINED ${_test_option_name} OR ${_test_option_name})
      set(_sources)
      if(MODULE_C_TEST_SOURCES)
        foreach(_src IN LISTS MODULE_C_TEST_SOURCES)
          if(IS_ABSOLUTE "${_src}")
            list(APPEND _sources "${_src}")
          else()
            list(APPEND _sources "${CMAKE_CURRENT_SOURCE_DIR}/${_src}")
          endif()
        endforeach()
      elseif(MODULE_C_HEADERS)
        set(_generated "${CMAKE_CURRENT_BINARY_DIR}/capi_smoke.c")
        set(_available_def "${${_upper}_AVAILABLE_DEF}")
        if(NOT _available_def)
          string(TOUPPER "${TARGET}" _target_upper)
          set(_available_def "${_target_upper}_AVAILABLE")
        endif()
        set(_body "#ifdef ${_available_def}\n#error \"C translation unit received ${_available_def}\"\n#endif\n")
        foreach(_header IN LISTS MODULE_C_HEADERS)
          if(_header MATCHES "^(capi/)?include/(.+)$")
            string(APPEND _body "#include <${CMAKE_MATCH_2}>\n")
          endif()
        endforeach()
        string(APPEND _body "int main(void) { return 0; }\n")
        file(WRITE "${_generated}" "${_body}")
        set(_sources "${_generated}")
      endif()

      if(_sources)
        set(_test_target "${TARGET}_c_tests")
        add_executable(${_test_target} ${_sources})
        helios_get_module_link_target(${MODULE_NAME} _link_target)
        target_link_libraries(${_test_target} PRIVATE ${_link_target})
        set_target_properties(${_test_target} PROPERTIES
            C_STANDARD ${MODULE_C_STANDARD}
            C_STANDARD_REQUIRED ON
            C_EXTENSIONS OFF
            LINKER_LANGUAGE CXX
        )
        helios_apply_conventions(${_test_target})
        helios_target_set_output_dirs(${_test_target})
        helios_target_set_folder(${_test_target} "Helios/Tests")
        add_test(NAME "${_test_target}" COMMAND ${_test_target})
      endif()
    endif()
  endif()
endmacro()
