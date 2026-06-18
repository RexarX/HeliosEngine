helios_dependency(
    NAME concurrentqueue
    VERSION "^1.0.0"

    INSTALL_HINTS
        apt concurrentqueue-dev
        brew concurrentqueue
        pkg_config concurrentqueue

    CPM_REPOSITORY cameron314/concurrentqueue
    CPM_VERSION 1.0.5

    ALIASES
        helios::concurrentqueue::concurrentqueue concurrentqueue::concurrentqueue
        helios::concurrentqueue::concurrentqueue concurrentqueue
)

if(TARGET helios::concurrentqueue::concurrentqueue AND NOT TARGET helios::concurrentqueue)
  add_library(_helios_concurrentqueue_all INTERFACE)
  target_link_libraries(_helios_concurrentqueue_all INTERFACE helios::concurrentqueue::concurrentqueue)
  add_library(helios::concurrentqueue ALIAS _helios_concurrentqueue_all)
endif()
