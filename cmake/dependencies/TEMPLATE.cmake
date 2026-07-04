# TEMPLATE.cmake — Minimal dependency file template for helios_dependency
#
# Copy this file to cmake/dependencies/<name>.cmake and fill in the fields.
#
# The helios_require_dependency mechanism automatically includes this file
# when a module declares USES <name> ... or calls helios_require_dependency(<name>).

helios_dependency(
    NAME packagename
    VERSION "^1.0.0"

    INSTALL_HINTS
        apt libpackage-dev
        dnf package-devel
        pacman package
        brew package
        pkg_config package

    CPM_REPOSITORY owner/repo
    CPM_VERSION 1.0.0
    CPM_OPTIONS
        "BUILD_TESTS OFF"
        "BUILD_SHARED OFF"

    # Per-dependency CMAKE_BUILD_TYPE override (Debug, RelWithDebInfo, Release).
    # Falls back to HELIOS_DEPENDENCY_BUILD_TYPE when not set.
    # BUILD_TYPE RelWithDebInfo

    # Ordered fallback chain — first existing target wins for the alias
    ALIASES
        helios::lib::package package::package
        helios::lib::package package
)

# For advanced cases that cannot be expressed via helios_dependency, use the
# internal _helios_dep_begin / _helios_dep_end pair and document why the file
# cannot stay declarative.
