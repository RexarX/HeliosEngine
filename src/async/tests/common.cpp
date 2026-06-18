#include <doctest/doctest.h>

#include <helios/async/common.hpp>
#include <helios/async/sub_task_graph.hpp>

#include <cstdint>
#include <string>
#include <type_traits>

using namespace helios::async;

TEST_SUITE("helios::async::Common") {
  TEST_CASE("async::Common: async::TaskType enum values") {
    CHECK_EQ(std::to_underlying(TaskType::kUndefined), 0);
    CHECK_EQ(std::to_underlying(TaskType::kStatic), 1);
    CHECK_EQ(std::to_underlying(TaskType::kSubTask), 2);
    CHECK_EQ(std::to_underlying(TaskType::kAsync), 3);
  }

  TEST_CASE("async::Common: async::AsyncError enum values") {
    CHECK_EQ(std::to_underlying(AsyncError::kInvalidTask), 0);
    CHECK_EQ(std::to_underlying(AsyncError::kExecutorShutdown), 1);
    CHECK_EQ(std::to_underlying(AsyncError::kTaskNotFound), 2);
    CHECK_EQ(std::to_underlying(AsyncError::kInvalidDependency), 3);
    CHECK_EQ(std::to_underlying(AsyncError::kCircularDependency), 4);
    CHECK_EQ(std::to_underlying(AsyncError::kSchedulingFailed), 5);
    CHECK_EQ(std::to_underlying(AsyncError::kThreadNotAvailable), 6);
  }

  TEST_CASE("async::Common: async::ToString for AsyncError") {
    CHECK_EQ(ToString(AsyncError::kInvalidTask), "Invalid task");
    CHECK_EQ(ToString(AsyncError::kExecutorShutdown), "Executor is shutdown");
    CHECK_EQ(ToString(AsyncError::kTaskNotFound), "Task not found");
    CHECK_EQ(ToString(AsyncError::kInvalidDependency), "Invalid dependency");
    CHECK_EQ(ToString(AsyncError::kCircularDependency),
             "Circular dependency detected");
    CHECK_EQ(ToString(AsyncError::kSchedulingFailed), "Task scheduling failed");
    CHECK_EQ(ToString(AsyncError::kThreadNotAvailable), "Thread not available");
  }

  TEST_CASE("async::Common: async::AsyncResult type alias") {
    SUBCASE("Success case") {
      AsyncResult<int> success_result = 42;
      CHECK(success_result.has_value());
      CHECK_EQ(success_result.value(), 42);
    }

    SUBCASE("Error case") {
      AsyncResult<int> error_result = std::unexpected(AsyncError::kInvalidTask);
      CHECK_FALSE(error_result.has_value());
      CHECK_EQ(error_result.error(), AsyncError::kInvalidTask);
    }

    SUBCASE("Void result") {
      AsyncResult<void> void_result{};
      CHECK(void_result.has_value());
    }
  }

  TEST_CASE("async::Common: async::StaticTask concept") {
    SUBCASE("Valid static tasks") {
      auto lambda_no_args = []() { /* void task */ };
      auto function_pointer = +[]() { /* void task */ };

      CHECK(StaticTask<decltype(lambda_no_args)>);
      CHECK(StaticTask<decltype(function_pointer)>);
    }

    SUBCASE("Invalid static tasks") {
      auto invalid_lambda =
          []([[maybe_unused]] SubTaskGraph& graph) { /* void task */ };
      CHECK_FALSE(StaticTask<decltype(invalid_lambda)>);
      CHECK_FALSE(StaticTask<int>);
      CHECK_FALSE(StaticTask<std::string>);
    }
  }

  TEST_CASE("async::Common: async::SubTask concept") {
    SUBCASE("Valid sub tasks") {
      auto sub_task_lambda = [](SubTaskGraph& graph) { graph.Join(); };
      auto sub_task_function = +[](SubTaskGraph& graph) { graph.Join(); };

      CHECK(SubTask<decltype(sub_task_lambda)>);
      CHECK(SubTask<decltype(sub_task_function)>);
    }

    SUBCASE("Invalid sub tasks") {
      auto invalid_lambda = []() { /* void task */ };
      auto wrong_param_lambda =
          []([[maybe_unused]] int value) { /* void task */ };

      CHECK_FALSE(SubTask<decltype(invalid_lambda)>);
      CHECK_FALSE(SubTask<decltype(wrong_param_lambda)>);
      CHECK_FALSE(SubTask<int>);
    }
  }

  TEST_CASE("async::Common: async::AnyTask concept") {
    SUBCASE("Valid any tasks") {
      auto static_task = []() { /* void task */ };
      auto sub_task = [](SubTaskGraph& graph) { graph.Join(); };

      CHECK(AnyTask<decltype(static_task)>);
      CHECK(AnyTask<decltype(sub_task)>);
    }

    SUBCASE("Invalid any tasks") {
      CHECK_FALSE(AnyTask<int>);
      CHECK_FALSE(AnyTask<std::string>);
    }
  }

  TEST_CASE("async::Common: async::details::ConvertTaskType") {
    CHECK_EQ(details::ConvertTaskType(tf::TaskType::STATIC), TaskType::kStatic);
    CHECK_EQ(details::ConvertTaskType(tf::TaskType::SUBFLOW),
             TaskType::kSubTask);
    CHECK_EQ(details::ConvertTaskType(tf::TaskType::ASYNC), TaskType::kAsync);
  }

}  // TEST_SUITE
