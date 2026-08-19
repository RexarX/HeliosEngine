helios_dependency(
    NAME SDL3
    VERSION "^3.4.0"

    INSTALL_HINTS
        apt libsdl3-dev
        dnf SDL3
        pacman sdl3
        brew sdl3
        pkg_config sdl3

    VENDORED_DIR ${HELIOS_THIRD_PARTY_DIR}/SDL3

    CPM_REPOSITORY libsdl-org/SDL
    CPM_GIT_TAG release-3.4.14
    CPM_OPTIONS
        "SDL_SHARED OFF"
        "SDL_STATIC ON"
        "SDL_TEST_LIBRARY OFF"
        "SDL_TESTS OFF"
        "SDL_EXAMPLES OFF"
        "SDL_INSTALL OFF"
        "SDL_UNINSTALL OFF"

    UMBRELLA_ALIAS helios::lib::sdl3

    ALIASES
        helios::lib::sdl3::sdl3 SDL3::SDL3
        helios::lib::sdl3::sdl3 SDL3::SDL3-static
        helios::lib::sdl3::sdl3 SDL3::SDL3-shared
        helios::lib::sdl3::sdl3 SDL3
)
