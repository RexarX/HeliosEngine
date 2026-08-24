# Helios Engine Examples
#
# This directory contains example applications demonstrating Helios Engine features.

message(STATUS "")
message(STATUS "========================================")
message(STATUS "Configuring Helios Examples...")
message(STATUS "========================================")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ExampleUtils)

#if(HELIOS_BUILD_APP)
add_subdirectory(app)
#endif()
#if(HELIOS_BUILD_ECS)
add_subdirectory(ecs)
#endif()
#if(HELIOS_BUILD_PROFILE)
add_subdirectory(profile)
#endif()
add_subdirectory(simple)
#if(HELIOS_BUILD_WINDOW)
add_subdirectory(window)
#endif()
#if(HELIOS_BUILD_INPUT)
add_subdirectory(input)
#endif()

message(STATUS "Examples configured successfully")
message(STATUS "")
