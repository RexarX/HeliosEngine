# Helios Engine Examples

Focused executables that demonstrate one engine feature at a time. Each example
auto-exits after a short run and logs its lifecycle to the console.

## Build

From the repository root (`HeliosEngine/`), with MSVC environment active:

```bat
cmake --preset windows-msvc-debug -DHELIOS_DEVELOPER_MODE=ON -DHELIOS_BUILD_PROFILE=ON
cmake --build --preset windows-msvc-debug
```

Build a single example:

```bat
cmake --build --preset windows-msvc-debug --target simple_example
```

Binaries are written to `bin/examples/<config>-<os>-<arch>/`.

## Examples

| Directory               | Target                         | Modules              | Demonstrates                                         |
| ----------------------- | ------------------------------ | -------------------- | ---------------------------------------------------- |
| `simple/`               | `simple_example`               | app, log             | Minimal `App`, resources, systems, exit message      |
| `profile/`              | `profile_example`              | app, profile, log    | Tracy + custom logging backend, scopes, sub-apps     |
| `profile_global_alloc/` | `profile_global_alloc_example` | profile, memory, log | Global `new`/`delete` hooks in Tracy Memory tab      |
| `sub_apps/`             | `sub_apps_example`             | app, log             | Blocking sub-app extract/update                      |
| `overlapping_sub_apps/` | `overlapping_sub_apps_example` | app, log             | Overlapping sub-app updates                          |
| `async_sub_apps/`       | `async_sub_apps_example`       | app, log             | Async background sub-app                             |
| `plugins/`              | `plugins_example`              | app, log             | Static plugin lifecycle                              |
| `dynamic_plugins/`      | `dynamic_plugins_example`      | app, log             | Shared-library plugin loading                        |
| `ecs/schedules/`        | `schedules_example`            | app, log             | Custom schedule ordering in update stage             |
| `ecs/stages/`           | `stages_example`               | app, log             | Custom `Scheduler` stages via `App`                  |
| `ecs/systems/`          | `systems_example`              | app, log             | System sets, ordering, run conditions                |
| `ecs/commands/`         | `commands_example`             | app, log             | Deferred `Commands` + custom commands                |
| `ecs/components/`       | `components_example`           | app, log             | Archetype vs sparse-set components                   |
| `ecs/entities/`         | `entities_example`             | app, log             | Entity create/reserve/destroy/generation             |
| `ecs/messages/`         | `messages_example`             | app, log             | Regular + async messages                             |
| `ecs/queries/`          | `queries_example`              | app, log             | `Query` filters, adapters, optional components       |
| `ecs/resources/`        | `resources_example`            | app, log             | Resources, lifecycle callbacks, thread-safe resource |

## Other examples

- `custom_module/` — out-of-tree Helios module (registered via root CMake, not
  `examples/CMakeLists.txt`). See [`custom_module/README.md`](custom_module/README.md).

## CMake helper

`examples/cmake/ExampleUtils.cmake` defines `helios_add_example()` to reduce
boilerplate for new executables.
