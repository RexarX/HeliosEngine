# `log` — Logging

Type-tagged logging built on [spdlog](https://github.com/gabime/spdlog). Singleton `Logger` with per-type logger registration, console + file output, async mode, and rotation. Integrates with the assert system via a weak-symbol handler when linked.

## Public API

| Type                    | Purpose                                                                 |
| ----------------------- | ----------------------------------------------------------------------- |
| `Logger`                | Singleton (`Logger::Instance()`). Registers backends, routes log calls. |
| `Config`                | Output configuration: console/file patterns, rotation, async settings.  |
| `Level`                 | `kTrace`, `kDebug`, `kInfo`, `kWarn`, `kError`, `kCritical`.            |
| `DefaultLogger`         | Built-in logger type (`kName = "HELIOS"`).                              |
| `kDefaultLogger`        | Constexpr instance of `DefaultLogger`.                                  |
| `LoggerTrait`           | Concept: empty struct with `static constexpr std::string_view kName`.   |
| `LoggerWithConfigTrait` | Extends `LoggerTrait` with `static constexpr Config kConfig`.           |

### Free Functions

Convenience wrappers around `Logger::Instance()`:

```cpp
helios::log::Trace("...");
helios::log::Debug("...");
helios::log::Info("...");
helios::log::Warn("...");
helios::log::Error("...");
helios::log::Critical("...");
```

All accept `std::format_string` overloads and typed-logger variants: `Info(MyLogger{}, "msg")`.

`Trace` and `Debug` are compiled out in release builds (`HELIOS_RELEASE_MODE`).

## Quick Start

```cpp
#include <helios/log/logger.hpp>

helios::log::Info("Engine starting");
helios::log::Warn("Low memory: {} MB free", free_mb);
helios::log::Error("Failed to load '{}'", path);
```

No explicit initialization required — the default logger is created on first use with `Config::Debug()` (debug) or `Config::Release()` (release).

## Custom Logger Types

```cpp
struct RenderLogger {
  static constexpr std::string_view kName = "Render";
  static constexpr auto kConfig = helios::log::Config::ConsoleOnly();
};

helios::log::Logger::Instance().AddLogger(RenderLogger{});
helios::log::Info(RenderLogger{}, "Pipeline initialized");
```

## Configuration

```cpp
#include <helios/log/config.hpp>

auto config = helios::log::Config{
    .log_directory = "logs",
    .file_name_pattern = "{name}_{timestamp}.log",
    .enable_console = true,
    .enable_file = true,
    .async_logging = false,
    .max_file_size = 10 * 1024 * 1024,
    .max_files = 5,
};

// Presets
helios::log::Config::Default();
helios::log::Config::ConsoleOnly();
helios::log::Config::FileOnly();
helios::log::Config::Debug();
helios::log::Config::Release();
```

## Runtime Control

```cpp
auto& logger = helios::log::Logger::Instance();

logger.SetLevel(helios::log::Level::kWarn);
logger.SetLevel(RenderLogger{}, helios::log::Level::kDebug);
logger.FlushAll();
logger.RemoveLogger(RenderLogger{});
```

## Assert Integration

When the `log` module is linked, assertion failures route through the log plugin handler (priority: custom handler → log plugin → stderr). On MSVC the handler is registered at initialization; on GCC/Clang it uses a weak symbol.

```cpp
helios::SetAssertionHandler(
    [](std::string_view condition, const std::source_location& loc,
       std::string_view message) noexcept {
      helios::log::Critical("Assert: {} | {} [{}:{}]", condition, message,
                            loc.file_name(), loc.line());
    });
```

## Dependencies

- `container` — `MultiTypeMap` for logger registry
- `core` — asserts
- `utils` — `TypeIndex`
- External: spdlog
