# Third-party dependencies

Runtime and test libraries are **vendored as release source trees** (not git
submodules). Helios resolves them via `VENDORED_DIR` paths in
`cmake/dependencies/*.cmake`, rooted at `HELIOS_THIRD_PARTY_DIR` (default:
this directory).

Point CMake at another tree with:

```bash
cmake --preset ... -DHELIOS_THIRD_PARTY_DIR=/path/to/your/deps
```

## Vendored packages

| Directory          | Upstream                   | Pinned tag |
| ------------------ | -------------------------- | ---------- |
| `concurrentqueue/` | cameron314/concurrentqueue | `v1.0.5`   |
| `doctest/`         | doctest/doctest            | `v2.5.3`   |
| `glfw/`            | glfw/glfw                  | `3.5.1`    |
| `mimalloc/`        | microsoft/mimalloc         | `v3.4.4`   |
| `spdlog/`          | gabime/spdlog              | `v1.17.0`  |
| `stduuid/`         | mariusbancila/stduuid      | `v1.2.3`   |
| `Taskflow/`        | taskflow/taskflow          | `v4.1.0`   |
| `tracy/`           | wolfpld/tracy              | `v0.13.1`  |

Each directory must contain the upstream `CMakeLists.txt` at its root.

## Overrides

Per package (example: `spdlog`):

| Option                         | Effect                                     |
| ------------------------------ | ------------------------------------------ |
| (default)                      | Use `${HELIOS_THIRD_PARTY_DIR}/spdlog`     |
| `HELIOS_FORCE_DOWNLOAD_SPDLOG` | Fetch via CPM from the configured repo/tag |
| `HELIOS_USE_SYSTEM_SPDLOG`     | Use system `find_package` / pkg-config     |

If both force-download and use-system are set, force-download wins.

## Refreshing a package

1. Download the release/tag archive from GitHub.
2. Replace `<HELIOS_THIRD_PARTY_DIR>/<dir>/` with the extracted sources (keep the directory name).
3. Update `CPM_GIT_TAG` / `CPM_VERSION` in `cmake/dependencies/<name>.cmake` to match.
4. Update the pin in this README.
