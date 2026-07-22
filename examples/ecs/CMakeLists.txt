helios_add_example(
    NAME command
    SOURCES command.cpp
    MODULES app ecs log
)

helios_add_example(
    NAME component
    SOURCES component.cpp
    MODULES app ecs log
)

helios_add_example(
    NAME component_bundle
    SOURCES component_bundle.cpp
    MODULES app ecs log
)

helios_add_example(
    NAME entity
    SOURCES entity.cpp
    MODULES app ecs log
)

helios_add_example(
    NAME message
    SOURCES message.cpp
    MODULES app ecs log
)

helios_add_example(
    NAME query
    SOURCES query.cpp
    MODULES app ecs log
)

helios_add_example(
    NAME resource
    SOURCES resource.cpp
    MODULES app ecs log
)

helios_add_example(
    NAME schedule
    SOURCES schedule.cpp
    MODULES app ecs log
)

helios_add_example(
    NAME stage
    SOURCES stage.cpp
    MODULES app ecs log
)

helios_add_example(
    NAME system
    SOURCES system.cpp
    MODULES app ecs log
)
