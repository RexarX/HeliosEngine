# `memory` — PMR Allocators & Reference Counting

Custom `std::pmr::memory_resource` allocators for frame-scoped, arena, pool, and general-purpose allocation, plus `Rc`/`Arc` smart pointers.

Namespace: `helios::mem`.

## Public API

### Allocators

| Allocator              | Strategy                                                    | Thread safety      |
| ---------------------- | ----------------------------------------------------------- | ------------------ |
| `ArenaAllocator`       | Lock-free bump-pointer; individual deallocation is a no-op. | Lock-free allocate |
| `StackAllocator`       | LIFO-aware with marker/rewind.                              | Lock-free          |
| `PoolAllocator`        | Fixed-size blocks, intrusive freelist (atomic CAS).         | Lock-free          |
| `FreeListAllocator`    | Best-fit with coalescing.                                   | Mutex-protected    |
| `NFrameAllocator<N>`   | N arenas; `Advance()` rotates and resets.                   | Delegates to arena |
| `FrameAllocator`       | Alias for `NFrameAllocator<1>`.                             |                    |
| `DoubleFrameAllocator` | Alias for `NFrameAllocator<2>`.                             |                    |

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

| Type / Function                | Purpose                                                |
| ------------------------------ | ------------------------------------------------------ |
| `GrowthPolicy`                 | Shared growth config (`Fixed`, `Linear`, `Geometric`). |
| `AllocatorStats`               | Snapshot of allocation counters and capacity.          |
| `AlignedAlloc` / `AlignedFree` | Cross-platform aligned allocation.                     |

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

helios::mem::DoubleFrameAllocator frames(64 * 1024);
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
- `platform` — aligned allocation backends
