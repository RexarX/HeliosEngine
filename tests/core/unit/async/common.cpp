#include <doctest/doctest.h>

#include <helios/core/async/common.hpp>
#include <helios/core/async/sub_task_graph.hpp>

#include <cstdint>
#include <string>
#include <type_traits>

using namespace helios::async;

TEST_SUITE("async::Common") {
  TEST_CASE("async::TaskType enum values") {
    CHECK_EQ(std::to_underlying(TaskType::Undefined), 0);
    CHECK_EQ(std::to_underlying(TaskType::Static), 1);
    CHECK_EQ(std::to_underlying(TaskType::SubTask), 2);
    CHECK_EQ(std::to_underlying(TaskType::Async), 3);
  }

  TEST_CASE("async::AsyncError enum values") {
    CHECK_EQ(std::to_underlying(AsyncError::InvalidTask), 0);
    CHECK_EQ(std::to_underlying(AsyncError::ExecutorShutdown), 1);
    CHECK_EQ(std::to_underlying(AsyncError::TaskNotFound), 2);
    CHECK_EQ(std::to_underlying(AsyncError::InvalidDependency), 3);
    CHECK_EQ(std::to_underlying(AsyncError::CircularDependency), 4);
    CHECK_EQ(std::to_underlying(AsyncError::SchedulingFailed), 5);
    CHECK_EQ(std::to_underlying(AsyncError::ThreadNotAvailable), 6);
  }

  TEST_CASE("async::ToString for AsyncError") {
    CHECK_EQ(ToString(AsyncError::InvalidTask), "Invalid task");
    CHECK_EQ(ToString(AsyncError::ExecutorShutdown), "Executor is shutdown");
    CHECK_EQ(ToString(AsyncError::TaskNotFound), "Task not found");
    CHECK_EQ(ToString(AsyncError::InvalidDependency), "Invalid dependency");
    CHECK_EQ(ToString(AsyncError::CircularDependency), "Circular dependency detected");
    CHECK_EQ(ToString(AsyncError::SchedulingFailed), "Task scheduling failed");
    CHECK_EQ(ToString(AsyncError::ThreadNotAvailable), "Thread not available");
  }

  TEST_CASE("async::AsyncResult type alias") {
    SUBCASE("Success case") {
      AsyncResult<int> success_result = 42;
      CHECK(success_result.has_value());
      CHECK_EQ(success_result.value(), 42);
    }

    SUBCASE("Error case") {
      AsyncResult<int> error_result = std::unexpected(AsyncError::InvalidTask);
      CHECK_FALSE(error_result.has_value());
      CHECK_EQ(error_result.error(), AsyncError::InvalidTask);
    }

    SUBCASE("Void result") {
      AsyncResult<void> void_result{};
      CHECK(void_result.has_value());
    }
  }

  TEST_CASE("async::StaticTask concept") {
    SUBCASE("Valid static tasks") {
      auto lambda_no_args = []() { /* void task */ };
      auto function_pointer = +[]() { /* void task */ };

      CHECK(StaticTask<decltype(lambda_no_args)>);
      CHECK(StaticTask<decltype(function_pointer)>);
    }

    SUBCASE("Invalid static tasks") {
      auto invalid_lambda = []([[maybe_unused]] SubTaskGraph& graph) { /* void task */ };
      CHECK_FALSE(StaticTask<decltype(invalid_lambda)>);
      CHECK_FALSE(StaticTask<int>);
      CHECK_FALSE(StaticTask<std::string>);
    }
  }

  TEST_CASE("async::SubTask concept") {
    SUBCASE("Valid sub tasks") {
      auto sub_task_lambda = [](SubTaskGraph& graph) { graph.Join(); };
      auto sub_task_function = +[](SubTaskGraph& graph) { graph.Join(); };

      CHECK(SubTask<decltype(sub_task_lambda)>);
      CHECK(SubTask<decltype(sub_task_function)>);
    }

    SUBCASE("Invalid sub tasks") {
      auto invalid_lambda = []() { /* void task */ };
      auto wrong_param_lambda = []([[maybe_unused]] int value) { /* void task */ };

      CHECK_FALSE(SubTask<decltype(invalid_lambda)>);
      CHECK_FALSE(SubTask<decltype(wrong_param_lambda)>);
      CHECK_FALSE(SubTask<int>);
    }
  }

  TEST_CASE("async::AnyTask concept") {
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

  TEST_CASE("async::details::ConvertTaskType") {
    CHECK_EQ(details::ConvertTaskType(tf::TaskType::STATIC), TaskType::Static);
    CHECK_EQ(details::ConvertTaskType(tf::TaskType::SUBFLOW), TaskType::SubTask);
    CHECK_EQ(details::ConvertTaskType(tf::TaskType::ASYNC), TaskType::Async);
  }

}  // TEST_SUITE
