helios_register_module(
    NAME profile
    DESCRIPTION "CPU/memory/lock profiling with composable backends"
    DEPENDS compiler platform utils core container
    DEFAULT OFF
)
