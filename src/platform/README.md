# `platform` — Platform Detection & API Macros

Header-only module providing platform identification and DLL visibility macros. Leaf module with no dependencies.

## Public API

Macros only — no classes, structs, or functions.

## Usage

```cpp
#include <helios/platform/platform.hpp>

class HELIOS_API MyClass { };             // DLL export/import

HELIOS_DEBUG_BREAK();                     // architecture-specific breakpoint

#if defined(HELIOS_PLATFORM_WINDOWS)
  // Windows-specific code
#elif defined(HELIOS_PLATFORM_LINUX)
  // Linux-specific code
#elif defined(HELIOS_PLATFORM_MACOS)
  // macOS-specific code
#endif
```

## Macros

| Macro                     | Purpose                                                                                         |
| ------------------------- | ----------------------------------------------------------------------------------------------- |
| `HELIOS_API`              | `__declspec(dllexport/dllimport)` on Windows; `__attribute__((visibility("default")))` on Unix. |
| `HELIOS_EXPORT`           | Explicit export annotation (used for plugin entry points).                                      |
| `HELIOS_DEBUG_BREAK()`    | `__debugbreak()` on MSVC, `__builtin_trap()` on GCC/Clang.                                      |
| `HELIOS_PLATFORM_WINDOWS` | Defined when `_WIN32` is set.                                                                   |
| `HELIOS_PLATFORM_LINUX`   | Defined when `__linux__` is set.                                                                |
| `HELIOS_PLATFORM_MACOS`   | Defined when `__APPLE__` is set.                                                                |
