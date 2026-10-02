helios_header_only_library(helios_compiler
    CPP_MODULE modules/helios.compiler.cppm
)

target_sources(helios_compiler ${HELIOS_CURRENT_MODULE_SCOPE}
    FILE_SET HEADERS BASE_DIRS include FILES
        include/helios/config.hpp
        include/helios/compiler/compiler.hpp
)

if(HELIOS_CURRENT_MODULE_BUILD_TESTS)
  helios_add_module_tests(helios_compiler SOURCES
      tests/compiler.cpp
      tests/main.cpp
      tests/module_import.cpp
  )
endif()
