# Helios Engine Examples
#
# This directory contains example applications demonstrating Helios Engine features.

message(STATUS "")
message(STATUS "========================================")
message(STATUS "Configuring Helios Examples...")
message(STATUS "========================================")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ExampleUtils)

add_subdirectory(async_sub_apps)
add_subdirectory(dynamic_plugins)
add_subdirectory(ecs)
add_subdirectory(overlapping_sub_apps)
add_subdirectory(plugins)
if(HELIOS_BUILD_PROFILE)
  add_subdirectory(profile)
endif()
add_subdirectory(simple)
add_subdirectory(sub_apps)

message(STATUS "Examples configured successfully")
message(STATUS "")
