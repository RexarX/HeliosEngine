# Installation configuration for Helios Engine
#
# Requires HELIOS_ENABLE_INSTALL=ON.
# Produces:
#   HeliosConfig.cmake
#   HeliosConfigVersion.cmake
#   HeliosTargets.cmake
# installed to ${CMAKE_INSTALL_LIBDIR}/cmake/Helios

include_guard(GLOBAL)

include(CMakePackageConfigHelpers)

# Install all targets accumulated in the HeliosTargets export set
install(EXPORT HeliosTargets
    FILE HeliosTargets.cmake
    NAMESPACE helios::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Helios
)

# Generate HeliosConfigVersion.cmake
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/HeliosConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Generate HeliosConfig.cmake from template
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/HeliosConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/HeliosConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Helios
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

# Install the generated config files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/HeliosConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/HeliosConfigVersion.cmake"
    "${CMAKE_BINARY_DIR}/helios/HeliosAliases.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/HeliosPrelude.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/HeliosModuleLinking.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ModuleDeclarations.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ModuleFinalize.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ModuleLinking.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ModuleRegistry.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/TargetUtils.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/TestUtils.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CppModules.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Sanitizers.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/SimdUtils.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Helios
)

install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/helpers/Primitives.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/helpers/ConfigureGenex.cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/helpers/Linker.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Helios/helpers
)

if(EXISTS "${CMAKE_BINARY_DIR}/generated/helios/abi_check.hpp")
  install(FILES "${CMAKE_BINARY_DIR}/generated/helios/abi_check.hpp"
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/helios)
endif()

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/scripts/helios_modules/"
    DESTINATION share/helios/tools/helios_modules
    PATTERN "__pycache__" EXCLUDE
)
install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/helios_modules.py"
    DESTINATION share/helios/tools
)
install(FILES "${CMAKE_BINARY_DIR}/helios/modules.json"
    DESTINATION share/helios
)

set(_sdk_cfg "${CMAKE_BUILD_TYPE}")
install(CODE "
  set(_cfg \"${CMAKE_BUILD_TYPE}\")
  if(NOT \"\${CMAKE_INSTALL_CONFIG_NAME}\" STREQUAL \"\")
    set(_cfg \"\${CMAKE_INSTALL_CONFIG_NAME}\")
  endif()
  if(NOT EXISTS \"${CMAKE_BINARY_DIR}/helios/helios_artifacts_\${_cfg}.toml\")
    message(FATAL_ERROR \"Missing helios_artifacts_\${_cfg}.toml. Build this configuration before installing.\")
  endif()
  execute_process(
      COMMAND \"${Python3_EXECUTABLE}\"
          \"${CMAKE_CURRENT_SOURCE_DIR}/scripts/helios_modules.py\"
          sdk
          --graph \"${CMAKE_BINARY_DIR}/helios/modules.json\"
          --artifacts \"${CMAKE_BINARY_DIR}/helios/helios_artifacts_\${_cfg}.toml\"
          --prefix \"\${CMAKE_INSTALL_PREFIX}\"
          --include-dir \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/helios\"
          --cmake-dir \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/cmake/Helios\"
      RESULT_VARIABLE _sdk_rc
  )
  if(NOT _sdk_rc EQUAL 0)
    message(FATAL_ERROR \"helios_modules sdk failed\")
  endif()
")

message(STATUS "Install rules configured (install prefix: ${CMAKE_INSTALL_PREFIX})")
