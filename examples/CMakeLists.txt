# Helios Engine Examples
#
# This directory contains example applications demonstrating Helios Engine features.

message(STATUS "")
message(STATUS "========================================")
message(STATUS "Configuring Helios Examples...")
message(STATUS "========================================")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ExampleUtils)

add_subdirectory(app)
add_subdirectory(ecs)
if(HELIOS_BUILD_PROFILE)
  add_subdirectory(profile)
endif()
add_subdirectory(simple)

message(STATUS "Examples configured successfully")
message(STATUS "")
