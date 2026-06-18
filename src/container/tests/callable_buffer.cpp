#include <doctest/doctest.h>

#include <helios/container/callable_buffer.hpp>

#include <memory_resource>
#include <string>
#include <vector>

using namespace helios::container;

namespace {

struct InvocationTracker {
  static inline std::vector<int> call_order;

  static void Reset() { call_order.clear(); }
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

  explicit CallableWithArg(int i) : id(i) {}

  void operator()(int value) {
    InvocationTracker::call_order.push_back(id + value);
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

TEST_SUITE("helios::container::CallableBuffer") {
  TEST_CASE("container::CallableBuffer::ctor: default construction") {
    CallableBuffer<void()> buf;

    CHECK(buf.Empty());
  }

  TEST_CASE("container::CallableBuffer::ctor: allocator construction") {
    std::allocator<std::byte> alloc;
    CallableBuffer<std::allocator<std::byte>, void()> buf(alloc);

    CHECK(buf.Empty());
  }

  TEST_CASE("container::CallableBuffer::ctor: move construction") {
    InvocationTracker::Reset();

    CallableBuffer<void()> original;
    original.Set(SimpleCallable{7});

    CallableBuffer<void()> moved(std::move(original));

    CHECK(original.Empty());
    CHECK_FALSE(moved.Empty());

    moved.Invoke();
    CHECK_EQ(InvocationTracker::call_order.size(), 1);
    CHECK_EQ(InvocationTracker::call_order[0], 7);
  }

  TEST_CASE("container::CallableBuffer::operator=: move assignment") {
    InvocationTracker::Reset();

    CallableBuffer<void()> original;
    original.Set(SimpleCallable{42});

    CallableBuffer<void()> target;
    target.Set(SimpleCallable{99});

    target = std::move(original);

    CHECK(original.Empty());
    CHECK_FALSE(target.Empty());

    target.Invoke();
    CHECK_EQ(InvocationTracker::call_order.size(), 1);
    CHECK_EQ(InvocationTracker::call_order[0], 42);
  }

  TEST_CASE("container::CallableBuffer::operator=: self-assignment is no-op") {
    InvocationTracker::Reset();

    CallableBuffer<void()> buf;
    buf.Set(SimpleCallable{5});

    auto& ref = buf;
    buf = std::move(ref);

    // Buffer should still be valid after self-move-assign
    CHECK_FALSE(buf.Empty());
  }

  TEST_CASE("container::CallableBuffer::Clear: destroys stored callable") {
    CountingCallable::Reset();

    CallableBuffer<void()> buf;
    buf.Set(CountingCallable{1});
    CHECK_EQ(CountingCallable::construct_count, 2);  // 1 temp + 1 stored

    buf.Clear();

    CHECK(buf.Empty());
    CHECK_EQ(CountingCallable::destruct_count, 2);
  }

  TEST_CASE("container::CallableBuffer::Clear: on empty buffer is safe") {
    CallableBuffer<void()> buf;
    CHECK_NOTHROW(buf.Clear());
    CHECK(buf.Empty());
  }

  TEST_CASE("container::CallableBuffer::Set: stores and replaces") {
    SUBCASE("trivial callable via operator()") {
      InvocationTracker::Reset();

      CallableBuffer<void()> buf;
      buf.Set(SimpleCallable{3});

      CHECK_FALSE(buf.Empty());

      buf.Invoke();
      CHECK_EQ(InvocationTracker::call_order.size(), 1);
      CHECK_EQ(InvocationTracker::call_order[0], 3);
    }

    SUBCASE("replaces previous callable and destroys it") {
      CountingCallable::Reset();

      CallableBuffer<void()> buf;
      buf.Set(CountingCallable{1});

      const int constructs_after_first = CountingCallable::construct_count;
      const int destructs_after_first = CountingCallable::destruct_count;

      buf.Set(CountingCallable{2});

      // First callable was destroyed
      CHECK_GT(CountingCallable::destruct_count, destructs_after_first);
      CHECK_GT(CountingCallable::construct_count, constructs_after_first);
    }

    SUBCASE("rvalue callable") {
      InvocationTracker::Reset();

      CallableBuffer<void()> buf;
      SimpleCallable c{77};
      buf.Set(std::move(c));

      buf.Invoke();
      CHECK_EQ(InvocationTracker::call_order.size(), 1);
      CHECK_EQ(InvocationTracker::call_order[0], 77);
    }

    SUBCASE("non-trivial callable") {
      InvocationTracker::Reset();

      CallableBuffer<void()> buf;
      buf.Set(NonTrivialCallable{"test", 9});

      CHECK_FALSE(buf.Empty());

      buf.Invoke();
      CHECK_EQ(InvocationTracker::call_order.size(), 1);
      CHECK_EQ(InvocationTracker::call_order[0], 9);
    }
  }

  TEST_CASE("container::CallableBuffer::Set: custom methods") {
    SUBCASE("single operation with custom method") {
      InvocationTracker::Reset();

      CallableBuffer<void()> buf;
      buf.Set<&CustomMethodCallable::Execute>(CustomMethodCallable{4});

      buf.Invoke();
      CHECK_EQ(InvocationTracker::call_order.size(), 1);
      CHECK_EQ(InvocationTracker::call_order[0], 4);
    }

    SUBCASE("multi-operation with custom methods") {
      InvocationTracker::Reset();

      CallableBuffer<void(), void(int)> buf;
      buf.Set<&CustomMethodCallable::Execute, &CustomMethodCallable::Log>(
          CustomMethodCallable{5});

      buf.Invoke<0>();
      CHECK_EQ(InvocationTracker::call_order.size(), 1);
      CHECK_EQ(InvocationTracker::call_order[0], 5);

      InvocationTracker::Reset();

      buf.Invoke<1>(3);
      CHECK_EQ(InvocationTracker::call_order.size(), 1);
      CHECK_EQ(InvocationTracker::call_order[0], 5 + 3 * 1000);
    }
  }

  TEST_CASE("container::CallableBuffer::Invoke: single signature no args") {
    InvocationTracker::Reset();

    CallableBuffer<void()> buf;
    buf.Set(SimpleCallable{11});
    buf.Invoke();

    CHECK_EQ(InvocationTracker::call_order.size(), 1);
    CHECK_EQ(InvocationTracker::call_order[0], 11);
  }

  TEST_CASE("container::CallableBuffer::Invoke: single signature with args") {
    InvocationTracker::Reset();

    CallableBuffer<void(int)> buf;
    buf.Set(CallableWithArg{10});
    buf.Invoke(5);

    CHECK_EQ(InvocationTracker::call_order.size(), 1);
    CHECK_EQ(InvocationTracker::call_order[0], 15);
  }

  TEST_CASE("container::CallableBuffer::Invoke: multi-signature indexed") {
    InvocationTracker::Reset();

    CallableBuffer<void(), void(int)> buf;
    buf.Set(MultiSignatureCallable{2});

    buf.Invoke<0>();
    CHECK_EQ(InvocationTracker::call_order.size(), 1);
    CHECK_EQ(InvocationTracker::call_order[0], 20);  // 2 * 10

    InvocationTracker::Reset();

    buf.Invoke<1>(7);
    CHECK_EQ(InvocationTracker::call_order.size(), 1);
    CHECK_EQ(InvocationTracker::call_order[0], 207);  // 2 * 100 + 7
  }

  TEST_CASE("container::CallableBuffer::Invoke: free function callable") {
    CallableBuffer<void(int&)> buf;
    buf.Set(&IncrementFn);

    int value = 0;
    buf.Invoke(value);

    CHECK_EQ(value, 1);
  }

  TEST_CASE("container::CallableBuffer::Invoke: stateful callable") {
    int counter = 0;

    CallableBuffer<void()> buf;
    buf.Set(StatefulCallable{counter});

    buf.Invoke();
    buf.Invoke();
    buf.Invoke();

    CHECK_EQ(counter, 3);
  }

  TEST_CASE("container::CallableBuffer::Swap: swaps two buffers") {
    InvocationTracker::Reset();

    CallableBuffer<void()> buf1;
    buf1.Set(SimpleCallable{1});

    CallableBuffer<void()> buf2;
    buf2.Set(SimpleCallable{2});

    buf1.Swap(buf2);

    buf1.Invoke();
    CHECK_EQ(InvocationTracker::call_order.size(), 1);
    CHECK_EQ(InvocationTracker::call_order[0], 2);

    InvocationTracker::Reset();

    buf2.Invoke();
    CHECK_EQ(InvocationTracker::call_order.size(), 1);
    CHECK_EQ(InvocationTracker::call_order[0], 1);
  }

  TEST_CASE("container::CallableBuffer::Swap: swap with empty buffer") {
    InvocationTracker::Reset();

    CallableBuffer<void()> buf1;
    buf1.Set(SimpleCallable{5});

    CallableBuffer<void()> buf2;

    buf1.Swap(buf2);

    CHECK(buf1.Empty());
    CHECK_FALSE(buf2.Empty());

    buf2.Invoke();
    CHECK_EQ(InvocationTracker::call_order.size(), 1);
    CHECK_EQ(InvocationTracker::call_order[0], 5);
  }

  TEST_CASE("container::CallableBuffer::swap: friend function") {
    InvocationTracker::Reset();

    CallableBuffer<void()> buf1;
    buf1.Set(SimpleCallable{1});

    CallableBuffer<void()> buf2;
    buf2.Set(SimpleCallable{2});

    swap(buf1, buf2);

    buf1.Invoke();
    CHECK_EQ(InvocationTracker::call_order.size(), 1);
    CHECK_EQ(InvocationTracker::call_order[0], 2);
  }

  TEST_CASE("container::CallableBuffer::Empty: reflects state") {
    CallableBuffer<void()> buf;
    CHECK(buf.Empty());

    buf.Set(SimpleCallable{1});
    CHECK_FALSE(buf.Empty());

    buf.Clear();
    CHECK(buf.Empty());
  }

  TEST_CASE("container::CallableBuffer::CapacityBytes: grows on Set") {
    CallableBuffer<void()> buf;
    CHECK_EQ(buf.CapacityBytes(), 0);

    buf.Set(SimpleCallable{1});
    CHECK_GT(buf.CapacityBytes(), 0);
  }

  TEST_CASE("container::CallableBuffer::GetAllocator: returns allocator") {
    std::allocator<std::byte> alloc;
    CallableBuffer<std::allocator<std::byte>, void()> buf(alloc);

    auto retrieved = buf.GetAllocator();
    CHECK(retrieved == alloc);
  }

  TEST_CASE(
      "container::CallableBuffer::destructor: calls callable destructor") {
    CountingCallable::Reset();

    {
      CallableBuffer<void()> buf;
      buf.Set(CountingCallable{1});
    }

    CHECK_EQ(CountingCallable::construct_count,
             CountingCallable::destruct_count);
  }

  TEST_CASE("container::CallableBuffer::multiple Set calls reuse storage") {
    InvocationTracker::Reset();

    CallableBuffer<void()> buf;

    buf.Set(SimpleCallable{1});
    buf.Set(SimpleCallable{2});
    buf.Set(SimpleCallable{3});

    buf.Invoke();

    CHECK_EQ(InvocationTracker::call_order.size(), 1);
    CHECK_EQ(InvocationTracker::call_order[0], 3);
  }

  TEST_CASE("container::CallableBuffer::alias deduction: signatures only") {
    InvocationTracker::Reset();

    CallableBuffer<void()> buf;
    buf.Set(SimpleCallable{20});
    buf.Invoke();

    CHECK_EQ(InvocationTracker::call_order.size(), 1);
    CHECK_EQ(InvocationTracker::call_order[0], 20);
  }

  TEST_CASE("container::CallableBuffer::alias deduction: multiple signatures") {
    InvocationTracker::Reset();

    CallableBuffer<void(), void(int)> buf;
    buf.Set(MultiSignatureCallable{3});

    buf.template Invoke<0>();
    CHECK_EQ(InvocationTracker::call_order.size(), 1);
    CHECK_EQ(InvocationTracker::call_order[0], 30);

    InvocationTracker::Reset();

    buf.template Invoke<1>(1);
    CHECK_EQ(InvocationTracker::call_order.size(), 1);
    CHECK_EQ(InvocationTracker::call_order[0], 301);
  }

  TEST_CASE(
      "container::CallableBuffer::alias deduction: allocator + signatures") {
    InvocationTracker::Reset();

    CallableBuffer<std::allocator<std::byte>, void()> buf;
    buf.Set(SimpleCallable{8});
    buf.Invoke();

    CHECK_EQ(InvocationTracker::call_order.size(), 1);
    CHECK_EQ(InvocationTracker::call_order[0], 8);
  }

  TEST_CASE("container::PmrCallableBuffer: works with memory_resource") {
    InvocationTracker::Reset();

    auto* resource = std::pmr::get_default_resource();
    PmrCallableBuffer<void()> buf{resource};
    buf.Set(SimpleCallable{13});
    buf.Invoke();

    CHECK_EQ(InvocationTracker::call_order.size(), 1);
    CHECK_EQ(InvocationTracker::call_order[0], 13);
  }
}  // TEST_SUITE("container::CallableBuffer")
