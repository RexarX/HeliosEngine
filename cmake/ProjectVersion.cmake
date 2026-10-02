# Reads the repository VERSION file (single-line semver) before project().
set(_helios_version_file "${CMAKE_CURRENT_LIST_DIR}/../VERSION")
if(NOT EXISTS "${_helios_version_file}")
  message(FATAL_ERROR "Missing VERSION file: ${_helios_version_file}")
endif()

file(READ "${_helios_version_file}" _helios_version_raw)
string(STRIP "${_helios_version_raw}" HELIOS_PROJECT_VERSION)
if(NOT HELIOS_PROJECT_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+")
  message(FATAL_ERROR
      "Invalid VERSION '${HELIOS_PROJECT_VERSION}' in ${_helios_version_file} "
      "(expected MAJOR.MINOR.PATCH)"
  )
endif()
