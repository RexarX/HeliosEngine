helios_dependency(
    NAME stduuid
    VERSION "^1.0.0"

    INSTALL_HINTS
        dnf stduuid-devel

    CPM_REPOSITORY mariusbancila/stduuid
    CPM_VERSION 1.2.3
    CPM_OPTIONS
        "STDUUID_BUILD_TESTS OFF"

    UMBRELLA_ALIAS helios::lib::stduuid

    ALIASES
        helios::lib::stduuid::stduuid stduuid::stduuid
        helios::lib::stduuid::stduuid stduuid
)
