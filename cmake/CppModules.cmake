# Helios C++20 named modules
#
# Configure-time support for HELIOS_ENABLE_CPP_MODULES / HELIOS_ENABLE_IMPORT_STD.
# helios_cpp_modules_set_experimental_gate() must run before project().

include_guard(GLOBAL)

#[[
    helios_cpp_modules_import_std_uuid() -> sets HELIOS_CXX_IMPORT_STD_UUID

    Maps CMAKE_VERSION to CMAKE_EXPERIMENTAL_CXX_IMPORT_STD. UUIDs change when
    CMake revises the experimental import-std feature.
]]
function(helios_cpp_modules_import_std_uuid)
  set(_uuid "")
  if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.4.0")
    set(_uuid "f35a9ac6-8463-4d38-8eec-5d6008153e7d")
  elseif(CMAKE_VERSION VERSION_GREATER_EQUAL "4.3.0")
    set(_uuid "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")
  elseif(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0.3")
    set(_uuid "d0edc3af-4c50-42ea-a356-e2862fe7a444")
  elseif(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0.0")
    set(_uuid "a9e1cf81-9932-4810-974b-6eccaf14e457")
  elseif(CMAKE_VERSION VERSION_GREATER_EQUAL "3.31.8")
    set(_uuid "d0edc3af-4c50-42ea-a356-e2862fe7a444")
  elseif(CMAKE_VERSION VERSION_GREATER_EQUAL "3.30.0")
    set(_uuid "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")
  endif()
  set(HELIOS_CXX_IMPORT_STD_UUID "${_uuid}" PARENT_SCOPE)
endfunction()

#[[
    helios_cpp_modules_set_experimental_gate()

    Enables CMake experimental import std support. Call before project().
]]
macro(helios_cpp_modules_set_experimental_gate)
  if(HELIOS_ENABLE_CPP_MODULES AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.30")
    helios_cpp_modules_import_std_uuid()
    if(HELIOS_CXX_IMPORT_STD_UUID)
      set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "${HELIOS_CXX_IMPORT_STD_UUID}")
    endif()
  endif()
endmacro()

function(_helios_cpp_modules_try_import_std OUT_VAR)
  set(_ok FALSE)
  if(DEFINED CMAKE_CXX_COMPILER_IMPORT_STD)
    if("23" IN_LIST CMAKE_CXX_COMPILER_IMPORT_STD OR
       "26" IN_LIST CMAKE_CXX_COMPILER_IMPORT_STD)
      set(_ok TRUE)
    endif()
  endif()

  if(_ok)
    set(_src "${CMAKE_BINARY_DIR}/helios_check_import_std.cpp")
    file(WRITE "${_src}" "import std;\nint main() { return 0; }\n")
    try_compile(_compiled
        "${CMAKE_BINARY_DIR}/helios_check_import_std"
        "${_src}"
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        OUTPUT_VARIABLE _import_std_output
    )
    if(NOT _compiled)
      set(_ok FALSE)
      message(STATUS "import std try_compile failed:\n${_import_std_output}")
    endif()
  endif()

  set(${OUT_VAR} ${_ok} PARENT_SCOPE)
endfunction()

#[[
    helios_cpp_modules_configure()

    Validates generator, ignores unity builds, probes import std, and disables
    default module scanning so third-party targets are not scanned.
]]
function(helios_cpp_modules_configure)
  if(NOT HELIOS_ENABLE_CPP_MODULES)
    return()
  endif()

  if(CMAKE_GENERATOR MATCHES "Makefiles" AND NOT CMAKE_GENERATOR MATCHES "Ninja")
    message(FATAL_ERROR
        "HELIOS_ENABLE_CPP_MODULES requires Ninja or Visual Studio 17.4+. "
        "Current generator: ${CMAKE_GENERATOR}")
  endif()

  if(HELIOS_ENABLE_UNITY_BUILD)
    message(WARNING
        "HELIOS_ENABLE_UNITY_BUILD is ignored when HELIOS_ENABLE_CPP_MODULES is ON")
    set(CMAKE_UNITY_BUILD OFF PARENT_SCOPE)
  endif()

  set(CMAKE_CXX_SCAN_FOR_MODULES OFF PARENT_SCOPE)

  _helios_cpp_modules_try_import_std(_import_std_ok)

  if(NOT DEFINED HELIOS_ENABLE_IMPORT_STD)
    set(HELIOS_ENABLE_IMPORT_STD ${_import_std_ok} CACHE BOOL
        "Use C++23 import std in Helios named modules")
  endif()

  if(HELIOS_ENABLE_IMPORT_STD AND NOT _import_std_ok)
    message(FATAL_ERROR
        "HELIOS_ENABLE_IMPORT_STD=ON but this toolchain cannot compile `import std;`. "
        "Use CMake 3.30+, a compiler with std module support, or set "
        "HELIOS_ENABLE_IMPORT_STD=OFF.")
  endif()

  if(HELIOS_ENABLE_IMPORT_STD)
    set(CMAKE_CXX_MODULE_STD ON PARENT_SCOPE)
  endif()

  message(STATUS "C++20 named modules: ON (import std: ${HELIOS_ENABLE_IMPORT_STD})")
endfunction()

#[[
    helios_target_enable_cxx_modules(<target> <module-sources...>)

    Attaches named-module interface units from MODULE_SOURCES and enables
    scanning / import-std on the target.
]]
function(helios_target_enable_cxx_modules TARGET)
  if(NOT HELIOS_ENABLE_CPP_MODULES)
    return()
  endif()

  set(_files ${ARGN})
  if(NOT _files)
    return()
  endif()

  set(_abs_files)
  set(_base_dirs)
  foreach(_file IN LISTS _files)
    if(IS_ABSOLUTE "${_file}")
      set(_abs "${_file}")
    else()
      set(_abs "${CMAKE_CURRENT_SOURCE_DIR}/${_file}")
    endif()
    list(APPEND _abs_files "${_abs}")
    get_filename_component(_dir "${_abs}" DIRECTORY)
    if(_dir)
      list(APPEND _base_dirs "${_dir}")
    else()
      list(APPEND _base_dirs "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()
  endforeach()
  list(REMOVE_DUPLICATES _base_dirs)

  target_sources(${TARGET} PUBLIC
      FILE_SET CXX_MODULES
      TYPE CXX_MODULES
      BASE_DIRS ${_base_dirs}
      FILES ${_abs_files}
  )
  get_target_property(_helios_name ${TARGET} HELIOS_MODULE_NAME)
  set(_building_defs "HELIOS_BUILDING_MODULE")
  if(_helios_name AND NOT _helios_name STREQUAL "_helios_name-NOTFOUND")
    string(TOUPPER "${_helios_name}" _helios_name_upper)
    list(APPEND _building_defs "HELIOS_BUILDING_MODULE_${_helios_name_upper}")
  endif()
  set_source_files_properties(${_abs_files} PROPERTIES
      COMPILE_DEFINITIONS "${_building_defs}"
  )
  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # export extern "C++" { #include ... } is the intended dual-header pattern.
    # /Zc:preprocessor must be on the source: CMake synth BMI TUs do not
    # inherit the target's PRIVATE /Zc:preprocessor, and the traditional
    # preprocessor breaks empty `#__VA_ARGS__` in HELIOS_ASSERT (C2760).
    set_property(SOURCE ${_abs_files} APPEND PROPERTY
        COMPILE_OPTIONS /wd5244 /Zc:preprocessor)
  endif()
  set_target_properties(${TARGET} PROPERTIES
      CXX_SCAN_FOR_MODULES ON
      HELIOS_HAS_NAMED_MODULE TRUE
  )
  if(HELIOS_ENABLE_IMPORT_STD)
    set_target_properties(${TARGET} PROPERTIES CXX_MODULE_STD ON)
  endif()

  target_compile_definitions(${TARGET} PUBLIC HELIOS_ENABLE_CPP_MODULES)
  if(HELIOS_ENABLE_IMPORT_STD)
    target_compile_definitions(${TARGET} PUBLIC HELIOS_ENABLE_IMPORT_STD)
  endif()
endfunction()

#[[
    helios_target_consume_cxx_modules(<target>)

    Marks a consumer of Helios named modules (tests, examples, apps). Does
    **not** enable target-wide module scanning: MSVC injects `import` of
    linked BMIs into every scanned TU, which then breaks `#include` of
    standard headers (incomplete `ostream`, C2572).

    TUs that actually `import` must opt in with
    `helios_source_scan_for_cxx_modules()`. Header-only TUs stay unscanned.
]]
function(helios_target_consume_cxx_modules TARGET)
  if(NOT HELIOS_ENABLE_CPP_MODULES)
    return()
  endif()
  if(NOT TARGET ${TARGET})
    return()
  endif()
  if(HELIOS_ENABLE_IMPORT_STD)
    set_target_properties(${TARGET} PROPERTIES CXX_MODULE_STD ON)
  endif()
endfunction()

#[[
    helios_source_scan_for_cxx_modules(<target> <sources...>)

    Enables C++ module scanning on specific sources so they can `import`.
    Paths may be relative to CMAKE_CURRENT_SOURCE_DIR.
]]
function(helios_source_scan_for_cxx_modules TARGET)
  if(NOT HELIOS_ENABLE_CPP_MODULES)
    return()
  endif()
  if(NOT TARGET ${TARGET})
    return()
  endif()
  set(_abs)
  foreach(_file IN LISTS ARGN)
    if(IS_ABSOLUTE "${_file}")
      list(APPEND _abs "${_file}")
    else()
      list(APPEND _abs "${CMAKE_CURRENT_SOURCE_DIR}/${_file}")
    endif()
  endforeach()
  if(_abs)
    set_source_files_properties(${_abs} PROPERTIES
        CXX_SCAN_FOR_MODULES ON
        SKIP_PRECOMPILE_HEADERS ON)
  endif()
endfunction()

#[[
    helios_add_umbrella_cxx_module()

    Generates export module helios; re-exporting every enabled named module.
]]
function(helios_add_umbrella_cxx_module)
  if(NOT HELIOS_ENABLE_CPP_MODULES)
    return()
  endif()

  get_property(_targets GLOBAL PROPERTY HELIOS_MODULES)
  if(NOT _targets)
    return()
  endif()

  set(_imports)
  set(_link_libs)
  foreach(_target IN LISTS _targets)
    if(NOT TARGET ${_target})
      continue()
    endif()
    get_target_property(_name ${_target} HELIOS_MODULE_NAME)
    if(NOT _name OR _name STREQUAL "_name-NOTFOUND")
      continue()
    endif()
    if(_name STREQUAL "helios")
      continue()
    endif()
    get_target_property(_has_named ${_target} HELIOS_HAS_NAMED_MODULE)
    if(NOT _has_named)
      continue()
    endif()
    get_target_property(_cxx_name ${_target} HELIOS_CXX_MODULE_NAME)
    if(NOT _cxx_name OR _cxx_name STREQUAL "_cxx_name-NOTFOUND")
      string(REPLACE "_" "." _cxx_name "${_name}")
      set(_cxx_name "helios.${_cxx_name}")
    endif()
    string(APPEND _imports "export import ${_cxx_name};\n")
    list(APPEND _link_libs helios::module::${_name})
  endforeach()

  set(_cppm "${CMAKE_BINARY_DIR}/helios.cppm")
  file(WRITE "${_cppm}" "export module helios;\n${_imports}")

  add_library(helios_module_helios STATIC)
  target_sources(helios_module_helios PUBLIC
      FILE_SET CXX_MODULES
      TYPE CXX_MODULES
      BASE_DIRS "${CMAKE_BINARY_DIR}"
      FILES "${_cppm}"
  )
  set_target_properties(helios_module_helios PROPERTIES
      EXPORT_NAME "helios"
      HELIOS_MODULE_NAME "helios"
      CXX_SCAN_FOR_MODULES ON
      CXX_STANDARD 23
      CXX_STANDARD_REQUIRED ON
      CXX_EXTENSIONS OFF
      FOLDER "Helios/Modules"
  )
  if(HELIOS_ENABLE_IMPORT_STD)
    set_target_properties(helios_module_helios PROPERTIES CXX_MODULE_STD ON)
  endif()
  target_compile_definitions(helios_module_helios PUBLIC HELIOS_ENABLE_CPP_MODULES)
  if(HELIOS_ENABLE_IMPORT_STD)
    target_compile_definitions(helios_module_helios PUBLIC HELIOS_ENABLE_IMPORT_STD)
  endif()
  if(_link_libs)
    target_link_libraries(helios_module_helios PUBLIC ${_link_libs})
  endif()
  add_library(helios::helios ALIAS helios_module_helios)

  if(HELIOS_ENABLE_INSTALL)
    install(TARGETS helios_module_helios
        EXPORT HeliosTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        FILE_SET CXX_MODULES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/helios/modules
    )
  endif()

  message(STATUS "Helios Module: helios (umbrella named module)")
endfunction()
