# Window Module Registration
#
# This file is processed during module discovery to register the module
# and create its build option before the CMakeLists.txt is processed.

helios_register_module(
    NAME window
    DESCRIPTION "Cross-platform window management module using GLFW"
    DEFAULT ON
    EXTERNAL_DEPENDS glfw
)
