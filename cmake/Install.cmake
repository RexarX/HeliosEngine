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
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/HeliosModuleLinking.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Helios
)

message(STATUS "Install rules configured (install prefix: ${CMAKE_INSTALL_PREFIX})")
