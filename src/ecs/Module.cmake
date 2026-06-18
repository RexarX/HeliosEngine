helios_register_module(
    NAME ecs
    DESCRIPTION "Entity Component System module"
    DEFAULT ON
    DEPENDS async compiler container core log memory utils
    OPTIONAL_DEPENDS profile
)
