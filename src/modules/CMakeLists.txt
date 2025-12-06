# This directory contains all engine modules.
# Each module is a self-contained library that builds upon the core.
#
# Modules are organized as:
#   modules/{name}/
#     ├── include/helios/modules/{name}/
#     ├── src/
#     └── CMakeLists.txt
#
# Each module CMakeLists.txt should use helios_add_module() function.

message(STATUS "Configuring Helios Modules...")

# Example future modules:
# add_subdirectory(window)
# add_subdirectory(input)
# add_subdirectory(scripting)
# add_subdirectory(physics)

# Print all registered modules at the end
helios_print_modules()
