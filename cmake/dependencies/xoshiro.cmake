helios_dependency(
    NAME xoshiro
    VERSION "^1.0.0"

    INSTALL_HINTS
        pkg_config xoshiro

    VENDORED_DIR ${HELIOS_THIRD_PARTY_DIR}/xoshiro

    CPM_REPOSITORY nessan/xoshiro
    CPM_VERSION v1.2.0

    ALIASES
        helios::lib::xoshiro xoshiro::xoshiro
        helios::lib::xoshiro xoshiro
)
