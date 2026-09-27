if(HELIOS_BUILD_HELIOS_SDL3_WINDOW)
  helios_add_example(
      NAME window
      SOURCES window.cpp
      MODULES helios_app helios_ecs helios_log helios_window helios_sdl3_window
  )
endif()
