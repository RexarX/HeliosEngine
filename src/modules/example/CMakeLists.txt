# Example Module CMakeLists.txt
#
# This is a minimal example demonstrating how to create a Helios Engine module.
# See docs/guides/creating-modules.md for comprehensive documentation.

# Define the module headers
set(EXAMPLE_HEADERS
    include/helios/example/example.hpp
)

# Define the module sources
set(EXAMPLE_SOURCES
    src/example.cpp
)

# Create the module using helios_add_module
# Note: Registration was already done in Module.cmake
helios_add_module(
    NAME example
    VERSION 1.0.0

    HEADERS ${EXAMPLE_HEADERS}
    SOURCES ${EXAMPLE_SOURCES}

    # Add external dependencies here:
    # PUBLIC_DEPENDENCIES
    #     some_library

    # Add other Helios module dependencies here:
    # MODULE_DEPENDENCIES
    #     window
    #     input
)
