add_library(helios_utils)

target_sources(helios_utils PUBLIC FILE_SET HEADERS BASE_DIRS include FILES
    include/helios/utils/utils.hpp
    include/helios/utils/common_traits.hpp
    include/helios/utils/defer.hpp
    include/helios/utils/dynamic_library.hpp
    include/helios/utils/fast_pimpl.hpp
    include/helios/utils/filesystem.hpp
    include/helios/utils/format.hpp
    include/helios/utils/functional_adapters.hpp
    include/helios/utils/hash.hpp
    include/helios/utils/macro.hpp
    include/helios/utils/random.hpp
    include/helios/utils/sleep.hpp
    include/helios/utils/string_hash.hpp
    include/helios/utils/timer.hpp
    include/helios/utils/type_info.hpp
)

target_sources(helios_utils PRIVATE
    src/dynamic_library.cpp
    src/sleep.cpp
)

if(HELIOS_BUILD_C_API)
  target_sources(helios_utils PUBLIC
      FILE_SET c_headers TYPE HEADERS BASE_DIRS capi/include FILES
          capi/include/helios/utils/utils.h
  )
endif()

helios_require_dependency(xoshiro)
target_link_libraries(helios_utils PUBLIC helios::lib::xoshiro)

helios_apply_conventions(helios_utils)
helios_target_set_folder(helios_utils "Helios/Modules")
helios_target_set_output_dirs(helios_utils)
helios_target_precompile_headers(helios_utils src/pch.hpp)
helios_target_cpp_module_sources(helios_utils modules/helios.utils.cppm)

if(HELIOS_CURRENT_MODULE_BUILD_TESTS)
  helios_add_module_tests(helios_utils
      SOURCES
          tests/common_traits.cpp
          tests/defer.cpp
          tests/dynamic_library.cpp
          tests/fast_pimpl.cpp
          tests/filesystem.cpp
          tests/format.cpp
          tests/functional_adapters.cpp
          tests/hash.cpp
          tests/macro.cpp
          tests/main.cpp
          tests/module_import.cpp
          tests/random.cpp
          tests/sleep.cpp
          tests/string_hash.cpp
          tests/timer.cpp
          tests/type_info.cpp
      C_SOURCES
          capi/tests/c_smoke.c
  )
endif()
