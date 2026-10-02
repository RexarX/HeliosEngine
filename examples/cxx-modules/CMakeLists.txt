if(HELIOS_BUILD_HELIOS_APP AND HELIOS_ENABLE_CPP_MODULES)
  # MSVC: importing helios.app and instantiating App hits Taskflow C1116 in the BMI.
  if(MSVC)
    message(STATUS "Skipping cxx_modules example on MSVC (see README C++ modules)")
  else()
    helios_add_example(
        NAME cxx_modules
        SOURCES cxx_modules.cpp
        MODULES helios_app helios_ecs helios_log
        CXX_MODULES
    )
  endif()
endif()
