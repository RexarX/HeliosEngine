helios_register_module(
    NAME async
    DESCRIPTION "Asynchronous task execution module built on Taskflow"
    DEFAULT ON
    HEADER_ONLY
    DEPENDS core
    OPTIONAL_DEPENDS profile
)
