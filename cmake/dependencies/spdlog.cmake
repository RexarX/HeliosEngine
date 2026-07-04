# Ubuntu's system spdlog links external fmt, which fails to compile with Clang
# (consteval format-string checks in spdlog/details/fmt_helper.h). Use the CPM
# build with std::format instead.
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if(NOT DEFINED HELIOS_FORCE_DOWNLOAD_SPDLOG)
    set(HELIOS_FORCE_DOWNLOAD_SPDLOG ON CACHE BOOL
        "Force CPM spdlog when Clang would otherwise pick a system fmt-backed package")
  endif()
endif()

helios_dependency(
    NAME spdlog
    VERSION "^1.12.0"

    INSTALL_HINTS
        apt libspdlog-dev
        dnf spdlog-devel
        pacman spdlog
        brew spdlog
        pkg_config spdlog

    CPM_REPOSITORY gabime/spdlog
    CPM_VERSION 1.16.0
    CPM_OPTIONS
        "SPDLOG_BUILD_SHARED OFF"
        "SPDLOG_BUILD_EXAMPLE OFF"
        "SPDLOG_BUILD_TESTS OFF"
        "SPDLOG_USE_STD_FORMAT ON"
        "SPDLOG_FMT_EXTERNAL OFF"

    ALIASES
        helios::lib::spdlog spdlog::spdlog
        helios::lib::spdlog spdlog::spdlog_header_only
        helios::lib::spdlog spdlog
        helios::lib::spdlog::spdlog_header_only spdlog::spdlog_header_only
        helios::lib::spdlog::spdlog_header_only spdlog::spdlog
        helios::lib::spdlog::spdlog_header_only spdlog
)

# Bridge fallback: if only compiled spdlog exists, create header-only compat alias
if(TARGET helios::lib::spdlog AND NOT TARGET helios::lib::spdlog::spdlog_header_only)
  add_library(_helios_spdlog_ho_compat INTERFACE)
  target_link_libraries(_helios_spdlog_ho_compat INTERFACE helios::lib::spdlog)
  add_library(helios::lib::spdlog::spdlog_header_only ALIAS _helios_spdlog_ho_compat)
  helios_mark_system_includes(_helios_spdlog_ho_compat)
endif()
