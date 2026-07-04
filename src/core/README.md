# `core` — Core Engine Primitives

Foundational utilities used by every other module: assertions, UUIDs, stack traces, null-terminated string views.

## Public API

| Type               | Purpose                                                                                    |
| ------------------ | ------------------------------------------------------------------------------------------ |
| `Uuid`             | UUID value type. Thread-local MT19937 generation via `Uuid::Generate()`.                   |
| `UuidGenerator`    | Standalone UUID generator (multiple instances supported).                                  |
| `Stacktrace`       | Stack capture via C++23 `<stacktrace>` or Boost.Stacktrace fallback. Configurable filters. |
| `StacktraceConfig` | Configuration for stack trace capture (frames, skip, filters).                             |
| `CStringView`      | Null-terminated string view for APIs requiring `\0`.                                       |
| `WCStringView`     | Wide-char variant. Also `U8CStringView`, `U16CStringView`, `U32CStringView`.               |
| `AssertionHandler` | `void(*)(std::string_view, std::source_location, std::string_view)` — custom handler.      |

## Assert System

```cpp
#include <helios/assert.hpp>

HELIOS_ASSERT(ptr != nullptr);        // debug-only (no-op in release)
HELIOS_INVARIANT(count > 0);          // debug: assert; release: still checks
HELIOS_VERIFY(fclose(f));             // always evaluates (both debug/release)

// With formatted message (uses std::format)
HELIOS_ASSERT(size <= capacity, "Buffer overflow: {} > {}", size, capacity);
```

Priority-based handler dispatch: custom handler → log plugin (weak symbol) → default stderr.

Custom assertion handler:

```cpp
SetAssertionHandler([](std::string_view condition,
                       const std::source_location& loc,
                       std::string_view message) noexcept {
  // Custom logging / crash reporter
});
AbortWithStacktrace("Unrecoverable state detected");
```

## UUID

```cpp
#include <helios/uuid.hpp>

Uuid id = Uuid::Generate();           // thread-local random UUID
Uuid invalid;                         // default = invalid (all zeros)
Uuid from_str = Uuid("550e8400-e29b-41d4-a716-446655440000");

CHECK(id.Valid());
CHECK_NE(id, invalid);
CHECK_LT(id, other);                  // ordered comparison (for maps)
CHECK_EQ(id.Hash(), other.Hash());    // hashing

std::string str = id.ToString();      // canonical string representation
```

Thread-local Mersenne Twister 19937 ensures lock-free generation. Use `UuidGenerator` for explicit generator instances.

## Stacktrace

```cpp
#include <helios/stacktrace.hpp>

auto trace = Stacktrace::Capture();   // static factory with default config
std::string output = trace.ToString();

auto filtered = Stacktrace::Capture(StacktraceConfig{
    .max_frames = 32,
    .skip_frames = 2,
});
```

Configurable max frames, skip frames, include/exclude filters, and source-location anchoring. Uses C++23 `<stacktrace>` when available and falls back to Boost.Stacktrace when the standard library backend is missing.

## CStringView

```cpp
#include <helios/cstring_view.hpp>

CStringView name("Window");           // guaranteed null-terminated
WCStringView wname(L"Window");        // wide-char variant
const char* raw = name.c_str();       // safe to pass to C APIs
```

## Dependencies

- `compiler` — intrinsics
- `platform` — platform detection
- `utils` — macros, type utilities
- External: stduuid, Boost.Stacktrace fallback when needed
