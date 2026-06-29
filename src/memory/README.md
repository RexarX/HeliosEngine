# `memory` — PMR Allocators & Reference Counting

Custom `std::pmr::memory_resource` allocators for frame-scoped, arena, pool, and general-purpose allocation, plus `Rc`/`Arc` smart pointers.

Namespace: `helios::mem`.

## Public API

### Allocators

| Allocator                | Strategy                                           | Thread safety                                |
| ------------------------ | -------------------------------------------------- | -------------------------------------------- |
| `ArenaAllocator`         | Growable lock-free bump allocator.                 | Lock-free allocate                           |
| `FixedArenaAllocator`    | Fixed-capacity bump allocator.                     | Lock-free allocate                           |
| `StackAllocator`         | Growable LIFO-aware allocator with marker/rewind.  | Lock-free allocate + LIFO deallocate on head |
| `FixedStackAllocator`    | Fixed-capacity stack allocator.                    | Lock-free allocate + LIFO deallocate         |
| `PoolAllocator`          | Growable fixed-size-block pool.                    | Lock-free                                    |
| `FixedPoolAllocator`     | Fixed-capacity block pool.                         | Lock-free                                    |
| `FreeListAllocator`      | Growable best-fit allocator with coalescing.       | Mutex-protected                              |
| `FixedFreeListAllocator` | Fixed-capacity best-fit allocator.                 | Mutex-protected                              |
| `FrameAllocator<N = 2>`  | N growable arenas; `Advance()` rotates and resets. | Delegates to arena                           |

### Reference Counting

| Type                               | Purpose                                                     |
| ---------------------------------- | ----------------------------------------------------------- |
| `Rc<T>`                            | Non-atomic reference-counted handle (`RefCounted` alias).   |
| `Arc<T>`                           | Atomic reference-counted handle (`AtomicRefCounted` alias). |
| `PmrRc<T>` / `PmrArc<T>`           | PMR allocator variants.                                     |
| `RcFromThis<T>` / `ArcFromThis<T>` | CRTP bases embedding ref counters.                          |
| `MakeRc<T>(args...)`               | Factory for `Rc<T>`.                                        |
| `MakeArc<T>(args...)`              | Factory for `Arc<T>`.                                       |
| `MakeRcWith<T>(alloc, args...)`    | Factory with custom allocator.                              |
| `MakeArcWith<T>(alloc, args...)`   | Factory with custom allocator.                              |

### Utilities

| Type / Function                | Purpose                                              |
| ------------------------------ | ---------------------------------------------------- |
| `GrowthPolicy`                 | Growable allocator policy (`Linear` or `Geometric`). |
| `AllocatorStats`               | Snapshot of allocation counters and capacity.        |
| `AlignedAlloc` / `AlignedFree` | Cross-platform aligned allocation.                   |

## mimalloc and global allocation

| CMake option                          | Default | Effect                                                              |
| ------------------------------------- | ------- | ------------------------------------------------------------------- |
| `HELIOS_MEMORY_USE_MIMALLOC`          | ON      | Routes backing `AlignedAlloc` / growable `Create*` through mimalloc |
| `HELIOS_MEMORY_ENABLE_PROFILE`        | ON      | Enables memory-module profile macros when profile module is linked  |
| `HELIOS_MEMORY_OVERRIDE_GLOBAL_ALLOC` | ON      | Inherits global `operator new`/`delete` hooks when linking memory   |

Global `operator new`/`delete` hooks are inherited automatically when you link `helios::module::memory`. The hook implementation lives in the memory module and reports allocations to the profiler when the profile module is linked and `HELIOS_MEMORY_ENABLE_PROFILE=ON`. Disable with `-DHELIOS_MEMORY_OVERRIDE_GLOBAL_ALLOC=OFF`.

## Arena Allocator

```cpp
#include <helios/memory/arena_allocator.hpp>

helios::mem::ArenaAllocator arena(64 * 1024);
std::pmr::polymorphic_allocator<std::byte> alloc(&arena);

auto* block = alloc.allocate(256);
// no individual deallocate — memory reclaimed on Reset() or destruction
arena.Reset();
```

## Frame Allocator

```cpp
#include <helios/memory/frame_allocator.hpp>

helios::mem::FrameAllocator frames(64 * 1024);
std::pmr::polymorphic_allocator<std::byte> alloc(&frames);

auto* data = alloc.allocate(128);  // current frame

frames.Advance();  // next frame; previous arena reset
```

## Pool Allocator

```cpp
#include <helios/memory/pool_allocator.hpp>

helios::mem::PoolAllocator pool(/*block_size=*/64, /*block_count=*/1024);
std::pmr::polymorphic_allocator<MyStruct> alloc(&pool);

auto* obj = alloc.new_object<MyStruct>(args...);
alloc.delete_object(obj);
```

## Reference Counting

```cpp
#include <helios/memory/ref_counted.hpp>

class Mesh final : public helios::mem::RcFromThis<Mesh> {
public:
  explicit Mesh(int id) : id_(id) {}
  int Id() const { return id_; }
private:
  int id_ = 0;
};

auto mesh = helios::mem::MakeRc<Mesh>(42);
auto copy = mesh;  // shared ownership (single-threaded ref count)

class Texture final : public helios::mem::ArcFromThis<Texture> {};
auto tex = helios::mem::MakeArc<Texture>();  // thread-safe ref count
```

## Dependencies

- `core` — asserts
- `mimalloc` (default ON) — backing heap via `helios::lib::mimalloc::static`
- `profile` (optional) — Tracy scope/zone/lock and backing ALLOC/FREE macros
