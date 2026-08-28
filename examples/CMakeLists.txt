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
add_subdirectory(cxx_modules)
add_subdirectory(ecs)
add_subdirectory(input)
add_subdirectory(profile)
add_subdirectory(simple)
add_subdirectory(window)

message(STATUS "Examples configured successfully")
message(STATUS "")
