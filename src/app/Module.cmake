helios_register_module(
    NAME app
    DESCRIPTION "Application framework module"
    DEFAULT ON
    DEPENDS async ecs log utils
)
