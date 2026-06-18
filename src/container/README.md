# `container` — Data-Oriented Containers

Header-only containers used throughout the engine: sparse sets for ECS storage, type-indexed maps for plugin/resource registries, and type-erased buffers for commands and resources.

## Public API

| Type                       | Purpose                                                                           |
| -------------------------- | --------------------------------------------------------------------------------- |
| `SparseSet<T>`             | O(1) insert/remove/lookup. Sparse + dense + reverse arrays; swap-and-pop removal. |
| `MultiTypeMap<Storage>`    | Type-indexed map keyed by `TypeIndex`. `Ensure<T>()`, `Get<T>()`, `ForEach()`.    |
| `TypedBuffer<T>`           | Type-erased single-instance storage. Fast path for trivially-copyable types.      |
| `TypedBufferArray<T>`      | Contiguous storage for multiple instances of one type. PMR support.               |
| `CallableBuffer<...>`      | Single-instance type-erased callable. `Set()`, `Invoke()`.                        |
| `CallableBufferArray<...>` | Inline array of heterogeneous callables without virtual dispatch.                 |
| `StaticString<N>`          | Stack-allocated fixed-capacity string (`BasicStaticString` alias).                |

## SparseSet

```cpp
#include <helios/container/sparse_set.hpp>

helios::container::SparseSet<int> set;

set.Insert(42, 100);          // index 42 → value 100
set.Insert(7, 200);

CHECK(set.Contains(42));
CHECK_EQ(set.Get(42), 100);

set.Remove(42);               // O(1) swap-and-pop

for (int value : set) {       // cache-friendly dense iteration
  // ...
}
```

## MultiTypeMap

```cpp
#include <helios/container/multi_type_map.hpp>

helios::container::MultiTypeMap<std::string> map;

map.Ensure<int>() = "integer slot";
map.Ensure<float>() = "float slot";

map.ForEach([](auto type_index, std::string& value) {
  // iterate all registered types
});
```

## TypedBuffer

```cpp
#include <helios/container/typed_buffer.hpp>

helios::container::TypedBuffer<> buffer;

buffer.Set<PhysicsConfig>({.gravity = -9.81F});
auto& config = buffer.Value<PhysicsConfig>();

buffer.Reset();                  // destroy value and clear type info
buffer.Set<RenderConfig>();      // type can change across lifetimes
```

## CallableBuffer

```cpp
#include <helios/container/callable_buffer.hpp>

helios::container::CallableBuffer<void(int&)> cmd;
cmd.Set([](int& x) { x += 1; });

int value = 0;
cmd.Invoke(value);
```

Multiple signatures:

```cpp
helios::container::CallableBuffer<void(World&), void(Logger&)> multi;
multi.Set<&Cmd::Execute, &Cmd::Log>(Cmd{data});
multi.Invoke<0>(world);
multi.Invoke<1>(logger);
```

## StaticString

```cpp
#include <helios/container/static_string.hpp>

helios::container::StaticString<64> name("Player");
CHECK_EQ(name.Size(), 6);
CHECK(name == "Player");
```

No heap allocation — storage lives on the stack.

## Dependencies

- `core` — asserts
- `utils` — `TypeIndex` for `MultiTypeMap`
- External: Boost `flat_map` (when `std::flat_map` unavailable)
