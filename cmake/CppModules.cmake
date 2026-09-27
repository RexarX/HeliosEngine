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

    Creates helios::helios (output name helios), linking every enabled module.
    When HELIOS_ENABLE_CPP_MODULES is on, the same target owns export module helios.
]]
function(helios_add_umbrella_cxx_module)
  if(TARGET helios_module_helios)
    return()
  endif()

  helios_get_enabled_modules(_modules)

  set(_build_type "AUTO")
  if(DEFINED HELIOS_BUILD_OPTION_HELIOS AND NOT HELIOS_BUILD_OPTION_HELIOS STREQUAL "")
    set(_build_type "${HELIOS_BUILD_OPTION_HELIOS}")
  endif()
  set(HELIOS_BUILD_OPTION_HELIOS "${_build_type}" CACHE STRING
      "Library type for the helios umbrella (AUTO, STATIC, SHARED)")
  set_property(CACHE HELIOS_BUILD_OPTION_HELIOS PROPERTY STRINGS "AUTO" "STATIC" "SHARED")

  set(_library_type)
  if(HELIOS_BUILD_OPTION_HELIOS STREQUAL "STATIC")
    set(_library_type STATIC)
  elseif(HELIOS_BUILD_OPTION_HELIOS STREQUAL "SHARED")
    set(_library_type SHARED)
  endif()

  set(_anchor "${CMAKE_BINARY_DIR}/helios_umbrella.cpp")
  file(WRITE "${_anchor}"
      "namespace helios::details {\nvoid HeliosUmbrellaAnchor() {}\n}\n")

  if(_library_type)
    add_library(helios_module_helios ${_library_type} "${_anchor}")
  else()
    add_library(helios_module_helios "${_anchor}")
  endif()

  set(_imports)
  set(_has_named_module FALSE)
  foreach(_name IN LISTS _modules)
    if(_name STREQUAL "helios")
      continue()
    endif()
    if(NOT TARGET helios::module::${_name})
      continue()
    endif()
    set(_mod helios::module::${_name})
    get_target_property(_mod_type helios_module_${_name} TYPE)

    if(HELIOS_BUILD_OPTION_HELIOS STREQUAL "SHARED" AND _mod_type STREQUAL "STATIC_LIBRARY")
      target_link_libraries(helios_module_helios PRIVATE
          $<LINK_LIBRARY:WHOLE_ARCHIVE,${_mod}>)
      target_include_directories(helios_module_helios PUBLIC
          $<TARGET_PROPERTY:helios_module_${_name},INTERFACE_INCLUDE_DIRECTORIES>)
    elseif(HELIOS_BUILD_OPTION_HELIOS STREQUAL "SHARED")
      target_link_libraries(helios_module_helios PRIVATE ${_mod})
      target_include_directories(helios_module_helios PUBLIC
          $<TARGET_PROPERTY:helios_module_${_name},INTERFACE_INCLUDE_DIRECTORIES>)
    else()
      target_link_libraries(helios_module_helios PUBLIC ${_mod})
    endif()

    get_target_property(_has_named helios_module_${_name} HELIOS_HAS_NAMED_MODULE)
    if(HELIOS_ENABLE_CPP_MODULES AND _has_named)
      get_target_property(_cxx_name helios_module_${_name} HELIOS_CXX_MODULE_NAME)
      if(NOT _cxx_name OR _cxx_name STREQUAL "_cxx_name-NOTFOUND")
        string(REPLACE "_" "." _cxx_name "${_name}")
        set(_cxx_name "helios.${_cxx_name}")
      endif()
      string(APPEND _imports "export import ${_cxx_name};\n")
      set(_has_named_module TRUE)
    endif()
  endforeach()

  if(_has_named_module)
    set(_cppm "${CMAKE_BINARY_DIR}/helios.cppm")
    file(WRITE "${_cppm}" "export module helios;\n${_imports}")
    target_sources(helios_module_helios PUBLIC
        FILE_SET CXX_MODULES
        TYPE CXX_MODULES
        BASE_DIRS "${CMAKE_BINARY_DIR}"
        FILES "${_cppm}"
    )
    set_target_properties(helios_module_helios PROPERTIES
        CXX_SCAN_FOR_MODULES ON
    )
    target_compile_definitions(helios_module_helios PUBLIC
        "$<$<COMPILE_LANGUAGE:CXX>:HELIOS_ENABLE_CPP_MODULES>")
    if(HELIOS_ENABLE_IMPORT_STD)
      set_target_properties(helios_module_helios PROPERTIES CXX_MODULE_STD ON)
      target_compile_definitions(helios_module_helios PUBLIC
          "$<$<COMPILE_LANGUAGE:CXX>:HELIOS_ENABLE_IMPORT_STD>")
    endif()
  endif()

  set_target_properties(helios_module_helios PROPERTIES
      EXPORT_NAME "helios"
      OUTPUT_NAME "helios"
      HELIOS_MODULE_NAME "helios"
      FOLDER "Helios/Modules"
  )
  helios_apply_conventions(helios_module_helios)
  if(COMMAND helios_target_set_output_dirs)
    helios_target_set_output_dirs(helios_module_helios)
  endif()
  add_library(helios::helios ALIAS helios_module_helios)

  if(HELIOS_ENABLE_INSTALL)
    if(_has_named_module)
      install(TARGETS helios_module_helios
          EXPORT HeliosTargets
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          FILE_SET CXX_MODULES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/helios/modules
      )
    else()
      install(TARGETS helios_module_helios
          EXPORT HeliosTargets
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      )
    endif()
  endif()

  if(HELIOS_BUILD_C_API AND HELIOS_BUILD_TESTS AND TARGET helios::module::core)
    set(_c_test_src "${CMAKE_BINARY_DIR}/helios_c_api_link_test.c")
    file(WRITE "${_c_test_src}"
        "#include <helios/version.h>\n"
        "#ifdef HELIOS_MODULE_CORE_AVAILABLE\n"
        "#error \"C translation unit received HELIOS_MODULE_CORE_AVAILABLE\"\n"
        "#endif\n"
        "int main(void) {\n"
        "  return helios_compatible_with_headers() ? 0 : 1;\n"
        "}\n")
    add_executable(helios_c_api_link_test "${_c_test_src}")
    target_link_libraries(helios_c_api_link_test PRIVATE helios::helios)
    set_target_properties(helios_c_api_link_test PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF
        LINKER_LANGUAGE CXX
        FOLDER "Helios/Tests"
    )
    helios_apply_conventions(helios_c_api_link_test)
    if(COMMAND helios_target_set_output_dirs)
      helios_target_set_output_dirs(helios_c_api_link_test)
    endif()
    add_test(NAME helios_c_api_link_test COMMAND helios_c_api_link_test)
  endif()

  message(STATUS "Helios Module: helios (umbrella, ${HELIOS_BUILD_OPTION_HELIOS})")
endfunction()
endfunction()
