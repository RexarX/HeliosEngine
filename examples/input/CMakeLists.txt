if(HELIOS_BUILD_SDL3_WINDOW AND HELIOS_BUILD_SDL3_INPUT)
  helios_add_example(
      NAME input
      SOURCES input.cpp
      MODULES app ecs log window input sdl3_window sdl3_input
  )
endif()
