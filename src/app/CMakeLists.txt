option(HELIOS_APP_ENABLE_PROFILE "Enable profiling in the app module if profile module is available" ON)

helios_module(
    NAME app
    VERSION 0.1.0
    DESCRIPTION "Application framework module"

    HEADERS
        include/helios/app/details/profile.hpp
        include/helios/app/app.hpp
        include/helios/app/application.hpp
        include/helios/app/dynamic_plugin.hpp
        include/helios/app/plugin.hpp
        include/helios/app/runners.hpp
        include/helios/app/scheduler.hpp
        include/helios/app/schedules.hpp
        include/helios/app/sub_app.hpp

    SOURCES
        src/application.cpp
        src/scheduler.cpp
        src/sub_app.cpp

    PCH src/pch.hpp

    DEPENDS
        PUBLIC async
        PUBLIC core
        PUBLIC ecs
        PUBLIC log
        PUBLIC utils

    OPTIONAL_DEPENDS
        PUBLIC profile

    USES
        FlatMap PUBLIC helios::flat_map

    COMPILE_DEFINITIONS
        PUBLIC
            $<$<BOOL:HELIOS_APP_ENABLE_PROFILE>:HELIOS_APP_ENABLE_PROFILE>

    TEST_SOURCES
        tests/application.cpp
        tests/dynamic_plugin.cpp
        tests/main.cpp
        tests/plugin.cpp
        tests/runners.cpp
        tests/scheduler.cpp
        tests/schedules.cpp
        tests/sub_app.cpp
)

if(TARGET helios_app_tests AND HELIOS_BUILD_TESTS)
  add_library(helios_app_test_plugin SHARED tests/plugins/test_plugin.cpp)
  helios_target_set_platform(helios_app_test_plugin)
  target_link_libraries(helios_app_test_plugin PRIVATE
        helios::module::platform
        helios::module::app
    )
  target_include_directories(helios_app_test_plugin PRIVATE include)
  set_target_properties(helios_app_test_plugin PROPERTIES POSITION_INDEPENDENT_CODE ON)
  set_target_properties(helios_app_test_plugin PROPERTIES
        OUTPUT_NAME "helios_app_test_plugin"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
    )
  helios_target_set_output_dirs(helios_app_test_plugin CUSTOM_FOLDER tests)

  add_dependencies(helios_app_tests helios_app_test_plugin)
  target_compile_definitions(helios_app_tests PRIVATE
        HELIOS_TEST_PLUGIN_PATH="$<TARGET_FILE:helios_app_test_plugin>")
endif()
