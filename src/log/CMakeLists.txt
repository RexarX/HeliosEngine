add_library(helios_log)

target_sources(helios_log PUBLIC FILE_SET HEADERS BASE_DIRS include FILES
    include/helios/log/log.hpp
    include/helios/log/config.hpp
    include/helios/log/logger.hpp
)

target_sources(helios_log PRIVATE
    src/logger.cpp
)

if(HELIOS_BUILD_C_API)
  target_sources(helios_log PUBLIC
      FILE_SET c_headers TYPE HEADERS BASE_DIRS capi/include FILES
          capi/include/helios/log/log.h
  )
endif()

helios_require_dependency(spdlog)
target_link_libraries(helios_log PRIVATE
    helios::lib::spdlog::spdlog_header_only
)

target_compile_definitions(helios_log PRIVATE
    SPDLOG_HEADER_ONLY
)

helios_apply_conventions(helios_log)
helios_target_set_folder(helios_log "Helios/Modules")
helios_target_set_output_dirs(helios_log)
helios_target_precompile_headers(helios_log src/pch.hpp)
helios_target_cpp_module_sources(helios_log modules/helios.log.cppm)

if(HELIOS_CURRENT_MODULE_BUILD_TESTS)
  helios_add_module_tests(helios_log
      SOURCES
          tests/config.cpp
          tests/logger.cpp
          tests/main.cpp
          tests/module_import.cpp

      C_SOURCES
          capi/tests/c_smoke.c

      DEPENDENCIES
          helios::lib::spdlog::spdlog_header_only
  )
endif()
