# Project Guidelines

Coding standards, module layout, testing, and tooling for HeliosEngine development.

---

## Code rules

### Non-negotiable

- **C++23** throughout — use standard library features aggressively.
- **Google Style Guide** — naming, formatting, and organizational conventions (enforced via `.clang-format`).
- **No exceptions** — use `HELIOS_ASSERT` for invariants; `std::expected<T, ErrorEnum>` or `std::optional` for recoverable failures.
- **No backward-compat debt** — change or remove deprecated code freely.
- **Include what you use** — every file includes exactly what it needs.
- **Priority order:** performance → simplicity → maintainability.
- **Avoid locks in hot paths** — prefer atomics and lock-free structures.

### Types and APIs

- `std::span` over raw pointer + size for array parameters.
- `std::string_view` for read-only string parameters.
- `std::reference_wrapper` can be used instead of raw pointers or references to existing objects.
- `auto` when the type is obvious, or when the return type exceeds ~30 characters — use trailing return type in that case.
- `explicit` on single-arg constructors and conversion operators; conditional `explicit` when it depends on a template parameter.
- `final` on all non-base classes; `override` (not `virtual`) in derived classes.
- Template constraints: concepts > `static_assert` > SFINAE.
- Conditional `noexcept` when it depends on template parameters.
- `std::ranges` algorithms over manual loops wherever possible.

### Naming

- Concise accessor names: `Empty()` not `IsEmpty()`, `Size()` not `GetSize()`.
- Use `Set*` prefix only when the name would otherwise be ambiguous (e.g. `SetSize()`).

---

## Class and struct layout

Members and methods must appear in this order:

1. Constructors
2. Destructor
3. Copy / move assignment operators
4. Main methods
5. Operator overloads
6. Setters
7. Bool methods (only ones that check something)
8. Getters

Keep related methods together unless doing so would violate the order above.

---

## Method implementation

- If a method body exceeds 1 line, declare it in the class and define it outside.
- Header-file out-of-line definitions → add `inline` (or `constexpr`, which is implicitly inline — do not combine both).

---

## Documentation

Use **Doxygen** for all public API. Document private API only when not self-explanatory.

### Tag order

Omit tags that do not apply:

```
@brief → @details → @note → @warning → @tparam → @param → @return → @throw → @code / @endcode
```

- If a function triggers `HELIOS_ASSERT`, document the triggering condition in `@warning`.
- Use `@code` / `@endcode` for inline usage examples (do not prefix with `@example` — Doxygen treats `@example` as an external file reference and will discard the doc block).

---

## Module structure

Every module under `src/` follows this layout:

```
src/<module>/
├── CMakeLists.txt           # helios_module(...) call
├── Module.cmake             # helios_register_module(...) call
├── README.md                # Module overview and usage
├── include/helios/<module>/ # Public headers
├── src/                     # Private sources + pch.hpp
└── tests/                   # doctest test files + main.cpp
```

### Registration (`Module.cmake`)

```cmake
helios_register_module(
    NAME ecs
    DESCRIPTION "Entity Component System module"
    DEFAULT ON
    DEPENDS async compiler container core log memory utils
    OPTIONAL_DEPENDS profile
)
```

- `DEFAULT ON/OFF` — whether the module builds unless explicitly overridden.
- `DEPENDS` — required Helios modules (auto-enabled).
- `OPTIONAL_DEPENDS` — integrated when present (e.g. `profile`).

### Build definition (`CMakeLists.txt`)

```cmake
helios_module(
    NAME ecs
    VERSION 0.1.0
    DESCRIPTION "Entity Component System module"
    HEADERS ...
    SOURCES ...
    PCH src/pch.hpp
    DEPENDS
        PUBLIC async
        PUBLIC core
        ...
    USES
        concurrentqueue PUBLIC helios::concurrentqueue::concurrentqueue
    TEST_SOURCES
        tests/main.cpp
        ...
)
```

External dependencies are declared per-module via `USES` and resolved from `cmake/dependencies/` (system packages first, CPM fallback).

### Selective module builds

```bash
cmake --preset linux-gcc-debug -DHELIOS_BUILD_WINDOW=OFF
cmake --preset linux-gcc-debug -DHELIOS_BUILD_PROFILE=ON
```

---

## Testing

- **Framework:** doctest — one test binary per module, driven by `tests/main.cpp`.
- Test files mirror source layout under each module's `tests/` directory.
- Write tests only for non-trivial behavior.

### Structure

```cpp
TEST_SUITE("helios::module::ClassName") {
  TEST_CASE("ClassName::methodName") {
    SUBCASE("description") {
      ...
    }
  }
}
```

- Test order should mirror class definition order.
- All public methods should be covered.

### Check macros

| Comparison     | Macro             |
| -------------- | ----------------- |
| `==`           | `CHECK_EQ`        |
| `!=`           | `CHECK_NE`        |
| `<`            | `CHECK_LT`        |
| `<=`           | `CHECK_LE`        |
| `>`            | `CHECK_GT`        |
| `>=`           | `CHECK_GE`        |
| `!x`           | `CHECK_FALSE`     |
| Floating point | `doctest::Approx` |

Never use plain `CHECK` where a specialized macro exists.

### Running tests

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug
```

---

## Building

### Prerequisites

| Tool      | Minimum                         |
| --------- | ------------------------------- |
| CMake     | 3.25+                           |
| Compiler  | GCC 14+, Clang 19+, MSVC 19.34+ |
| Generator | Ninja (recommended)             |

### CMake presets

Primary workflow — pattern: `{os}-{compiler}-{build_type}`

```bash
cmake --list-presets
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug
```

---

## Developer scripts

| Script      | Purpose                                                  |
| ----------- | -------------------------------------------------------- |
| `format.py` | `clang-format` on `src/`, `examples/` (`--check` for CI) |
| `lint.py`   | `clang-tidy` — requires `compile_commands.json`          |
| `docs.py`   | Build Doxygen HTML documentation (`docs/doxygen/html/`)  |

Theme: [doxygen-awesome-css](https://github.com/jothepro/doxygen-awesome-css) base layout with dark mode, copy buttons, interactive TOC, and Markdown tabs. Build after submodule init: `git submodule update --init third-party/doxygen-awesome-css`.

```bash
python scripts/format.py
python scripts/lint.py --build-dir build/linux-gcc-debug
python scripts/docs.py
```

### Formatting enforcement

| When          | Mechanism                                                                   |
| ------------- | --------------------------------------------------------------------------- |
| Local commits | `pre-commit install` — runs `python scripts/format.py` (auto-fixes sources) |
| Push / PR     | `.github/workflows/format.yaml` — `python scripts/format.py --check`        |

```bash
pip install pre-commit
pre-commit install
python scripts/format.py          # format manually
python scripts/format.py --check  # CI-equivalent check
```

---

## Further reading

| Document                  | Audience                           |
| ------------------------- | ---------------------------------- |
| [README.md](../README.md) | Project overview and quick start   |
| `src/<module>/README.md`  | Per-module public API and examples |
