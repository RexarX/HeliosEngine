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
    CPM_VERSION 4.0.0
    CPM_GIT_TAG v4.0.0
    CPM_OPTIONS
        "TF_BUILD_TESTS OFF"
        "TF_BUILD_EXAMPLES OFF"

    ALIASES
        helios::taskflow::taskflow Taskflow::Taskflow
        helios::taskflow::taskflow Taskflow
)

if(TARGET helios::taskflow::taskflow AND NOT TARGET helios::taskflow)
  add_library(_helios_taskflow_all INTERFACE)
  target_link_libraries(_helios_taskflow_all INTERFACE helios::taskflow::taskflow)
  add_library(helios::taskflow ALIAS _helios_taskflow_all)
endif()
