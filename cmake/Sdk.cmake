# SDK artifacts, ABI header, and install helpers.

include_guard(GLOBAL)

function(helios_write_abi_header)
  string(REGEX MATCH "^[0-9]+" _major "${CMAKE_CXX_COMPILER_VERSION}")
  if(NOT _major)
    set(_major 0)
  endif()
  set(HELIOS_ABI_COMPILER_MAJOR "${_major}")
  set(HELIOS_ABI_CXX_STANDARD "202302L")
  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR
      (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
    set(HELIOS_ABI_STDLIB_CHECK
        "#if defined(_LIBCPP_VERSION) || defined(__GLIBCXX__)\n#error \"Helios was built against the MSVC standard library.\"\n#endif\n")
    set(HELIOS_ABI_FINGERPRINT "msvc-${_major}")
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(HELIOS_ABI_STDLIB_CHECK
        "#if defined(_LIBCPP_VERSION)\n#error \"Helios was built against libstdc++.\"\n#endif\n")
    set(HELIOS_ABI_FINGERPRINT "libstdc++-${_major}")
  else()
    set(HELIOS_ABI_STDLIB_CHECK
        "#if defined(_MSC_VER) && !defined(__clang__)\n#error \"Helios was not built with the MSVC standard library.\"\n#endif\n")
    set(HELIOS_ABI_FINGERPRINT "cxx-${_major}")
  endif()
  set(HELIOS_ABI_FINGERPRINT "${HELIOS_ABI_FINGERPRINT}" CACHE INTERNAL
      "ABI fingerprint recorded in helios_artifacts and abi_check.hpp")
  set(_dir "${CMAKE_BINARY_DIR}/generated/helios")
  file(MAKE_DIRECTORY "${_dir}")
  configure_file(
      "${HELIOS_ROOT_DIR}/cmake/abi_check.hpp.in"
      "${_dir}/abi_check.hpp"
      @ONLY
  )
endfunction()

function(helios_write_sdk_artifacts)
  set(_chunks
      "version = \"${PROJECT_VERSION}\"\n"
      "config = \"$<CONFIG>\"\n"
      "compiler_id = \"${CMAKE_CXX_COMPILER_ID}\"\n"
      "compiler_version = \"${CMAKE_CXX_COMPILER_VERSION}\"\n"
      "cxx_standard = \"23\"\n"
      "stdlib = \"${HELIOS_ABI_FINGERPRINT}\"\n"
  )
  if(NOT HELIOS_ABI_FINGERPRINT)
    set(_chunks
        "version = \"${PROJECT_VERSION}\"\n"
        "config = \"$<CONFIG>\"\n"
        "compiler_id = \"${CMAKE_CXX_COMPILER_ID}\"\n"
        "compiler_version = \"${CMAKE_CXX_COMPILER_VERSION}\"\n"
        "cxx_standard = \"23\"\n"
        "stdlib = \"unknown\"\n"
    )
  endif()
  get_property(_modules GLOBAL PROPERTY HELIOS_MODULES)
  foreach(_name IN LISTS _modules)
    string(TOUPPER "${_name}" _upper)
    set(_target "${${_upper}_TARGET}")
    if(NOT TARGET ${_target})
      continue()
    endif()
    get_target_property(_type ${_target} TYPE)
    if(_type STREQUAL "INTERFACE_LIBRARY")
      continue()
    endif()
    string(APPEND _chunks "\n[libraries.${_name}]\n")
    string(APPEND _chunks "kind = \"${_type}\"\n")
    string(APPEND _chunks "file_name = \"$<TARGET_FILE_NAME:${_target}>\"\n")
    string(APPEND _chunks "linker_file = \"$<TARGET_LINKER_FILE_NAME:${_target}>\"\n")
  endforeach()
  if(TARGET helios_module_helios)
    string(APPEND _chunks "\n[libraries.helios]\n")
    string(APPEND _chunks "kind = \"$<TARGET_PROPERTY:helios_module_helios,TYPE>\"\n")
    string(APPEND _chunks "file_name = \"$<TARGET_FILE_NAME:helios_module_helios>\"\n")
    string(APPEND _chunks "linker_file = \"$<TARGET_LINKER_FILE_NAME:helios_module_helios>\"\n")
  endif()
  file(GENERATE
      OUTPUT "${CMAKE_BINARY_DIR}/helios/helios_artifacts_$<CONFIG>.toml"
      CONTENT "${_chunks}"
  )
endfunction()
