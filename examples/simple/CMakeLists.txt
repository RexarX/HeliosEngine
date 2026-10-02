if(HELIOS_BUILD_HELIOS_APP)
  helios_add_example(
      NAME simple
      SOURCES simple.cpp
      MODULES helios_app helios_ecs helios_log
  )
endif()
