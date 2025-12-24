# Example Module Registration
#
# This file is processed during module discovery to register the module
# and create its build option before the CMakeLists.txt is processed.

helios_register_module(
    NAME example
    DESCRIPTION "Example module demonstrating the Helios module system"
    DEFAULT ON
    # DEPENDS window              # Uncomment to add required module dependencies
    # OPTIONAL_DEPENDS audio      # Uncomment to add optional module dependencies
    # EXTERNAL_DEPENDS glfw       # Uncomment to add external library dependencies
)
