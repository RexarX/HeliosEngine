helios_header_only_library(helios_container
    CPP_MODULE modules/helios.container.cppm
)

target_sources(helios_container
    ${HELIOS_CURRENT_MODULE_SCOPE} FILE_SET HEADERS BASE_DIRS include FILES
        include/helios/container/details/callable_buffer_common.hpp
        include/helios/container/details/typed_buffer_common.hpp
        include/helios/container/callable_buffer.hpp
        include/helios/container/callable_buffer_array.hpp
        include/helios/container/container.hpp
        include/helios/container/flat_map.hpp
        include/helios/container/multi_type_map.hpp
        include/helios/container/sparse_set.hpp
        include/helios/container/static_string.hpp
        include/helios/container/typed_buffer.hpp
        include/helios/container/typed_buffer_array.hpp
)

if(HELIOS_CURRENT_MODULE_BUILD_TESTS)
  helios_add_module_tests(helios_container SOURCES
      tests/callable_buffer.cpp
      tests/callable_buffer_array.cpp
      tests/flat_map.cpp
      tests/main.cpp
      tests/module_import.cpp
      tests/multi_type_map.cpp
      tests/sparse_set.cpp
      tests/static_string.cpp
      tests/typed_buffer.cpp
      tests/typed_buffer_array.cpp
  )
endif()
