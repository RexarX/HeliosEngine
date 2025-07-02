# Renderer Module CMakeLists.txt
cmake_minimum_required(VERSION 3.25.0)

project(HeliosRenderer)

# Set C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(VulkanHeaders CONFIG REQUIRED)

# Note: NVRHI would be added as a dependency here
# find_package(nvrhi CONFIG REQUIRED)

# Collect source files
file(GLOB_RECURSE RENDERER_HEADERS
    "include/helios/renderer/*.h"
    "include/helios/renderer/**/*.h"
)

file(GLOB_RECURSE RENDERER_SOURCES
    "src/*.cpp"
    "src/**/*.cpp"
)

file(GLOB_RECURSE EXAMPLE_SOURCES
    "examples/*.cpp"
    "examples/*.h"
)

file(GLOB_RECURSE TEST_SOURCES
    "tests/*.cpp"
)

# Create the renderer library
add_library(HeliosRenderer STATIC
    ${RENDERER_HEADERS}
    ${RENDERER_SOURCES}
)

# Set target properties
set_target_properties(HeliosRenderer PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Include directories
target_include_directories(HeliosRenderer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(HeliosRenderer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link to HeliosEngine for core utilities (Log, ThreadPool, etc.)
target_include_directories(HeliosRenderer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../HeliosEngine/src
)

# Link dependencies
target_link_libraries(HeliosRenderer PUBLIC
    glfw
    glm::glm-header-only
    Vulkan::Headers
)

# Note: When NVRHI is available, add:
# target_link_libraries(HeliosRenderer PUBLIC nvrhi::nvrhi)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(HeliosRenderer PRIVATE
        /W4
        /permissive-
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(HeliosRenderer PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
    )
endif()

# Compile definitions
target_compile_definitions(HeliosRenderer PRIVATE
    $<$<CONFIG:Debug>:DEBUG_MODE>
    $<$<CONFIG:RelWithDebInfo>:RELEASE_WITH_DEBUG_INFO_MODE>
    $<$<CONFIG:Release>:RELEASE_MODE>
    GLFW_INCLUDE_NONE
    GLM_FORCE_DEPTH_ZERO_TO_ONE
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(HeliosRenderer PRIVATE
        PLATFORM_WINDOWS
        UNICODE
        _UNICODE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
elseif(UNIX)
    target_compile_definitions(HeliosRenderer PRIVATE PLATFORM_LINUX)
    target_compile_options(HeliosRenderer PRIVATE -fPIC)
endif()

# Source groups for IDE organization
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include PREFIX "Header Files" FILES ${RENDERER_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src PREFIX "Source Files" FILES ${RENDERER_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/examples PREFIX "Examples" FILES ${EXAMPLE_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/tests PREFIX "Tests" FILES ${TEST_SOURCES})

# Optionally build examples and tests
option(HELIOS_RENDERER_BUILD_EXAMPLES "Build renderer examples" ON)
option(HELIOS_RENDERER_BUILD_TESTS "Build renderer tests" ON)

if(HELIOS_RENDERER_BUILD_EXAMPLES AND EXAMPLE_SOURCES)
    add_library(HeliosRendererExamples STATIC ${EXAMPLE_SOURCES})
    target_link_libraries(HeliosRendererExamples PUBLIC HeliosRenderer)
    target_include_directories(HeliosRendererExamples PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/examples
    )
endif()

if(HELIOS_RENDERER_BUILD_TESTS AND TEST_SOURCES)
    add_executable(HeliosRendererTests ${TEST_SOURCES})
    target_link_libraries(HeliosRendererTests PRIVATE HeliosRenderer)
    target_include_directories(HeliosRendererTests PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    
    # Set output directory for tests
    set_target_properties(HeliosRendererTests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )
endif()

# Installation rules (optional)
install(TARGETS HeliosRenderer
    EXPORT HeliosRendererTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Export targets
install(EXPORT HeliosRendererTargets
    FILE HeliosRendererTargets.cmake
    NAMESPACE Helios::
    DESTINATION lib/cmake/HeliosRenderer
)