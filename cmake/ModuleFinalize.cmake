# Applies manifest-derived usage requirements after a module directory is processed.

include_guard(GLOBAL)

include(TargetUtils)
include(CppModules)

function(helios_header_only_library NAME)
  cmake_parse_arguments(ARG "" "CPP_MODULE" "" ${ARGN})
  if(HELIOS_ENABLE_CPP_MODULES AND ARG_CPP_MODULE)
    add_library(${NAME} STATIC)
    set(HELIOS_CURRENT_MODULE_SCOPE PUBLIC PARENT_SCOPE)
    set_target_properties(${NAME} PROPERTIES HELIOS_HEADER_ONLY TRUE)
    helios_target_cpp_module_sources(${NAME} ${ARG_CPP_MODULE})
  else()
    add_library(${NAME} INTERFACE)
    set(HELIOS_CURRENT_MODULE_SCOPE INTERFACE PARENT_SCOPE)
    set_target_properties(${NAME} PROPERTIES HELIOS_HEADER_ONLY TRUE)
  endif()
endfunction()

function(helios_target_precompile_headers TARGET PCH_FILE)
  if(NOT IS_ABSOLUTE "${PCH_FILE}")
    set(PCH_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${PCH_FILE}")
  endif()
  helios_target_add_pch(${TARGET} "${PCH_FILE}")
  set_target_properties(${TARGET} PROPERTIES HELIOS_HAS_PCH TRUE)
endfunction()

function(helios_target_cpp_module_sources TARGET)
  helios_target_enable_cxx_modules(${TARGET} ${ARGN})
  if(HELIOS_ENABLE_CPP_MODULES)
    set(_abs)
    foreach(_file IN LISTS ARGN)
      if(IS_ABSOLUTE "${_file}")
        list(APPEND _abs "${_file}")
      else()
        list(APPEND _abs "${CMAKE_CURRENT_SOURCE_DIR}/${_file}")
      endif()
    endforeach()
    if(_abs)
      set_source_files_properties(${_abs} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
    endif()
  endif()
endfunction()

# C API implementation units need HELIOS_C_BUILDING so their declarations
# export. A target-wide definition is baked into the C++ PCH, and test
# targets that REUSE_FROM that PCH then export the C API too. These sources
# are C and C++ (capi/src/*.cpp), so a COMPILE_LANGUAGE:C genex misses them.
function(_helios_mark_c_api_sources TARGET)
  get_target_property(_sources ${TARGET} SOURCES)
  if(NOT _sources OR _sources STREQUAL "_sources-NOTFOUND")
    return()
  endif()
  foreach(_src IN LISTS _sources)
    string(REPLACE "\\" "/" _norm "${_src}")
    if(NOT _norm MATCHES "(^|/)capi/")
      continue()
    endif()
    get_filename_component(_ext "${_src}" LAST_EXT)
    string(TOLOWER "${_ext}" _ext)
    if(NOT _ext MATCHES "^\\.(c|cc|cpp|cxx)$")
      continue()
    endif()
    set_property(SOURCE "${_src}" TARGET_DIRECTORY "${TARGET}"
        APPEND PROPERTY COMPILE_DEFINITIONS HELIOS_C_BUILDING)
    set_property(SOURCE "${_src}" TARGET_DIRECTORY "${TARGET}"
        PROPERTY SKIP_PRECOMPILE_HEADERS ON)
    set_property(SOURCE "${_src}" TARGET_DIRECTORY "${TARGET}"
        PROPERTY SKIP_UNITY_BUILD_INCLUSION ON)
  endforeach()
endfunction()

function(_helios_module_scope TARGET OUT)
  get_target_property(_type ${TARGET} TYPE)
  if(_type STREQUAL "INTERFACE_LIBRARY")
    set(${OUT} INTERFACE PARENT_SCOPE)
  else()
    set(${OUT} PUBLIC PARENT_SCOPE)
  endif()
endfunction()

# Imported name for a module that is not built in this configure.
# Prefers the alias recorded for an installed package, then the
# name-derived default (helios_app -> helios::app, helios_sdl3_window -> helios::sdl3::window).
function(_helios_imported_module_target NAME OUT)
  string(TOUPPER "${NAME}" _upper)
  if(DEFINED ${_upper}_ALIAS AND NOT "${${_upper}_ALIAS}" STREQUAL "")
    set(${OUT} "${${_upper}_ALIAS}" PARENT_SCOPE)
    return()
  endif()
  if("${NAME}" MATCHES "^([^_]+)_(.+)$")
    string(REPLACE "_" "::" _rest "${CMAKE_MATCH_2}")
    set(${OUT} "${CMAKE_MATCH_1}::${_rest}" PARENT_SCOPE)
    return()
  endif()
  set(${OUT} "" PARENT_SCOPE)
endfunction()

function(_helios_link_module_dep TARGET DEP VISIBILITY)
  if(NOT DEP)
    return()
  endif()
  helios_module_enabled(${DEP} _on)
  if(_on)
    helios_get_module_link_target(${DEP} _link)
  else()
    _helios_imported_module_target(${DEP} _link)
    if(NOT _link OR NOT TARGET "${_link}")
      return()
    endif()
  endif()
  if(NOT TARGET ${_link})
    return()
  endif()
  _helios_module_scope(${TARGET} _scope)
  if(_scope STREQUAL "INTERFACE")
    set(VISIBILITY INTERFACE)
  endif()
  target_link_libraries(${TARGET} ${VISIBILITY} ${_link})
  string(TOUPPER "${DEP}" _dep_upper)
  set(_def "${${_dep_upper}_AVAILABLE_DEF}")
  if(_def)
    target_compile_definitions(${TARGET} ${VISIBILITY}
        "$<$<COMPILE_LANGUAGE:CXX>:${_def}>")
  endif()
endfunction()

function(_helios_finalize_checks TARGET MODULE_PATH C_API IN_TREE)
  if(NOT TARGET ${TARGET})
    get_directory_property(_created DIRECTORY "${MODULE_PATH}" BUILDSYSTEM_TARGETS)
    message(FATAL_ERROR
        "Expected target '${TARGET}' from module.toml, but ${MODULE_PATH}/CMakeLists.txt created: ${_created}. "
        "Rename the target to match module.name or fix module.toml name.")
  endif()

  get_target_property(_sources ${TARGET} SOURCES)
  if(NOT _sources OR _sources STREQUAL "_sources-NOTFOUND")
    set(_sources "")
  endif()
  set(_capi_sources FALSE)
  foreach(_src IN LISTS _sources)
    string(REPLACE "\\" "/" _norm "${_src}")
    if(_norm MATCHES "/capi/" OR _norm MATCHES "^capi/")
      set(_capi_sources TRUE)
    endif()
  endforeach()
  get_target_property(_header_sets ${TARGET} HEADER_SETS)
  get_target_property(_iface_sets ${TARGET} INTERFACE_HEADER_SETS)
  if(NOT _header_sets OR _header_sets STREQUAL "_header_sets-NOTFOUND")
    set(_header_sets "")
  endif()
  if(_iface_sets AND NOT _iface_sets STREQUAL "_iface_sets-NOTFOUND")
    list(APPEND _header_sets ${_iface_sets})
    list(REMOVE_DUPLICATES _header_sets)
  endif()
  set(_capi_headers FALSE)
  set(_bad_base FALSE)
  if(_header_sets AND NOT _header_sets STREQUAL "_header_sets-NOTFOUND")
    foreach(_set IN LISTS _header_sets)
      get_target_property(_dirs ${TARGET} HEADER_DIRS_${_set})
      foreach(_dir IN LISTS _dirs)
        string(REPLACE "\\" "/" _norm "${_dir}")
        if(_norm MATCHES "/capi/include$")
          set(_capi_headers TRUE)
        endif()
        if(NOT _norm MATCHES "/include$" AND NOT _norm MATCHES "/capi/include$")
          set(_bad_base TRUE)
        endif()
      endforeach()
    endforeach()
  endif()

  if(NOT C_API AND (_capi_sources OR _capi_headers))
    message(FATAL_ERROR
        "${TARGET}: sources under capi/ require c_api = true in module.toml")
  endif()
  if(C_API AND HELIOS_BUILD_C_API AND NOT _capi_headers)
    message(FATAL_ERROR
        "${TARGET}: c_api = true but no header set rooted at capi/include")
  endif()
  if(C_API AND NOT HELIOS_BUILD_C_API AND (_capi_sources OR _capi_headers))
    message(FATAL_ERROR
        "${TARGET}: capi/ sources are on the target while HELIOS_BUILD_C_API is OFF. "
        "Gate them with if(HELIOS_BUILD_C_API).")
  endif()
  if(_bad_base)
    message(FATAL_ERROR
        "${TARGET}: HEADERS file sets must use BASE_DIRS include/ or capi/include/")
  endif()

  if(IN_TREE)
    get_target_property(_type ${TARGET} TYPE)
    get_target_property(_header_only ${TARGET} HELIOS_HEADER_ONLY)
    get_target_property(_applied ${TARGET} HELIOS_CONVENTIONS_APPLIED)
    if(NOT _type STREQUAL "INTERFACE_LIBRARY" AND NOT _header_only AND NOT _applied)
      set(_msg "${TARGET}: in-tree module did not call helios_apply_conventions()")
      if(HELIOS_DEVELOPER_MODE OR DEFINED ENV{CI})
        message(FATAL_ERROR "${_msg}")
      else()
        message(WARNING "${_msg}")
      endif()
    endif()
  endif()
endfunction()

function(_helios_finalize_module NAME)
  string(TOUPPER "${NAME}" _upper)
  set(_target "${${_upper}_TARGET}")
  set(_path "${${_upper}_PATH}")
  set(_alias "${${_upper}_ALIAS}")
  set(_origin "${${_upper}_ORIGIN}")
  set(_c_api OFF)
  if(${_upper}_C_API STREQUAL "ON")
    set(_c_api ON)
  endif()

  set(_in_tree FALSE)
  set(_src_root "${HELIOS_ROOT_DIR}/src")
  cmake_path(IS_PREFIX _src_root "${_path}" NORMALIZE _in_src)
  if(_in_src)
    set(_in_tree TRUE)
  endif()

  if(NOT TARGET ${_target})
    _helios_finalize_checks("${_target}" "${_path}" ${_c_api} ${_in_tree})
  endif()

  if(_alias)
    if(TARGET ${_alias})
      message(FATAL_ERROR
          "Alias '${_alias}' is already a target. Rename module '${NAME}' or the conflicting target.")
    endif()
    add_library(${_alias} ALIAS ${_target})
    # install(EXPORT NAMESPACE helios::) prepends that prefix. The remainder of
    # the alias is the export name, so helios_app installs as helios::app and
    # helios_sdl3_window installs as helios::sdl3::window.
    if(_alias MATCHES "^[^:]+::(.+)$")
      set_target_properties(${_target} PROPERTIES EXPORT_NAME "${CMAKE_MATCH_1}")
    endif()
  endif()

  _helios_module_scope(${_target} _scope)
  set(_version "${${_upper}_VERSION}")
  if(NOT _scope STREQUAL "INTERFACE" AND _version)
    string(REGEX MATCH "^[0-9]+" _major "${_version}")
    set_target_properties(${_target} PROPERTIES
        VERSION "${_version}"
        SOVERSION "${_major}"
    )
  endif()
  set_target_properties(${_target} PROPERTIES
      HELIOS_MODULE_NAME "${NAME}"
      HELIOS_MODULE_VERSION "${_version}"
      HELIOS_MODULE_DESCRIPTION "${${_upper}_DESCRIPTION}"
      HELIOS_MODULE_LANGUAGES "${${_upper}_LANGUAGES}"
  )

  string(TOUPPER "${_target}" _target_upper)
  target_compile_definitions(${_target} ${_scope}
      "$<$<COMPILE_LANGUAGE:CXX>:${_target_upper}_AVAILABLE>")

  foreach(_dep IN LISTS ${_upper}_DEPENDS_PUBLIC)
    _helios_link_module_dep(${_target} ${_dep} PUBLIC)
  endforeach()
  foreach(_dep IN LISTS ${_upper}_DEPENDS_PRIVATE)
    _helios_link_module_dep(${_target} ${_dep} PRIVATE)
  endforeach()
  foreach(_dep IN LISTS ${_upper}_OPTIONAL_PUBLIC)
    _helios_link_module_dep(${_target} ${_dep} PUBLIC)
  endforeach()
  foreach(_dep IN LISTS ${_upper}_OPTIONAL_PRIVATE)
    _helios_link_module_dep(${_target} ${_dep} PRIVATE)
  endforeach()

  if(EXISTS "${_path}/include")
    target_include_directories(${_target} ${_scope}
        $<BUILD_INTERFACE:${_path}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
  endif()
  if(NOT _scope STREQUAL "INTERFACE" AND EXISTS "${_path}/src")
    target_include_directories(${_target} PRIVATE "${_path}/src")
  endif()
  if(EXISTS "${CMAKE_BINARY_DIR}/generated")
    target_include_directories(${_target} ${_scope}
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/generated>
    )
  endif()

  get_target_property(_conventions ${_target} HELIOS_CONVENTIONS_APPLIED)
  if(NOT _scope STREQUAL "INTERFACE" AND NOT _conventions)
    helios_target_set_platform(${_target})
  endif()

  get_property(_features GLOBAL PROPERTY HELIOS_FEATURES_${_upper})
  foreach(_feature IN LISTS _features)
    string(REPLACE "|" ";" _parts "${_feature}")
    list(GET _parts 0 _option)
    if(${_option})
      target_compile_definitions(${_target} ${_scope} ${_option})
    endif()
  endforeach()

  foreach(_tag IN LISTS ${_upper}_BACKEND_WINS)
    string(TOUPPER "${_tag}" _tag_upper)
    set(HELIOS_BACKEND_${_tag_upper}_TARGET "${_target}" CACHE INTERNAL "")
    target_compile_definitions(${_target} ${_scope}
        HELIOS_${_tag_upper}_BACKEND_${_upper})
  endforeach()

  if(_c_api AND HELIOS_BUILD_C_API AND EXISTS "${_path}/capi/include")
    set(_c_scope ${_scope})
    target_include_directories(${_target} ${_c_scope}
        $<BUILD_INTERFACE:${_path}/capi/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
    if(NOT _scope STREQUAL "INTERFACE")
      _helios_mark_c_api_sources(${_target})
      get_target_property(_type ${_target} TYPE)
      if(_type STREQUAL "STATIC_LIBRARY")
        target_compile_definitions(${_target} INTERFACE HELIOS_C_STATIC)
      endif()
    else()
      target_compile_definitions(${_target} INTERFACE HELIOS_C_STATIC)
    endif()
  endif()

  if(HELIOS_ENABLE_INSTALL AND _origin STREQUAL "engine")
    set(_install_args
        TARGETS ${_target}
        EXPORT HeliosTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    get_target_property(_hsets ${_target} HEADER_SETS)
    get_target_property(_iface_sets ${_target} INTERFACE_HEADER_SETS)
    if(NOT _hsets OR _hsets STREQUAL "_hsets-NOTFOUND")
      set(_hsets "")
    endif()
    if(_iface_sets AND NOT _iface_sets STREQUAL "_iface_sets-NOTFOUND")
      list(APPEND _hsets ${_iface_sets})
    endif()
    if(_hsets AND NOT _hsets STREQUAL "_hsets-NOTFOUND")
      list(REMOVE_DUPLICATES _hsets)
      foreach(_set IN LISTS _hsets)
        list(APPEND _install_args FILE_SET ${_set} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
      endforeach()
    elseif(EXISTS "${_path}/include")
      install(DIRECTORY "${_path}/include/" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    endif()
    get_target_property(_has_modules ${_target} HELIOS_HAS_NAMED_MODULE)
    if(_has_modules)
      list(APPEND _install_args FILE_SET CXX_MODULES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/helios/modules)
    endif()
    install(${_install_args})
    if(_c_api AND HELIOS_BUILD_C_API AND EXISTS "${_path}/capi/include")
      install(DIRECTORY "${_path}/capi/include/" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
          FILES_MATCHING PATTERN "*.h")
    endif()
  elseif(HELIOS_EXTRA_MODULES_EXPORT AND _origin STREQUAL "user")
    install(TARGETS ${_target} EXPORT ${HELIOS_EXTRA_MODULES_EXPORT}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()

  _helios_finalize_checks(${_target} "${_path}" ${_c_api} ${_in_tree})
  set_property(GLOBAL APPEND PROPERTY HELIOS_MODULES "${NAME}")
endfunction()

function(helios_build_declared_modules)
  if(NOT HELIOS_ENABLED_MODULES)
    message(STATUS "No Helios modules enabled")
    return()
  endif()
  foreach(_name IN LISTS HELIOS_ENABLED_MODULES)
    string(TOUPPER "${_name}" _upper)
    set(_path "${${_upper}_PATH}")
    set(HELIOS_CURRENT_MODULE "${_name}")
    set(HELIOS_CURRENT_MODULE_TARGET "${${_upper}_TARGET}")
    if(HELIOS_BUILD_TESTS AND ${_upper}_BUILD_TESTS)
      set(HELIOS_CURRENT_MODULE_BUILD_TESTS ON)
    else()
      set(HELIOS_CURRENT_MODULE_BUILD_TESTS OFF)
    endif()
    set(_inside FALSE)
    if(HELIOS_ROOT_DIR)
      set(_engine_root "${HELIOS_ROOT_DIR}")
      cmake_path(IS_PREFIX _engine_root "${_path}" NORMALIZE _inside)
    endif()
    message(STATUS "Building module: ${_name}")
    if(_inside)
      add_subdirectory("${_path}")
    else()
      add_subdirectory("${_path}" "${CMAKE_BINARY_DIR}/helios-modules/${_name}")
    endif()
    _helios_finalize_module(${_name})
  endforeach()
endfunction()

function(helios_add_modules)
  cmake_parse_arguments(ARG "" "EXPORT" "DIRS" ${ARGN})
  if(NOT ARG_DIRS)
    message(FATAL_ERROR "helios_add_modules: DIRS is required")
  endif()
  find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)
  set(_graph "${Helios_DIR}/../../../share/helios/modules.json")
  cmake_path(ABSOLUTE_PATH _graph NORMALIZE)
  if(NOT EXISTS "${_graph}")
    message(FATAL_ERROR "helios_add_modules: installed module graph not found at ${_graph}")
  endif()
  set(_cmake_out "${CMAKE_BINARY_DIR}/helios/UserModules.cmake")
  set(_json_out "${CMAKE_BINARY_DIR}/helios/user-modules.json")
  set(_cmd
      "${Python3_EXECUTABLE}"
      "${Helios_DIR}/../../../share/helios/tools/helios_modules.py"
      resolve
      --installed-graph "${_graph}"
      --cmake-out "${_cmake_out}"
      --json-out "${_json_out}"
      --user-only
  )
  foreach(_dir IN LISTS ARG_DIRS)
    list(APPEND _cmd --extra-dir "${_dir}")
  endforeach()
  execute_process(COMMAND ${_cmd} RESULT_VARIABLE _rc ERROR_VARIABLE _err OUTPUT_VARIABLE _out)
  if(NOT _rc EQUAL 0)
    message(FATAL_ERROR "helios_modules failed: ${_err}\n${_out}")
  endif()
  if(ARG_EXPORT)
    set(HELIOS_EXTRA_MODULES_EXPORT "${ARG_EXPORT}" CACHE INTERNAL
        "Export set for user modules added with helios_add_modules()")
  endif()
  include("${_cmake_out}")
  helios_build_declared_modules()
endfunction()
