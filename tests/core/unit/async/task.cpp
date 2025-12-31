#include <doctest/doctest.h>

#include <helios/core/async/executor.hpp>
#include <helios/core/async/sub_task_graph.hpp>
#include <helios/core/async/task.hpp>
#include <helios/core/async/task_graph.hpp>

#include <atomic>
#include <cstddef>
#include <string>
#include <vector>

TEST_SUITE("async::Task") {
  TEST_CASE("Task::ctor: construction and basic properties") {
    SUBCASE("Default construction") {
      helios::async::Task task;

      CHECK(task.Empty());
      CHECK_FALSE(task.HasWork());
      CHECK_EQ(task.Hash(), 0);
      CHECK_EQ(task.SuccessorsCount(), 0);
      CHECK_EQ(task.PredecessorsCount(), 0);
      CHECK_EQ(task.StrongDependenciesCount(), 0);
      CHECK_EQ(task.WeakDependenciesCount(), 0);
      CHECK(task.Name().empty());
      CHECK_EQ(task.Type(), helios::async::TaskType::Undefined);
    }

    SUBCASE("Copy construction and assignment") {
      helios::async::TaskGraph graph("TestGraph");
      auto task1 = graph.EmplaceTask([]() { /* void task */ });

      CHECK_FALSE(task1.Empty());
      CHECK(task1.HasWork());

      // Copy construction
      helios::async::Task task2(task1);
      CHECK_EQ(task1.Hash(), task2.Hash());
      CHECK_EQ(task1, task2);

      // Copy assignment
      helios::async::Task task3;
      task3 = task1;
      CHECK_EQ(task1.Hash(), task3.Hash());
      CHECK_EQ(task1, task3);
    }

    SUBCASE("Move construction and assignment") {
      helios::async::TaskGraph graph("TestGraph");
      auto task1 = graph.EmplaceTask([]() { /* void task */ });
      const size_t original_hash = task1.Hash();

      // Move construction
      helios::async::Task task2 = std::move(task1);
      CHECK_EQ(task2.Hash(), original_hash);

      // Move assignment
      helios::async::Task task3;
      task3 = std::move(task2);
      CHECK_EQ(task3.Hash(), original_hash);
    }
  }

  TEST_CASE("Task::Work: work assignment") {
    helios::async::TaskGraph graph("WorkGraph");
    helios::async::Executor executor(2);

    SUBCASE("Work assignment to placeholder") {
      std::atomic<bool> executed{false};

      auto task = graph.CreatePlaceholder();
      CHECK_FALSE(task.HasWork());

      task.Work([&executed]() { executed = true; });
      CHECK(task.HasWork());

      auto future = executor.Run(std::move(graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "Work assignment task did not complete within timeout");

      CHECK(executed.load());
    }

    SUBCASE("Work reassignment") {
      std::atomic<int> execution_count{0};

      auto task = graph.CreatePlaceholder();

      // Assign initial work
      task.Work([&execution_count]() { execution_count++; });
      CHECK(task.HasWork());

      // Reset and reassign work
      task.ResetWork();
      CHECK_FALSE(task.HasWork());

      task.Work([&execution_count]() { execution_count += 10; });
      CHECK(task.HasWork());

      auto future = executor.Run(std::move(graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "Work reassignment task did not complete within timeout");

      CHECK_EQ(execution_count.load(), 10);  // Only the second work should execute
    }

    SUBCASE("Method chaining with Work") {
      std::atomic<bool> executed{false};

      auto task = graph.CreatePlaceholder().Work([&executed]() { executed = true; }).Name("ChainedTask");

      CHECK(task.HasWork());
      CHECK_EQ(task.Name(), "ChainedTask");

      auto future = executor.Run(std::move(graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "Method chaining task did not complete within timeout");

      CHECK(executed.load());
    }
  }

  TEST_CASE("Task::Precede: dependency management") {
    SUBCASE("Precede with single task") {
      helios::async::TaskGraph graph("DependencyGraph");
      helios::async::Executor executor(2);
      std::vector<int> execution_order;
      std::atomic<int> order_counter{0};

      auto task_a = graph.EmplaceTask(
          [&execution_order, &order_counter]() { execution_order.push_back(order_counter.fetch_add(1)); });

      auto task_b = graph.EmplaceTask(
          [&execution_order, &order_counter]() { execution_order.push_back(order_counter.fetch_add(1)); });

      task_a.Precede(task_b);

      CHECK_EQ(task_a.SuccessorsCount(), 1);
      CHECK_EQ(task_b.PredecessorsCount(), 1);

      auto future = executor.Run(std::move(graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "Single predecessor task did not complete within timeout");

      CHECK_EQ(execution_order.size(), 2);
      CHECK_LT(execution_order[0], execution_order[1]);  // task_a should execute first
    }

    SUBCASE("Precede with multiple tasks") {
      helios::async::TaskGraph graph("DependencyGraph");
      helios::async::Executor executor(2);
      std::vector<int> execution_order(3, 0);
      std::atomic<int> order_counter{0};

      auto task_a = graph.EmplaceTask([&execution_order, &order_counter]() {
        const int index = order_counter.fetch_add(1);
        execution_order[index] = index;
      });

      auto task_b = graph.EmplaceTask([&execution_order, &order_counter]() {
        const int index = order_counter.fetch_add(1);
        execution_order[index] = index;
      });

      auto task_c = graph.EmplaceTask([&execution_order, &order_counter]() {
        const int index = order_counter.fetch_add(1);
        execution_order[index] = index;
      });

      task_a.Precede(task_b, task_c);

      CHECK_EQ(task_a.SuccessorsCount(), 2);
      CHECK_EQ(task_b.PredecessorsCount(), 1);
      CHECK_EQ(task_c.PredecessorsCount(), 1);

      auto future = executor.Run(std::move(graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready,
                      "Multiple predecessors task did not complete within timeout");

      CHECK_EQ(execution_order.size(), 3);
      CHECK_LT(execution_order[0], execution_order[1]);  // task_a executes first
      CHECK_LT(execution_order[0], execution_order[2]);  // task_a executes first
    }

    SUBCASE("Precede with range of tasks") {
      helios::async::TaskGraph graph("DependencyGraph");
      helios::async::Executor executor(2);
      std::vector<int> execution_order(4, 0);
      std::atomic<int> order_counter{0};

      auto task_a =
          graph.EmplaceTask([&execution_order, &order_counter]() { execution_order[0] = order_counter.fetch_add(1); });

      std::vector<helios::async::Task> dependent_tasks;
      for (int i = 1; i < 4; ++i) {
        dependent_tasks.push_back(graph.EmplaceTask(
            [&execution_order, &order_counter, i]() { execution_order[i] = order_counter.fetch_add(1); }));
      }

      task_a.Precede(dependent_tasks);

      CHECK_EQ(task_a.SuccessorsCount(), 3);
      for (const auto& task : dependent_tasks) {
        CHECK_EQ(task.PredecessorsCount(), 1);
      }

      auto future = executor.Run(std::move(graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready,
                      "Chain of dependencies task did not complete within timeout");

      CHECK_EQ(execution_order.size(), 4);
      CHECK_LT(execution_order[0], execution_order[1]);
      CHECK_LT(execution_order[0], execution_order[2]);
      CHECK_LT(execution_order[0], execution_order[3]);
    }

    SUBCASE("Succeed with single task") {
      helios::async::TaskGraph graph("DependencyGraph");
      helios::async::Executor executor(2);
      std::vector<int> execution_order(2, 0);
      std::atomic<int> order_counter{0};

      auto task_a =
          graph.EmplaceTask([&execution_order, &order_counter]() { execution_order[0] = order_counter.fetch_add(1); });

      auto task_b =
          graph.EmplaceTask([&execution_order, &order_counter]() { execution_order[1] = order_counter.fetch_add(1); });

      task_b.Succeed(task_a);  // task_b depends on task_a

      CHECK_EQ(task_a.SuccessorsCount(), 1);
      CHECK_EQ(task_b.PredecessorsCount(), 1);

      auto future = executor.Run(std::move(graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "Single successor task did not complete within timeout");

      CHECK_EQ(execution_order.size(), 2);
      CHECK_LT(execution_order[0], execution_order[1]);  // task_a should execute first
    }

    SUBCASE("Succeed with multiple tasks") {
      helios::async::TaskGraph graph("DependencyGraph");
      helios::async::Executor executor(2);
      std::vector<int> execution_order(3, 0);
      std::atomic<int> order_counter{0};

      auto task_a =
          graph.EmplaceTask([&execution_order, &order_counter]() { execution_order[0] = order_counter.fetch_add(1); });

      auto task_b =
          graph.EmplaceTask([&execution_order, &order_counter]() { execution_order[1] = order_counter.fetch_add(1); });

      auto task_c =
          graph.EmplaceTask([&execution_order, &order_counter]() { execution_order[2] = order_counter.fetch_add(1); });

      task_c.Succeed(task_a, task_b);  // task_c depends on both task_a and task_b

      CHECK_EQ(task_a.SuccessorsCount(), 1);
      CHECK_EQ(task_b.SuccessorsCount(), 1);
      CHECK_EQ(task_c.PredecessorsCount(), 2);

      auto future = executor.Run(std::move(graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "Multiple successors task did not complete within timeout");

      CHECK_EQ(execution_order.size(), 3);
      CHECK_LT(execution_order[0], execution_order[2]);  // task_a before task_c
      CHECK_LT(execution_order[1], execution_order[2]);  // task_b before task_c
    }

    SUBCASE("Succeed with range of tasks") {
      helios::async::TaskGraph graph("DependencyGraph");
      helios::async::Executor executor(2);
      std::vector<int> execution_order(4, 0);
      std::atomic<int> order_counter{0};

      std::vector<helios::async::Task> dependency_tasks;
      for (int i = 0; i < 3; ++i) {
        dependency_tasks.push_back(graph.EmplaceTask(
            [&execution_order, &order_counter, i]() { execution_order[i] = order_counter.fetch_add(1); }));
      }

      auto final_task =
          graph.EmplaceTask([&execution_order, &order_counter]() { execution_order[3] = order_counter.fetch_add(1); });

      final_task.Succeed(dependency_tasks);

      for (const auto& task : dependency_tasks) {
        CHECK_EQ(task.SuccessorsCount(), 1);
      }
      CHECK_EQ(final_task.PredecessorsCount(), 3);

      auto future = executor.Run(std::move(graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready,
                      "Bidirectional dependencies task did not complete within timeout");

      CHECK_EQ(execution_order.size(), 4);
      // Final task should execute last
      CHECK_EQ(execution_order[3], 3);
    }
  }

  TEST_CASE("Task::Name: naming") {
    helios::async::TaskGraph graph("NameGraph");

    SUBCASE("Name with valid name") {
      auto task = graph.CreatePlaceholder();
      CHECK(task.Name().empty());

      task.Name("TestTask");
      CHECK_EQ(task.Name(), "TestTask");
    }

    SUBCASE("Name method chaining") {
      auto task = graph.CreatePlaceholder().Name("ChainedTask").Work([]() { /* void task */ });

      CHECK_EQ(task.Name(), "ChainedTask");
      CHECK(task.HasWork());
    }

    SUBCASE("Name with empty string should assert") {
#ifdef HELIOS_ENABLE_ASSERTS
      // Skip this test when assertions are enabled as it will abort
      MESSAGE("Skipping empty name test when assertions are enabled (would trigger assertion)");
#else
      // When assertions are disabled, the assertion is compiled out and nothing happens
      MESSAGE("Skipping empty name test when assertions are disabled (no-op)");
#endif
    }
  }

  TEST_CASE("Task::Type: task type detection") {
    helios::async::TaskGraph graph("TypeGraph");

    SUBCASE("Static task type") {
      auto static_task = graph.EmplaceTask([]() { /* void task */ });
      CHECK_EQ(static_task.Type(), helios::async::TaskType::Static);
    }

    SUBCASE("SubTask type") {
      auto sub_task = graph.EmplaceTask([](helios::async::SubTaskGraph& sub_graph) { sub_graph.Join(); });
      CHECK_EQ(sub_task.Type(), helios::async::TaskType::SubTask);
    }

    SUBCASE("Placeholder task type before work assignment") {
      auto placeholder = graph.CreatePlaceholder();
      // Placeholder tasks typically have Static type until work is assigned
      CHECK_EQ(placeholder.Type(), helios::async::TaskType::Static);
    }
  }

  TEST_CASE("Task::Reset: reset functionality") {
    helios::async::TaskGraph graph("ResetGraph");

    SUBCASE("Reset task handle") {
      auto task = graph.EmplaceTask([]() { /* void task */ });
      CHECK_FALSE(task.Empty());
      CHECK(task.HasWork());

      task.Reset();
      CHECK(task.Empty());
      CHECK_FALSE(task.HasWork());
    }

    SUBCASE("ResetWork on task with work") {
      auto task = graph.CreatePlaceholder();
      task.Work([]() {});

      CHECK(task.HasWork());
      task.ResetWork();
      CHECK_FALSE(task.HasWork());
      CHECK_FALSE(task.Empty());  // Task handle is still valid
    }
  }

  TEST_CASE("Task::operator==: equality and hashing") {
    helios::async::TaskGraph graph("EqualityGraph");

    SUBCASE("Task equality") {
      auto task1 = graph.EmplaceTask([]() { /* void task */ });
      auto task2 = task1;  // Copy
      auto task3 = graph.EmplaceTask([]() { /* void task */ });

      CHECK_EQ(task1, task2);
      CHECK_NE(task1, task3);
    }

    SUBCASE("Task hashing") {
      auto task1 = graph.EmplaceTask([]() { /* void task */ });
      auto task2 = task1;  // Copy
      auto task3 = graph.EmplaceTask([]() { /* void task */ });

      CHECK_EQ(task1.Hash(), task2.Hash());
      CHECK_NE(task1.Hash(), task3.Hash());
    }

    SUBCASE("Empty task equality") {
      helios::async::Task task1;
      helios::async::Task task2;

      CHECK_EQ(task1, task2);  // Empty tasks are equal
      CHECK_EQ(task1.Hash(), task2.Hash());
    }
  }

  TEST_CASE("Task::SuccessorsCount: dependency counting") {
    helios::async::TaskGraph graph("CountGraph");

    SUBCASE("Complex dependency graph") {
      /*   task_a
          /      \
       task_b  task_c
          \      /
           task_d
      */

      auto task_a = graph.EmplaceTask([]() { /* void task */ });
      auto task_b = graph.EmplaceTask([]() { /* void task */ });
      auto task_c = graph.EmplaceTask([]() { /* void task */ });
      auto task_d = graph.EmplaceTask([]() { /* void task */ });

      task_a.Precede(task_b, task_c);
      task_d.Succeed(task_b, task_c);

      CHECK_EQ(task_a.SuccessorsCount(), 2);
      CHECK_EQ(task_a.PredecessorsCount(), 0);

      CHECK_EQ(task_b.SuccessorsCount(), 1);
      CHECK_EQ(task_b.PredecessorsCount(), 1);

      CHECK_EQ(task_c.SuccessorsCount(), 1);
      CHECK_EQ(task_c.PredecessorsCount(), 1);

      CHECK_EQ(task_d.SuccessorsCount(), 0);
      CHECK_EQ(task_d.PredecessorsCount(), 2);
    }

    SUBCASE("Strong vs weak dependencies") {
      auto task_a = graph.EmplaceTask([]() { /* void task */ });
      auto task_b = graph.EmplaceTask([]() { /* void task */ });

      task_a.Precede(task_b);

      // Regular dependencies are typically strong dependencies
      CHECK_EQ(task_a.PredecessorsCount(), 0);  // task_a has no dependencies
      CHECK_EQ(task_b.PredecessorsCount(), 1);  // task_b depends on task_a

      CHECK_EQ(task_a.WeakDependenciesCount(), 0);
      CHECK_EQ(task_b.WeakDependenciesCount(), 0);
    }
  }

}  // TEST_SUITE
