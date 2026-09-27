if(HELIOS_BUILD_HELIOS_SDL3_WINDOW AND HELIOS_BUILD_HELIOS_SDL3_INPUT)
  helios_add_example(
      NAME input
      SOURCES input.cpp
      MODULES helios_app helios_ecs helios_log helios_window helios_input helios_sdl3_window helios_sdl3_input
  )
endif()
