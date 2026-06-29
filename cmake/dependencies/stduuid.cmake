helios_dependency(
    NAME stduuid
    VERSION "^1.0.0"

    INSTALL_HINTS
        dnf stduuid-devel

    CPM_REPOSITORY mariusbancila/stduuid
    CPM_VERSION 1.2.3
    CPM_OPTIONS
        "STDUUID_BUILD_TESTS OFF"

    ALIASES
        helios::lib::stduuid::stduuid stduuid::stduuid
        helios::lib::stduuid::stduuid stduuid
)

if(TARGET helios::lib::stduuid::stduuid AND NOT TARGET helios::lib::stduuid)
  add_library(_helios_stduuid_all INTERFACE)
  target_link_libraries(_helios_stduuid_all INTERFACE helios::lib::stduuid::stduuid)
  add_library(helios::lib::stduuid ALIAS _helios_stduuid_all)
endif()
