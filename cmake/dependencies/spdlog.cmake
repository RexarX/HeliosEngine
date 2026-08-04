# Prefer vendored third-party/spdlog (SPDLOG_USE_STD_FORMAT). System spdlog on
# some distros links external fmt and can fail with Clang; use
# HELIOS_USE_SYSTEM_SPDLOG=ON only when the system package is known-good.

helios_dependency(
    NAME spdlog
    VERSION "^1.12.0"

    INSTALL_HINTS
        apt libspdlog-dev
        dnf spdlog-devel
        pacman spdlog
        brew spdlog
        pkg_config spdlog

    VENDORED_DIR ${HELIOS_THIRD_PARTY_DIR}/spdlog

    CPM_REPOSITORY gabime/spdlog
    CPM_VERSION 1.17.0
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
