helios_dependency(
    NAME mimalloc
    VERSION "^3.0.0"

    INSTALL_HINTS
        apt libmimalloc-dev
        dnf mimalloc-devel
        pacman mimalloc
        brew mimalloc
        pkg_config mimalloc

    CPM_REPOSITORY microsoft/mimalloc
    CPM_GIT_TAG v3.3.2
    CPM_OPTIONS
        "MI_BUILD_SHARED OFF"
        "MI_BUILD_STATIC ON"
        "MI_BUILD_OBJECT OFF"
        "MI_BUILD_TESTS OFF"
        "MI_OVERRIDE OFF"

    ALIASES
        helios::lib::mimalloc::static mimalloc-static
        helios::lib::mimalloc mimalloc-static
)
