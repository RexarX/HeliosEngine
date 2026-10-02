add_library(helios_sdl3_window)

target_sources(helios_sdl3_window PUBLIC
    FILE_SET HEADERS BASE_DIRS include FILES
        include/helios/sdl3/window/systems/apply.hpp
        include/helios/sdl3/window/systems/create.hpp
        include/helios/sdl3/window/systems/destroy.hpp
        include/helios/sdl3/window/systems/init.hpp
        include/helios/sdl3/window/systems/poll.hpp
        include/helios/sdl3/window/systems/shutdown.hpp
        include/helios/sdl3/window/event_handlers.hpp
        include/helios/sdl3/window/native_handle.hpp
        include/helios/sdl3/window/plugin.hpp
        include/helios/sdl3/window/state.hpp
        include/helios/sdl3/window/sync.hpp
        include/helios/sdl3/window/window.hpp
        include/helios/sdl3/window/window_map.hpp
)

target_sources(helios_sdl3_window PRIVATE
    src/systems/apply.cpp
    src/systems/create.cpp
    src/systems/destroy.cpp
    src/systems/init.cpp
    src/systems/poll.cpp
    src/systems/shutdown.cpp
    src/event_handlers.cpp
    src/native_handle.cpp
    src/plugin.cpp
    src/sync.cpp
)

if(HELIOS_BUILD_C_API)
  target_sources(helios_sdl3_window PUBLIC
      FILE_SET c_headers TYPE HEADERS BASE_DIRS capi/include FILES
          capi/include/helios/sdl3/window/window.h
  )
endif()

helios_require_dependency(SDL3)
target_link_libraries(helios_sdl3_window PRIVATE helios::lib::sdl3::sdl3)

helios_apply_conventions(helios_sdl3_window)
helios_target_set_folder(helios_sdl3_window "Helios/Modules")
helios_target_set_output_dirs(helios_sdl3_window)
helios_target_precompile_headers(helios_sdl3_window src/pch.hpp)
helios_target_cpp_module_sources(helios_sdl3_window modules/helios.sdl3.window.cppm)

if(HELIOS_CURRENT_MODULE_BUILD_TESTS)
  helios_add_module_tests(helios_sdl3_window
      SOURCES
          tests/systems/apply.cpp
          tests/systems/create.cpp
          tests/systems/destroy.cpp
          tests/systems/init.cpp
          tests/systems/poll.cpp
          tests/systems/shutdown.cpp
          tests/available.hpp
          tests/event_handlers.cpp
          tests/main.cpp
          tests/module_import.cpp
          tests/native_handle.cpp
          tests/native_windows.cpp
          tests/plugin.cpp
          tests/sync.cpp
          tests/window_map.cpp

      C_SOURCES
          capi/tests/c_smoke.c
  )
endif()

if(TARGET helios_sdl3_window_tests AND UNIX AND HELIOS_ENABLE_SANITIZERS AND
    HELIOS_SANITIZER_ADDRESS)
  set_property(TEST helios_sdl3_window_tests APPEND PROPERTY ENVIRONMENT
      "LSAN_OPTIONS=suppressions=${HELIOS_ROOT_DIR}/cmake/sdl3_lsan.supp:print_suppressions=0"
  )
endif()
