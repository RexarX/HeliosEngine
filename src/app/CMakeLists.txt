option(HELIOS_APP_ENABLE_PROFILE "Enable profiling in the app module if profile module is available" ON)

helios_module(
    NAME app
    VERSION 0.1.0
    DESCRIPTION "Application framework module"

    HEADERS
        include/helios/app/builtin/app_exit.hpp
        include/helios/app/builtin/executor.hpp
        include/helios/app/builtin/frame_count.hpp
        include/helios/app/builtin/frame_limiter.hpp
        include/helios/app/builtin/time.hpp
        include/helios/app/details/profile.hpp
        include/helios/app/app.hpp
        include/helios/app/application.hpp
        include/helios/app/dynamic_plugin.hpp
        include/helios/app/frame_order.hpp
        include/helios/app/plugin.hpp
        include/helios/app/plugin_group.hpp
        include/helios/app/runners.hpp
        include/helios/app/scheduler.hpp
        include/helios/app/schedules.hpp
        include/helios/app/sub_app.hpp

    SOURCES
        src/builtin/executor.cpp
        src/builtin/frame_count.cpp
        src/builtin/frame_limiter.cpp
        src/builtin/time.cpp
        src/application.cpp
        src/dynamic_plugin.cpp
        src/plugin_group.cpp
        src/scheduler.cpp
        src/schedules.cpp
        src/sub_app.cpp

    PCH src/pch.hpp

    DEPENDS
        PUBLIC async
        PUBLIC compiler
        PUBLIC container
        PUBLIC core
        PUBLIC ecs
        PUBLIC log
        PUBLIC memory
        PUBLIC utils

    OPTIONAL_DEPENDS
        PUBLIC profile

    COMPILE_DEFINITIONS
        PUBLIC
            $<$<BOOL:HELIOS_APP_ENABLE_PROFILE>:HELIOS_APP_ENABLE_PROFILE>

    TEST_SOURCES
        tests/builtin/executor.cpp
        tests/builtin/frame_count.cpp
        tests/builtin/frame_limiter.cpp
        tests/builtin/time.cpp
        tests/application.cpp
        tests/dynamic_plugin.cpp
        tests/frame_order.cpp
        tests/main.cpp
        tests/plugin.cpp
        tests/plugin_group.cpp
        tests/runners.cpp
        tests/scheduler.cpp
        tests/schedules.cpp
        tests/sub_app.cpp
)

if(TARGET helios_app_tests AND HELIOS_BUILD_TESTS)
  add_library(helios_app_test_plugin SHARED tests/plugins/test_plugin.cpp)
  helios_apply_conventions(helios_app_test_plugin)
  helios_target_set_shared_lib_platform(helios_app_test_plugin)
  # Headers only: linking helios::module::app embeds a second TracyClient
  # (and mimalloc) in the .so, which AddressSanitizer reports as ODR on dlopen.
  target_link_libraries(helios_app_test_plugin PRIVATE helios::module::platform)
  target_include_directories(helios_app_test_plugin PRIVATE
      include
      $<TARGET_PROPERTY:helios_module_app,INTERFACE_INCLUDE_DIRECTORIES>
      $<TARGET_PROPERTY:helios_module_utils,INTERFACE_INCLUDE_DIRECTORIES>
  )
  set_target_properties(helios_app_tests PROPERTIES ENABLE_EXPORTS ON)
  set_target_properties(helios_app_test_plugin PROPERTIES POSITION_INDEPENDENT_CODE ON)
  set_target_properties(helios_app_test_plugin PROPERTIES
      OUTPUT_NAME "helios_app_test_plugin"
      LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
      HELIOS_TEST_TARGET TRUE
  )
  helios_target_set_output_dirs(helios_app_test_plugin CUSTOM_FOLDER tests)

  add_dependencies(helios_app_tests helios_app_test_plugin)
  target_compile_definitions(helios_app_tests PRIVATE
        HELIOS_TEST_PLUGIN_PATH="$<TARGET_FILE:helios_app_test_plugin>")
endif()
