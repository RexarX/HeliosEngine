helios_module(
    NAME log
    VERSION 0.1.0
    DESCRIPTION "Logging module built on spdlog"

    HEADERS
        include/helios/log/log.hpp
        include/helios/log/config.hpp
        include/helios/log/logger.hpp

    SOURCES
        src/logger.cpp

    PCH src/pch.hpp

    DEPENDS
        PUBLIC container
        PUBLIC core
        PUBLIC utils

    USES
        spdlog PRIVATE helios::lib::spdlog::spdlog_header_only

    COMPILE_DEFINITIONS
        PRIVATE
            SPDLOG_HEADER_ONLY

    TEST_DEPENDENCIES
        helios::lib::spdlog::spdlog_header_only

    TEST_SOURCES
        tests/config.cpp
        tests/logger.cpp
        tests/main.cpp
)
