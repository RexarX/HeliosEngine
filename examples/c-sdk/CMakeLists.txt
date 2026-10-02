if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  cmake_minimum_required(VERSION 3.25)
  project(helios_c_sdk_example LANGUAGES C CXX)
  set(HELIOS_SDK_EXAMPLE_STANDALONE TRUE)
endif()

if(NOT HELIOS_SDK_EXAMPLE_STANDALONE AND NOT HELIOS_BUILD_C_API)
  return()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../cmake")
include(SdkExample)

helios_add_c_sdk_example()
