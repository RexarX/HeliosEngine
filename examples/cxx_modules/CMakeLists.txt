if(HELIOS_BUILD_APP AND HELIOS_ENABLE_CPP_MODULES)
  helios_add_example(
      NAME cxx_modules
      SOURCES cxx_modules.cpp
      MODULES app ecs log
      CXX_MODULES
  )
endif()
