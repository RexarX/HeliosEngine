add_library(example_greeting)

target_sources(example_greeting
    PUBLIC FILE_SET HEADERS BASE_DIRS include FILES
        include/helios/greeting/greeting.hpp
)

target_sources(example_greeting
    PRIVATE
        src/greeting.cpp
)

helios_apply_conventions(example_greeting)
helios_target_set_folder(example_greeting "Helios/Modules")
helios_target_set_output_dirs(example_greeting)
helios_target_cpp_module_sources(example_greeting modules/helios.greeting.cppm)

if(HELIOS_CURRENT_MODULE_BUILD_TESTS)
    helios_add_module_tests(example_greeting
        SOURCES
            tests/greeting.cpp
            tests/main.cpp
            tests/module_import.cpp
    )
endif()

add_executable(greeting_demo src/demo.cpp)

helios_target_set_cxx_standard(greeting_demo STANDARD 23)
helios_target_set_warnings(greeting_demo)
helios_target_set_platform(greeting_demo)
helios_target_set_optimization(greeting_demo)
helios_target_set_output_dirs(greeting_demo CUSTOM_FOLDER examples)

helios_link_modules(
    TARGET greeting_demo
    MODULES
        PUBLIC example_greeting
        PUBLIC helios_log
)
