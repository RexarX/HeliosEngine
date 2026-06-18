set(SIMPLE_EXAMPLE_HEADERS
)

set(SIMPLE_EXAMPLE_SOURCES
    src/main.cpp
)

set(ALL_FILES
    ${SIMPLE_EXAMPLE_HEADERS}
    ${SIMPLE_EXAMPLE_SOURCES}
)

add_executable(simple_example ${ALL_FILES})

helios_target_set_cxx_standard(simple_example STANDARD 23)
helios_target_set_warnings(simple_example)
helios_target_set_platform(simple_example)
helios_target_set_optimization(simple_example)
helios_target_set_output_dirs(simple_example CUSTOM_FOLDER examples)
helios_target_enable_sanitizers(simple_example)

helios_link_modules(
    TARGET simple_example
    MODULES
        app
        profile
        log
)

if(HELIOS_PROFILE_GLOBAL_ALLOC)
  helios_profile_link_global_alloc(simple_example)
endif()
