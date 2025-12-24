# Helios Engine Modules
#
# This directory contains all engine modules.
# Each module is a self-contained library that builds upon the core.
#
# Modules are organized as:
#   modules/{name}/
#     ├── Module.cmake              # Optional: Module registration (helios_register_module)
#     ├── CMakeLists.txt            # Module build configuration (helios_add_module)
#     ├── include/helios/modules/{name}/
#     └── src/
#
# Each module can be enabled/disabled via:
#   -DHELIOS_BUILD_{NAME}_MODULE=ON|OFF
#
# See docs/guides/creating-modules.md for detailed documentation.

message(STATUS "")
message(STATUS "========================================")
message(STATUS "Configuring Helios Modules...")
message(STATUS "========================================")

# Step 1: Discover all modules in this directory
# This scans for subdirectories with CMakeLists.txt files
# and includes any Module.cmake files for registration
helios_discover_modules(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    # EXCLUDE deprecated_module  # Uncomment to exclude specific modules
)

# Step 2: Resolve module dependencies
# This enables any required dependencies that are currently disabled
helios_resolve_module_dependencies()

# Step 3: Validate that all dependencies are satisfied
helios_validate_module_dependencies()

# Step 4: Build all enabled modules
helios_build_discovered_modules(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Step 5: Print summary of all modules
helios_print_modules()

# Print available build options for reference
helios_print_module_build_options()
