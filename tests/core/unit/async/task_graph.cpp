#include <doctest/doctest.h>

#include <helios/core/async/executor.hpp>
#include <helios/core/async/sub_task_graph.hpp>
#include <helios/core/async/task_graph.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

TEST_SUITE("async::TaskGraph") {
  TEST_CASE("TaskGraph: construction and basic properties") {
    SUBCASE("Default construction") {
      helios::async::TaskGraph graph;

      CHECK(graph.Empty());
      CHECK_EQ(graph.TaskCount(), 0);
      CHECK_FALSE(graph.Name().empty());  // Should have default name
    }

    SUBCASE("Named construction") {
      const std::string graph_name = "TestGraph";
      helios::async::TaskGraph graph(graph_name);

      CHECK(graph.Empty());
      CHECK_EQ(graph.TaskCount(), 0);
      CHECK_EQ(graph.Name(), graph_name);
    }

    SUBCASE("Move construction") {
      helios::async::TaskGraph graph1("OriginalGraph");
      graph1.EmplaceTask([]() { /* void task */ });

      CHECK_FALSE(graph1.Empty());
      CHECK_EQ(graph1.TaskCount(), 1);

      helios::async::TaskGraph graph2 = std::move(graph1);

      CHECK_FALSE(graph2.Empty());
      CHECK_EQ(graph2.TaskCount(), 1);
      CHECK_EQ(graph2.Name(), "OriginalGraph");
    }

    SUBCASE("Move assignment") {
      helios::async::TaskGraph graph1("OriginalGraph");
      graph1.EmplaceTask([]() { /* void task */ });

      helios::async::TaskGraph graph2("NewGraph");
      graph2 = std::move(graph1);

      CHECK_FALSE(graph2.Empty());
      CHECK_EQ(graph2.TaskCount(), 1);
      CHECK_EQ(graph2.Name(), "OriginalGraph");
    }
  }

  TEST_CASE("TaskGraph: task creation") {
    helios::async::TaskGraph graph("TaskCreationGraph");
    helios::async::Executor executor(2);

    SUBCASE("EmplaceTask with static callable") {
      std::atomic<bool> executed{false};

      auto task = graph.EmplaceTask([&executed]() { executed = true; });

      CHECK_FALSE(task.Empty());
      CHECK(task.HasWork());
      CHECK_EQ(task.Type(), helios::async::TaskType::Static);
      CHECK_FALSE(graph.Empty());
      CHECK_EQ(graph.TaskCount(), 1);

      auto future = executor.Run(std::move(graph));
      future.Wait();

      CHECK(executed.load());
    }

    SUBCASE("EmplaceTask with sub task") {
      std::atomic<bool> executed{false};

      auto task = graph.EmplaceTask([&executed](helios::async::SubTaskGraph& sub_graph) {
        executed = true;
        sub_graph.Join();
      });

      CHECK_FALSE(task.Empty());
      CHECK(task.HasWork());
      CHECK_EQ(task.Type(), helios::async::TaskType::SubTask);
      CHECK_FALSE(graph.Empty());
      CHECK_EQ(graph.TaskCount(), 1);

      auto future = executor.Run(std::move(graph));
      future.Wait();

      CHECK(executed.load());
    }

    SUBCASE("EmplaceTasks with multiple callables") {
      std::atomic<int> execution_count{0};

      auto tasks =
          graph.EmplaceTasks([&execution_count]() { ++execution_count; }, [&execution_count]() { ++execution_count; },
                             [&execution_count]() { ++execution_count; });

      CHECK_EQ(tasks.size(), 3);
      CHECK_EQ(graph.TaskCount(), 3);

      for (const auto& task : tasks) {
        CHECK_FALSE(task.Empty());
        CHECK(task.HasWork());
      }

      auto future = executor.Run(std::move(graph));
      future.Wait();

      CHECK_EQ(execution_count.load(), 3);
    }

    SUBCASE("CreatePlaceholder") {
      auto placeholder = graph.CreatePlaceholder();

      CHECK_FALSE(placeholder.Empty());
      CHECK_FALSE(placeholder.HasWork());
      CHECK_EQ(graph.TaskCount(), 1);

      std::atomic<bool> executed{false};
      placeholder.Work([&executed]() { executed = true; });
      CHECK(placeholder.HasWork());

      auto future = executor.Run(std::move(graph));
      future.Wait();

      CHECK(executed.load());
    }
  }

  TEST_CASE("TaskGraph: basic parallel operations - simplified") {
    helios::async::TaskGraph graph("ParallelOpsGraph");
    helios::async::Executor executor(4);

    SUBCASE("Manual parallel processing simulation") {
      // Simulate ForEach by creating individual tasks
      std::vector<int> input_data = {1, 2, 3, 4, 5};
      std::vector<int> results(input_data.size(), 0);
      std::mutex results_mutex;

      for (size_t i = 0; i < input_data.size(); ++i) {
        auto task = graph.EmplaceTask([&results, &results_mutex, i, value = input_data[i]]() {
          std::lock_guard<std::mutex> lock(results_mutex);
          results[i] = value * 2;
        });
        CHECK_FALSE(task.Empty());
        CHECK(task.HasWork());
      }

      CHECK_EQ(graph.TaskCount(), 5);

      auto future = executor.Run(std::move(graph));
      future.Wait();

      std::vector<int> expected = {2, 4, 6, 8, 10};
      CHECK_EQ(results, expected);
    }

    SUBCASE("Manual transform simulation") {
      std::vector<int> input_data = {1, 2, 3, 4, 5};
      std::vector<int> output_data(input_data.size(), 0);

      for (size_t i = 0; i < input_data.size(); ++i) {
        auto task =
            graph.EmplaceTask([&output_data, &input_data, i]() { output_data[i] = input_data[i] * input_data[i]; });
        CHECK_FALSE(task.Empty());
        CHECK(task.HasWork());
      }

      auto future = executor.Run(std::move(graph));
      future.Wait();

      std::vector<int> expected = {1, 4, 9, 16, 25};
      CHECK_EQ(output_data, expected);
    }

    SUBCASE("Manual reduce simulation") {
      std::vector<int> input_data = {1, 2, 3, 4, 5};
      std::atomic<int> result{0};

      for (int value : input_data) {
        auto task = graph.EmplaceTask([&result, value]() { result.fetch_add(value); });
        CHECK_FALSE(task.Empty());
        CHECK(task.HasWork());
      }

      auto future = executor.Run(std::move(graph));
      future.Wait();

      CHECK_EQ(result.load(), 15);  // 1+2+3+4+5
    }

    SUBCASE("Manual sort simulation") {
      std::vector<int> data = {5, 3, 8, 1, 9, 2, 7, 4, 6};

      auto sort_task = graph.EmplaceTask([&data]() { std::sort(data.begin(), data.end()); });
      CHECK_FALSE(sort_task.Empty());
      CHECK(sort_task.HasWork());

      auto future = executor.Run(std::move(graph));
      future.Wait();

      const std::vector<int> expected = {1, 2, 3, 4, 5, 6, 7, 8, 9};
      CHECK_EQ(data, expected);
    }
  }

  TEST_CASE("TaskGraph: actual parallel algorithms") {
    helios::async::TaskGraph graph("ActualParallelOpsGraph");
    helios::async::Executor executor(4);

    SUBCASE("ForEach with vector") {
      std::vector<int> input = {1, 2, 3, 4, 5};
      std::atomic<int> sum{0};

      auto foreach_task = graph.ForEach(input, [&sum](int value) { sum.fetch_add(value); });

      CHECK_FALSE(foreach_task.Empty());
      CHECK(foreach_task.HasWork());

      auto future = executor.Run(std::move(graph));
      future.Wait();

      CHECK_EQ(sum.load(), 15);
    }

    SUBCASE("ForEachIndex with range") {
      std::atomic<int> sum{0};
      std::atomic<int> count{0};

      auto foreach_index_task = graph.ForEachIndex(0, 10, 2, [&sum, &count](int index) {
        sum.fetch_add(index);
        count.fetch_add(1);
      });

      CHECK_FALSE(foreach_index_task.Empty());
      CHECK(foreach_index_task.HasWork());

      auto future = executor.Run(std::move(graph));
      future.Wait();

      CHECK_EQ(count.load(), 5);  // 0, 2, 4, 6, 8
      CHECK_EQ(sum.load(), 20);   // 0+2+4+6+8
    }

    SUBCASE("Transform operation") {
      std::vector<int> input = {1, 2, 3, 4, 5};
      std::vector<int> output(input.size());

      auto transform_task = graph.Transform(input, output, [](int x) { return x * x; });

      CHECK_FALSE(transform_task.Empty());
      CHECK(transform_task.HasWork());

      auto future = executor.Run(std::move(graph));
      future.Wait();

      std::vector<int> expected = {1, 4, 9, 16, 25};
      CHECK_EQ(output, expected);
    }

    SUBCASE("Reduce operation") {
      std::vector<int> input = {1, 2, 3, 4, 5};
      int result = 0;

      auto reduce_task = graph.Reduce(input, result, std::plus<int>());

      CHECK_FALSE(reduce_task.Empty());
      CHECK(reduce_task.HasWork());

      auto future = executor.Run(std::move(graph));
      future.Wait();

      CHECK_EQ(result, 15);
    }

    SUBCASE("Sort with default comparator") {
      std::vector<int> data = {5, 3, 8, 1, 9, 2, 7, 4, 6};

      auto sort_task = graph.Sort(data);

      CHECK_FALSE(sort_task.Empty());
      CHECK(sort_task.HasWork());

      auto future = executor.Run(std::move(graph));
      future.Wait();

      std::vector<int> expected = {1, 2, 3, 4, 5, 6, 7, 8, 9};
      CHECK_EQ(data, expected);
    }

    SUBCASE("Sort with custom comparator") {
      std::vector<int> data = {5, 3, 8, 1, 9, 2, 7, 4, 6};

      auto sort_task = graph.Sort(data, std::greater<int>());

      auto future = executor.Run(std::move(graph));
      future.Wait();

      std::vector<int> expected = {9, 8, 7, 6, 5, 4, 3, 2, 1};
      CHECK_EQ(data, expected);
    }
  }

  TEST_CASE("TaskGraph: linearization") {
    helios::async::TaskGraph graph("LinearGraph");
    helios::async::Executor executor(2);
    std::vector<int> execution_order;
    std::atomic<int> order_counter{0};

    SUBCASE("Linearize with vector of tasks") {
      std::vector<helios::async::Task> tasks;

      for (int i = 0; i < 5; ++i) {
        tasks.push_back(graph.EmplaceTask(
            [&execution_order, &order_counter, i]() { execution_order.push_back(order_counter.fetch_add(1)); }));
      }

      graph.Linearize(tasks);

      auto future = executor.Run(std::move(graph));
      future.Wait();

      CHECK_EQ(execution_order.size(), 5);
      for (size_t i = 0; i < execution_order.size(); ++i) {
        CHECK_EQ(execution_order[i], static_cast<int>(i));
      }
    }

    SUBCASE("Linearize with array of tasks") {
      constexpr size_t task_count = 3;
      std::array<helios::async::Task, task_count> tasks;

      for (size_t i = 0; i < task_count; ++i) {
        tasks[i] = graph.EmplaceTask(
            [&execution_order, &order_counter]() { execution_order.push_back(order_counter.fetch_add(1)); });
      }

      graph.Linearize(tasks);

      auto future = executor.Run(std::move(graph));
      future.Wait();

      CHECK_EQ(execution_order.size(), task_count);
      for (size_t i = 0; i < execution_order.size(); ++i) {
        CHECK_EQ(execution_order[i], static_cast<int>(i));
      }
    }
  }

  TEST_CASE("TaskGraph: task management") {
    helios::async::TaskGraph graph("ManagementGraph");
    helios::async::Executor executor(2);

    SUBCASE("RemoveTask") {
      auto task1 = graph.EmplaceTask([]() { /* void task */ });
      auto task2 = graph.EmplaceTask([]() { /* void task */ });

      CHECK_EQ(graph.TaskCount(), 2);

      graph.RemoveTask(task1);
      CHECK_EQ(graph.TaskCount(), 1);

      // The remaining task should still execute
      std::atomic<bool> task2_executed{false};
      task2.Work([&task2_executed]() { task2_executed = true; });

      auto future = executor.Run(std::move(graph));
      future.Wait();

      CHECK(task2_executed.load());
    }

    SUBCASE("RemoveDependency") {
      auto task_a = graph.EmplaceTask([]() { /* void task */ });
      auto task_b = graph.EmplaceTask([]() { /* void task */ });

      task_a.Precede(task_b);
      CHECK_EQ(task_a.SuccessorsCount(), 1);
      CHECK_EQ(task_b.PredecessorsCount(), 1);

      graph.RemoveDependency(task_a, task_b);
      CHECK_EQ(task_a.SuccessorsCount(), 0);
      CHECK_EQ(task_b.PredecessorsCount(), 0);

      // Both tasks should still execute, but without dependency
      auto future = executor.Run(std::move(graph));
      CHECK_NOTHROW(future.Wait());
    }
  }

  TEST_CASE("TaskGraph: composition") {
    helios::async::TaskGraph main_graph("MainGraph");
    helios::async::TaskGraph composed_graph("ComposedGraph");
    helios::async::Executor executor(2);

    SUBCASE("Compose another graph") {
      std::atomic<bool> main_executed{false};
      std::atomic<bool> composed_executed{false};

      // Add task to composed graph
      composed_graph.EmplaceTask([&composed_executed]() { composed_executed = true; });

      // Add task to main graph
      main_graph.EmplaceTask([&main_executed]() { main_executed = true; });

      // Compose the graphs
      auto composed_task = main_graph.Compose(composed_graph);
      CHECK_FALSE(composed_task.Empty());
      CHECK(composed_task.HasWork());
      CHECK_EQ(main_graph.TaskCount(), 2);  // Original task + composed task

      auto future = executor.Run(std::move(main_graph));
      future.Wait();

      CHECK(main_executed.load());
      CHECK(composed_executed.load());
    }

    SUBCASE("Compose with dependencies") {
      std::vector<int> execution_order;
      std::atomic<int> order_counter{0};

      // Add task to composed graph
      composed_graph.EmplaceTask(
          [&execution_order, &order_counter]() { execution_order.push_back(order_counter.fetch_add(1)); });

      // Add tasks to main graph with dependencies
      auto task_before = main_graph.EmplaceTask(
          [&execution_order, &order_counter]() { execution_order.push_back(order_counter.fetch_add(1)); });

      auto composed_task = main_graph.Compose(composed_graph);

      auto task_after = main_graph.EmplaceTask(
          [&execution_order, &order_counter]() { execution_order.push_back(order_counter.fetch_add(1)); });

      // Set up dependencies: task_before -> composed_task -> task_after
      task_before.Precede(composed_task);
      composed_task.Precede(task_after);

      auto future = executor.Run(std::move(main_graph));
      future.Wait();

      CHECK_EQ(execution_order.size(), 3);
      // Should execute in order: task_before, composed_task, task_after
      CHECK_EQ(execution_order[0], 0);
      CHECK_EQ(execution_order[1], 1);
      CHECK_EQ(execution_order[2], 2);
    }
  }

  TEST_CASE("TaskGraph: visitor pattern") {
    helios::async::TaskGraph graph("VisitorGraph");

    SUBCASE("Visit all tasks") {
      std::vector<helios::async::Task> created_tasks;

      // Create some tasks
      created_tasks.push_back(graph.EmplaceTask([]() {}));
      created_tasks.push_back(graph.EmplaceTask([]() {}));
      created_tasks.push_back(graph.EmplaceTask([]() {}));

      std::vector<helios::async::Task> visited_tasks;

      graph.ForEachTask([&visited_tasks](helios::async::Task& task) { visited_tasks.push_back(task); });

      CHECK_EQ(visited_tasks.size(), 3);
      CHECK_EQ(visited_tasks.size(), created_tasks.size());

      // All tasks should be valid
      for (const auto& task : visited_tasks) {
        CHECK_FALSE(task.Empty());
      }
    }
  }

  TEST_CASE("TaskGraph: utility operations") {
    helios::async::TaskGraph graph("UtilityGraph");

    SUBCASE("Clear graph") {
      graph.EmplaceTask([]() {});
      graph.EmplaceTask([]() {});

      CHECK_FALSE(graph.Empty());
      CHECK_EQ(graph.TaskCount(), 2);

      graph.Clear();

      CHECK(graph.Empty());
      CHECK_EQ(graph.TaskCount(), 0);
    }

    SUBCASE("Set name") {
      graph.Name("NewName");
      CHECK_EQ(graph.Name(), "NewName");
    }
  }

  TEST_CASE("TaskGraph: complex dependency patterns") {
    helios::async::TaskGraph graph("ComplexGraph");
    helios::async::Executor executor(4);

    SUBCASE("Diamond dependency pattern") {
      /*
           task_a
          /      \
       task_b  task_c
          \      /
           task_d
      */

      std::vector<int> execution_order;
      std::atomic<int> order_counter{0};
      std::mutex order_mutex;

      auto task_a = graph
                        .EmplaceTask([&execution_order, &order_counter, &order_mutex]() {
                          std::lock_guard<std::mutex> lock(order_mutex);
                          execution_order.push_back(order_counter.fetch_add(1));
                        })
                        .Name("TaskA");

      auto task_b = graph
                        .EmplaceTask([&execution_order, &order_counter, &order_mutex]() {
                          std::lock_guard<std::mutex> lock(order_mutex);
                          execution_order.push_back(order_counter.fetch_add(1));
                        })
                        .Name("TaskB");

      auto task_c = graph
                        .EmplaceTask([&execution_order, &order_counter, &order_mutex]() {
                          std::lock_guard<std::mutex> lock(order_mutex);
                          execution_order.push_back(order_counter.fetch_add(1));
                        })
                        .Name("TaskC");

      auto task_d = graph
                        .EmplaceTask([&execution_order, &order_counter, &order_mutex]() {
                          std::lock_guard<std::mutex> lock(order_mutex);
                          execution_order.push_back(order_counter.fetch_add(1));
                        })
                        .Name("TaskD");

      // Set up diamond dependencies
      task_a.Precede(task_b, task_c);
      task_d.Succeed(task_b, task_c);

      auto future = executor.Run(std::move(graph));
      future.Wait();

      CHECK_EQ(execution_order.size(), 4);
      CHECK_EQ(execution_order[0], 0);  // task_a executes first
      CHECK_EQ(execution_order[3], 3);  // task_d executes last
      // task_b and task_c can execute in any order (parallel)
      bool valid_order =
          (execution_order[1] == 1 && execution_order[2] == 2) || (execution_order[1] == 2 && execution_order[2] == 1);
      CHECK(valid_order);
    }

    SUBCASE("Linear chain") {
      constexpr int chain_length = 10;
      std::vector<int> execution_order;
      std::atomic<int> order_counter{0};
      std::vector<helios::async::Task> tasks;
      std::mutex order_mutex;

      // Create linear chain of tasks
      for (int i = 0; i < chain_length; ++i) {
        tasks.push_back(graph
                            .EmplaceTask([&execution_order, &order_counter, &order_mutex, i]() {
                              std::lock_guard<std::mutex> lock(order_mutex);
                              execution_order.push_back(order_counter.fetch_add(1));
                            })
                            .Name("ChainTask" + std::to_string(i)));
      }

      // Link them in sequence
      for (int i = 0; i < chain_length - 1; ++i) {
        tasks[i].Precede(tasks[i + 1]);
      }

      auto future = executor.Run(std::move(graph));
      future.Wait();

      CHECK_EQ(execution_order.size(), chain_length);
      for (int i = 0; i < chain_length; ++i) {
        CHECK_EQ(execution_order[i], i);
      }
    }
  }

  TEST_CASE("TaskGraph: performance characteristics") {
    helios::async::TaskGraph graph("PerformanceGraph");
    helios::async::Executor executor(std::thread::hardware_concurrency());

    SUBCASE("Large number of independent tasks") {
      constexpr int task_count = 1000;
      std::atomic<int> completion_count{0};

      for (int i = 0; i < task_count; ++i) {
        graph.EmplaceTask([&completion_count]() { ++completion_count; });
      }

      CHECK_EQ(graph.TaskCount(), task_count);

      auto start_time = std::chrono::high_resolution_clock::now();
      auto future = executor.Run(std::move(graph));
      future.Wait();
      auto end_time = std::chrono::high_resolution_clock::now();

      CHECK_EQ(completion_count.load(), task_count);

      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
      // This is just a sanity check - actual performance will vary
      CHECK_LT(duration.count(), 5000);  // Should complete within 5 seconds
    }
  }

}  // TEST_SUITE
