if(HELIOS_BUILD_C_API)
  add_library(helios_capi)

  target_sources(helios_capi PRIVATE src/error.cpp)
  target_sources(helios_capi PUBLIC
      FILE_SET c_headers TYPE HEADERS BASE_DIRS capi/include FILES
          capi/include/helios/capi/capi.h
          capi/include/helios/capi/decl.h
          capi/include/helios/capi/error.h
  )

  helios_apply_conventions(helios_capi)
  helios_target_set_folder(helios_capi "Helios/Modules")
  helios_target_set_output_dirs(helios_capi)
  helios_target_cpp_module_sources(helios_capi modules/helios.capi.cppm)
else()
  helios_header_only_library(helios_capi
      CPP_MODULE modules/helios.capi.cppm
  )

  helios_target_set_folder(helios_capi "Helios/Modules")
  if(HELIOS_ENABLE_CPP_MODULES)
    helios_target_set_output_dirs(helios_capi)
  endif()
endif()

if(HELIOS_CURRENT_MODULE_BUILD_TESTS)
  helios_add_module_tests(helios_capi C_SOURCES tests/c_smoke.c)
endif()
