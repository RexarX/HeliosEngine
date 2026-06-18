#include <doctest/doctest.h>

#include <helios/container/callable_buffer_array.hpp>

#include <memory_resource>
#include <string>
#include <vector>

using namespace helios::container;

namespace {

struct InvocationTracker {
  static inline std::vector<int> call_order;
  static inline int total_calls = 0;

  static void Reset() {
    call_order.clear();
    total_calls = 0;
  }
};

// Trivial single-signature callable
struct SimpleCallable {
  int id = 0;

  explicit SimpleCallable(int i) : id(i) {}

  void operator()() { InvocationTracker::call_order.push_back(id); }
};

// Callable with a single argument
struct CallableWithArg {
  int id = 0;
  int multiplier = 1;

  explicit CallableWithArg(int i, int m = 1) : id(i), multiplier(m) {}

  void operator()(int value) {
    InvocationTracker::call_order.push_back(id * multiplier + value);
  }
};

// Callable with multiple arguments
struct CallableWithMultipleArgs {
  int id = 0;

  explicit CallableWithMultipleArgs(int i) : id(i) {}

  void operator()(int a, int b) {
    InvocationTracker::call_order.push_back(id + a + b);
  }
};

// Multi-signature callable using index dispatch
struct MultiSignatureCallable {
  int id = 0;

  explicit MultiSignatureCallable(int i) : id(i) {}

  void operator()(std::integral_constant<size_t, 0>) {
    InvocationTracker::call_order.push_back(id * 10);
  }

  void operator()(std::integral_constant<size_t, 1>, int value) {
    InvocationTracker::call_order.push_back(id * 100 + value);
  }
};

// Callable with custom method names
struct CustomMethodCallable {
  int id = 0;

  explicit CustomMethodCallable(int i) : id(i) {}

  void Execute() { InvocationTracker::call_order.push_back(id); }

  void Log(int level) {
    InvocationTracker::call_order.push_back(id + level * 1000);
  }
};

// Non-trivial callable (contains std::string)
struct NonTrivialCallable {
  std::string name;
  int id = 0;

  NonTrivialCallable() = default;
  explicit NonTrivialCallable(std::string n, int i)
      : name(std::move(n)), id(i) {}

  NonTrivialCallable(const NonTrivialCallable&) = default;
  NonTrivialCallable(NonTrivialCallable&&) noexcept = default;
  ~NonTrivialCallable() = default;

  void operator()() { InvocationTracker::call_order.push_back(id); }
};

// Tracks construction and destruction counts
struct CountingCallable {
  static inline int construct_count = 0;
  static inline int destruct_count = 0;
  static inline int invoke_count = 0;

  int id = 0;

  CountingCallable() { ++construct_count; }
  explicit CountingCallable(int i) : id(i) { ++construct_count; }
  CountingCallable(const CountingCallable& other) : id(other.id) {
    ++construct_count;
  }
  CountingCallable(CountingCallable&& other) noexcept : id(other.id) {
    ++construct_count;
  }
  ~CountingCallable() { ++destruct_count; }

  CountingCallable& operator=(const CountingCallable&) = default;
  CountingCallable& operator=(CountingCallable&&) noexcept = default;

  void operator()() { ++invoke_count; }

  static void Reset() {
    construct_count = 0;
    destruct_count = 0;
    invoke_count = 0;
  }
};

// Callable that writes via a pointer to an external counter
struct StatefulCallable {
  int* counter = nullptr;

  explicit StatefulCallable(int& cnt) : counter(&cnt) {}

  void operator()() const {
    if (counter != nullptr) {
      ++(*counter);
    }
  }
};

void IncrementFn(int& value) {
  ++value;
}

}  // namespace

TEST_SUITE("helios::container::CallableBufferArray") {
  TEST_CASE("container::CallableBufferArray::ctor: default construction") {
    CallableBufferArray<void()> arr;

    CHECK(arr.Empty());
    CHECK_EQ(arr.Size(), 0);
  }

  TEST_CASE("container::CallableBufferArray::ctor: allocator construction") {
    std::allocator<std::byte> alloc;
    CallableBufferArray<std::allocator<std::byte>, void()> arr(alloc);

    CHECK(arr.Empty());
    CHECK_EQ(arr.Size(), 0);
  }

  TEST_CASE("container::CallableBufferArray::ctor: move construction") {
    InvocationTracker::Reset();

    CallableBufferArray<void()> original;
    original.Push(SimpleCallable{1});
    original.Push(SimpleCallable{2});

    CHECK_EQ(original.Size(), 2);

    CallableBufferArray<void()> moved(std::move(original));

    CHECK_EQ(moved.Size(), 2);
    CHECK(original.Empty());

    moved.Invoke();

    CHECK_EQ(InvocationTracker::call_order.size(), 2);
    CHECK_EQ(InvocationTracker::call_order[0], 1);
    CHECK_EQ(InvocationTracker::call_order[1], 2);
  }

  TEST_CASE("container::CallableBufferArray::operator=: move assignment") {
    InvocationTracker::Reset();

    CallableBufferArray<void()> original;
    original.Push(SimpleCallable{10});

    CallableBufferArray<void()> target;
    target.Push(SimpleCallable{99});

    target = std::move(original);

    CHECK_EQ(target.Size(), 1);
    CHECK(original.Empty());

    target.Invoke();

    CHECK_EQ(InvocationTracker::call_order.size(), 1);
    CHECK_EQ(InvocationTracker::call_order[0], 10);
  }

  TEST_CASE(
      "container::CallableBufferArray::operator=: self-assignment is no-op") {
    InvocationTracker::Reset();

    CallableBufferArray<void()> arr;
    arr.Push(SimpleCallable{5});

    auto& ref = arr;
    arr = std::move(ref);

    CHECK_EQ(arr.Size(), 1);
  }

  TEST_CASE("container::CallableBufferArray::Clear: destroys callables") {
    CountingCallable::Reset();

    CallableBufferArray<void()> arr;
    arr.Push(CountingCallable{1});
    arr.Push(CountingCallable{2});
    arr.Push(CountingCallable{3});

    CHECK_EQ(arr.Size(), 3);

    const int destructs_before = CountingCallable::destruct_count;
    arr.Clear();

    CHECK(arr.Empty());
    CHECK_EQ(arr.Size(), 0);
    CHECK_EQ(CountingCallable::destruct_count - destructs_before, 3);
  }

  TEST_CASE("container::CallableBufferArray::Clear: on empty array is safe") {
    CallableBufferArray<void()> arr;
    CHECK_NOTHROW(arr.Clear());
    CHECK(arr.Empty());
  }

  TEST_CASE("container::CallableBufferArray::Push: via default operator()") {
    SUBCASE("single callable") {
      CallableBufferArray<void()> arr;
      arr.Push(SimpleCallable{1});

      CHECK_FALSE(arr.Empty());
      CHECK_EQ(arr.Size(), 1);
    }

    SUBCASE("multiple callables grow size") {
      CallableBufferArray<void()> arr;
      arr.Push(SimpleCallable{1});
      arr.Push(SimpleCallable{2});
      arr.Push(SimpleCallable{3});

      CHECK_EQ(arr.Size(), 3);
    }

    SUBCASE("rvalue callable") {
      CallableBufferArray<void()> arr;
      SimpleCallable c{100};
      arr.Push(std::move(c));

      CHECK_EQ(arr.Size(), 1);
    }

    SUBCASE("mixed callable types") {
      InvocationTracker::Reset();

      CallableBufferArray<void()> arr;
      arr.Push(SimpleCallable{1});
      arr.Push(NonTrivialCallable{"test", 2});

      CHECK_EQ(arr.Size(), 2);

      arr.Invoke();
      CHECK_EQ(InvocationTracker::call_order.size(), 2);
      CHECK_EQ(InvocationTracker::call_order[0], 1);
      CHECK_EQ(InvocationTracker::call_order[1], 2);
    }
  }

  TEST_CASE("container::CallableBufferArray::Push: with custom methods") {
    InvocationTracker::Reset();

    CallableBufferArray<void(), void(int)> arr;
    arr.Push<&CustomMethodCallable::Execute, &CustomMethodCallable::Log>(
        CustomMethodCallable{5});

    CHECK_EQ(arr.Size(), 1);

    arr.Invoke<0>();
    CHECK_EQ(InvocationTracker::call_order.size(), 1);
    CHECK_EQ(InvocationTracker::call_order[0], 5);

    InvocationTracker::Reset();

    arr.Invoke<1>(3);
    CHECK_EQ(InvocationTracker::call_order.size(), 1);
    CHECK_EQ(InvocationTracker::call_order[0], 5 + 3 * 1000);
  }

  TEST_CASE(
      "container::CallableBufferArray::Invoke: single signature no args") {
    InvocationTracker::Reset();

    CallableBufferArray<void()> arr;
    arr.Push(SimpleCallable{1});
    arr.Push(SimpleCallable{2});
    arr.Push(SimpleCallable{3});

    arr.Invoke();

    CHECK_EQ(InvocationTracker::call_order.size(), 3);
    CHECK_EQ(InvocationTracker::call_order[0], 1);
    CHECK_EQ(InvocationTracker::call_order[1], 2);
    CHECK_EQ(InvocationTracker::call_order[2], 3);
  }

  TEST_CASE(
      "container::CallableBufferArray::Invoke: single signature with arg") {
    InvocationTracker::Reset();

    CallableBufferArray<void(int)> arr;
    arr.Push(CallableWithArg{10, 1});
    arr.Push(CallableWithArg{20, 1});

    arr.Invoke(5);

    CHECK_EQ(InvocationTracker::call_order.size(), 2);
    CHECK_EQ(InvocationTracker::call_order[0], 10 + 5);
    CHECK_EQ(InvocationTracker::call_order[1], 20 + 5);
  }

  TEST_CASE(
      "container::CallableBufferArray::Invoke: single signature multiple "
      "args") {
    InvocationTracker::Reset();

    CallableBufferArray<void(int, int)> arr;
    arr.Push(CallableWithMultipleArgs{100});
    arr.Push(CallableWithMultipleArgs{200});

    arr.Invoke(10, 20);

    CHECK_EQ(InvocationTracker::call_order.size(), 2);
    CHECK_EQ(InvocationTracker::call_order[0], 100 + 10 + 20);
    CHECK_EQ(InvocationTracker::call_order[1], 200 + 10 + 20);
  }

  TEST_CASE(
      "container::CallableBufferArray::Invoke: multi-signature indexed "
      "dispatch") {
    InvocationTracker::Reset();

    CallableBufferArray<void(), void(int)> arr;
    arr.Push(MultiSignatureCallable{1});
    arr.Push(MultiSignatureCallable{2});

    arr.Invoke<0>();

    CHECK_EQ(InvocationTracker::call_order.size(), 2);
    CHECK_EQ(InvocationTracker::call_order[0], 10);
    CHECK_EQ(InvocationTracker::call_order[1], 20);

    InvocationTracker::Reset();

    arr.Invoke<1>(5);

    CHECK_EQ(InvocationTracker::call_order.size(), 2);
    CHECK_EQ(InvocationTracker::call_order[0], 105);
    CHECK_EQ(InvocationTracker::call_order[1], 205);
  }

  TEST_CASE("container::CallableBufferArray::Invoke: empty array is safe") {
    InvocationTracker::Reset();

    CallableBufferArray<void()> arr;
    CHECK_NOTHROW(arr.Invoke());
    CHECK(InvocationTracker::call_order.empty());
  }

  TEST_CASE(
      "container::CallableBufferArray::Invoke: preserves insertion order") {
    InvocationTracker::Reset();

    CallableBufferArray<void()> arr;
    arr.Push(SimpleCallable{100});
    arr.Push(SimpleCallable{200});
    arr.Push(SimpleCallable{300});
    arr.Push(SimpleCallable{400});
    arr.Push(SimpleCallable{500});

    arr.Invoke();

    CHECK_EQ(InvocationTracker::call_order.size(), 5);
    CHECK_EQ(InvocationTracker::call_order[0], 100);
    CHECK_EQ(InvocationTracker::call_order[1], 200);
    CHECK_EQ(InvocationTracker::call_order[2], 300);
    CHECK_EQ(InvocationTracker::call_order[3], 400);
    CHECK_EQ(InvocationTracker::call_order[4], 500);
  }

  TEST_CASE(
      "container::CallableBufferArray::Invoke: can be called multiple times") {
    InvocationTracker::Reset();

    CallableBufferArray<void()> arr;
    arr.Push(SimpleCallable{1});
    arr.Push(SimpleCallable{2});

    arr.Invoke();
    CHECK_EQ(InvocationTracker::call_order.size(), 2);

    arr.Invoke();
    CHECK_EQ(InvocationTracker::call_order.size(), 4);
  }

  TEST_CASE("container::CallableBufferArray::Invoke: free function callable") {
    CallableBufferArray<void(int&)> arr;
    arr.Push(&IncrementFn);
    arr.Push(&IncrementFn);

    int value = 0;
    arr.Invoke(value);

    CHECK_EQ(value, 2);
  }

  TEST_CASE("container::CallableBufferArray::Invoke: stateful callable") {
    int counter = 0;

    CallableBufferArray<void()> arr;
    arr.Push(StatefulCallable{counter});
    arr.Push(StatefulCallable{counter});
    arr.Push(StatefulCallable{counter});

    arr.Invoke();

    CHECK_EQ(counter, 3);
  }

  TEST_CASE("container::CallableBufferArray::Reserve: reserves capacity") {
    CallableBufferArray<void()> arr;
    arr.Reserve(10);

    for (int i = 0; i < 10; ++i) {
      arr.Push(SimpleCallable{i});
    }

    CHECK_EQ(arr.Size(), 10);
  }

  TEST_CASE("container::CallableBufferArray::ReserveBytes: reserves bytes") {
    CallableBufferArray<void()> arr;
    arr.ReserveBytes(1024);

    CHECK_GE(arr.CapacityBytes(), 1024);
  }

  TEST_CASE("container::CallableBufferArray::Swap: swaps two arrays") {
    InvocationTracker::Reset();

    CallableBufferArray<void()> arr1;
    arr1.Push(SimpleCallable{1});
    arr1.Push(SimpleCallable{2});

    CallableBufferArray<void()> arr2;
    arr2.Push(SimpleCallable{10});

    arr1.Swap(arr2);

    CHECK_EQ(arr1.Size(), 1);
    CHECK_EQ(arr2.Size(), 2);

    arr1.Invoke();
    CHECK_EQ(InvocationTracker::call_order.size(), 1);
    CHECK_EQ(InvocationTracker::call_order[0], 10);

    InvocationTracker::Reset();

    arr2.Invoke();
    CHECK_EQ(InvocationTracker::call_order.size(), 2);
    CHECK_EQ(InvocationTracker::call_order[0], 1);
    CHECK_EQ(InvocationTracker::call_order[1], 2);
  }

  TEST_CASE("container::CallableBufferArray::swap: friend function") {
    InvocationTracker::Reset();

    CallableBufferArray<void()> arr1;
    arr1.Push(SimpleCallable{1});

    CallableBufferArray<void()> arr2;
    arr2.Push(SimpleCallable{2});

    swap(arr1, arr2);

    arr1.Invoke();
    CHECK_EQ(InvocationTracker::call_order.size(), 1);
    CHECK_EQ(InvocationTracker::call_order[0], 2);
  }

  TEST_CASE(
      "container::CallableBufferArray::Merge: moves callables from other") {
    SUBCASE("non-empty into non-empty") {
      InvocationTracker::Reset();

      CallableBufferArray<void()> arr1;
      arr1.Push(SimpleCallable{1});
      arr1.Push(SimpleCallable{2});

      CallableBufferArray<void()> arr2;
      arr2.Push(SimpleCallable{3});
      arr2.Push(SimpleCallable{4});

      arr1.Merge(std::move(arr2));

      CHECK_EQ(arr1.Size(), 4);
      CHECK(arr2.Empty());

      arr1.Invoke();
      CHECK_EQ(InvocationTracker::call_order.size(), 4);
      CHECK_EQ(InvocationTracker::call_order[0], 1);
      CHECK_EQ(InvocationTracker::call_order[1], 2);
      CHECK_EQ(InvocationTracker::call_order[2], 3);
      CHECK_EQ(InvocationTracker::call_order[3], 4);
    }

    SUBCASE("empty other is a no-op") {
      InvocationTracker::Reset();

      CallableBufferArray<void()> arr1;
      arr1.Push(SimpleCallable{10});

      CallableBufferArray<void()> arr2;

      arr1.Merge(std::move(arr2));

      CHECK_EQ(arr1.Size(), 1);
      CHECK(arr2.Empty());

      arr1.Invoke();
      CHECK_EQ(InvocationTracker::call_order.size(), 1);
      CHECK_EQ(InvocationTracker::call_order[0], 10);
    }

    SUBCASE("self-merge is a no-op") {
      InvocationTracker::Reset();

      CallableBufferArray<void()> arr;
      arr.Push(SimpleCallable{1});
      arr.Push(SimpleCallable{2});

      arr.Merge(std::move(arr));

      CHECK_EQ(arr.Size(), 2);

      arr.Invoke();
      CHECK_EQ(InvocationTracker::call_order.size(), 2);
    }

    SUBCASE("preserves invocation order across merge") {
      InvocationTracker::Reset();

      CallableBufferArray<void()> arr1;
      for (int i = 0; i < 3; ++i) {
        arr1.Push(SimpleCallable{i * 10});
      }

      CallableBufferArray<void()> arr2;
      for (int i = 3; i < 6; ++i) {
        arr2.Push(SimpleCallable{i * 10});
      }

      arr1.Merge(std::move(arr2));

      CHECK_EQ(arr1.Size(), 6);

      arr1.Invoke();
      for (int i = 0; i < 6; ++i) {
        CHECK_EQ(InvocationTracker::call_order[i], i * 10);
      }
    }

    SUBCASE("non-trivial callables are properly relocated") {
      InvocationTracker::Reset();

      CallableBufferArray<void()> arr1;
      arr1.Push(NonTrivialCallable{"first", 1});

      CallableBufferArray<void()> arr2;
      arr2.Push(NonTrivialCallable{"second", 2});

      arr1.Merge(std::move(arr2));

      CHECK_EQ(arr1.Size(), 2);
      CHECK(arr2.Empty());

      arr1.Invoke();
      CHECK_EQ(InvocationTracker::call_order.size(), 2);
      CHECK_EQ(InvocationTracker::call_order[0], 1);
      CHECK_EQ(InvocationTracker::call_order[1], 2);
    }
  }

  TEST_CASE("container::CallableBufferArray::Empty: reflects state") {
    CallableBufferArray<void()> arr;
    CHECK(arr.Empty());

    arr.Push(SimpleCallable{1});
    CHECK_FALSE(arr.Empty());

    arr.Clear();
    CHECK(arr.Empty());
  }

  TEST_CASE("container::CallableBufferArray::Size: tracks count") {
    CallableBufferArray<void()> arr;
    CHECK_EQ(arr.Size(), 0);

    arr.Push(SimpleCallable{1});
    CHECK_EQ(arr.Size(), 1);

    arr.Push(SimpleCallable{2});
    CHECK_EQ(arr.Size(), 2);

    arr.Clear();
    CHECK_EQ(arr.Size(), 0);
  }

  TEST_CASE("container::CallableBufferArray::CapacityBytes: grows on Push") {
    CallableBufferArray<void()> arr;
    CHECK_EQ(arr.CapacityBytes(), 0);

    arr.Push(SimpleCallable{1});
    CHECK_GT(arr.CapacityBytes(), 0);
  }

  TEST_CASE("container::CallableBufferArray::GetAllocator: returns allocator") {
    std::allocator<std::byte> alloc;
    CallableBufferArray<std::allocator<std::byte>, void()> arr(alloc);

    auto retrieved = arr.GetAllocator();
    CHECK(retrieved == alloc);
  }

  TEST_CASE(
      "container::CallableBufferArray::destructor: calls callable "
      "destructors") {
    CountingCallable::Reset();

    {
      CallableBufferArray<void()> arr;
      arr.Push(CountingCallable{1});
      arr.Push(CountingCallable{2});

      CHECK_GE(CountingCallable::construct_count, 2);
    }

    CHECK_EQ(CountingCallable::construct_count,
             CountingCallable::destruct_count);
  }

  TEST_CASE(
      "container::CallableBufferArray::large number of callables: order and "
      "count") {
    InvocationTracker::Reset();

    CallableBufferArray<void()> arr;

    constexpr int kCount = 100;
    for (int i = 0; i < kCount; ++i) {
      arr.Push(SimpleCallable{i});
    }

    CHECK_EQ(arr.Size(), kCount);

    arr.Invoke();

    CHECK_EQ(InvocationTracker::call_order.size(), kCount);
    for (int i = 0; i < kCount; ++i) {
      CHECK_EQ(InvocationTracker::call_order[i], i);
    }
  }

  TEST_CASE(
      "container::CallableBufferArray::alias deduction: signatures only") {
    InvocationTracker::Reset();

    CallableBufferArray<void()> arr;
    arr.Push(SimpleCallable{1});
    arr.Push(SimpleCallable{2});

    arr.Invoke();

    CHECK_EQ(InvocationTracker::call_order.size(), 2);
    CHECK_EQ(InvocationTracker::call_order[0], 1);
    CHECK_EQ(InvocationTracker::call_order[1], 2);
  }

  TEST_CASE(
      "container::CallableBufferArray::alias deduction: multiple signatures") {
    InvocationTracker::Reset();

    CallableBufferArray<void(), void(int)> arr;
    arr.Push(MultiSignatureCallable{1});

    arr.template Invoke<0>();
    CHECK_EQ(InvocationTracker::call_order.size(), 1);
    CHECK_EQ(InvocationTracker::call_order[0], 10);

    InvocationTracker::Reset();

    arr.template Invoke<1>(42);
    CHECK_EQ(InvocationTracker::call_order.size(), 1);
    CHECK_EQ(InvocationTracker::call_order[0], 142);
  }

  TEST_CASE(
      "container::CallableBufferArray::alias deduction: allocator + multiple "
      "signatures") {
    InvocationTracker::Reset();

    CallableBufferArray<std::allocator<std::byte>, void(), void(int)> arr;
    arr.Push(MultiSignatureCallable{5});

    arr.template Invoke<0>();
    CHECK_EQ(InvocationTracker::call_order.size(), 1);
    CHECK_EQ(InvocationTracker::call_order[0], 50);
  }

  TEST_CASE("container::PmrCallableBufferArray: works with memory_resource") {
    InvocationTracker::Reset();

    auto* resource = std::pmr::get_default_resource();
    PmrCallableBufferArray<void()> arr{resource};
    arr.Push(SimpleCallable{7});
    arr.Invoke();

    CHECK_EQ(InvocationTracker::call_order.size(), 1);
    CHECK_EQ(InvocationTracker::call_order[0], 7);
  }
}  // TEST_SUITE("container::CallableBufferArray")
