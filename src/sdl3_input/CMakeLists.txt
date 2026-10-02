add_library(helios_sdl3_input)

target_sources(helios_sdl3_input PUBLIC FILE_SET HEADERS BASE_DIRS include FILES
    include/helios/sdl3/input/systems/apply_cursors.hpp
    include/helios/sdl3/input/systems/apply_raw_mouse.hpp
    include/helios/sdl3/input/systems/init.hpp
    include/helios/sdl3/input/systems/poll_gamepads.hpp
    include/helios/sdl3/input/systems/poll_sensors.hpp
    include/helios/sdl3/input/cursor_cache.hpp
    include/helios/sdl3/input/event_handlers.hpp
    include/helios/sdl3/input/input.hpp
    include/helios/sdl3/input/plugin.hpp
    include/helios/sdl3/input/state.hpp
)

target_sources(helios_sdl3_input PRIVATE
    src/details/input_map.hpp
    src/systems/apply_cursors.cpp
    src/systems/apply_raw_mouse.cpp
    src/systems/init.cpp
    src/systems/poll_gamepads.cpp
    src/systems/poll_sensors.cpp
    src/event_handlers.cpp
    src/plugin.cpp
)

if(HELIOS_BUILD_C_API)
  target_sources(helios_sdl3_input PUBLIC
      FILE_SET c_headers TYPE HEADERS BASE_DIRS capi/include FILES
          capi/include/helios/sdl3/input/input.h
  )
endif()

helios_require_dependency(SDL3)
target_link_libraries(helios_sdl3_input PRIVATE helios::lib::sdl3::sdl3)

helios_apply_conventions(helios_sdl3_input)
helios_target_set_folder(helios_sdl3_input "Helios/Modules")
helios_target_set_output_dirs(helios_sdl3_input)
helios_target_precompile_headers(helios_sdl3_input src/pch.hpp)
helios_target_cpp_module_sources(helios_sdl3_input modules/helios.sdl3.input.cppm)

if(HELIOS_CURRENT_MODULE_BUILD_TESTS)
  helios_add_module_tests(helios_sdl3_input
      SOURCES
          tests/systems/apply_cursors.cpp
          tests/systems/apply_raw_mouse.cpp
          tests/systems/init.cpp
          tests/systems/poll_gamepads.cpp
          tests/systems/poll_sensors.cpp
          tests/available.hpp
          tests/event_handlers.cpp
          tests/main.cpp
          tests/module_import.cpp
          tests/plugin.cpp

      C_SOURCES
          capi/tests/c_smoke.c
  )
endif()

if(TARGET helios_sdl3_input_tests AND UNIX AND HELIOS_ENABLE_SANITIZERS AND
    HELIOS_SANITIZER_ADDRESS)
  set_property(TEST helios_sdl3_input_tests APPEND PROPERTY ENVIRONMENT
      "LSAN_OPTIONS=suppressions=${HELIOS_ROOT_DIR}/cmake/sdl3_lsan.supp:print_suppressions=0"
  )
endif()
