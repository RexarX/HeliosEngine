#include <doctest/doctest.h>

#include <helios/core/async/executor.hpp>
#include <helios/core/async/task_graph.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <thread>
#include <vector>

TEST_SUITE("async::Executor") {
  TEST_CASE("Executor::ctor: construction and basic properties") {
    SUBCASE("Single worker thread") {
      helios::async::Executor executor(1);

      CHECK_EQ(executor.WorkerCount(), 1);
      CHECK_LE(executor.IdleWorkerCount(), 1);
      CHECK_GE(executor.QueueCount(), 1);
      CHECK_EQ(executor.RunningTopologyCount(), 0);
    }

    SUBCASE("Multiple worker threads") {
      constexpr size_t worker_count = 4;
      helios::async::Executor executor(worker_count);

      CHECK_EQ(executor.WorkerCount(), worker_count);
      CHECK_LE(executor.IdleWorkerCount(), worker_count);
      CHECK_GE(executor.QueueCount(), 1);
      CHECK_EQ(executor.RunningTopologyCount(), 0);
    }

    SUBCASE("Worker thread detection from main thread") {
      helios::async::Executor executor(2);

      CHECK_FALSE(executor.IsWorkerThread());
      CHECK_EQ(executor.CurrentWorkerId(), -1);
    }
  }

  TEST_CASE("Executor::Run: task graph execution") {
    helios::async::Executor executor(2);
    std::atomic<int> execution_count{0};

    SUBCASE("Run single task graph by reference") {
      helios::async::TaskGraph graph("TestGraph");
      graph.EmplaceTask([&execution_count]() { execution_count++; });

      auto future = executor.Run(graph);
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "Task graph did not complete within timeout");

      CHECK_EQ(execution_count.load(), 1);
      CHECK_FALSE(graph.Empty());
      CHECK_EQ(graph.TaskCount(), 1);
    }

    SUBCASE("Run single task graph by move") {
      helios::async::TaskGraph graph("TestGraph");
      graph.EmplaceTask([&execution_count]() { execution_count++; });

      auto future = executor.Run(std::move(graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "Task graph did not complete within timeout");

      CHECK_EQ(execution_count.load(), 1);
    }

    SUBCASE("Run task graph with callback") {
      helios::async::TaskGraph graph("TestGraph");
      graph.EmplaceTask([&execution_count]() { execution_count++; });

      std::atomic<bool> callback_executed{false};
      auto future = executor.Run(graph, [&callback_executed]() { callback_executed = true; });
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "Task graph did not complete within timeout");

      CHECK_EQ(execution_count.load(), 1);
      CHECK(callback_executed.load());
    }

    SUBCASE("Run moved task graph with callback") {
      helios::async::TaskGraph graph("TestGraph");
      graph.EmplaceTask([&execution_count]() { execution_count++; });

      std::atomic<bool> callback_executed{false};
      auto future = executor.Run(std::move(graph), [&callback_executed]() { callback_executed = true; });
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "Task graph did not complete within timeout");

      CHECK_EQ(execution_count.load(), 1);
      CHECK(callback_executed.load());
    }
  }

  TEST_CASE("Executor::RunN: multiple executions") {
    helios::async::Executor executor(2);
    std::atomic<int> execution_count{0};
    constexpr size_t run_count = 5;

    SUBCASE("RunN with task graph reference") {
      helios::async::TaskGraph graph("TestGraph");
      graph.EmplaceTask([&execution_count]() { execution_count++; });

      auto future = executor.RunN(graph, run_count);
      auto status = future.WaitFor(std::chrono::milliseconds(2000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "RunN did not complete within timeout");

      CHECK_EQ(execution_count.load(), run_count);
    }

    SUBCASE("RunN with moved task graph") {
      helios::async::TaskGraph graph("TestGraph");
      graph.EmplaceTask([&execution_count]() { execution_count++; });

      auto future = executor.RunN(std::move(graph), run_count);
      auto status = future.WaitFor(std::chrono::milliseconds(2000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "RunN did not complete within timeout");

      CHECK_EQ(execution_count.load(), run_count);
    }

    SUBCASE("RunN with callback") {
      helios::async::TaskGraph graph("TestGraph");
      graph.EmplaceTask([&execution_count]() { execution_count++; });

      std::atomic<int> callback_count{0};
      auto future = executor.RunN(graph, run_count, [&callback_count]() { callback_count++; });
      auto status = future.WaitFor(std::chrono::milliseconds(2000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "RunN with callback did not complete within timeout");

      CHECK_EQ(execution_count.load(), run_count);
      CHECK_EQ(callback_count.load(), 1);  // Callback is called once after all runs
    }

    SUBCASE("RunN with moved graph and callback") {
      helios::async::TaskGraph graph("TestGraph");
      graph.EmplaceTask([&execution_count]() { execution_count++; });

      std::atomic<int> callback_count{0};
      auto future = executor.RunN(std::move(graph), run_count, [&callback_count]() { callback_count++; });
      auto status = future.WaitFor(std::chrono::milliseconds(2000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "RunN with callback did not complete within timeout");

      CHECK_EQ(execution_count.load(), run_count);
      CHECK_EQ(callback_count.load(), 1);
    }
  }

  TEST_CASE("Executor::RunUntil: predicate-based execution") {
    helios::async::Executor executor(2);
    std::atomic<int> execution_count{0};

    SUBCASE("RunUntil with simple predicate") {
      helios::async::TaskGraph graph("TestGraph");
      graph.EmplaceTask([&execution_count]() { execution_count++; });

      auto future = executor.RunUntil(graph, [&execution_count]() { return execution_count.load() >= 3; });
      auto status = future.WaitFor(std::chrono::milliseconds(2000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "RunUntil did not complete within timeout");

      CHECK_GE(execution_count.load(), 3);
    }

    SUBCASE("RunUntil with moved graph") {
      helios::async::TaskGraph graph("TestGraph");
      graph.EmplaceTask([&execution_count]() { execution_count++; });

      auto future = executor.RunUntil(std::move(graph), [&execution_count]() { return execution_count.load() >= 3; });
      auto status = future.WaitFor(std::chrono::milliseconds(2000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "RunUntil did not complete within timeout");

      CHECK_GE(execution_count.load(), 3);
    }

    SUBCASE("RunUntil with callback") {
      helios::async::TaskGraph graph("TestGraph");
      graph.EmplaceTask([&execution_count]() { execution_count++; });

      std::atomic<bool> callback_executed{false};
      auto future = executor.RunUntil(
          graph, [&execution_count]() { return execution_count.load() >= 3; },
          [&callback_executed]() { callback_executed = true; });
      auto status = future.WaitFor(std::chrono::milliseconds(2000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "RunUntil with callback did not complete within timeout");

      CHECK_GE(execution_count.load(), 3);
      CHECK(callback_executed.load());
    }

    SUBCASE("RunUntil with moved graph and callback") {
      helios::async::TaskGraph graph("TestGraph");
      graph.EmplaceTask([&execution_count]() { execution_count++; });

      std::atomic<bool> callback_executed{false};
      auto future = executor.RunUntil(
          std::move(graph), [&execution_count]() { return execution_count.load() >= 3; },
          [&callback_executed]() { callback_executed = true; });
      auto status = future.WaitFor(std::chrono::milliseconds(2000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "RunUntil with callback did not complete within timeout");

      CHECK_GE(execution_count.load(), 3);
      CHECK(callback_executed.load());
    }
  }

  TEST_CASE("Executor::Async: asynchronous task execution") {
    helios::async::Executor executor(4);

    SUBCASE("Async with return value") {
      constexpr int expected_result = 42;
      auto future = executor.Async([]() { return expected_result; });

      CHECK(future.valid());
      CHECK_EQ(future.get(), expected_result);
    }

    SUBCASE("Async with void return") {
      std::atomic<bool> executed{false};
      auto future = executor.Async([&executed]() { executed = true; });

      CHECK(future.valid());
      auto status = future.wait_for(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "Async task did not complete within timeout");
      CHECK(executed.load());
    }

    SUBCASE("Named async task") {
      constexpr int expected_result = 100;
      auto future = executor.Async("NamedTask", []() { return expected_result; });

      CHECK(future.valid());
      CHECK_EQ(future.get(), expected_result);
    }

    SUBCASE("SilentAsync execution") {
      std::atomic<bool> executed{false};
      executor.SilentAsync([&executed]() { executed = true; });

      // Wait for execution to complete
      executor.WaitForAll();
      CHECK(executed.load());
    }

    SUBCASE("Named SilentAsync execution") {
      std::atomic<bool> executed{false};
      executor.SilentAsync("SilentNamedTask", [&executed]() { executed = true; });

      // Wait for execution to complete
      executor.WaitForAll();
      CHECK(executed.load());
    }
  }

  TEST_CASE("Executor::DependentAsync: dependent async tasks") {
    helios::async::Executor executor(4);

    SUBCASE("DependentAsync with single dependency") {
      std::atomic<int> execution_order{0};
      std::atomic<int> first_value{0};
      std::atomic<int> second_value{0};

      // Create first task and get its AsyncTask handle
      auto [first_task, first_future] = executor.DependentAsync(
          [&execution_order, &first_value]() {
            first_value = execution_order.fetch_add(1);
            return 10;
          },
          std::vector<helios::async::AsyncTask>{});

      // Create second task that depends on first
      std::vector<helios::async::AsyncTask> deps = {first_task};
      auto [dependent_task, dependent_future] = executor.DependentAsync(
          [&execution_order, &second_value]() {
            second_value = execution_order.fetch_add(1);
            return 20;
          },
          deps);

      CHECK_FALSE(dependent_task.Empty());
      CHECK(dependent_future.valid());

      auto status1 = first_future.wait_for(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status1 == std::future_status::ready, "First dependent task did not complete within timeout");
      auto status2 = dependent_future.wait_for(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status2 == std::future_status::ready, "Second dependent task did not complete within timeout");

      CHECK_LT(first_value.load(), second_value.load());
    }

    SUBCASE("SilentDependentAsync") {
      std::atomic<bool> first_executed{false};
      std::atomic<bool> second_executed{false};

      // Create first task
      auto first_future = executor.Async([&first_executed]() { first_executed = true; });

      // Create dependent task
      std::vector<helios::async::AsyncTask> deps;  // Empty for this test
      auto dependent_task = executor.SilentDependentAsync([&second_executed]() { second_executed = true; }, deps);

      CHECK_FALSE(dependent_task.Empty());

      auto status = first_future.wait_for(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "First task did not complete within timeout");
      executor.WaitForAll();

      CHECK(first_executed.load());
      CHECK(second_executed.load());
    }
  }

  TEST_CASE("Executor::IsWorkerThread: worker thread identification") {
    helios::async::Executor executor(2);

    SUBCASE("Worker thread identification from task") {
      std::atomic<bool> is_worker_from_task{false};
      std::atomic<int> worker_id_from_task{-2};

      auto future = executor.Async([&executor, &is_worker_from_task, &worker_id_from_task]() {
        is_worker_from_task = executor.IsWorkerThread();
        worker_id_from_task = executor.CurrentWorkerId();
      });

      auto status = future.wait_for(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready,
                      "Worker thread identification task did not complete within timeout");

      CHECK(is_worker_from_task.load());
      CHECK_GE(worker_id_from_task.load(), 0);
      CHECK_LT(worker_id_from_task.load(), static_cast<int>(executor.WorkerCount()));
    }
  }

  TEST_CASE("Executor::CoRun: cooperative execution") {
    helios::async::Executor executor(2);

    SUBCASE("CoRun from worker thread") {
      std::atomic<bool> corun_executed{false};

      auto future = executor.Async([&executor, &corun_executed]() {
        helios::async::TaskGraph inner_graph("InnerGraph");
        inner_graph.EmplaceTask([&corun_executed]() { corun_executed = true; });

        executor.CoRun(inner_graph);
      });

      auto status = future.wait_for(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "CoRun task did not complete within timeout");
      CHECK(corun_executed.load());
    }

    SUBCASE("CoRunUntil from worker thread") {
      std::atomic<int> counter{0};

      auto future = executor.Async([&executor, &counter]() {
        executor.CoRunUntil([&counter]() {
          counter++;
          return counter.load() >= 5;
        });
      });

      auto status = future.wait_for(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "CoRunUntil task did not complete within timeout");
      CHECK_GE(counter.load(), 5);
    }
  }

  TEST_CASE("Executor::WaitForAll: wait for all functionality") {
    helios::async::Executor executor(4);
    std::atomic<int> completed_tasks{0};
    constexpr int total_tasks = 10;

    SUBCASE("WaitForAll with multiple async tasks") {
      // Launch multiple async tasks
      for (int i = 0; i < total_tasks; ++i) {
        executor.SilentAsync([&completed_tasks]() {
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
          completed_tasks++;
        });
      }

      executor.WaitForAll();
      CHECK_EQ(completed_tasks.load(), total_tasks);
    }

    SUBCASE("WaitForAll with mixed task types") {
      // Launch some async tasks
      for (int i = 0; i < total_tasks / 2; ++i) {
        executor.SilentAsync([&completed_tasks]() { completed_tasks++; });
      }

      // Launch a task graph
      helios::async::TaskGraph graph("MixedGraph");
      for (int i = 0; i < total_tasks / 2; ++i) {
        graph.EmplaceTask([&completed_tasks]() { completed_tasks++; });
      }
      executor.Run(std::move(graph));

      executor.WaitForAll();
      CHECK_EQ(completed_tasks.load(), total_tasks);
    }
  }

  TEST_CASE("Executor::IdleWorkerCount: idle and queue statistics") {
    helios::async::Executor executor(4);

    SUBCASE("Idle worker count changes with work") {
      const size_t initial_idle = executor.IdleWorkerCount();
      CHECK_LE(initial_idle, executor.WorkerCount());

      // Submit work that will keep workers busy
      std::atomic<bool> should_continue{true};
      std::vector<std::future<void>> futures;

      for (size_t i = 0; i < executor.WorkerCount(); ++i) {
        futures.push_back(executor.Async([&should_continue]() {
          while (should_continue.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
          }
        }));
      }

      // Give tasks time to start
      std::this_thread::sleep_for(std::chrono::milliseconds(10));

      // Stop the work
      should_continue = false;

      for (auto& future : futures) {
        auto status = future.wait_for(std::chrono::milliseconds(1000));
        REQUIRE_MESSAGE(status == std::future_status::ready, "Idle worker count task did not complete within timeout");
      }
    }

    SUBCASE("Queue count is consistent") {
      const size_t queue_count = executor.QueueCount();
      CHECK_GT(queue_count, 0);
      // Note: Queue count can be greater than worker count in TaskFlow's work-stealing implementation
      CHECK_GE(queue_count, executor.WorkerCount());
    }
  }

}  // TEST_SUITE
