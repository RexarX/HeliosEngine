#include <doctest/doctest.h>

#include <helios/core/async/executor.hpp>
#include <helios/core/async/sub_task_graph.hpp>
#include <helios/core/async/task_graph.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

TEST_SUITE("async::SubTaskGraph") {
  TEST_CASE("SubTaskGraph: basic construction and properties") {
    helios::async::TaskGraph main_graph("MainGraph");
    helios::async::Executor executor(2);

    SUBCASE("SubTaskGraph creation and basic operations") {
      std::atomic<bool> sub_task_executed{false};
      std::atomic<bool> join_called{false};

      auto main_task = main_graph.EmplaceTask([&](helios::async::SubTaskGraph& sub_graph) {
        CHECK(sub_graph.Joinable());
        CHECK_FALSE(sub_graph.WillBeRetained());

        auto sub_task = sub_graph.EmplaceTask([&sub_task_executed]() { sub_task_executed = true; });

        CHECK_FALSE(sub_task.Empty());
        CHECK(sub_task.HasWork());
        CHECK_EQ(sub_task.Type(), helios::async::TaskType::Static);

        sub_graph.Join();
        join_called = true;
      });

      auto future = executor.Run(std::move(main_graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "Basic sub task did not complete within timeout");

      CHECK(sub_task_executed.load());
      CHECK(join_called.load());
    }

    SUBCASE("SubTaskGraph with retention") {
      std::atomic<bool> retained_flag_set{false};

      auto main_task = main_graph.EmplaceTask([&](helios::async::SubTaskGraph& sub_graph) {
        sub_graph.Retain(true);
        CHECK(sub_graph.WillBeRetained());
        retained_flag_set = true;

        sub_graph.Retain(false);
        CHECK_FALSE(sub_graph.WillBeRetained());

        sub_graph.Join();
      });

      auto future = executor.Run(std::move(main_graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "Detached sub task did not complete within timeout");

      CHECK(retained_flag_set.load());
    }
  }

  TEST_CASE("SubTaskGraph: task creation methods") {
    helios::async::TaskGraph main_graph("TaskCreationGraph");
    helios::async::Executor executor(4);

    SUBCASE("EmplaceTask with static callable") {
      std::atomic<int> execution_count{0};

      auto main_task = main_graph.EmplaceTask([&](helios::async::SubTaskGraph& sub_graph) {
        // Create multiple static tasks
        for (int i = 0; i < 5; ++i) {
          auto task = sub_graph.EmplaceTask([&execution_count, i]() { ++execution_count; });
          CHECK_EQ(task.Type(), helios::async::TaskType::Static);
          CHECK(task.HasWork());
        }
        sub_graph.Join();
      });

      auto future = executor.Run(std::move(main_graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "Multiple sub tasks did not complete within timeout");

      CHECK_EQ(execution_count.load(), 5);
    }

    SUBCASE("EmplaceTask with nested sub task") {
      std::atomic<int> nested_execution_count{0};
      std::atomic<int> deep_nested_count{0};

      auto main_task = main_graph.EmplaceTask([&](helios::async::SubTaskGraph& sub_graph) {
        auto nested_task = sub_graph.EmplaceTask([&](helios::async::SubTaskGraph& nested_graph) {
          ++nested_execution_count;

          // Create a deeply nested sub task
          auto deep_task = nested_graph.EmplaceTask([&](helios::async::SubTaskGraph& deep_graph) {
            ++deep_nested_count;
            auto inner_task = deep_graph.EmplaceTask([&deep_nested_count]() { ++deep_nested_count; });
            deep_graph.Join();
          });

          CHECK_EQ(deep_task.Type(), helios::async::TaskType::SubTask);
          nested_graph.Join();
        });

        CHECK_EQ(nested_task.Type(), helios::async::TaskType::SubTask);
        sub_graph.Join();
      });

      auto future = executor.Run(std::move(main_graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "Nested sub task did not complete within timeout");

      CHECK_EQ(nested_execution_count.load(), 1);
      CHECK_EQ(deep_nested_count.load(), 2);
    }

    SUBCASE("EmplaceTasks with multiple callables") {
      std::atomic<int> total_executions{0};

      auto main_task = main_graph.EmplaceTask([&](helios::async::SubTaskGraph& sub_graph) {
        auto tasks = sub_graph.EmplaceTasks(
            [&total_executions]() { ++total_executions; }, [&total_executions]() { ++total_executions; },
            [&total_executions]() { ++total_executions; }, [&total_executions]() { ++total_executions; });

        CHECK_EQ(tasks.size(), 4);
        for (const auto& task : tasks) {
          CHECK_FALSE(task.Empty());
          CHECK(task.HasWork());
          CHECK_EQ(task.Type(), helios::async::TaskType::Static);
        }

        sub_graph.Join();
      });

      auto future = executor.Run(std::move(main_graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "Multiple nesting levels did not complete within timeout");

      CHECK_EQ(total_executions.load(), 4);
    }

    SUBCASE("CreatePlaceholder and work assignment") {
      std::atomic<bool> placeholder_executed{false};

      auto main_task = main_graph.EmplaceTask([&](helios::async::SubTaskGraph& sub_graph) {
        auto placeholder = sub_graph.CreatePlaceholder();
        CHECK_FALSE(placeholder.Empty());
        CHECK_FALSE(placeholder.HasWork());

        placeholder.Work([&placeholder_executed]() { placeholder_executed = true; });
        CHECK(placeholder.HasWork());

        sub_graph.Join();
      });

      auto future = executor.Run(std::move(main_graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "Placeholder sub task did not complete within timeout");

      CHECK(placeholder_executed.load());
    }
  }

  TEST_CASE("SubTaskGraph: basic parallel operations - simplified") {
    helios::async::TaskGraph main_graph("SubTaskParallelOpsGraph");
    helios::async::Executor executor(4);

    SUBCASE("Manual parallel processing simulation") {
      std::atomic<int> total_sum{0};
      std::atomic<int> processed_count{0};

      auto main_task = main_graph.EmplaceTask([&](helios::async::SubTaskGraph& sub_graph) {
        // Simulate ForEach by creating individual tasks
        std::vector<int> input_data = {1, 2, 3, 4, 5};

        for (int value : input_data) {
          auto task = sub_graph.EmplaceTask([&total_sum, &processed_count, value]() {
            total_sum.fetch_add(value * 2);  // Double each value
            processed_count.fetch_add(1);
          });
          CHECK_FALSE(task.Empty());
          CHECK(task.HasWork());
        }

        sub_graph.Join();
      });

      auto future = executor.Run(std::move(main_graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "For with modification did not complete within timeout");

      CHECK_EQ(processed_count.load(), 5);
      CHECK_EQ(total_sum.load(), 30);  // (1+2+3+4+5)*2 = 30
    }

    SUBCASE("Manual transform simulation") {
      std::vector<int> input_data = {1, 2, 3, 4, 5};
      std::vector<int> output_data(input_data.size(), 0);
      std::mutex output_mutex;

      auto main_task = main_graph.EmplaceTask([&](helios::async::SubTaskGraph& sub_graph) {
        for (size_t i = 0; i < input_data.size(); ++i) {
          auto task = sub_graph.EmplaceTask([&output_data, &output_mutex, &input_data, i]() {
            std::lock_guard<std::mutex> lock(output_mutex);
            output_data[i] = input_data[i] * input_data[i];  // Square
          });
          CHECK_FALSE(task.Empty());
          CHECK(task.HasWork());
        }

        sub_graph.Join();
      });

      auto future = executor.Run(std::move(main_graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "For with transform did not complete within timeout");

      std::vector<int> expected = {1, 4, 9, 16, 25};
      CHECK_EQ(output_data, expected);
    }

    SUBCASE("Manual reduce simulation") {
      std::vector<int> input_data = {1, 2, 3, 4, 5};
      std::atomic<int> result{0};

      auto main_task = main_graph.EmplaceTask([&](helios::async::SubTaskGraph& sub_graph) {
        for (int value : input_data) {
          auto task = sub_graph.EmplaceTask([&result, value]() { result.fetch_add(value); });
          CHECK_FALSE(task.Empty());
          CHECK(task.HasWork());
        }

        sub_graph.Join();
      });

      auto future = executor.Run(std::move(main_graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "For with accumulation did not complete within timeout");

      CHECK_EQ(result.load(), 15);  // 1+2+3+4+5
    }

    SUBCASE("Manual sort simulation") {
      std::vector<int> data = {5, 3, 8, 1, 9, 2, 7, 4, 6};

      auto main_task = main_graph.EmplaceTask([&](helios::async::SubTaskGraph& sub_graph) {
        auto sort_task = sub_graph.EmplaceTask([&data]() { std::sort(data.begin(), data.end()); });
        CHECK_FALSE(sort_task.Empty());
        CHECK(sort_task.HasWork());

        sub_graph.Join();
      });

      auto future = executor.Run(std::move(main_graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "Manual sort simulation did not complete within timeout");

      const std::vector<int> expected = {1, 2, 3, 4, 5, 6, 7, 8, 9};
      CHECK_EQ(data, expected);
    }
  }

  TEST_CASE("SubTaskGraph: actual parallel algorithms") {
    helios::async::TaskGraph main_graph("SubTaskActualParallelGraph");
    helios::async::Executor executor(4);

    SUBCASE("ForEach with vector") {
      std::atomic<int> sum{0};

      auto main_task = main_graph.EmplaceTask([&](helios::async::SubTaskGraph& sub_graph) {
        std::vector<int> input = {1, 2, 3, 4, 5};

        auto foreach_task = sub_graph.ForEach(input, [&sum](int value) { sum.fetch_add(value); });

        CHECK_FALSE(foreach_task.Empty());
        CHECK(foreach_task.HasWork());

        sub_graph.Join();
      });

      auto future = executor.Run(std::move(main_graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "ForEach with vector did not complete within timeout");

      CHECK_EQ(sum.load(), 15);
    }

    SUBCASE("ForEachIndex with range") {
      std::atomic<int> sum{0};
      std::atomic<int> count{0};

      auto main_task = main_graph.EmplaceTask([&](helios::async::SubTaskGraph& sub_graph) {
        auto foreach_index_task = sub_graph.ForEachIndex(0, 10, 2, [&sum, &count](int index) {
          sum.fetch_add(index);
          count.fetch_add(1);
        });

        CHECK_FALSE(foreach_index_task.Empty());
        CHECK(foreach_index_task.HasWork());

        sub_graph.Join();
      });

      auto future = executor.Run(std::move(main_graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "ForEach with index did not complete within timeout");

      CHECK_EQ(count.load(), 5);  // 0, 2, 4, 6, 8
      CHECK_EQ(sum.load(), 20);   // 0+2+4+6+8
    }

    SUBCASE("Transform operation") {
      std::vector<int> input = {1, 2, 3, 4, 5};
      std::vector<int> output(input.size());

      auto main_task = main_graph.EmplaceTask([&](helios::async::SubTaskGraph& sub_graph) {
        auto transform_task = sub_graph.Transform(input, output, [](int x) { return x * x; });

        CHECK_FALSE(transform_task.Empty());
        CHECK(transform_task.HasWork());

        sub_graph.Join();
      });

      auto future = executor.Run(std::move(main_graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "Transform operation did not complete within timeout");

      std::vector<int> expected = {1, 4, 9, 16, 25};
      CHECK_EQ(output, expected);
    }

    SUBCASE("Reduce operation") {
      std::vector<int> input = {1, 2, 3, 4, 5};
      int result = 0;

      auto main_task = main_graph.EmplaceTask([&](helios::async::SubTaskGraph& sub_graph) {
        auto reduce_task = sub_graph.Reduce(input, result, std::plus<int>());

        CHECK_FALSE(reduce_task.Empty());
        CHECK(reduce_task.HasWork());

        sub_graph.Join();
      });

      auto future = executor.Run(std::move(main_graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "Reduce operation did not complete within timeout");

      CHECK_EQ(result, 15);
    }

    SUBCASE("Sort with default comparator") {
      std::vector<int> data = {5, 3, 8, 1, 9, 2, 7, 4, 6};

      auto main_task = main_graph.EmplaceTask([&](helios::async::SubTaskGraph& sub_graph) {
        auto sort_task = sub_graph.Sort(data);

        CHECK_FALSE(sort_task.Empty());
        CHECK(sort_task.HasWork());

        sub_graph.Join();
      });

      auto future = executor.Run(std::move(main_graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready,
                      "Sort with default comparator did not complete within timeout");

      std::vector<int> expected = {1, 2, 3, 4, 5, 6, 7, 8, 9};
      CHECK_EQ(data, expected);
    }

    SUBCASE("Sort with custom comparator") {
      std::vector<int> data = {5, 3, 8, 1, 9, 2, 7, 4, 6};

      auto main_task = main_graph.EmplaceTask([&](helios::async::SubTaskGraph& sub_graph) {
        auto sort_task = sub_graph.Sort(data, std::greater<int>());

        sub_graph.Join();
      });

      auto future = executor.Run(std::move(main_graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready,
                      "Sort with custom comparator did not complete within timeout");

      std::vector<int> expected = {9, 8, 7, 6, 5, 4, 3, 2, 1};
      CHECK_EQ(data, expected);
    }
  }

  TEST_CASE("SubTaskGraph: linearization") {
    helios::async::TaskGraph main_graph("SubTaskLinearGraph");
    helios::async::Executor executor(2);

    SUBCASE("Linearize tasks within subflow") {
      std::vector<int> execution_order;
      std::atomic<int> order_counter{0};
      std::mutex order_mutex;

      auto main_task = main_graph.EmplaceTask([&](helios::async::SubTaskGraph& sub_graph) {
        std::vector<helios::async::Task> ordered_tasks;

        // Create tasks that will be linearized
        for (int i = 0; i < 5; ++i) {
          ordered_tasks.push_back(sub_graph.EmplaceTask([&order_mutex, &execution_order, &order_counter, i]() {
            std::lock_guard<std::mutex> lock(order_mutex);
            execution_order.push_back(order_counter.fetch_add(1));
          }));
        }

        // Linearize the tasks to ensure sequential execution
        sub_graph.Linearize(ordered_tasks);

        sub_graph.Join();
      });

      auto future = executor.Run(std::move(main_graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "Manual dependencies did not complete within timeout");

      // Verify tasks executed in order
      CHECK_EQ(execution_order.size(), 5);
      for (size_t i = 0; i < execution_order.size(); ++i) {
        CHECK_EQ(execution_order[i], static_cast<int>(i));
      }
    }

    SUBCASE("Linearize with array") {
      std::vector<int> execution_order;
      std::atomic<int> order_counter{0};
      std::mutex order_mutex;

      auto main_task = main_graph.EmplaceTask([&](helios::async::SubTaskGraph& sub_graph) {
        std::array<helios::async::Task, 3> tasks = {
            sub_graph.EmplaceTask([&order_mutex, &execution_order, &order_counter]() {
              std::lock_guard<std::mutex> lock(order_mutex);
              execution_order.push_back(order_counter.fetch_add(1));
            }),
            sub_graph.EmplaceTask([&order_mutex, &execution_order, &order_counter]() {
              std::lock_guard<std::mutex> lock(order_mutex);
              execution_order.push_back(order_counter.fetch_add(1));
            }),
            sub_graph.EmplaceTask([&order_mutex, &execution_order, &order_counter]() {
              std::lock_guard<std::mutex> lock(order_mutex);
              execution_order.push_back(order_counter.fetch_add(1));
            })};

        sub_graph.Linearize(tasks);

        sub_graph.Join();
      });

      auto future = executor.Run(std::move(main_graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "Complex dependencies did not complete within timeout");

      // Verify tasks executed in order
      CHECK_EQ(execution_order.size(), 3);
      for (size_t i = 0; i < execution_order.size(); ++i) {
        CHECK_EQ(execution_order[i], static_cast<int>(i));
      }
    }
  }

  TEST_CASE("SubTaskGraph: task management") {
    helios::async::TaskGraph main_graph("SubTaskManagementGraph");
    helios::async::Executor executor(2);

    SUBCASE("RemoveTask from subflow") {
      std::atomic<int> execution_count{0};

      auto main_task = main_graph.EmplaceTask([&](helios::async::SubTaskGraph& sub_graph) {
        auto task1 = sub_graph.EmplaceTask([&execution_count]() { ++execution_count; });
        auto task2 = sub_graph.EmplaceTask([&execution_count]() { ++execution_count; });
        auto task3 = sub_graph.EmplaceTask([&execution_count]() { ++execution_count; });

        // Remove the middle task
        sub_graph.RemoveTask(task2);

        sub_graph.Join();
      });

      auto future = executor.Run(std::move(main_graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "Conditional execution did not complete within timeout");

      CHECK_EQ(execution_count.load(), 2);  // Only task1 and task3 should execute
    }

    SUBCASE("ComposedOf with external graph") {
      helios::async::TaskGraph external_graph("ExternalGraph");
      std::atomic<bool> external_executed{false};
      std::atomic<bool> sub_executed{false};

      // Add task to external graph
      external_graph.EmplaceTask([&external_executed]() { external_executed = true; });

      auto main_task = main_graph.EmplaceTask([&](helios::async::SubTaskGraph& sub_graph) {
        auto composed_task = sub_graph.ComposedOf(external_graph);
        CHECK_FALSE(composed_task.Empty());
        CHECK(composed_task.HasWork());

        auto sub_task = sub_graph.EmplaceTask([&sub_executed]() { sub_executed = true; });

        sub_graph.Join();
      });

      auto future = executor.Run(std::move(main_graph));
      auto status = future.WaitFor(std::chrono::milliseconds(1000));
      REQUIRE_MESSAGE(status == std::future_status::ready, "External dependency did not complete within timeout");

      CHECK(external_executed.load());
      CHECK(sub_executed.load());
    }
  }

  TEST_CASE("SubTaskGraph: complex dependency patterns") {
    helios::async::TaskGraph main_graph("SubTaskComplexGraph");
    helios::async::Executor executor(4);

    SUBCASE("Diamond pattern within subflow") {
      std::vector<int> execution_order;
      std::atomic<int> order_counter{0};
      std::mutex order_mutex;

      auto main_task = main_graph.EmplaceTask([&](helios::async::SubTaskGraph& sub_graph) {
        // Create diamond dependency pattern: task_a -> (task_b, task_c) -> task_d

        auto task_a = sub_graph
                          .EmplaceTask([&execution_order, &order_counter, &order_mutex]() {
                            std::lock_guard<std::mutex> lock(order_mutex);
                            execution_order.push_back(order_counter.fetch_add(1));
                          })
                          .Name("SubTaskA");

        auto task_b = sub_graph
                          .EmplaceTask([&execution_order, &order_counter, &order_mutex]() {
                            std::lock_guard<std::mutex> lock(order_mutex);
                            execution_order.push_back(order_counter.fetch_add(1));
                          })
                          .Name("SubTaskB");

        auto task_c = sub_graph
                          .EmplaceTask([&execution_order, &order_counter, &order_mutex]() {
                            std::lock_guard<std::mutex> lock(order_mutex);
                            execution_order.push_back(order_counter.fetch_add(1));
                          })
                          .Name("SubTaskC");

        auto task_d = sub_graph
                          .EmplaceTask([&execution_order, &order_counter, &order_mutex]() {
                            std::lock_guard<std::mutex> lock(order_mutex);
                            execution_order.push_back(order_counter.fetch_add(1));
                          })
                          .Name("SubTaskD");

        // Set up diamond dependencies
        task_a.Precede(task_b, task_c);
        task_d.Succeed(task_b, task_c);

        CHECK_EQ(task_a.SuccessorsCount(), 2);
        CHECK_EQ(task_d.PredecessorsCount(), 2);

        sub_graph.Join();
      });

      auto future = executor.Run(std::move(main_graph));
      future.Wait();

      CHECK_EQ(execution_order.size(), 4);
      CHECK_EQ(execution_order[0], 0);  // task_a first
      CHECK_EQ(execution_order[3], 3);  // task_d last
      // task_b and task_c can execute in parallel
      bool valid_order =
          (execution_order[1] == 1 && execution_order[2] == 2) || (execution_order[1] == 2 && execution_order[2] == 1);
      CHECK(valid_order);
    }
  }

  TEST_CASE("SubTaskGraph: executor delegation methods") {
    helios::async::TaskGraph main_graph("SubTaskExecutorDelegationGraph");
    helios::async::Executor executor(4);

    SUBCASE("Run external graph from subflow") {
      helios::async::TaskGraph external_graph("ExternalFromSub");
      std::atomic<bool> external_executed{false};

      external_graph.EmplaceTask([&external_executed]() { external_executed = true; });

      auto main_task = main_graph.EmplaceTask([&](helios::async::SubTaskGraph& sub_graph) {
        auto run_future = sub_graph.Run(external_graph);
        run_future.Wait();
        sub_graph.Join();
      });

      auto future = executor.Run(std::move(main_graph));
      future.Wait();

      CHECK(external_executed.load());
    }

    SUBCASE("Async task creation from subflow") {
      std::atomic<int> async_result{0};

      auto main_task = main_graph.EmplaceTask([&](helios::async::SubTaskGraph& sub_graph) {
        auto future = sub_graph.Async([&async_result]() {
          async_result = 42;
          return 100;
        });

        CHECK(future.valid());
        int result = future.get();
        CHECK_EQ(result, 100);

        sub_graph.Join();
      });

      auto future = executor.Run(std::move(main_graph));
      future.Wait();

      CHECK_EQ(async_result.load(), 42);
    }

    SUBCASE("SilentAsync from subflow") {
      std::atomic<bool> silent_executed{false};

      auto main_task = main_graph.EmplaceTask([&silent_executed](helios::async::SubTaskGraph& sub_graph) {
        sub_graph.SilentAsync([&silent_executed]() { silent_executed = true; });

        // sub_graph.WaitForAll();  // Deadlock
        sub_graph.Join();
      });

      executor.Run(std::move(main_graph));
      executor.WaitForAll();  // Need to wait for task graph and silent async task

      CHECK(silent_executed.load());
    }
  }

  TEST_CASE("SubTaskGraph: worker thread information") {
    helios::async::TaskGraph main_graph("SubTaskWorkerInfoGraph");
    helios::async::Executor executor(4);

    SUBCASE("Worker thread detection from subflow") {
      std::atomic<bool> is_worker_thread{false};
      std::atomic<int> worker_id{-2};
      std::atomic<size_t> worker_count{0};
      std::atomic<size_t> queue_count{0};

      auto main_task = main_graph.EmplaceTask([&](helios::async::SubTaskGraph& sub_graph) {
        is_worker_thread = sub_graph.IsWorkerThread();
        worker_id = sub_graph.CurrentWorkerId();
        worker_count = sub_graph.WorkerCount();
        queue_count = sub_graph.QueueCount();

        sub_graph.Join();
      });

      auto future = executor.Run(std::move(main_graph));
      future.Wait();

      CHECK(is_worker_thread.load());
      CHECK_GE(worker_id.load(), 0);
      CHECK_LT(worker_id.load(), static_cast<int>(executor.WorkerCount()));
      CHECK_EQ(worker_count.load(), executor.WorkerCount());
      CHECK_GT(queue_count.load(), 0);
    }
  }

  TEST_CASE("SubTaskGraph: error handling and edge cases") {
    helios::async::TaskGraph main_graph("SubTaskErrorHandlingGraph");
    helios::async::Executor executor(2);

    SUBCASE("Exception in sub task") {
      std::atomic<bool> normal_task_executed{false};

      auto main_task = main_graph.EmplaceTask([&](helios::async::SubTaskGraph& sub_graph) {
        auto throwing_task = sub_graph.EmplaceTask([]() { throw std::runtime_error("Sub task exception"); });

        auto normal_task = sub_graph.EmplaceTask([&normal_task_executed]() { normal_task_executed = true; });

        sub_graph.Join();
      });

      auto future = executor.Run(std::move(main_graph));
      future.Wait();

      // Normal task should still execute despite the throwing task
      CHECK(normal_task_executed.load());
    }

    SUBCASE("Multiple joins should be safe") {
      std::atomic<bool> task_executed{false};

      auto main_task = main_graph.EmplaceTask([&](helios::async::SubTaskGraph& sub_graph) {
        auto task = sub_graph.EmplaceTask([&task_executed]() { task_executed = true; });

        CHECK(sub_graph.Joinable());
        sub_graph.Join();

        // Multiple joins should throw
        CHECK_THROWS(sub_graph.Join());
      });

      auto future = executor.Run(std::move(main_graph));
      future.Wait();

      CHECK(task_executed.load());
    }

    SUBCASE("Empty subflow join") {
      auto main_task = main_graph.EmplaceTask([](helios::async::SubTaskGraph& sub_graph) {
        // Create empty subflow and join
        CHECK(sub_graph.Joinable());
        sub_graph.Join();
      });

      auto future = executor.Run(std::move(main_graph));
      CHECK_NOTHROW(future.Wait());
    }
  }

}  // TEST_SUITE
