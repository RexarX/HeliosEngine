include_guard(GLOBAL)

function(helios_normalize_path_for_doxygen input_path output_var)
  file(TO_CMAKE_PATH "${input_path}" _normalized)
  string(REPLACE "\\" "/" _normalized "${_normalized}")
  set("${output_var}" "${_normalized}" PARENT_SCOPE)
endfunction()

function(helios_resolve_doxygen_project_number output_var)
  if(DEFINED ENV{DOCS_LABEL} AND NOT "$ENV{DOCS_LABEL}" STREQUAL "")
    set(_label "$ENV{DOCS_LABEL}")
  else()
    set(_label "")
  endif()

  if(_label STREQUAL "" OR _label STREQUAL "main")
    set("${output_var}" "${PROJECT_VERSION}" PARENT_SCOPE)
  else()
    set("${output_var}" "${PROJECT_VERSION} (${_label})" PARENT_SCOPE)
  endif()
endfunction()

function(helios_add_doxygen_docs)
  find_package(Doxygen REQUIRED)
  find_package(Python3 COMPONENTS Interpreter REQUIRED)

  helios_normalize_path_for_doxygen("${CMAKE_SOURCE_DIR}" HELIOS_SOURCE_DIR)
  helios_normalize_path_for_doxygen(
    "${CMAKE_SOURCE_DIR}/docs/doxygen"
    HELIOS_DOXYGEN_OUTPUT_DIR
  )
  helios_resolve_doxygen_project_number(HELIOS_DOXYGEN_PROJECT_NUMBER)

  set(_doxygen_src_dir "${CMAKE_SOURCE_DIR}/docs/doxygen")
  set(_doxygen_bin_dir "${CMAKE_BINARY_DIR}/docs/doxygen")
  file(MAKE_DIRECTORY "${_doxygen_bin_dir}")

  configure_file(
    "${_doxygen_src_dir}/Doxyfile.in"
    "${_doxygen_bin_dir}/Doxyfile"
    @ONLY
  )

  add_custom_target(
    helios_docs
    COMMAND "${DOXYGEN_EXECUTABLE}" Doxyfile
    COMMAND
      "${CMAKE_COMMAND}" -E env
      "HELIOS_DOXYGEN_PROJECT_NUMBER=${HELIOS_DOXYGEN_PROJECT_NUMBER}"
      "${Python3_EXECUTABLE}"
      "${CMAKE_SOURCE_DIR}/scripts/docs.py"
      --post-process
    WORKING_DIRECTORY "${_doxygen_bin_dir}"
    COMMENT "Generating API documentation with Doxygen ${DOXYGEN_VERSION}"
    VERBATIM
  )

  set_target_properties(helios_docs PROPERTIES FOLDER "docs")
endfunction()
