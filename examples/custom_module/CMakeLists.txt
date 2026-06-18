helios_module(
    NAME greeting
    VERSION 0.1.0
    DESCRIPTION "Example custom Helios module (greeting utilities)"

    HEADERS
        include/helios/greeting/greeting.hpp

    SOURCES
        src/greeting.cpp

    DEPENDS
        PUBLIC core
        PUBLIC utils

    TEST_SOURCES
        tests/greeting.cpp
        tests/main.cpp
)

add_executable(greeting_demo src/demo.cpp)

helios_target_set_cxx_standard(greeting_demo STANDARD 23)
helios_target_set_warnings(greeting_demo)
helios_target_set_platform(greeting_demo)
helios_target_set_optimization(greeting_demo)
helios_target_set_output_dirs(greeting_demo CUSTOM_FOLDER examples)

helios_link_modules(
    TARGET greeting_demo
    MODULES
        PUBLIC greeting
        PUBLIC log
)
