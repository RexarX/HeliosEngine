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
        helios::lib::taskflow::taskflow Taskflow::Taskflow
        helios::lib::taskflow::taskflow Taskflow
)

if(TARGET helios::lib::taskflow::taskflow AND NOT TARGET helios::lib::taskflow)
  add_library(_helios_taskflow_all INTERFACE)
  target_link_libraries(_helios_taskflow_all INTERFACE helios::lib::taskflow::taskflow)
  add_library(helios::lib::taskflow ALIAS _helios_taskflow_all)
endif()
