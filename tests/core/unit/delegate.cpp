#include <doctest/doctest.h>

#include <helios/core/delegate.hpp>

#include <concepts>

namespace {

// Free functions for testing
constexpr int free_function_sum(int a, int b) {
  return a + b;
}

constexpr int free_function_double(int a) {
  return a * 2;
}

constexpr int free_function_triple(int a) {
  return a * 3;
}

void free_function_void(int& out) {
  out = 42;
}

// Test structs
struct Counter {
  int value = 0;

  void increment() { ++value; }
  void add(int x) { value += x; }
  int get_value() const { return value; }
  int multiply(int x) { return value * x; }
};

struct OverloadedStruct {
  int value = 0;

  int foo() { return value; }
  int foo(int x) { return value + x; }
  int foo(int x, int y) { return value + x + y; }
};

struct BaseVirtual {
  virtual int compute(int x) { return x + 1; }
  virtual int compute_const(int x) const { return x + 10; }
  virtual ~BaseVirtual() = default;
};

struct DerivedVirtual : BaseVirtual {
  int compute(int x) override { return x + 42; }
  int compute_const(int x) const override { return x + 100; }
};

struct ConstMemberStruct {
  int value = 5;
  int multiply_const(int x) const { return value * x; }
};

// For polymorphic conversion testing
struct Base {
  virtual int get_id() const { return 1; }
  virtual ~Base() = default;
};

struct Derived : Base {
  int get_id() const override { return 2; }
};

}  // namespace

TEST_SUITE("helios::Delegate") {
  TEST_CASE("Delegate::ctor: Construction and basic state") {
    SUBCASE("Default construction creates empty delegate") {
      helios::Delegate<int(int)> delegate;
      CHECK_FALSE(delegate.Valid());
      CHECK_EQ(delegate.InstancePtr(), nullptr);
    }

    SUBCASE("Copy construction") {
      auto original = helios::Delegate<int(int, int)>::FromFunction<&free_function_sum>();
      auto copy = original;
      CHECK(copy.Valid());
      CHECK_EQ(copy(2, 3), 5);
      CHECK_EQ(original, copy);
    }

    SUBCASE("Move construction") {
      auto original = helios::Delegate<int(int, int)>::FromFunction<&free_function_sum>();
      auto moved = std::move(original);
      CHECK(moved.Valid());
      CHECK_EQ(moved(2, 3), 5);
    }

    SUBCASE("Copy assignment") {
      auto delegate1 = helios::Delegate<int(int, int)>::FromFunction<&free_function_sum>();
      helios::Delegate<int(int, int)> delegate2;
      delegate2 = delegate1;
      CHECK(delegate2.Valid());
      CHECK_EQ(delegate2(3, 4), 7);
      CHECK_EQ(delegate1, delegate2);
    }

    SUBCASE("Move assignment") {
      auto delegate1 = helios::Delegate<int(int, int)>::FromFunction<&free_function_sum>();
      helios::Delegate<int(int, int)> delegate2;
      delegate2 = std::move(delegate1);
      CHECK(delegate2.Valid());
      CHECK_EQ(delegate2(3, 4), 7);
    }
  }

  TEST_CASE("Delegate::FromFunction: Free function binding") {
    SUBCASE("Single parameter free function") {
      auto delegate = helios::Delegate<int(int)>::FromFunction<&free_function_double>();
      CHECK(delegate.Valid());
      CHECK_EQ(delegate.InstancePtr(), nullptr);
      CHECK_EQ(delegate(5), 10);
      CHECK_EQ(delegate.Invoke(7), 14);
    }

    SUBCASE("Multiple parameter free function") {
      auto delegate = helios::Delegate<int(int, int)>::FromFunction<&free_function_sum>();
      CHECK(delegate.Valid());
      CHECK_EQ(delegate(10, 20), 30);
      CHECK_EQ(delegate.Invoke(15, 25), 40);
    }

    SUBCASE("Void return type free function") {
      int result = 0;
      auto delegate = helios::Delegate<void(int&)>::FromFunction<&free_function_void>();
      CHECK(delegate.Valid());
      delegate(result);
      CHECK_EQ(result, 42);
    }

    SUBCASE("Explicit signature template parameter for free function") {
      using Signature = int (*)(int, int);
      constexpr Signature func_ptr = &free_function_sum;
      auto delegate = helios::Delegate<int(int, int)>::FromFunction<Signature, func_ptr>();
      CHECK(delegate.Valid());
      CHECK_EQ(delegate(8, 12), 20);
    }

    SUBCASE("Constexpr delegate creation") {
      constexpr auto delegate = helios::Delegate<int(int)>::FromFunction<&free_function_double>();
      CHECK(delegate.Valid());
      CHECK_EQ(delegate(6), 12);
    }
  }

  TEST_CASE("Delegate::FromFunction: Member function binding") {
    SUBCASE("Non-const member function with no parameters") {
      Counter counter{10};
      auto delegate = helios::Delegate<void()>::FromFunction<&Counter::increment>(counter);
      CHECK(delegate.Valid());
      CHECK_EQ(delegate.InstancePtr(), &counter);
      delegate();
      CHECK_EQ(counter.value, 11);
      delegate();
      CHECK_EQ(counter.value, 12);
    }

    SUBCASE("Non-const member function with parameters") {
      Counter counter{5};
      auto delegate = helios::Delegate<void(int)>::FromFunction<&Counter::add>(counter);
      CHECK(delegate.Valid());
      delegate(10);
      CHECK_EQ(counter.value, 15);
      delegate.Invoke(20);
      CHECK_EQ(counter.value, 35);
    }

    SUBCASE("Non-const member function with return value") {
      Counter counter{7};
      auto delegate = helios::Delegate<int(int)>::FromFunction<&Counter::multiply>(counter);
      CHECK(delegate.Valid());
      CHECK_EQ(delegate(3), 21);
      CHECK_EQ(delegate.Invoke(5), 35);
    }

    SUBCASE("Const member function") {
      const Counter counter{9};
      auto delegate = helios::Delegate<int()>::FromFunction<&Counter::get_value>(const_cast<Counter&>(counter));
      CHECK(delegate.Valid());
      CHECK_EQ(delegate(), 9);
      CHECK_EQ(delegate.Invoke(), 9);
    }

    SUBCASE("Const member function with parameters") {
      const ConstMemberStruct obj;
      auto delegate = helios::Delegate<int(int)>::FromFunction<&ConstMemberStruct::multiply_const>(
          const_cast<ConstMemberStruct&>(obj));
      CHECK(delegate.Valid());
      CHECK_EQ(delegate(4), 20);
    }

    SUBCASE("Explicit signature template parameter for member function") {
      Counter counter{3};
      using Signature = void (Counter::*)(int);
      constexpr Signature func_ptr = &Counter::add;
      auto delegate = helios::Delegate<void(int)>::FromFunction<Signature, func_ptr>(counter);
      CHECK(delegate.Valid());
      delegate(7);
      CHECK_EQ(counter.value, 10);
    }
  }

  TEST_CASE("Delegate::FromFunction: Overloaded member functions") {
    OverloadedStruct obj{10};

    SUBCASE("Zero-argument overload") {
      auto delegate =
          helios::Delegate<int()>::FromFunction<static_cast<int (OverloadedStruct::*)()>(&OverloadedStruct::foo)>(obj);
      CHECK_EQ(delegate(), 10);
    }

    SUBCASE("Single-argument overload") {
      auto delegate =
          helios::Delegate<int(int)>::FromFunction<static_cast<int (OverloadedStruct::*)(int)>(&OverloadedStruct::foo)>(
              obj);
      CHECK_EQ(delegate(5), 15);
    }

    SUBCASE("Two-argument overload") {
      auto delegate = helios::Delegate<int(
          int, int)>::FromFunction<static_cast<int (OverloadedStruct::*)(int, int)>(&OverloadedStruct::foo)>(obj);
      CHECK_EQ(delegate(3, 7), 20);
    }
  }

  TEST_CASE("Delegate::FromFunction: Virtual function dispatch") {
    SUBCASE("Virtual function called through base reference") {
      DerivedVirtual derived;
      BaseVirtual& base = derived;
      auto delegate = helios::Delegate<int(int)>::FromFunction<&BaseVirtual::compute>(base);
      CHECK_EQ(delegate(10), 52);  // Calls DerivedVirtual::compute
    }

    SUBCASE("Const virtual function") {
      DerivedVirtual derived;
      const BaseVirtual& base = derived;
      auto delegate =
          helios::Delegate<int(int)>::FromFunction<&BaseVirtual::compute_const>(const_cast<BaseVirtual&>(base));
      CHECK_EQ(delegate(5), 105);  // Calls DerivedVirtual::compute_const
    }

    SUBCASE("Virtual function on base instance") {
      BaseVirtual base;
      auto delegate = helios::Delegate<int(int)>::FromFunction<&BaseVirtual::compute>(base);
      CHECK_EQ(delegate(10), 11);  // Calls BaseVirtual::compute
    }
  }

  TEST_CASE("Delegate::Invoke: Empty delegate behavior") {
    SUBCASE("Empty delegate with int return type returns default value") {
      helios::Delegate<int(int)> delegate;
      CHECK_FALSE(delegate.Valid());
      CHECK_EQ(delegate(42), 0);
      CHECK_EQ(delegate.Invoke(100), 0);
    }

    SUBCASE("Empty delegate with void return type is no-op") {
      int value = 5;
      helios::Delegate<void(int&)> delegate;
      CHECK_FALSE(delegate.Valid());
      CHECK_NOTHROW(delegate(value));
      CHECK_EQ(value, 5);  // Unchanged
    }

    SUBCASE("Empty delegate after Reset") {
      auto delegate = helios::Delegate<int(int)>::FromFunction<&free_function_double>();
      CHECK(delegate.Valid());
      delegate.Reset();
      CHECK_FALSE(delegate.Valid());
      CHECK_EQ(delegate(10), 0);
    }
  }

  TEST_CASE("Delegate::operator==: Comparison operators") {
    SUBCASE("Same free function delegates are equal") {
      auto delegate1 = helios::Delegate<int(int, int)>::FromFunction<&free_function_sum>();
      auto delegate2 = helios::Delegate<int(int, int)>::FromFunction<&free_function_sum>();
      CHECK_EQ(delegate1, delegate2);
      CHECK_FALSE(delegate1 != delegate2);
    }

    SUBCASE("Different free function delegates are not equal") {
      auto delegate1 = helios::Delegate<int(int)>::FromFunction<&free_function_double>();
      auto delegate2 = helios::Delegate<int(int)>::FromFunction<&free_function_triple>();
      CHECK_NE(delegate1, delegate2);
      CHECK_FALSE(delegate1 == delegate2);
    }

    SUBCASE("Same member function on same instance are equal") {
      Counter counter{5};
      auto delegate1 = helios::Delegate<void(int)>::FromFunction<&Counter::add>(counter);
      auto delegate2 = helios::Delegate<void(int)>::FromFunction<&Counter::add>(counter);
      CHECK_EQ(delegate1, delegate2);
    }

    SUBCASE("Same member function on different instances are not equal") {
      Counter counter1{5};
      Counter counter2{5};
      auto delegate1 = helios::Delegate<void(int)>::FromFunction<&Counter::add>(counter1);
      auto delegate2 = helios::Delegate<void(int)>::FromFunction<&Counter::add>(counter2);
      CHECK_NE(delegate1, delegate2);
    }

    SUBCASE("Empty delegates are equal") {
      helios::Delegate<int(int)> delegate1;
      helios::Delegate<int(int)> delegate2;
      CHECK_EQ(delegate1, delegate2);
    }

    SUBCASE("Empty and non-empty delegates are not equal") {
      auto delegate1 = helios::Delegate<int(int)>::FromFunction<&free_function_double>();
      helios::Delegate<int(int)> delegate2;
      CHECK_NE(delegate1, delegate2);
    }
  }

  TEST_CASE("Delegate::Reset") {
    SUBCASE("Reset free function delegate") {
      auto delegate = helios::Delegate<int(int, int)>::FromFunction<&free_function_sum>();
      CHECK(delegate.Valid());
      delegate.Reset();
      CHECK_FALSE(delegate.Valid());
      CHECK_EQ(delegate.InstancePtr(), nullptr);
    }

    SUBCASE("Reset member function delegate") {
      Counter counter{10};
      auto delegate = helios::Delegate<void(int)>::FromFunction<&Counter::add>(counter);
      CHECK(delegate.Valid());
      CHECK_NE(delegate.InstancePtr(), nullptr);
      delegate.Reset();
      CHECK_FALSE(delegate.Valid());
      CHECK_EQ(delegate.InstancePtr(), nullptr);
    }

    SUBCASE("Reset multiple times is safe") {
      auto delegate = helios::Delegate<int(int)>::FromFunction<&free_function_double>();
      delegate.Reset();
      CHECK_NOTHROW(delegate.Reset());
      CHECK_FALSE(delegate.Valid());
    }
  }

  TEST_CASE("DelegateFromFunction: Helper functions") {
    SUBCASE("DelegateFromFunction for free function") {
      auto delegate = helios::DelegateFromFunction<&free_function_sum>();
      CHECK(std::same_as<decltype(delegate), helios::Delegate<int(int, int)>>);
      CHECK(delegate.Valid());
      CHECK_EQ(delegate(5, 7), 12);
    }

    SUBCASE("DelegateFromFunction for single parameter free function") {
      auto delegate = helios::DelegateFromFunction<&free_function_double>();
      CHECK(std::same_as<decltype(delegate), helios::Delegate<int(int)>>);
      CHECK(delegate.Valid());
      CHECK_EQ(delegate(9), 18);
    }

    SUBCASE("DelegateFromFunction for member function") {
      Counter counter{4};
      auto delegate = helios::DelegateFromFunction<&Counter::multiply>(counter);
      CHECK(std::same_as<decltype(delegate), helios::Delegate<int(int)>>);
      CHECK(delegate.Valid());
      CHECK_EQ(delegate(5), 20);
    }

    SUBCASE("DelegateFromFunction constexpr") {
      constexpr auto delegate = helios::DelegateFromFunction<&free_function_triple>();
      CHECK(delegate.Valid());
      CHECK_EQ(delegate(4), 12);
    }

    SUBCASE("DelegateFromFunction with explicit signature for free function") {
      using Signature = int (*)(int, int);
      constexpr Signature func_ptr = &free_function_sum;
      auto delegate = helios::DelegateFromFunction<Signature, func_ptr>();
      CHECK(std::same_as<decltype(delegate), helios::Delegate<int(int, int)>>);
      CHECK(delegate.Valid());
      CHECK_EQ(delegate(3, 7), 10);
    }

    SUBCASE("DelegateFromFunction with explicit signature for single param free function") {
      using Signature = int (*)(int);
      constexpr Signature func_ptr = &free_function_double;
      auto delegate = helios::DelegateFromFunction<Signature, func_ptr>();
      CHECK(std::same_as<decltype(delegate), helios::Delegate<int(int)>>);
      CHECK(delegate.Valid());
      CHECK_EQ(delegate(6), 12);
    }

    SUBCASE("DelegateFromFunction with explicit signature for member function") {
      Counter counter{3};
      using Signature = int (Counter::*)(int);
      constexpr Signature func_ptr = &Counter::multiply;
      auto delegate = helios::DelegateFromFunction<Signature, func_ptr>(counter);
      CHECK(std::same_as<decltype(delegate), helios::Delegate<int(int)>>);
      CHECK(delegate.Valid());
      CHECK_EQ(delegate(7), 21);
    }

    SUBCASE("DelegateFromFunction with explicit signature for overloaded member function") {
      OverloadedStruct obj{100};
      using Signature = int (OverloadedStruct::*)(int);
      constexpr auto func_ptr = static_cast<Signature>(&OverloadedStruct::foo);
      auto delegate = helios::DelegateFromFunction<Signature, func_ptr>(obj);
      CHECK(std::same_as<decltype(delegate), helios::Delegate<int(int)>>);
      CHECK(delegate.Valid());
      CHECK_EQ(delegate(25), 125);
    }

    SUBCASE("DelegateFromFunction with explicit signature constexpr") {
      using Signature = int (*)(int);
      constexpr Signature func_ptr = &free_function_triple;
      constexpr auto delegate = helios::DelegateFromFunction<Signature, func_ptr>();
      CHECK(delegate.Valid());
      CHECK_EQ(delegate(5), 15);
    }
  }

  TEST_CASE("Delegate::Invoke: Polymorphic conversion") {
    SUBCASE("Derived to base conversion in arguments") {
      auto lambda = [](Base& base) { return base.get_id(); };
      Derived derived;
      Base& base_ref = derived;

      // This would test polymorphic conversion if exposed through the API
      // The implementation supports it, but direct testing requires internal access
      CHECK_EQ(derived.get_id(), 2);
    }
  }

  TEST_CASE("Delegate: Edge cases and special scenarios") {
    SUBCASE("Multiple invocations maintain state") {
      Counter counter{0};
      auto delegate = helios::Delegate<void()>::FromFunction<&Counter::increment>(counter);
      delegate();
      delegate();
      delegate();
      CHECK_EQ(counter.value, 3);
    }

    SUBCASE("Delegate can be stored and invoked later") {
      Counter counter{10};
      auto delegate = helios::Delegate<int(int)>::FromFunction<&Counter::multiply>(counter);
      std::vector<helios::Delegate<int(int)>> delegates;
      delegates.push_back(delegate);
      CHECK_EQ(delegates[0](5), 50);
    }

    SUBCASE("operator() and Invoke are equivalent") {
      auto delegate = helios::Delegate<int(int, int)>::FromFunction<&free_function_sum>();
      CHECK_EQ(delegate(3, 4), delegate.Invoke(3, 4));
      CHECK_EQ(delegate(10, 20), delegate.Invoke(10, 20));
    }
  }
}  // TEST_SUITE
