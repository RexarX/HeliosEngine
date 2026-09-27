option(HELIOS_ASYNC_ENABLE_PROFILE "Enable profiling in the helios_async module if helios_profile module is available" ON)

helios_module(
    NAME helios_async
    ALIAS helios::async
    VERSION 0.1.0
    DESCRIPTION "Asynchronous task execution module built on Taskflow"

    HEADERS
        include/helios/async/async.hpp
        include/helios/async/async_task.hpp
        include/helios/async/common.hpp
        include/helios/async/details/profile.hpp
        include/helios/async/executor.hpp
        include/helios/async/future.hpp
        include/helios/async/sub_task_graph.hpp
        include/helios/async/task.hpp
        include/helios/async/task_graph.hpp

    MODULE_SOURCES
        modules/helios.async.cppm

    TEST_SOURCES
        tests/async_task.cpp
        tests/common.cpp
        tests/executor.cpp
        tests/future.cpp
        tests/main.cpp
        tests/module_import.cpp
        tests/sub_task_graph.cpp
        tests/task.cpp
        tests/task_graph.cpp

    C_HEADERS
        capi/include/helios/async/async.h

    C_TEST_SOURCES
        capi/tests/c_smoke.c

    DEPENDS
        PUBLIC
            $<$<BOOL:${HELIOS_BUILD_C_API}>:capi>
            helios_core

    OPTIONAL_DEPENDS
        PUBLIC helios_profile

    USES
        Taskflow PUBLIC helios::lib::taskflow::taskflow

    COMPILE_DEFINITIONS
        PUBLIC
            $<$<BOOL:HELIOS_ASYNC_ENABLE_PROFILE>:HELIOS_ASYNC_ENABLE_PROFILE>
)

# Link TBB on Linux with GCC — required for libstdc++ parallel STL backend
helios_require_dependency(TBB)
if(HELIOS_HAS_SYSTEM_TBB)
  target_link_libraries(helios_async INTERFACE helios::lib::tbb::tbb)
  message(STATUS "helios_async: Linking TBB for libstdc++ parallel STL support")
endif()
