#include <doctest/doctest.h>

#include <helios/core/async/async_task.hpp>
#include <helios/core/async/executor.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <thread>

using namespace helios::async;

TEST_SUITE("async::AsyncTask") {
  TEST_CASE("AsyncTask: default construction") {
    const AsyncTask task;

    CHECK_FALSE(task.Done());
    CHECK(task.Empty());
    CHECK_EQ(task.Hash(), 0);
    CHECK_EQ(task.UseCount(), 0);
    CHECK_EQ(task.GetTaskType(), TaskType::Async);
  }

  TEST_CASE("AsyncTask: copy and move semantics") {
    SUBCASE("Copy construction") {
      const AsyncTask original_task;
      AsyncTask copied_task = original_task;

      CHECK(original_task.Empty());
      CHECK(copied_task.Empty());
      CHECK_EQ(original_task.Hash(), copied_task.Hash());
    }

    SUBCASE("Move construction") {
      AsyncTask original_task;
      const size_t original_hash = original_task.Hash();

      AsyncTask moved_task = std::move(original_task);

      CHECK_EQ(moved_task.Hash(), original_hash);
    }

    SUBCASE("Copy assignment") {
      AsyncTask task1;
      const AsyncTask task2;

      task1 = task2;
      CHECK_EQ(task1.Hash(), task2.Hash());
    }

    SUBCASE("Move assignment") {
      AsyncTask task1;
      AsyncTask task2;
      const size_t task2_hash = task2.Hash();

      task1 = std::move(task2);
      CHECK_EQ(task1.Hash(), task2_hash);
    }
  }

  TEST_CASE("AsyncTask: equality operators") {
    const AsyncTask task1;
    const AsyncTask task2;

    SUBCASE("Empty tasks equality") {
      CHECK_EQ(task1, task2);  // Both empty tasks should be equal
    }

    SUBCASE("Same task equality") {
      const AsyncTask copied_task = task1;
      CHECK_EQ(task1, copied_task);  // Copied task should be equal to original
    }
  }

  TEST_CASE("AsyncTask: reset functionality") {
    AsyncTask task;

    SUBCASE("Reset empty task") {
      CHECK(task.Empty());
      task.Reset();
      CHECK(task.Empty());
    }
  }

  TEST_CASE("AsyncTask: task completion checking") {
    Executor executor(2);
    std::atomic<bool> should_complete{false};

    SUBCASE("DependentAsync task completion detection") {
      std::vector<AsyncTask> deps;
      auto [task, future] = executor.DependentAsync(
          [&should_complete]() {
            while (!should_complete.load()) {
              std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            return 42;
          },
          deps);

      // Initially task should not be done
      std::this_thread::sleep_for(std::chrono::milliseconds(5));

      // Complete the task
      should_complete = true;
      future.wait();

      CHECK_FALSE(task.Empty());
      CHECK(task.Done());
    }

    SUBCASE("SilentDependentAsync task completion detection") {
      auto deps = std::vector<AsyncTask>{};
      std::atomic<bool> executed{false};
      auto task = executor.SilentDependentAsync(
          [&executed]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            executed = true;
          },
          deps);

      std::this_thread::sleep_for(std::chrono::milliseconds(20));

      CHECK_FALSE(task.Empty());
      CHECK(task.Done());
      CHECK(executed.load());
    }
  }

  TEST_CASE("AsyncTask: hash values") {
    AsyncTask task1;
    AsyncTask task2;

    SUBCASE("Empty tasks have same hash") {
      CHECK_EQ(task1.Hash(), task2.Hash());
    }

    SUBCASE("Copied tasks have same hash") {
      AsyncTask copied_task = task1;
      CHECK_EQ(task1.Hash(), copied_task.Hash());
    }
  }

  TEST_CASE("AsyncTask: use count") {
    AsyncTask task;

    SUBCASE("Empty task use count") {
      CHECK_EQ(task.UseCount(), 0);
    }

    SUBCASE("Copied task use count") {
      AsyncTask copied_task = task;
      // Both should have the same use count for empty tasks
      CHECK_EQ(task.UseCount(), copied_task.UseCount());
    }
  }

  TEST_CASE("AsyncTask: task type") {
    AsyncTask task;

    CHECK_EQ(task.GetTaskType(), TaskType::Async);

    // Task type should remain constant regardless of state
    task.Reset();
    CHECK_EQ(task.GetTaskType(), TaskType::Async);
  }

  TEST_CASE("AsyncTask: empty state") {
    AsyncTask task;

    SUBCASE("Default constructed task is empty") {
      CHECK(task.Empty());
    }

    SUBCASE("Reset task becomes empty") {
      task.Reset();
      CHECK(task.Empty());
    }

    SUBCASE("Copied empty task is empty") {
      AsyncTask copied_task = task;
      CHECK(copied_task.Empty());
    }
  }

}  // TEST_SUITE
