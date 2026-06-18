# `utils` — Common Utilities

Header-only helpers shared across modules: compile-time type identification, type-erased delegates, scope guards, timers, random number generation, filesystem I/O, and lazy iterator adapters.

## Public API

| Type / Function                                                           | Purpose                                                                 |
| ------------------------------------------------------------------------- | ----------------------------------------------------------------------- |
| `TypeId` / `TypeIndex`                                                    | Compile-time type hashing (FNV-1a on type name).                        |
| `Delegate<R(Args...)>`                                                    | Non-owning type-erased free/member function wrapper.                    |
| `FastPimpl<T, Size, Align>`                                               | Stack-based pimpl with `consteval` size/alignment validation.           |
| `Defer<F>` / `HELIOS_DEFER`                                               | Scope-guard deferred execution.                                         |
| `Fnv1aHash` / `HashType`                                                  | FNV-1a hashing (`kFnvBasis`, `kFnvPrime`).                              |
| `Timer<Clock>`                                                            | High-resolution elapsed-time measurement.                               |
| `RandomGenerator<Engine>`                                                 | Typed random value generation.                                          |
| `DefaultEngine()` / `FastEngineInstance()`                                | Thread-local random engines.                                            |
| `ReadFileToString()`                                                      | Read entire file into `std::string` via `std::expected`.                |
| `DynamicLibrary`                                                          | Cross-platform shared library loading.                                  |
| `StringHash` / `StringEqual`                                              | Transparent hash/equality for heterogeneous `unordered_map` lookup.     |
| `UniqueTypes` / `AllConvertibleTo`                                        | Compile-time type-list traits.                                          |
| `FilterAdapter`, `MapAdapter`, `TakeAdapter`, `SkipAdapter`, …            | Lazy iterator adapters with chained `.Filter()`, `.Map()`, `.Take()`, … |
| `HELIOS_BIT`, `HELIOS_STRINGIFY`, `HELIOS_CONCAT`, `HELIOS_ANONYMOUS_VAR` | Utility macros (`macro.hpp`).                                           |

## Type Identification

```cpp
#include <helios/utils/type_info.hpp>

using Id = helios::utils::TypeId::From<MyComponent>();
using Index = helios::utils::TypeIndex::From<MyComponent>();

CHECK_EQ(Id.Hash(), Index.Hash());
CHECK_EQ(Id.QualifiedName(), "MyComponent");  // compiler-dependent
```

`TypeIndex` is a lightweight 64-bit hash; `TypeId` also carries the qualified type name string.

## Delegate

```cpp
#include <helios/utils/delegate.hpp>

void FreeFn(int x) { /* ... */ }

struct Handler {
  void OnEvent(int x) { /* ... */ }
};

auto free = helios::utils::Delegate<void(int)>::FromFunction<&FreeFn>();
free.Invoke(42);
free(42);  // operator() alias

Handler h;
auto member =
    helios::utils::Delegate<void(int)>::FromFunction<&Handler::OnEvent>(h);
member(42);

// optional helpers — deduce signature from the function pointer
auto deduced = helios::utils::DelegateFromFunction<&FreeFn>();

CHECK(free.Valid());
free.Reset();
```

`FromFunction` binds free functions (no instance) or member functions (pass `this`). For overloaded or explicit signatures use `FromFunction<ReturnType(*)(Args...), &Func>()` or `DelegateFromFunction<Signature, &Func>()`.

Non-owning, no heap allocation, exception-free. Empty delegates return default-constructed values (non-void) or no-op (void).

## Scope Guard

```cpp
#include <helios/utils/defer.hpp>

HELIOS_DEFER {
  fclose(file);
  ReleaseResource();
};

auto cleanup = [ptr]() { delete ptr; };
HELIOS_DEFER_CALL(cleanup);
```

## Timer

```cpp
#include <helios/utils/timer.hpp>

helios::utils::Timer timer;
// ... work ...
auto elapsed = timer.Elapsed();  // std::chrono::duration
```

## Random

```cpp
#include <helios/utils/random.hpp>

helios::utils::DefaultRandomGenerator gen(helios::utils::DefaultEngine());
int roll = gen.Value(1, 6);

float unit = gen.Value();  // [0.0, 1.0)
```

## Filesystem

```cpp
#include <helios/utils/filesystem.hpp>

auto result = helios::utils::ReadFileToString("config.json");
if (result) {
  ProcessJson(*result);
} else {
  // handle FileError::kCouldNotOpen / kReadError
}
```

## Dynamic Library

```cpp
#include <helios/utils/dynamic_library.hpp>

helios::utils::DynamicLibrary lib;
auto loaded = lib.Load("my_plugin.dll");
if (loaded) {
  auto* create = lib.GetSymbol<CreateFn>("helios_plugin_create");
}
```

## Iterator Adapters

Adapters wrap a range or iterator pair and chain via member methods (not pipe syntax). Start with a constructor adapter, then call `.Filter()`, `.Map()`, `.Take()`, and so on; finish with a terminal such as `.Collect()`:

```cpp
#include <helios/utils/functional_adapters.hpp>

std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

auto result = helios::utils::FilterAdapter(data, [](int v) { return v % 2 == 0; })
                  .Map([](int v) { return v * 2; })
                  .Take(3)
                  .Collect();
// {4, 8, 12}
```

ECS queries use the same chaining style: `query.Filter(pred).Map(fn).Collect()`.

Other adapters include `SkipAdapter`, `EnumerateAdapter`, `StepByAdapter`, `ReverseAdapter`, `ChainAdapter`, and more. Terminal operations: `Collect`, `Fold`, `Find`, `Count`, `Any`, `All`, `None`, `Partition`, `GroupBy`, `MaxBy`, `MinBy`.

## Dependencies

- `compiler` — intrinsics, feature detection
- `platform` — platform macros
