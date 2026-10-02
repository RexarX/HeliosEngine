add_library(helios_input)

target_sources(helios_input PUBLIC FILE_SET HEADERS BASE_DIRS include FILES
    include/helios/input/systems/clear.hpp
    include/helios/input/systems/gamepad.hpp
    include/helios/input/systems/joystick.hpp
    include/helios/input/systems/keyboard.hpp
    include/helios/input/systems/mouse.hpp
    include/helios/input/systems/pen.hpp
    include/helios/input/systems/sensor.hpp
    include/helios/input/systems/touch.hpp
    include/helios/input/axis.hpp
    include/helios/input/button_input.hpp
    include/helios/input/gamepad.hpp
    include/helios/input/ids.hpp
    include/helios/input/input.hpp
    include/helios/input/joystick.hpp
    include/helios/input/keyboard.hpp
    include/helios/input/mouse.hpp
    include/helios/input/params.hpp
    include/helios/input/pen.hpp
    include/helios/input/plugin.hpp
    include/helios/input/sensor.hpp
    include/helios/input/settings.hpp
    include/helios/input/touch.hpp
)

target_sources(helios_input PRIVATE
    src/systems/clear.cpp
    src/systems/gamepad.cpp
    src/systems/joystick.cpp
    src/systems/keyboard.cpp
    src/systems/mouse.cpp
    src/systems/pen.cpp
    src/systems/sensor.cpp
    src/systems/touch.cpp
    src/plugin.cpp
)

if(HELIOS_BUILD_C_API)
  target_sources(helios_input PUBLIC
      FILE_SET c_headers TYPE HEADERS BASE_DIRS capi/include FILES
          capi/include/helios/input/input.h
  )
endif()

helios_apply_conventions(helios_input)
helios_target_set_folder(helios_input "Helios/Modules")
helios_target_set_output_dirs(helios_input)
helios_target_precompile_headers(helios_input src/pch.hpp)
helios_target_cpp_module_sources(helios_input modules/helios.input.cppm)

if(HELIOS_CURRENT_MODULE_BUILD_TESTS)
  helios_add_module_tests(helios_input
      SOURCES
          tests/systems/clear.cpp
          tests/systems/gamepad.cpp
          tests/systems/joystick.cpp
          tests/systems/keyboard.cpp
          tests/systems/mouse.cpp
          tests/systems/pen.cpp
          tests/systems/sensor.cpp
          tests/systems/touch.cpp
          tests/axis.cpp
          tests/button_input.cpp
          tests/gamepad.cpp
          tests/joystick.cpp
          tests/keyboard.cpp
          tests/main.cpp
          tests/module_import.cpp
          tests/mouse.cpp
          tests/params.cpp
          tests/pen.cpp
          tests/plugin.cpp
          tests/sensor.cpp
          tests/touch.cpp

      C_SOURCES
          capi/tests/c_smoke.c
  )
endif()
