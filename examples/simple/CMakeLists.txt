if(HELIOS_BUILD_APP)
  helios_add_example(
      NAME simple
      SOURCES simple.cpp
      MODULES app ecs log
  )
endif()
