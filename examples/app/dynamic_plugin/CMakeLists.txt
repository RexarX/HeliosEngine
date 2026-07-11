helios_add_example(
    NAME dynamic_plugin
    SOURCES src/main.cpp
    MODULES app ecs log
)

target_include_directories(dynamic_plugin_example PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(example_dynamic_plugin SHARED plugins/example_plugin.cpp)
target_include_directories(example_dynamic_plugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
helios_target_set_cxx_standard(example_dynamic_plugin STANDARD 23)
helios_target_set_warnings(example_dynamic_plugin)
helios_target_set_platform(example_dynamic_plugin)
helios_target_set_optimization(example_dynamic_plugin)
helios_target_set_shared_lib_platform(example_dynamic_plugin)
helios_target_set_output_dirs(example_dynamic_plugin CUSTOM_FOLDER examples)
helios_target_set_folder(example_dynamic_plugin "Examples")
helios_link_modules(
    TARGET example_dynamic_plugin
    MODULES app log platform
)

set_target_properties(example_dynamic_plugin PROPERTIES
    OUTPUT_NAME "helios_example_dynamic_plugin"
    POSITION_INDEPENDENT_CODE ON
)

add_dependencies(dynamic_plugin_example example_dynamic_plugin)
target_compile_definitions(dynamic_plugin_example PRIVATE
    HELIOS_EXAMPLE_PLUGIN_PATH="$<TARGET_FILE:example_dynamic_plugin>"
)
