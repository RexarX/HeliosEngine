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
        helios::stduuid::stduuid stduuid::stduuid
        helios::stduuid::stduuid stduuid
)

if(TARGET helios::stduuid::stduuid AND NOT TARGET helios::stduuid)
  add_library(_helios_stduuid_all INTERFACE)
  target_link_libraries(_helios_stduuid_all INTERFACE helios::stduuid::stduuid)
  add_library(helios::stduuid ALIAS _helios_stduuid_all)
endif()
