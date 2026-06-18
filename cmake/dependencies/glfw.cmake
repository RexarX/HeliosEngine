helios_dependency(
    NAME  glfw3
    VERSION "^3.0.0"

    INSTALL_HINTS
        apt libglfw3-dev
        dnf glfw-devel
        pacman glfw
        brew glfw
        pkg_config glfw3

    CPM_REPOSITORY glfw/glfw
    CPM_VERSION 3.4
    CPM_GIT_TAG 3.4
    CPM_OPTIONS
        "GLFW_BUILD_DOCS OFF"
        "GLFW_BUILD_EXAMPLES OFF"
        "GLFW_BUILD_TESTS OFF"
        "GLFW_INSTALL OFF"

    ALIASES
        helios::glfw::glfw glfw::glfw
        helios::glfw::glfw glfw3
        helios::glfw::glfw glfw
)

if(TARGET helios::glfw::glfw AND NOT TARGET helios::glfw)
  add_library(_helios_glfw_all INTERFACE)
  target_link_libraries(_helios_glfw_all INTERFACE helios::glfw::glfw)
  add_library(helios::glfw ALIAS _helios_glfw_all)
endif()
