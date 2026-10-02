helios_header_only_library(helios_async CPP_MODULE modules/helios.async.cppm)

target_sources(helios_async ${HELIOS_CURRENT_MODULE_SCOPE}
    FILE_SET HEADERS BASE_DIRS include FILES
        include/helios/async/details/profile.hpp
        include/helios/async/async.hpp
        include/helios/async/async_task.hpp
        include/helios/async/common.hpp
        include/helios/async/executor.hpp
        include/helios/async/future.hpp
        include/helios/async/sub_task_graph.hpp
        include/helios/async/task.hpp
        include/helios/async/task_graph.hpp
)

if(HELIOS_BUILD_C_API)
  target_sources(helios_async ${HELIOS_CURRENT_MODULE_SCOPE}
      FILE_SET c_headers TYPE HEADERS BASE_DIRS capi/include FILES
          capi/include/helios/async/async.h
  )
endif()

helios_require_dependency(Taskflow)
target_link_libraries(helios_async ${HELIOS_CURRENT_MODULE_SCOPE}
    helios::lib::taskflow::taskflow)

helios_target_set_folder(helios_async "Helios/Modules")

if(HELIOS_CURRENT_MODULE_BUILD_TESTS)
  helios_add_module_tests(helios_async
      SOURCES
          tests/async_task.cpp
          tests/common.cpp
          tests/executor.cpp
          tests/future.cpp
          tests/main.cpp
          tests/module_import.cpp
          tests/sub_task_graph.cpp
          tests/task.cpp
          tests/task_graph.cpp

      C_SOURCES
          capi/tests/c_smoke.c
  )
endif()

# Link TBB on Linux with GCC - required for libstdc++ parallel STL backend
helios_require_dependency(TBB)
if(HELIOS_HAS_SYSTEM_TBB)
  target_link_libraries(helios_async INTERFACE helios::lib::tbb::tbb)
  message(STATUS "helios_async: Linking TBB for libstdc++ parallel STL support")
endif()
