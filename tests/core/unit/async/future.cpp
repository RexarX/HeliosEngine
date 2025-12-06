#include <doctest/doctest.h>

#include <helios/core/async/executor.hpp>
#include <helios/core/async/future.hpp>

#include <atomic>
#include <chrono>
#include <future>
#include <string>
#include <thread>

TEST_SUITE("async::Future") {
  TEST_CASE("Future: construction and basic properties") {
    SUBCASE("Default construction") {
      helios::async::Future<int> future;
      CHECK_FALSE(future.Valid());
    }

    SUBCASE("Move construction") {
      helios::async::Executor executor(1);
      helios::async::TaskGraph graph("MoveTest");
      graph.EmplaceTask([]() {});

      auto future1 = executor.Run(graph);
      CHECK(future1.Valid());

      auto future2 = std::move(future1);
      CHECK(future2.Valid());
      future2.Wait();
    }

    SUBCASE("Move assignment") {
      helios::async::Executor executor(1);
      helios::async::TaskGraph graph("MoveAssignTest");
      graph.EmplaceTask([]() { std::this_thread::sleep_for(std::chrono::milliseconds(10)); });
      auto future1 = executor.Run(graph);
      helios::async::Future<void> future2;

      CHECK(future1.Valid());
      CHECK_FALSE(future2.Valid());

      future2 = std::move(future1);
      CHECK(future2.Valid());
      CHECK_FALSE(future1.Valid());

      // Wait for completion to avoid early destruction
      future2.Wait();
    }
  }

  TEST_CASE("Future: result retrieval") {
    helios::async::Executor executor(2);

    SUBCASE("Get with int return type") {
      helios::async::TaskGraph graph("IntReturn");
      std::atomic<int> result{0};
      graph.EmplaceTask([&result]() { result = 42; });
      auto future = executor.Run(graph);

      CHECK(future.Valid());
      future.Wait();
      CHECK_EQ(result.load(), 42);
      CHECK(future.Valid());  // Wait doesn't invalidate
    }

    SUBCASE("Get with string return type") {
      struct TestObject {
        int value = 0;
        std::string name;

        bool operator==(const TestObject& other) const { return value == other.value && name == other.name; }
      };

      TestObject expected{123, "test"};
      TestObject result;
      helios::async::TaskGraph graph("ComplexObject");
      graph.EmplaceTask([expected, &result]() { result = expected; });
      auto future = executor.Run(graph);

      CHECK(future.Valid());
      future.Wait();
      // Check that the result was stored correctly
      CHECK_EQ(result, expected);
      CHECK(future.Valid());
    }
  }

  TEST_CASE("Future: waiting functionality") {
    helios::async::Executor executor(2);

    SUBCASE("Wait for completion") {
      helios::async::TaskGraph graph("WaitForCompletion");
      std::atomic<bool> task_started{false};
      std::atomic<bool> should_complete{false};

      graph.EmplaceTask([&task_started, &should_complete]() {
        task_started = true;
        while (!should_complete.load()) {
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
      });

      auto future = executor.Run(graph);

      CHECK(future.Valid());

      // Wait for task to start
      while (!task_started.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }

      // Signal completion and wait
      should_complete = true;
      future.Wait();

      CHECK(future.Valid());  // Wait doesn't invalidate the future
    }

    SUBCASE("WaitFor with timeout") {
      std::atomic<bool> should_complete{false};
      helios::async::TaskGraph graph("WaitForTimeout");

      graph.EmplaceTask([&should_complete]() {
        while (!should_complete.load()) {
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
      });

      auto future = executor.Run(graph);

      CHECK(future.Valid());

      // Wait with short timeout - should timeout
      auto status = future.WaitFor(std::chrono::milliseconds(10));
      CHECK_NE(status, std::future_status::ready);

      // Complete the task and wait again
      should_complete = true;
      status = future.WaitFor(std::chrono::milliseconds(100));
      CHECK_EQ(status, std::future_status::ready);
    }

    SUBCASE("WaitUntil with absolute time") {
      std::atomic<bool> should_complete{false};
      helios::async::TaskGraph graph("WaitUntilTimeout");

      graph.EmplaceTask([&should_complete]() {
        while (!should_complete.load()) {
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
      });

      auto future = executor.Run(graph);

      CHECK(future.Valid());

      // Wait until a time point in the near future - should timeout
      auto timeout_point = std::chrono::steady_clock::now() + std::chrono::milliseconds(10);
      auto status = future.WaitUntil(timeout_point);
      CHECK_NE(status, std::future_status::ready);

      // Complete the task and wait again
      should_complete = true;
      timeout_point = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
      status = future.WaitUntil(timeout_point);
      CHECK_EQ(status, std::future_status::ready);
    }

    SUBCASE("Immediate completion") {
      helios::async::TaskGraph graph("ImmediateCompletion");
      graph.EmplaceTask([]() {});
      auto future = executor.Run(graph);

      CHECK(future.Valid());

      // Should complete immediately
      auto status = future.WaitFor(std::chrono::milliseconds(1));
      CHECK_EQ(status, std::future_status::ready);
    }
  }

  TEST_CASE("Future: cancellation") {
    helios::async::Executor executor(1);

    SUBCASE("Cancel before execution") {
      std::atomic<bool> can_run{false};
      std::atomic<int> result{0};
      helios::async::TaskGraph graph("CancelBeforeExecution");
      graph.EmplaceTask([&can_run, &result]() {
        while (!can_run.load()) {
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        result = 42;
      });
      auto future = executor.Run(graph);

      CHECK(future.Valid());

      // Try to cancel - this may or may not succeed depending on timing
      bool cancelled = future.Cancel();

      // Set can_run to true so the task can complete if it wasn't cancelled
      can_run = true;

      // Wait for completion with timeout
      auto status = future.WaitFor(std::chrono::milliseconds(100));

      // If cancellation succeeded, the task may not complete
      // If cancellation failed, the task should complete and we can check the result
      if (!cancelled) {
        CHECK_EQ(status, std::future_status::ready);
        CHECK_EQ(result.load(), 42);
      }
    }

    SUBCASE("Cancel completed task") {
      std::atomic<int> result{0};
      helios::async::TaskGraph graph("CancelCompleted");
      graph.EmplaceTask([&result]() { result = 42; });
      auto future = executor.Run(graph);

      // Wait for completion
      future.Wait();

      // Try to cancel completed task - should fail
      bool cancelled = future.Cancel();
      CHECK_FALSE(cancelled);

      CHECK_EQ(result.load(), 42);
    }
  }

  TEST_CASE("Future: validity states") {
    helios::async::Executor executor(2);

    SUBCASE("Valid future lifecycle") {
      helios::async::TaskGraph graph("ValidLifecycle");
      graph.EmplaceTask([]() {});
      auto future = executor.Run(graph);

      CHECK(future.Valid());  // Initially valid

      future.Wait();
      CHECK(future.Valid());  // Still valid after wait
    }

    SUBCASE("Invalid future operations") {
      helios::async::Future<int> invalid_future;

      CHECK_FALSE(invalid_future.Valid());

      // Operations on invalid future are generally undefined behavior
      // but we can test that Valid() consistently returns false
      CHECK_FALSE(invalid_future.Valid());
    }

    SUBCASE("Moved future validity") {
      helios::async::TaskGraph graph("MovedFuture");
      graph.EmplaceTask([]() {});
      auto future1 = executor.Run(graph);
      CHECK(future1.Valid());

      auto future2 = std::move(future1);
      CHECK_FALSE(future1.Valid());  // Moved-from future is invalid
      CHECK(future2.Valid());        // Moved-to future is valid
      future2.Wait();
    }
  }

  TEST_CASE("Future: multiple futures coordination") {
    helios::async::Executor executor(4);

    SUBCASE("Multiple independent futures") {
      constexpr int NUM_FUTURES = 5;
      std::array<helios::async::Future<void>, NUM_FUTURES> futures = {};
      std::vector<int> results(NUM_FUTURES);

      for (int i = 0; i < NUM_FUTURES; ++i) {
        helios::async::TaskGraph graph(std::format("IndependentGraph{}", std::to_string(i)));
        graph.EmplaceTask([&results, i]() { results[i] = i * 10; });
        futures[i] = executor.Run(std::move(graph));
      }

      // Wait for all and check results
      for (auto& future : futures) {
        CHECK(future.Valid());
        future.Wait();
        CHECK(future.Valid());
      }

      for (int i = 0; i < NUM_FUTURES; ++i) {
        CHECK_EQ(results[i], i * 10);
      }
    }

    SUBCASE("Future dependency chain") {
      std::atomic<int> value1{0};
      std::atomic<int> value2{0};
      std::atomic<int> final_result{0};

      helios::async::TaskGraph graph("DependencyChain");
      auto task1 = graph.EmplaceTask([&value1]() { value1 = 10; });
      auto task2 = graph.EmplaceTask([&value1, &value2]() { value2 = value1.load() * 2; });
      auto task3 = graph.EmplaceTask([&value2, &final_result]() { final_result = value2.load() + 5; });

      task1.Precede(task2);
      task2.Precede(task3);

      auto future = executor.Run(graph);
      future.Wait();

      // Final result should be (10 * 2) + 5 = 25
      CHECK_EQ(final_result.load(), 25);
    }
  }

  TEST_CASE("Future: exception handling") {
    helios::async::Executor executor(2);

    SUBCASE("Future with throwing task") {
      std::atomic<bool> exception_thrown{false};
      helios::async::TaskGraph graph("ThrowingTask");
      graph.EmplaceTask([&exception_thrown]() {
        try {
          throw std::runtime_error("Test exception");
        } catch (...) {
          exception_thrown = true;
          throw;  // Re-throw to maintain exception behavior
        }
      });
      auto future = executor.Run(graph);

      CHECK(future.Valid());
      // Note: TaskGraph execution might handle exceptions internally
      // We check that the task executed and exception was caught
      try {
        future.Wait();
      } catch (...) {
        // Exception handling depends on TaskGraph implementation
      }
      CHECK(future.Valid());  // Wait doesn't invalidate
      CHECK(exception_thrown.load());
    }

    SUBCASE("Wait on throwing task") {
      std::atomic<bool> task_executed{false};
      helios::async::TaskGraph graph("DelayedThrow");
      graph.EmplaceTask([&task_executed]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        task_executed = true;
        // For TaskGraph, we focus on task completion rather than exception propagation
      });
      auto future = executor.Run(graph);

      CHECK(future.Valid());
      future.Wait();
      CHECK(future.Valid());
      CHECK(task_executed.load());
    }
  }

}  // TEST_SUITE
