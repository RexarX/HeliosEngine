# Example-only module — discovered via HELIOS_EXTRA_MODULE_DIRS / HELIOS_BUILD_EXAMPLES.
helios_register_module(
    NAME greeting
    DESCRIPTION "Example custom Helios module (greeting utilities)"
    DEFAULT ON
    DEPENDS core utils
)
