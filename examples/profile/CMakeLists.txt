helios_add_example(
    NAME profile
    SOURCES profile.cpp
    MODULES app ecs log profile
)

target_compile_definitions(profile_example PRIVATE HELIOS_ENABLE_PROFILE)
