if(HELIOS_BUILD_HELIOS_APP AND HELIOS_ENABLE_CPP_MODULES)
  helios_add_example(
      NAME cxx_modules
      SOURCES cxx_modules.cpp
      MODULES helios_app helios_ecs helios_log
      CXX_MODULES
  )
endif()
