add_subdirectory(dynamic_plugin)

helios_add_example(
    NAME async_sub_app
    SOURCES async_sub_app.cpp
    MODULES app ecs log
)

helios_add_example(
    NAME overlapping_sub_app
    SOURCES overlapping_sub_app.cpp
    MODULES app ecs log
)

helios_add_example(
    NAME plugin
    SOURCES plugin.cpp
    MODULES app ecs log
)

helios_add_example(
    NAME plugin_group
    SOURCES plugin_group.cpp
    MODULES app log
)

helios_add_example(
    NAME runner
    SOURCES runner.cpp
    MODULES app log
)

helios_add_example(
    NAME sub_app
    SOURCES sub_app.cpp
    MODULES app ecs log
)
