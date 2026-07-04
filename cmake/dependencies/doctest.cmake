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
    CPM_GIT_TAG v2.4.11
    CPM_OPTIONS
        "DOCTEST_WITH_TESTS OFF"
        "DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF"

    UMBRELLA_ALIAS helios::lib::doctest

    ALIASES
        helios::lib::doctest::doctest doctest::doctest
        helios::lib::doctest::doctest doctest
)
