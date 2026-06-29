helios_dependency(
    NAME doctest
    VERSION "^2.0.0"

    INSTALL_HINTS
        apt doctest-dev
        dnf doctest-devel
        pacman doctest
        brew doctest
        pkg_config doctest

    CPM_REPOSITORY doctest/doctest
    CPM_VERSION 2.4.11
    CPM_GIT_TAG v2.4.11
    CPM_OPTIONS
        "DOCTEST_WITH_TESTS OFF"
        "DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF"

    ALIASES
        helios::lib::doctest::doctest doctest::doctest
        helios::lib::doctest::doctest doctest
)

if(TARGET helios::lib::doctest::doctest AND NOT TARGET helios::lib::doctest)
  add_library(_helios_doctest_all INTERFACE)
  target_link_libraries(_helios_doctest_all INTERFACE helios::lib::doctest::doctest)
  add_library(helios::lib::doctest ALIAS _helios_doctest_all)
endif()
