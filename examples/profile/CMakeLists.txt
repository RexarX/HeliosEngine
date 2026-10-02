if(HELIOS_BUILD_HELIOS_APP AND HELIOS_BUILD_HELIOS_PROFILE)
  helios_add_example(
      NAME profile
      SOURCES profile.cpp
      MODULES helios_app helios_ecs helios_log helios_profile
  )

  target_compile_definitions(profile_example PRIVATE HELIOS_ENABLE_PROFILE)
endif()
