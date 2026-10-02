# Helios CMake Architecture

Helios CMake is organized as small helpers composed into higher-level behavior.
The rule of thumb is Linux-like: each function does one simple job well, and
larger functions read as recipes made from those smaller pieces.

## Public API Index

Module authoring:

- `module.toml` plus `helios_declare_module` / `_helios_finalize_module` (generated)
- `helios_add_extra_module_dirs(...)` in `HeliosPrelude.cmake`
- `helios_add_modules(...)` for an installed Helios
- `helios_apply_conventions(...)`, `helios_header_only_library(...)`
- `helios_link_modules(...)`

Dependencies:

- `helios_dependency(...)`
- `helios_require_dependency(<name>)`
- `helios_parse_version_constraint(...)`

Targets and tests:

- `helios_target_set_*`
- `helios_target_reuse_module_pch(...)`
- `helios_add_test_executable(...)`
- `helios_add_module_test(...)`
- `helios_add_integration_test(...)`

Primitives:

- `helios_parse_visibility(...)`
- `helios_target_apply(...)`
- `helios_mark_system_includes(...)`
- `helios_copy_shared_lib(...)`

## Adding A Module

Put `module.toml` next to `CMakeLists.txt`. The tool resolves the graph. CMake creates the target and calls opt-in helpers. See `agents/modules/manifest.md`.

```cmake
add_library(helios_foo)
target_sources(helios_foo
    PRIVATE src/foo.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS include FILES
        include/helios/foo/foo.hpp
)
helios_require_dependency(spdlog)
target_link_libraries(helios_foo PRIVATE helios::lib::spdlog::spdlog_header_only)
helios_apply_conventions(helios_foo)
helios_target_precompile_headers(helios_foo src/pch.hpp)
```

## Adding A Dependency

Prefer `helios_dependency()` for normal packages. Vendored packages declare
`VENDORED_DIR` as an absolute path (typically under `HELIOS_THIRD_PARTY_DIR`):

```cmake
helios_dependency(
    NAME doctest
    VERSION "^2.0.0"
    VENDORED_DIR ${HELIOS_THIRD_PARTY_DIR}/doctest
    CPM_REPOSITORY doctest/doctest
    CPM_GIT_TAG v2.5.3
    UMBRELLA_ALIAS helios::lib::doctest
    ALIASES
        helios::lib::doctest::doctest doctest::doctest
        helios::lib::doctest::doctest doctest
)
```

`HELIOS_THIRD_PARTY_DIR` defaults to `${HELIOS_ROOT_DIR}/third-party`; point it
elsewhere with `-DHELIOS_THIRD_PARTY_DIR=...`. Relative `VENDORED_DIR` values are
resolved against that root. Overrides for a vendored package:
`HELIOS_FORCE_DOWNLOAD_<PKG>=ON` (CPM network) or `HELIOS_USE_SYSTEM_<PKG>=ON`
(`find_package`). Force-download wins if both are set. Packages without
`VENDORED_DIR` keep the system→CPM fallback chain.

Use a custom dependency script only when the package needs feature probes,
non-standard include normalization, platform-specific behavior, or multiple
wrapper targets that cannot be expressed clearly by the declarative API.
