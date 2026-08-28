if(HELIOS_BUILD_SDL3_WINDOW)
  helios_add_example(
      NAME window
      SOURCES window.cpp
      MODULES app ecs log window sdl3_window
  )
endif()
