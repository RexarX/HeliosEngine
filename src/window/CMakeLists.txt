add_library(helios_window)

target_sources(helios_window PUBLIC FILE_SET HEADERS BASE_DIRS include FILES
    include/helios/window/clipboard.hpp
    include/helios/window/ids.hpp
    include/helios/window/monitor.hpp
    include/helios/window/native_handle.hpp
    include/helios/window/params.hpp
    include/helios/window/plugin.hpp
    include/helios/window/properties.hpp
    include/helios/window/schedules.hpp
    include/helios/window/settings.hpp
    include/helios/window/window.hpp
)

target_sources(helios_window
    PRIVATE
        src/plugin.cpp
)

if(HELIOS_BUILD_C_API)
  target_sources(helios_window PUBLIC
      FILE_SET c_headers TYPE HEADERS BASE_DIRS capi/include FILES
          capi/include/helios/window/window.h
  )
endif()

helios_apply_conventions(helios_window)
helios_target_set_folder(helios_window "Helios/Modules")
helios_target_set_output_dirs(helios_window)
helios_target_precompile_headers(helios_window src/pch.hpp)
helios_target_cpp_module_sources(helios_window modules/helios.window.cppm)

if(HELIOS_CURRENT_MODULE_BUILD_TESTS)
  helios_add_module_tests(helios_window
      SOURCES
          tests/main.cpp
          tests/module_import.cpp
          tests/native_handle.cpp
          tests/params.cpp
          tests/plugin.cpp
          tests/properties.cpp
          tests/schedules.cpp
          tests/settings.cpp
          tests/systems/sync_frame_limiter.cpp

      C_SOURCES
          capi/tests/c_smoke.c
  )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_compile_definitions(helios_window PUBLIC
      HELIOS_PLATFORM_LINUX_X11 HELIOS_PLATFORM_LINUX_WAYLAND
  )
endif()
