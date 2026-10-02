helios_header_only_library(helios_platform
    CPP_MODULE modules/helios.platform.cppm
)

target_sources(helios_platform ${HELIOS_CURRENT_MODULE_SCOPE}
    FILE_SET HEADERS BASE_DIRS include FILES
        include/helios/platform/detail/platform.h
        include/helios/platform/platform.hpp
)

if(HELIOS_BUILD_C_API)
  target_sources(helios_platform ${HELIOS_CURRENT_MODULE_SCOPE}
      FILE_SET c_headers TYPE HEADERS BASE_DIRS capi/include FILES
          capi/include/helios/platform/platform.h
  )
endif()

if(HELIOS_CURRENT_MODULE_BUILD_TESTS)
  helios_add_module_tests(helios_platform
      SOURCES
          tests/main.cpp
          tests/module_import.cpp
          tests/platform.cpp
      C_SOURCES
          capi/tests/c_smoke.c
  )
endif()
