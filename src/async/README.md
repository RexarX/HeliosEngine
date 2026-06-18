# `async` — Task-Based Parallelism

Work-stealing thread pool and task dependency graphs built on [Taskflow](https://taskflow.github.io/taskflow/). Used by the ECS scheduler and the `app` module for parallel system execution.

## Public API

| Type           | Purpose                                                                                   |
| -------------- | ----------------------------------------------------------------------------------------- |
| `Executor`     | Thread pool. Runs `TaskGraph`s, fires independent async tasks, drains pending work.       |
| `TaskGraph`    | DAG of tasks. `EmplaceTask`, `ForEach`, `Reduce`, `Sort`, `Compose`, `Dump` (DOT export). |
| `Task`         | Handle within a graph. `Precede` / `Succeed` for ordering, `Work` to assign callables.    |
| `SubTaskGraph` | Dynamic subflow — create tasks from within a running task. `Join()` signals completion.   |
| `AsyncTask`    | Handle to an independently scheduled task. `Done()` checks completion.                    |
| `Future<T>`    | Move-only result handle wrapping `tf::Future`. `Get()`, `Wait()`, `Cancel()`.             |

### `Executor` Methods

| Method                                | Purpose                                              |
| ------------------------------------- | ---------------------------------------------------- |
| `Run(graph)`                          | Execute a task graph once; returns `Future<void>`.   |
| `RunN(graph, count)`                  | Execute a graph N times.                             |
| `RunUntil(graph, predicate)`          | Execute until predicate returns true.                |
| `Async(callable)`                     | Fire independent task; returns `std::future`.        |
| `SilentAsync(callable)`               | Fire-and-forget (no future overhead).                |
| `DependentAsync(callable, deps)`      | Chain after `AsyncTask` dependencies.                |
| `WaitForAll()`                        | Drain all pending work.                              |
| `CoRun(graph)`                        | Participate in graph execution from a worker thread. |
| `WorkerCount()` / `IdleWorkerCount()` | Pool statistics.                                     |

## Quick Start

```cpp
#include <helios/async/async.hpp>

helios::async::Executor executor(4);
helios::async::TaskGraph graph("Example");

auto a = graph.EmplaceTask([] { /* step A */ });
auto b = graph.EmplaceTask([] { /* step B */ });
auto c = graph.EmplaceTask([] { /* step C */ });

a.Precede(b);
b.Precede(c);

auto future = executor.Run(graph);
future.Wait();
```

## Parallel Algorithms

```cpp
std::vector<int> data = {3, 1, 4, 1, 5};

graph.ForEach(data, [](int& value) { value *= 2; });

graph.Reduce(data, 0, [](int acc, int v) { return acc + v; });

graph.Sort(data, std::less<>{});
```

## Independent Async Tasks

```cpp
// Returns std::future — use when you need the result
auto result = executor.Async([] { return 42; });

// Fire-and-forget — lower overhead
executor.SilentAsync([] { DoBackgroundWork(); });

executor.WaitForAll();
```

## Dynamic Subflows

```cpp
#include <helios/async/sub_task_graph.hpp>

graph.EmplaceTask([](helios::async::SubTaskGraph& subflow) {
  auto t1 = subflow.EmplaceTask([] { /* child 1 */ });
  auto t2 = subflow.EmplaceTask([] { /* child 2 */ });
  t1.Precede(t2);
  subflow.Join();
});
```

## Dispatch Paths

1. **Task graph** — explicit DAG submitted via `Run()` / `RunN()` / `RunUntil()`.
2. **Independent async** — fire-and-forget via `Async()` / `SilentAsync()` / `DependentAsync()`.

`CoRun(TaskGraph&)` lets a worker thread participate directly in graph execution (must already be a pool worker).

## Thread Safety

| API                           | Safe when                        |
| ----------------------------- | -------------------------------- |
| `Executor` methods            | Always (thread-safe)             |
| `TaskGraph` / `Task` mutation | Not while the graph is executing |

## Dependencies

- `core` — asserts
- `compiler`, `platform` — via core
- External: Taskflow (header-only), TBB (Linux/GCC parallel STL backend)
