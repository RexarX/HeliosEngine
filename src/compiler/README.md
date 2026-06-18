# `compiler` — Compiler Detection & Intrinsics

Header-only module providing compiler-specific macros and feature-detection. Leaf module with no dependencies.

## Public API

Macros only — no classes, structs, or functions.

## Usage

```cpp
#include <helios/compiler/compiler.hpp>

if (HELIOS_EXPECT_TRUE(ptr != nullptr)) {
  // __builtin_expect hints the branch predictor on GCC/Clang
}

#if HELIOS_MOVEONLY_FUNCTION_AVAILABLE
  using Fn = std::move_only_function<void()>;
#else
  using Fn = std::function<void()>;
#endif
```

## Macros

| Macro                                | Purpose                                                         |
| ------------------------------------ | --------------------------------------------------------------- |
| `HELIOS_EXPECT_TRUE(x)`              | Branch-predictor hint for likely-true paths.                    |
| `HELIOS_EXPECT_FALSE(x)`             | Branch-predictor hint for likely-false paths.                   |
| `HELIOS_FORCE_INLINE`                | Always inline (equivalent to `__attribute__((always_inline))`). |
| `HELIOS_ALWAYS_INLINE`               | Same as HELIOS_FORCE_INLINE.                                    |
| `HELIOS_NO_INLINE`                   | Prevent inlining.                                               |
| `HELIOS_MOVEONLY_FUNCTION_AVAILABLE` | Feature-test for `std::move_only_function`.                     |
| `HELIOS_CONTAINERS_RANGES_AVAILABLE` | Feature-test for C++23 `<ranges>` on containers.                |
| `HELIOS_STL_FLAT_MAP_AVAILABLE`      | Feature-test for `std::flat_map`.                               |
