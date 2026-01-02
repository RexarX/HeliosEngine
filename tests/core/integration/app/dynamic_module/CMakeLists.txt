# Test module for DynamicModule integration tests
# This builds a shared library that can be dynamically loaded at runtime

include(TargetUtils)

# Define test module sources
set(TEST_MODULE_SOURCES
    test_module.cpp
)

set(TEST_MODULE_HEADERS
    test_module.hpp
)

# Create shared library for the test module
add_library(helios_test_module SHARED
    ${TEST_MODULE_SOURCES}
    ${TEST_MODULE_HEADERS}
)

# Link to helios::core
target_link_libraries(helios_test_module PRIVATE
    helios::core
)

# Apply standard Helios configurations
helios_target_set_cxx_standard(helios_test_module STANDARD 23)
helios_target_set_platform(helios_test_module)
helios_target_set_shared_lib_platform(helios_test_module)
helios_target_set_optimization(helios_test_module)
helios_target_set_warnings(helios_test_module)

# Set output directory to match test executable location
helios_target_set_output_dirs(helios_test_module CUSTOM_FOLDER tests)

# Set IDE folder
helios_target_set_folder(helios_test_module "Helios/Tests/Core/Integration/DynamicModule")

# Enable LTO if available
if(HELIOS_ENABLE_LTO)
    helios_target_enable_lto(helios_test_module)
endif()

# Include current directory for test_module.hpp
target_include_directories(helios_test_module PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Set output name to avoid conflicts
set_target_properties(helios_test_module PROPERTIES
    OUTPUT_NAME "helios_test_module"
    PREFIX ""  # Remove 'lib' prefix on Unix to simplify test code
)

# On Windows, ensure proper symbol export
if(WIN32)
    target_compile_definitions(helios_test_module PRIVATE
        HELIOS_TEST_MODULE_EXPORTS
    )
endif()

# Copy runtime dependencies for test module
#helios_copy_runtime_dependencies(helios_core_integration LIBRARIES helios_test_module)

message(STATUS "Test module library configured: helios_test_module")
