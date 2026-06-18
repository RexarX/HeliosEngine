# TBB must be loaded before checking HELIOS_HAS_SYSTEM_TBB
helios_require_dependency(TBB)

option(HELIOS_ASYNC_ENABLE_PROFILE "Enable profiling in the async module if profile module is available" ON)

helios_module(
    NAME async
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

    DEPENDS
        PUBLIC core

    OPTIONAL_DEPENDS
        PUBLIC profile

    USES
        Taskflow PUBLIC helios::taskflow::taskflow

    COMPILE_DEFINITIONS
        PUBLIC
            $<$<BOOL:HELIOS_ASYNC_ENABLE_PROFILE>:HELIOS_ASYNC_ENABLE_PROFILE>

    TEST_SOURCES
        tests/async_task.cpp
        tests/common.cpp
        tests/executor.cpp
        tests/future.cpp
        tests/main.cpp
        tests/sub_task_graph.cpp
        tests/task.cpp
        tests/task_graph.cpp
)

# Link TBB on Linux with GCC — required for libstdc++ parallel STL backend
if(HELIOS_HAS_SYSTEM_TBB)
  target_link_libraries(helios_module_async INTERFACE helios::tbb::tbb)
  message(STATUS "helios_module_async: Linking TBB for libstdc++ parallel STL support")
endif()
