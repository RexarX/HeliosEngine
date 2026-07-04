helios_dependency(
    NAME Taskflow
    VERSION "^4.0.0"

    INSTALL_HINTS
        apt libtaskflow-cpp-dev
        dnf taskflow-devel
        pacman taskflow
        brew taskflow
        pkg_config taskflow

    CPM_REPOSITORY taskflow/taskflow
    CPM_GIT_TAG v4.0.0
    CPM_OPTIONS
        "TF_BUILD_TESTS OFF"
        "TF_BUILD_EXAMPLES OFF"

    UMBRELLA_ALIAS helios::lib::taskflow

    ALIASES
        helios::lib::taskflow::taskflow Taskflow::Taskflow
        helios::lib::taskflow::taskflow Taskflow
)
