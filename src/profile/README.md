# `profile` — CPU/Memory/Lock Profiling

Composable profiling with a backend-neutral public API. Tracy is the primary
backend; custom backends plug in via the `Backend` interface. **Opt-in module**
(`DEFAULT OFF`).

## Public API

| Type                | Purpose                                                                 |
| ------------------- | ----------------------------------------------------------------------- |
| `Profiler`          | Singleton. Registers backends, partitions zone storage, fans out.       |
| `Backend`           | Abstract base with virtual hooks + `ZoneStorageSize()`.                 |
| `TracyBackend`      | Tracy profiler GUI adapter (enabled via `HELIOS_PROFILE_BUNDLE_TRACY`). |
| `FlamegraphBackend` | Chrome Tracing JSON output. Configurable via `FlamegraphBackendConfig`. |
| `ScopedZone`        | RAII zone. Fixed buffer sliced per backend.                             |
| `ZoneSpec`          | Zone metadata (name, color, callstack depth).                           |
| `PlotFormat`        | Enum: `kNumber`, `kMemory`, `kPercentage`, `kWatts`.                    |

### Free Functions

```cpp
FrameMark();                              // frame mark (default name)
FrameMarkNamed("Render");                 // named frame
FrameMarkStart("Physics");               // split region start
FrameMarkEnd("Physics");                 // split region end
Message("Event!", 0x00FF00);             // timeline message with color
SetThreadName("Worker-3");               // label a thread
Plot("FrameTime", 16.6F);                // plot a value
PlotConfig("FPS", kNumber, /*step=*/true, /*fill=*/false, /*color=*/0);
Alloc(ptr, size);                        // track allocation
Free(ptr);                               // track deallocation
MemoryDiscard("FrameArena");             // arena/pool discard
```

## Quick Start

```cpp
#include <helios/profile/profile.hpp>
#include <helios/profile/backends/tracy.hpp>

// Startup (single-threaded):
auto& profiler = Profiler::Instance();
profiler.AddBackend<TracyBackend>();
profiler.Finalize();

// Game loop:
HELIOS_PROFILE_FRAME();
HELIOS_PROFILE_SCOPE_N("Update");
{
  HELIOS_PROFILE_SCOPE_N("Physics");
  // ...
}
```

Connect the [Tracy profiler GUI](https://github.com/wolfpld/tracy) to a Debug or RelWithDebInfo build.

## Macro Reference

| Macro                                         | Purpose                                                         |
| --------------------------------------------- | --------------------------------------------------------------- |
| `HELIOS_PROFILE_SCOPE()`                      | Function-named zone                                             |
| `HELIOS_PROFILE_SCOPE_N(name)`                | Named zone                                                      |
| `HELIOS_PROFILE_SCOPE_NS(name, depth)`        | Named zone with stack-capture depth                             |
| `HELIOS_PROFILE_SCOPE_C(color)`               | Colored zone                                                    |
| `HELIOS_PROFILE_SCOPE_NC(name, color)`        | Named + colored zone                                            |
| `HELIOS_PROFILE_SCOPE_IF(name, cond)`         | Conditional zone (runtime check)                                |
| `HELIOS_PROFILE_SCOPE_S(depth)`               | Zone with stack-capture depth                                   |
| `HELIOS_PROFILE_ZONE_TEXT(txt)`               | Attach text to active zone                                      |
| `HELIOS_PROFILE_ZONE_VALUE(val)`              | Attach numeric value                                            |
| `HELIOS_PROFILE_ZONE_NAME(name)`              | Rename active zone                                              |
| `HELIOS_PROFILE_FRAME()`                      | Frame boundary                                                  |
| `HELIOS_PROFILE_FRAME_N(name)`                | Named frame boundary                                            |
| `HELIOS_PROFILE_FRAME_START(name)`            | Split frame region start                                        |
| `HELIOS_PROFILE_FRAME_END(name)`              | Split frame region end                                          |
| `HELIOS_PROFILE_MESSAGE(msg)`                 | Timeline message                                                |
| `HELIOS_PROFILE_SET_THREAD_NAME(name)`        | Label a thread                                                  |
| `HELIOS_PROFILE_PLOT(name, value)`            | Plot a value over time                                          |
| `HELIOS_PROFILE_PLOT_CONFIG(...)`             | Plot configuration                                              |
| `HELIOS_PROFILE_ALLOC(ptr, size)`             | Track allocation                                                |
| `HELIOS_PROFILE_ALLOC_N(name, size)`          | Named allocation tracking                                       |
| `HELIOS_PROFILE_FREE(ptr)`                    | Track deallocation                                              |
| `HELIOS_PROFILE_FREE_N(name, ptr)`            | Named free tracking                                             |
| `HELIOS_PROFILE_MEMORY_DISCARD(name)`         | Arena/pool discard                                              |
| `HELIOS_PROFILE_MEMORY_DISCARD_S(n, d)`       | Discard with stack-capture depth                                |
| `HELIOS_MEMORY_OVERRIDE_GLOBAL_ALLOC` (CMake) | Inherit global `operator new` / `delete` hooks via memory links |

All macros are no-ops when `HELIOS_ENABLE_PROFILE` is undefined.

### Memory in Tracy

Open the **Memory** tab in the Tracy GUI. Link `helios::module::memory` (with `HELIOS_MEMORY_OVERRIDE_GLOBAL_ALLOC=ON`) to hook `new`/`delete`, or use `HELIOS_PROFILE_ALLOC` / `HELIOS_MEMORY_PROFILE_*` on custom allocators. Global hooks fan out through `Profiler::Alloc` / `Free` like all other memory events. Call `Profiler::Finalize()` before the workload you want to profile.

## Multi-Backend

```cpp
profiler.AddBackend<TracyBackend>();
profiler.AddBackend<FlamegraphBackend>();  // Chrome trace JSON
profiler.Finalize();
// Both backends receive all profiling events — macro usage unchanged.
```

Typed backend access:

```cpp
auto& tracy = Profiler::Instance().Get<TracyBackend>();
if (auto* custom = Profiler::Instance().TryGet<MyBackend>()) {
  // optional access
}
```

## Lock Profiling (Tracy-specific)

```cpp
#include <helios/profile/tracy/lock.hpp>

HELIOS_PROFILE_LOCKABLE(std::mutex, mtx_);
HELIOS_PROFILE_SHARED_LOCKABLE(std::shared_mutex, shared_mtx_);
HELIOS_PROFILE_LOCK_MARK(mtx_);          // mark specific lock event
HELIOS_PROFILE_LOCK_NAME(mtx_, "UI");    // rename lock for UI

// Base type variant:
class MyMutex { HELIOS_PROFILE_LOCKABLE_BASE(std::mutex) };
```

Lock profiling bypasses the backend abstraction because Tracy instruments mutex types at declaration sites.

## Custom Backends

Subclass `Backend` and register before `Finalize()`:

```cpp
class MyBackend final : public Backend {
public:
  [[nodiscard]] size_t ZoneStorageSize() const noexcept override {
    return sizeof(MyState);
  }

  void BeginZone(const ZoneSpec& spec, std::span<std::byte> storage) noexcept override {
    new (storage.data()) MyState{spec};
  }

  void EndZone(std::span<std::byte> storage) noexcept override {
    std::destroy_at(std::launder(reinterpret_cast<MyState*>(storage.data())));
  }

  [[nodiscard]] std::string_view Name() const noexcept override { return "my"; }
};

profiler.AddBackend<MyBackend>();
profiler.Finalize();
```

Ensure total `ZoneStorageSize()` across all backends fits within
`HELIOS_PROFILE_ZONE_STORAGE_BYTES` (default 256).
Tune via `-DHELIOS_PROFILE_ZONE_STORAGE_BYTES=512`.

## Build

```bash
cmake -DHELIOS_BUILD_PROFILE=ON ..
```

`HELIOS_ENABLE_PROFILE` is `ON` by default in Debug and RelWithDebInfo.
Force in Release with `-DHELIOS_ENABLE_PROFILE=ON`.
Tracy is auto-fetched via CPM if not installed system-wide.

## Thread Safety

| API                               | Safe when                     |
| --------------------------------- | ----------------------------- |
| `AddBackend`, `Finalize`, `Clear` | Single-threaded startup only  |
| Profiler fan-out, zone macros     | After `Finalize()`            |
| `Clear`                           | No active zones on any thread |

## Dependencies

- `compiler` — intrinsics
- `container` — MultiTypeMap
- `core` — asserts, CStringView
- `platform` — platform detection
- `utils` — type info, macros
- External: Tracy
