helios_dependency(
    NAME  glfw3
    VERSION "^3.0.0"

    INSTALL_HINTS
        apt libglfw3-dev
        dnf glfw-devel
        pacman glfw
        brew glfw
        pkg_config glfw3

    VENDORED_DIR ${HELIOS_THIRD_PARTY_DIR}/glfw

    CPM_REPOSITORY glfw/glfw
    CPM_GIT_TAG 3.5.1
    CPM_OPTIONS
        "GLFW_BUILD_DOCS OFF"
        "GLFW_BUILD_EXAMPLES OFF"
        "GLFW_BUILD_TESTS OFF"
        "GLFW_INSTALL OFF"

    UMBRELLA_ALIAS helios::lib::glfw

    ALIASES
        helios::lib::glfw::glfw glfw::glfw
        helios::lib::glfw::glfw glfw3
        helios::lib::glfw::glfw glfw
)
