#include <doctest/doctest.h>

#include <helios/core/utils/defer.hpp>

#include <functional>
#include <memory>
#include <string>
#include <vector>

TEST_SUITE("helios::utils::Defer") {
  TEST_CASE("DEFER: Basic inline execution") {
    int x = 5;
    {
      HELIOS_DEFER {
        x += 2;
      };
      CHECK_EQ(x, 5);
    }
    CHECK_EQ(x, 7);
  }

  TEST_CASE("DEFER: Multiple statements in defer block") {
    int x = 0;
    std::string str = "Hello";
    {
      HELIOS_DEFER {
        x = 42;
        str += " World";
      };
      CHECK_EQ(x, 0);
      CHECK_EQ(str, "Hello");
    }
    CHECK_EQ(x, 42);
    CHECK_EQ(str, "Hello World");
  }

  TEST_CASE("DEFER: LIFO order with inline syntax") {
    std::vector<int> order;
    {
      HELIOS_DEFER {
        order.push_back(1);
      };
      HELIOS_DEFER {
        order.push_back(2);
      };
      HELIOS_DEFER {
        order.push_back(3);
      };
    }
    REQUIRE_EQ(order.size(), 3);
    CHECK_EQ(order[0], 3);
    CHECK_EQ(order[1], 2);
    CHECK_EQ(order[2], 1);
  }

  TEST_CASE("DEFER: Automatic capture by reference") {
    int x = 10;
    std::string str = "Test";
    {
      HELIOS_DEFER {
        x *= 2;
        str += "!";
      };
      x += 5;
      str += "?";
    }
    CHECK_EQ(x, 30);  // (10 + 5) * 2
    CHECK_EQ(str, "Test?!");
  }

  TEST_CASE("DEFER: Resource cleanup with inline syntax") {
    bool cleaned = false;
    {
      auto* ptr = new int(42);
      HELIOS_DEFER {
        delete ptr;
        cleaned = true;
      };
      CHECK_EQ(*ptr, 42);
      CHECK_FALSE(cleaned);
    }
    CHECK(cleaned);
  }

  TEST_CASE("DEFER: Nested scopes with inline syntax") {
    int counter = 0;
    {
      HELIOS_DEFER {
        counter += 1;
      };
      {
        HELIOS_DEFER {
          counter += 10;
        };
        CHECK_EQ(counter, 0);
      }
      CHECK_EQ(counter, 10);
    }
    CHECK_EQ(counter, 11);
  }

  TEST_CASE("DEFER: Exception safety with inline syntax") {
    bool cleanup_called = false;
    CHECK_THROWS_AS(
        [&cleanup_called]() {
          HELIOS_DEFER {
            cleanup_called = true;
          };
          throw std::runtime_error("Test");
        }(),
        std::runtime_error);
    CHECK(cleanup_called);
  }

  TEST_CASE("DEFER: Complex resource management") {
    struct FileHandle {
      bool& closed;
      explicit FileHandle(bool& c) : closed(c) { closed = false; }
    };

    bool file1_closed = false;
    bool file2_closed = false;

    {
      FileHandle file1{file1_closed};
      HELIOS_DEFER {
        file1.closed = true;
      };

      FileHandle file2{file2_closed};
      HELIOS_DEFER {
        file2.closed = true;
      };

      CHECK_FALSE(file1_closed);
      CHECK_FALSE(file2_closed);
    }

    CHECK(file1_closed);
    CHECK(file2_closed);
  }

  TEST_CASE("DEFER: Conditional logic in defer block") {
    bool condition = true;
    int result = 0;
    {
      HELIOS_DEFER {
        if (condition) {
          result = 100;
        } else {
          result = 200;
        }
      };
      condition = false;
    }
    CHECK_EQ(result, 200);  // condition changed before defer executes
  }

  TEST_CASE("DEFER: Loop in defer block") {
    std::vector<int> vec;
    {
      HELIOS_DEFER {
        for (int i = 0; i < 5; ++i) {
          vec.push_back(i);
        }
      };
      CHECK(vec.empty());
    }
    REQUIRE_EQ(vec.size(), 5);
    for (int i = 0; i < 5; ++i) {
      CHECK_EQ(vec[i], i);
    }
  }

  TEST_CASE("DEFER_CALL: Basic execution on scope exit") {
    int x = 5;
    {
      auto lambda = [&x]() { x += 2; };
      HELIOS_DEFER_CALL(lambda);
      CHECK_EQ(x, 5);
    }
    CHECK_EQ(x, 7);
  }

  TEST_CASE("DEFER_CALL: Multiple defers execute in LIFO order") {
    float x = 5.0F;
    {
      auto defer1 = [&x]() { x += 2; };  // Executes second
      auto defer2 = [&x]() { x /= 2; };  // Executes first
      HELIOS_DEFER_CALL(defer1);
      HELIOS_DEFER_CALL(defer2);
    }
    CHECK_EQ(x, doctest::Approx(4.5F));
  }

  TEST_CASE("DEFER_CALL: LIFO execution order verification") {
    std::vector<int> execution_order;
    {
      auto defer1 = [&execution_order]() { execution_order.push_back(1); };
      auto defer2 = [&execution_order]() { execution_order.push_back(2); };
      auto defer3 = [&execution_order]() { execution_order.push_back(3); };
      HELIOS_DEFER_CALL(defer1);
      HELIOS_DEFER_CALL(defer2);
      HELIOS_DEFER_CALL(defer3);
    }
    REQUIRE_EQ(execution_order.size(), 3);
    CHECK_EQ(execution_order[0], 3);
    CHECK_EQ(execution_order[1], 2);
    CHECK_EQ(execution_order[2], 1);
  }

  TEST_CASE("DEFER_CALL: Resource cleanup") {
    bool resource_freed = false;
    {
      auto* ptr = new int(42);
      auto cleanup = [ptr, &resource_freed]() {
        delete ptr;
        resource_freed = true;
      };
      HELIOS_DEFER_CALL(cleanup);
      CHECK_EQ(*ptr, 42);
      CHECK_FALSE(resource_freed);
    }
    CHECK(resource_freed);
  }

  TEST_CASE("DEFER_CALL: Nested scopes") {
    int counter = 0;
    {
      auto defer1 = [&counter]() { counter += 1; };
      HELIOS_DEFER_CALL(defer1);
      CHECK_EQ(counter, 0);
      {
        auto defer2 = [&counter]() { counter += 10; };
        HELIOS_DEFER_CALL(defer2);
        CHECK_EQ(counter, 0);
      }
      CHECK_EQ(counter, 10);  // Inner defer executed
    }
    CHECK_EQ(counter, 11);  // Outer defer executed
  }

  TEST_CASE("DEFER_CALL: Capture by value") {
    int x = 100;
    int result = 0;
    {
      auto lambda = [x, &result]() { result = x * 2; };
      HELIOS_DEFER_CALL(lambda);
      x = 200;  // Modify original
    }
    CHECK_EQ(result, 200);  // Uses captured value (100 * 2)
    CHECK_EQ(x, 200);
  }

  TEST_CASE("DEFER_CALL: Capture by reference") {
    int x = 100;
    int result = 0;
    {
      auto lambda = [&x, &result]() { result = x * 2; };
      HELIOS_DEFER_CALL(lambda);
      x = 200;  // Modify original
    }
    CHECK_EQ(result, 400);  // Uses current value (200 * 2)
    CHECK_EQ(x, 200);
  }

  TEST_CASE("DEFER_CALL: String manipulation") {
    std::string str = "Hello";
    {
      auto lambda = [&str]() { str += " World"; };
      HELIOS_DEFER_CALL(lambda);
      str += ",";
      CHECK_EQ(str, "Hello,");
    }
    CHECK_EQ(str, "Hello, World");
  }

  TEST_CASE("DEFER_CALL: Multiple defers on same line") {
    int a = 0;
    int b = 0;
    {
      auto defer_a = [&a]() { a = 1; };
      auto defer_b = [&b]() { b = 2; };
      HELIOS_DEFER_CALL(defer_a);
      HELIOS_DEFER_CALL(defer_b);
      CHECK_EQ(a, 0);
      CHECK_EQ(b, 0);
    }
    CHECK_EQ(a, 1);
    CHECK_EQ(b, 2);
  }

  TEST_CASE("DEFER_CALL: Works with mutable lambda") {
    int counter = 0;
    {
      int local = 5;
      auto lambda = [local, &counter]() mutable {
        local *= 2;
        counter = local;
      };
      HELIOS_DEFER_CALL(lambda);
    }
    CHECK_EQ(counter, 10);
  }

  TEST_CASE("DEFER_CALL: Exception safety") {
    bool cleanup_called = false;

    SUBCASE("Defer executes even with exception") {
      CHECK_THROWS_AS(
          [&cleanup_called]() {
            auto lambda = [&cleanup_called]() { cleanup_called = true; };
            HELIOS_DEFER_CALL(lambda);
            throw std::runtime_error("Test exception");
          }(),
          std::runtime_error);
      CHECK(cleanup_called);
    }

    SUBCASE("Multiple defers all execute with exception") {
      int cleanup_count = 0;
      CHECK_THROWS_AS(
          [&cleanup_count]() {
            auto d1 = [&cleanup_count]() { ++cleanup_count; };
            auto d2 = [&cleanup_count]() { ++cleanup_count; };
            auto d3 = [&cleanup_count]() { ++cleanup_count; };
            HELIOS_DEFER_CALL(d1);
            HELIOS_DEFER_CALL(d2);
            HELIOS_DEFER_CALL(d3);
            throw std::runtime_error("Test exception");
          }(),
          std::runtime_error);
      CHECK_EQ(cleanup_count, 3);
    }
  }

  TEST_CASE("DEFER_CALL: Complex cleanup scenario") {
    struct Resource {
      bool& freed;

      explicit constexpr Resource(bool& f) noexcept : freed(f) { freed = false; }
      constexpr ~Resource() noexcept = default;
      Resource(const Resource&) = delete;
      Resource(Resource&&) = delete;

      Resource& operator=(const Resource&) = delete;
      Resource& operator=(Resource&&) = delete;

      constexpr void Free() noexcept { freed = true; }
    };

    bool resource1_freed = false;
    bool resource2_freed = false;

    {
      auto res1 = std::make_unique<Resource>(resource1_freed);
      auto cleanup1 = [&res1]() { res1->Free(); };
      HELIOS_DEFER_CALL(cleanup1);

      auto res2 = std::make_unique<Resource>(resource2_freed);
      auto cleanup2 = [&res2]() { res2->Free(); };
      HELIOS_DEFER_CALL(cleanup2);

      CHECK_FALSE(resource1_freed);
      CHECK_FALSE(resource2_freed);
    }

    CHECK(resource1_freed);
    CHECK(resource2_freed);
  }

  TEST_CASE("DEFER_CALL: Stateful lambda") {
    struct Counter {
      int count = 0;

      constexpr void Increment() noexcept { ++count; }
    };

    Counter counter;
    {
      auto lambda = [&counter]() {
        counter.Increment();
        counter.Increment();
      };
      HELIOS_DEFER_CALL(lambda);
      CHECK_EQ(counter.count, 0);
    }
    CHECK_EQ(counter.count, 2);
  }

  TEST_CASE("DEFER_CALL: Empty lambda") {
    int x = 5;
    {
      auto lambda = []() {};
      HELIOS_DEFER_CALL(lambda);
      x = 10;
    }
    CHECK_EQ(x, 10);
  }

  TEST_CASE("DEFER_CALL: Conditional execution in lambda") {
    bool condition = true;
    int result = 0;
    {
      auto lambda = [&condition, &result]() {
        if (condition) {
          result = 42;
        }
      };
      HELIOS_DEFER_CALL(lambda);
      CHECK_EQ(result, 0);
      condition = false;
    }
    CHECK_EQ(result, 0);
  }

  TEST_CASE("DEFER_CALL: Lambda with std::function wrapper") {
    int x = 0;
    std::function<void()> func = [&x]() { x = 100; };
    {
      HELIOS_DEFER_CALL(func);
      CHECK_EQ(x, 0);
    }
    CHECK_EQ(x, 100);
  }

  TEST_CASE("DEFER_CALL: Functor object") {
    struct Incrementer {
      int& value;

      explicit constexpr Incrementer(int& v) noexcept : value(v) {}

      constexpr void operator()() const noexcept { value += 5; }
    };

    int x = 10;
    {
      HELIOS_DEFER_CALL(Incrementer{x});
      CHECK_EQ(x, 10);
    }
    CHECK_EQ(x, 15);
  }

  TEST_CASE("DEFER_CALL: Function pointer with captured context") {
    struct Context {
      int counter = 0;

      static constexpr void Increment(Context* ctx) noexcept { ctx->counter++; }
    };

    Context ctx;
    {
      auto func_ptr = [](Context* c) { Context::Increment(c); };
      auto lambda = [&ctx, func_ptr]() { func_ptr(&ctx); };
      HELIOS_DEFER_CALL(lambda);
      CHECK_EQ(ctx.counter, 0);
    }
    CHECK_EQ(ctx.counter, 1);
  }

  TEST_CASE("DEFER_CALL: Lambda wrapping function with arguments") {
    auto add_to_value = [](int& target, int value) { target += value; };

    int x = 0;
    {
      auto lambda = [&x, add_to_value]() { add_to_value(x, 10); };
      HELIOS_DEFER_CALL(lambda);
      CHECK_EQ(x, 0);
    }
    CHECK_EQ(x, 10);
  }

  TEST_CASE("DEFER_CALL: Multiple callable types in LIFO order") {
    auto add_value = [](int& target, int value) { target += value; };

    int counter = 0;
    {
      auto d1 = [&counter, add_value]() { add_value(counter, 1); };
      auto d2 = [&counter, add_value]() { add_value(counter, 10); };
      auto d3 = [&counter, add_value]() { add_value(counter, 100); };
      HELIOS_DEFER_CALL(d1);
      HELIOS_DEFER_CALL(d2);
      HELIOS_DEFER_CALL(d3);
      CHECK_EQ(counter, 0);
    }
    // Executes in reverse: 100, then 10, then 1
    CHECK_EQ(counter, 111);
  }

  TEST_CASE("Mixed: DEFER_CALL and DEFER together") {
    std::vector<int> order;
    {
      auto lambda = [&order]() { order.push_back(1); };
      HELIOS_DEFER_CALL(lambda);
      HELIOS_DEFER {
        order.push_back(2);
      };
      auto lambda2 = [&order]() { order.push_back(3); };
      HELIOS_DEFER_CALL(lambda2);
      HELIOS_DEFER {
        order.push_back(4);
      };
    }
    REQUIRE_EQ(order.size(), 4);
    // LIFO: last defer (4), then lambda2 (3), then second defer (2), then first lambda (1)
    CHECK_EQ(order[0], 4);
    CHECK_EQ(order[1], 3);
    CHECK_EQ(order[2], 2);
    CHECK_EQ(order[3], 1);
  }
}  // TEST_SUITE
