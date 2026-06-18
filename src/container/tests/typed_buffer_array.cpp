#include <doctest/doctest.h>

#include <helios/container/typed_buffer_array.hpp>

#include <atomic>
#include <memory>
#include <memory_resource>
#include <string>
#include <utility>
#include <vector>

using namespace helios::container;

namespace {

struct TestValue {
  int x = 0;
  float y = 0.0F;

  constexpr bool operator==(const TestValue& other) const noexcept = default;
};

struct NonTrivial {
  std::string data;
  int value = 0;

  NonTrivial() = default;
  explicit NonTrivial(std::string s, int v = 0)
      : data(std::move(s)), value(v) {}
  NonTrivial(const NonTrivial&) = default;
  NonTrivial(NonTrivial&&) noexcept = default;
  ~NonTrivial() = default;

  NonTrivial& operator=(const NonTrivial&) = default;
  NonTrivial& operator=(NonTrivial&&) noexcept = default;

  bool operator==(const NonTrivial& other) const = default;
};

struct MoveOnly {
  int value = 0;

  MoveOnly() = default;
  explicit MoveOnly(int v) : value(v) {}
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly(MoveOnly&&) noexcept = default;
  ~MoveOnly() = default;

  MoveOnly& operator=(const MoveOnly&) = delete;
  MoveOnly& operator=(MoveOnly&&) noexcept = default;

  bool operator==(const MoveOnly& other) const = default;
};

struct CountingType {
  static inline int construct_count = 0;
  static inline int destruct_count = 0;
  static inline int copy_count = 0;
  static inline int move_count = 0;

  int value = 0;

  CountingType() { ++construct_count; }
  explicit CountingType(int v) : value(v) { ++construct_count; }
  CountingType(const CountingType& other) : value(other.value) {
    ++construct_count;
    ++copy_count;
  }
  CountingType(CountingType&& other) noexcept : value(other.value) {
    ++construct_count;
    ++move_count;
  }
  ~CountingType() { ++destruct_count; }

  CountingType& operator=(const CountingType& other) {
    value = other.value;
    ++copy_count;
    return *this;
  }

  CountingType& operator=(CountingType&& other) noexcept {
    value = other.value;
    ++move_count;
    return *this;
  }

  static void Reset() {
    construct_count = 0;
    destruct_count = 0;
    copy_count = 0;
    move_count = 0;
  }
};

/// @brief A tracking allocator distinct from std::allocator for cross-allocator
/// tests. Counts live allocations to detect leaks.
template <typename T>
struct TrackingAllocator {
  using value_type = T;

  static inline std::atomic<int> allocation_count{0};

  TrackingAllocator() noexcept = default;

  template <typename U>
  explicit TrackingAllocator(const TrackingAllocator<U>&) noexcept {}

  T* allocate(size_t n) {
    ++allocation_count;
    return std::allocator<T>{}.allocate(n);
  }

  void deallocate(T* p, size_t n) noexcept {
    --allocation_count;
    std::allocator<T>{}.deallocate(p, n);
  }

  template <typename U>
  bool operator==(const TrackingAllocator<U>&) const noexcept {
    return true;
  }

  static void ResetCount() noexcept { allocation_count = 0; }
};

}  // namespace

TEST_SUITE("helios::container::TypedBufferStorable") {
  TEST_CASE("container::TypedBufferStorable: concept validation") {
    SUBCASE("Trivial types satisfy concept") {
      CHECK(TypedBufferStorable<int>);
      CHECK(TypedBufferStorable<float>);
      CHECK(TypedBufferStorable<double>);
      CHECK(TypedBufferStorable<char>);
      CHECK(TypedBufferStorable<TestValue>);
    }

    SUBCASE("Non-trivial types satisfy concept") {
      CHECK(TypedBufferStorable<NonTrivial>);
      CHECK(TypedBufferStorable<std::string>);
      CHECK(TypedBufferStorable<std::vector<int>>);
    }

    SUBCASE("Move-only types satisfy concept") {
      CHECK(TypedBufferStorable<MoveOnly>);
      CHECK(TypedBufferStorable<std::unique_ptr<int>>);
    }
  }
}

TEST_SUITE("helios::container::TypedBufferArray") {
  TEST_CASE("container::TypedBufferArray::ctor: default construction") {
    TypedBufferArray buffer;

    CHECK(buffer.Empty());
    CHECK_EQ(buffer.Size(), 0);
    CHECK_EQ(buffer.SizeBytes(), 0);
    CHECK_FALSE(buffer.HasType());
  }

  TEST_CASE("container::TypedBufferArray::ctor: count construction") {
    SUBCASE("Trivial type") {
      TypedBufferArray buffer(5, int{42});

      CHECK_FALSE(buffer.Empty());
      CHECK_EQ(buffer.Size(), 5);
      CHECK(buffer.HasType());
      CHECK(buffer.IsType<int>());

      for (size_t i = 0; i < buffer.Size(); ++i) {
        CHECK_EQ(buffer.At<int>(i), 42);
      }
    }

    SUBCASE("Non-trivial type") {
      TypedBufferArray buffer(3, NonTrivial{"test", 100});

      CHECK_EQ(buffer.Size(), 3);
      CHECK(buffer.IsType<NonTrivial>());

      for (size_t i = 0; i < buffer.Size(); ++i) {
        CHECK_EQ(buffer.At<NonTrivial>(i).data, "test");
        CHECK_EQ(buffer.At<NonTrivial>(i).value, 100);
      }
    }
  }

  TEST_CASE(
      "container::TypedBufferArray::ctor: default-initialized count "
      "construction") {
    TypedBufferArray buffer(5, int{});

    CHECK_EQ(buffer.Size(), 5);
    CHECK(buffer.IsType<int>());
  }

  TEST_CASE("container::TypedBufferArray::ctor: iterator range construction") {
    SUBCASE("From vector") {
      std::vector<int> source = {1, 2, 3, 4, 5};
      TypedBufferArray buffer(source.begin(), source.end());

      CHECK_EQ(buffer.Size(), 5);
      CHECK(buffer.IsType<int>());
      for (size_t i = 0; i < buffer.Size(); ++i) {
        CHECK_EQ(buffer.At<int>(i), static_cast<int>(i + 1));
      }
    }

    SUBCASE("Empty range") {
      std::vector<int> source;
      TypedBufferArray buffer(source.begin(), source.end());

      CHECK(buffer.Empty());
      CHECK_FALSE(buffer.HasType());
    }
  }

  TEST_CASE(
      "container::TypedBufferArray::ctor: initializer list construction") {
    TypedBufferArray buffer = TypedBufferArray({1, 2, 3, 4, 5});

    CHECK_EQ(buffer.Size(), 5);
    CHECK(buffer.IsType<int>());
    for (size_t i = 0; i < buffer.Size(); ++i) {
      CHECK_EQ(buffer.At<int>(i), static_cast<int>(i + 1));
    }
  }

  TEST_CASE("container::TypedBufferArray::ctor: copy construction") {
    SUBCASE("Copy trivial type") {
      TypedBufferArray original(3, int{10});
      TypedBufferArray copy(original);

      CHECK_EQ(copy.Size(), 3);
      CHECK(copy.IsType<int>());
      for (size_t i = 0; i < copy.Size(); ++i) {
        CHECK_EQ(copy.At<int>(i), 10);
      }

      // Original unchanged
      CHECK_EQ(original.Size(), 3);
    }

    SUBCASE("Copy non-trivial type") {
      TypedBufferArray original(2, NonTrivial{"hello", 42});
      TypedBufferArray copy(original);

      CHECK_EQ(copy.Size(), 2);
      for (size_t i = 0; i < copy.Size(); ++i) {
        CHECK_EQ(copy.At<NonTrivial>(i).data, "hello");
        CHECK_EQ(copy.At<NonTrivial>(i).value, 42);
      }
    }
  }

  TEST_CASE("container::TypedBufferArray::ctor: move construction") {
    TypedBufferArray original(3, int{5});
    TypedBufferArray moved(std::move(original));

    CHECK_EQ(moved.Size(), 3);
    CHECK(moved.IsType<int>());
    for (size_t i = 0; i < moved.Size(); ++i) {
      CHECK_EQ(moved.At<int>(i), 5);
    }

    CHECK(original.Empty());
  }

  TEST_CASE("container::TypedBufferArray::operator=: copy assignment") {
    TypedBufferArray original(2, int{7});
    TypedBufferArray copy;
    copy = original;

    CHECK_EQ(copy.Size(), 2);
    CHECK(copy.IsType<int>());
    for (size_t i = 0; i < copy.Size(); ++i) {
      CHECK_EQ(copy.At<int>(i), 7);
    }
  }

  TEST_CASE("container::TypedBufferArray::operator=: move assignment") {
    TypedBufferArray original(2, int{7});
    TypedBufferArray moved;
    moved = std::move(original);

    CHECK_EQ(moved.Size(), 2);
    CHECK(moved.IsType<int>());
    CHECK(original.Empty());
  }

  TEST_CASE("container::TypedBufferArray::ChangeType: changes stored type") {
    TypedBufferArray buffer(3, int{1});
    CHECK(buffer.IsType<int>());
    CHECK_EQ(buffer.Size(), 3);

    buffer.ChangeType<float>();

    CHECK(buffer.IsType<float>());
    CHECK(buffer.Empty());
    CHECK_EQ(buffer.Size(), 0);
  }

  TEST_CASE("container::TypedBufferArray::Reset: resets to empty state") {
    TypedBufferArray buffer(3, int{1});
    buffer.Reset();

    CHECK(buffer.Empty());
    CHECK_FALSE(buffer.HasType());
    CHECK_EQ(buffer.Size(), 0);
  }

  TEST_CASE("container::TypedBufferArray::PushBack: appending elements") {
    SUBCASE("Push trivial type") {
      TypedBufferArray buffer;

      for (int i = 0; i < 5; ++i) {
        buffer.PushBack(int{i});
      }

      CHECK_EQ(buffer.Size(), 5);
      CHECK(buffer.IsType<int>());
      for (int i = 0; i < 5; ++i) {
        CHECK_EQ(buffer.At<int>(static_cast<size_t>(i)), i);
      }
    }

    SUBCASE("Push non-trivial type") {
      TypedBufferArray buffer;
      buffer.PushBack(NonTrivial{"first", 1});
      buffer.PushBack(NonTrivial{"second", 2});

      CHECK_EQ(buffer.Size(), 2);
      CHECK_EQ(buffer.At<NonTrivial>(0).data, "first");
      CHECK_EQ(buffer.At<NonTrivial>(1).data, "second");
    }
  }

  TEST_CASE("container::TypedBufferArray::EmplaceBack: in-place construction") {
    SUBCASE("Emplace trivial") {
      TypedBufferArray buffer;
      auto& ref = buffer.EmplaceBack<int>(42);

      CHECK_EQ(ref, 42);
      CHECK_EQ(buffer.Size(), 1);
    }

    SUBCASE("Emplace non-trivial with args") {
      TypedBufferArray buffer;
      auto& ref = buffer.EmplaceBack<NonTrivial>("hello", 10);

      CHECK_EQ(ref.data, "hello");
      CHECK_EQ(ref.value, 10);
    }
  }

  TEST_CASE(
      "container::TypedBufferArray::Emplace: in-place construction at "
      "position") {
    TypedBufferArray buffer;
    buffer.PushBack(1);
    buffer.PushBack(3);
    auto& ref = buffer.Emplace<int>(1, 2);

    CHECK_EQ(ref, 2);
    CHECK_EQ(buffer.Size(), 3);
    CHECK_EQ(buffer.At<int>(0), 1);
    CHECK_EQ(buffer.At<int>(1), 2);
    CHECK_EQ(buffer.At<int>(2), 3);
  }

  TEST_CASE("container::TypedBufferArray::Insert: inserting at position") {
    SUBCASE("Insert single value") {
      TypedBufferArray buffer;
      buffer.PushBack(1);
      buffer.PushBack(3);
      auto& ref = buffer.Insert<int>(1, 2);

      CHECK_EQ(ref, 2);
      CHECK_EQ(buffer.Size(), 3);
    }

    SUBCASE("Insert range at position") {
      std::vector<int> to_insert = {10, 20, 30};
      TypedBufferArray buffer;
      buffer.PushBack(1);
      buffer.PushBack(4);
      auto* ptr = buffer.Insert(1, to_insert.begin(), to_insert.end());

      CHECK_NE(ptr, nullptr);
      CHECK_EQ(buffer.Size(), 5);
      CHECK_EQ(buffer.At<int>(0), 1);
      CHECK_EQ(buffer.At<int>(1), 10);
      CHECK_EQ(buffer.At<int>(2), 20);
      CHECK_EQ(buffer.At<int>(3), 30);
      CHECK_EQ(buffer.At<int>(4), 4);
    }
  }

  TEST_CASE("container::TypedBufferArray::AppendRange: appending a range") {
    std::vector<int> to_append = {4, 5, 6};
    TypedBufferArray buffer;
    buffer.PushBack(1);
    buffer.PushBack(2);
    buffer.PushBack(3);
    buffer.AppendRange(to_append);

    CHECK_EQ(buffer.Size(), 6);
    for (int i = 0; i < 6; ++i) {
      CHECK_EQ(buffer.At<int>(static_cast<size_t>(i)), i + 1);
    }
  }

  TEST_CASE(
      "container::TypedBufferArray::InsertRange: inserting a range at "
      "position") {
    std::vector<int> extra = {10, 20};
    TypedBufferArray buffer;
    buffer.PushBack(1);
    buffer.PushBack(2);
    buffer.PushBack(5);
    auto* ptr = buffer.InsertRange(2, extra);

    CHECK_NE(ptr, nullptr);
    CHECK_EQ(buffer.Size(), 5);
    CHECK_EQ(buffer.At<int>(0), 1);
    CHECK_EQ(buffer.At<int>(1), 2);
    CHECK_EQ(buffer.At<int>(2), 10);
    CHECK_EQ(buffer.At<int>(3), 20);
    CHECK_EQ(buffer.At<int>(4), 5);
  }

  TEST_CASE("container::TypedBufferArray::PopBack: removing last element") {
    TypedBufferArray buffer;
    buffer.PushBack(1);
    buffer.PushBack(2);
    buffer.PushBack(3);

    buffer.PopBack();
    CHECK_EQ(buffer.Size(), 2);
    CHECK_EQ(buffer.Back<int>(), 2);
  }

  TEST_CASE(
      "container::TypedBufferArray::Erase: removing element at position") {
    TypedBufferArray buffer;
    buffer.PushBack(1);
    buffer.PushBack(2);
    buffer.PushBack(3);

    buffer.Erase(1);
    CHECK_EQ(buffer.Size(), 2);
    CHECK_EQ(buffer.At<int>(0), 1);
    CHECK_EQ(buffer.At<int>(1), 3);
  }

  TEST_CASE(
      "container::TypedBufferArray::Erase(range): removing element range") {
    TypedBufferArray buffer;
    for (int i = 1; i <= 5; ++i) {
      buffer.PushBack(int{i});
    }

    buffer.Erase(1, 4);
    CHECK_EQ(buffer.Size(), 2);
    CHECK_EQ(buffer.At<int>(0), 1);
    CHECK_EQ(buffer.At<int>(1), 5);
  }

  TEST_CASE("container::TypedBufferArray::Clear: clearing all elements") {
    TypedBufferArray buffer;
    buffer.PushBack(1);
    buffer.PushBack(2);

    buffer.Clear();
    CHECK(buffer.Empty());
    CHECK_EQ(buffer.Size(), 0);
  }

  TEST_CASE("container::TypedBufferArray::Reserve: reserving capacity") {
    TypedBufferArray buffer;
    buffer.PushBack(1);
    buffer.Reserve(100);

    CHECK_GE(buffer.Capacity(), 100);
    CHECK_EQ(buffer.Size(), 1);
  }

  TEST_CASE("container::TypedBufferArray::Resize: resizing storage") {
    SUBCASE("Resize larger (default-init)") {
      TypedBufferArray buffer;
      buffer.PushBack(1);
      buffer.Resize<int>(5);

      CHECK_EQ(buffer.Size(), 5);
      CHECK_EQ(buffer.At<int>(0), 1);
    }

    SUBCASE("Resize smaller") {
      TypedBufferArray buffer;
      for (int i = 0; i < 5; ++i) {
        buffer.PushBack(int{i});
      }
      buffer.Resize<int>(3);

      CHECK_EQ(buffer.Size(), 3);
    }

    SUBCASE("Resize with fill value") {
      TypedBufferArray buffer;
      buffer.PushBack(1);
      buffer.Resize<int>(4, 99);

      CHECK_EQ(buffer.Size(), 4);
      CHECK_EQ(buffer.At<int>(1), 99);
      CHECK_EQ(buffer.At<int>(2), 99);
      CHECK_EQ(buffer.At<int>(3), 99);
    }
  }

  TEST_CASE("container::TypedBufferArray::At: element access") {
    TypedBufferArray buffer;
    buffer.PushBack(10);
    buffer.PushBack(20);
    buffer.PushBack(30);

    CHECK_EQ(buffer.At<int>(0), 10);
    CHECK_EQ(buffer.At<int>(1), 20);
    CHECK_EQ(buffer.At<int>(2), 30);

    buffer.At<int>(1) = 200;
    CHECK_EQ(buffer.At<int>(1), 200);
  }

  TEST_CASE(
      "container::TypedBufferArray::Front/Back: first/last element access") {
    TypedBufferArray buffer;
    buffer.PushBack(10);
    buffer.PushBack(20);
    buffer.PushBack(30);

    CHECK_EQ(buffer.Front<int>(), 10);
    CHECK_EQ(buffer.Back<int>(), 30);
  }

  TEST_CASE("container::TypedBufferArray::Data: span access") {
    SUBCASE("Non-const span") {
      TypedBufferArray buffer;
      buffer.PushBack(1);
      buffer.PushBack(2);
      buffer.PushBack(3);
      auto span = buffer.Data<int>();

      CHECK_EQ(span.size(), 3);
      CHECK_EQ(span[0], 1);
      CHECK_EQ(span[1], 2);
      CHECK_EQ(span[2], 3);
    }

    SUBCASE("Const span") {
      TypedBufferArray buffer;
      buffer.PushBack(1);
      buffer.PushBack(2);
      const auto& const_buffer = buffer;
      auto cspan = const_buffer.Data<int>();

      CHECK_EQ(cspan.size(), 2);
      CHECK_EQ(cspan[0], 1);
    }
  }

  TEST_CASE("container::TypedBufferArray::Bytes: byte span access") {
    TypedBufferArray buffer;
    buffer.PushBack(1);
    buffer.PushBack(2);

    auto bytes = buffer.Bytes();
    CHECK_EQ(bytes.size(), 2 * sizeof(int));
  }

  TEST_CASE("container::TypedBufferArray::iterators") {
    TypedBufferArray buffer;
    buffer.PushBack(1);
    buffer.PushBack(2);
    buffer.PushBack(3);

    SUBCASE("Forward iteration") {
      int sum = 0;
      auto it = buffer.begin<int>();
      while (it != buffer.end<int>()) {
        sum += *it++;
      }
      CHECK_EQ(sum, 6);
    }

    SUBCASE("Const iteration") {
      const auto& const_buffer = buffer;
      auto it = const_buffer.begin<int>();
      CHECK_EQ(*it, 1);
    }

    SUBCASE("Reverse iteration") {
      std::vector<int> reversed;
      auto it = buffer.rbegin<int>();
      while (it != buffer.rend<int>()) {
        reversed.push_back(*it++);
      }
      CHECK_EQ(reversed.size(), 3);
      CHECK_EQ(reversed[0], 3);
      CHECK_EQ(reversed[1], 2);
      CHECK_EQ(reversed[2], 1);
    }
  }

  TEST_CASE("container::TypedBufferArray::object lifecycle") {
    SUBCASE("Destructor called on clear") {
      CountingType::Reset();

      {
        TypedBufferArray buffer;
        buffer.EmplaceBack<CountingType>(1);
        buffer.EmplaceBack<CountingType>(2);
        CHECK_GE(CountingType::construct_count, 2);

        const int destructs_before_clear = CountingType::destruct_count;
        buffer.Clear();
        CHECK_EQ(CountingType::destruct_count - destructs_before_clear, 2);
      }
    }

    SUBCASE("Destructor called on scope exit") {
      CountingType::Reset();
      {
        TypedBufferArray buffer;
        buffer.EmplaceBack<CountingType>(1);
        buffer.EmplaceBack<CountingType>(2);
      }
      CHECK_EQ(CountingType::construct_count, CountingType::destruct_count);
    }
  }

  TEST_CASE("container::TypedBufferArray::erase and erase_if") {
    SUBCASE("erase removes matching elements") {
      TypedBufferArray buffer;
      buffer.PushBack(1);
      buffer.PushBack(2);
      buffer.PushBack(3);
      buffer.PushBack(2);
      buffer.PushBack(4);

      auto erased = erase(buffer, 2);
      size_t i = buffer.Size();
      CHECK_EQ(erased, 2);
      CHECK_EQ(i, 3);
    }

    SUBCASE("erase_if removes elements matching predicate") {
      TypedBufferArray buffer;
      buffer.PushBack(1);
      buffer.PushBack(2);
      buffer.PushBack(3);
      buffer.PushBack(4);
      buffer.PushBack(5);

      auto erased =
          erase_if<int>(buffer, [](const int& v) { return v % 2 == 0; });
      size_t i = buffer.Size();
      CHECK_EQ(erased, 2);
      CHECK_EQ(i, 3);
    }
  }

  TEST_CASE("container::TypedBufferArray::ShrinkToFit: reduces capacity") {
    TypedBufferArray buffer;
    buffer.ChangeType<int>();
    buffer.Reserve(100);
    buffer.PushBack(1);
    buffer.PushBack(2);

    CHECK_GE(buffer.Capacity(), 100);
    buffer.ShrinkToFit();
    CHECK_LE(buffer.Capacity(), 10);
  }

  TEST_CASE("container::TypedBufferArray::large data stress test") {
    TypedBufferArray buffer;
    constexpr size_t kLargeCount = 10000;
    for (size_t i = 0; i < kLargeCount; ++i) {
      buffer.PushBack(static_cast<int>(i));
    }
    CHECK_EQ(buffer.Size(), kLargeCount);
    for (size_t i = 0; i < kLargeCount; ++i) {
      CHECK_EQ(buffer.At<int>(i), static_cast<int>(i));
    }
  }

  TEST_CASE(
      "container::TypedBufferArray::Merge: merging same-allocator buffers") {
    SUBCASE("Merge const lvalue source into non-empty destination") {
      TypedBufferArray buffer1;
      buffer1.PushBack(1);
      buffer1.PushBack(2);

      TypedBufferArray buffer2_mut;
      buffer2_mut.PushBack(3);
      buffer2_mut.PushBack(4);
      const TypedBufferArray<>& buffer2 = buffer2_mut;

      buffer1.Merge(buffer2);

      CHECK_EQ(buffer1.Size(), 4);
      CHECK_EQ(buffer1.At<int>(0), 1);
      CHECK_EQ(buffer1.At<int>(1), 2);
      CHECK_EQ(buffer1.At<int>(2), 3);
      CHECK_EQ(buffer1.At<int>(3), 4);
      CHECK_EQ(buffer2.Size(), 2);
      CHECK_EQ(buffer2.At<int>(0), 3);
      CHECK_EQ(buffer2.At<int>(1), 4);
    }

    SUBCASE("Merge non-empty into non-empty") {
      TypedBufferArray buffer1;
      buffer1.PushBack(1);
      buffer1.PushBack(2);

      TypedBufferArray buffer2;
      buffer2.PushBack(3);
      buffer2.PushBack(4);

      buffer1.Merge(std::move(buffer2));

      CHECK_EQ(buffer1.Size(), 4);
      CHECK_EQ(buffer1.At<int>(0), 1);
      CHECK_EQ(buffer1.At<int>(1), 2);
      CHECK_EQ(buffer1.At<int>(2), 3);
      CHECK_EQ(buffer1.At<int>(3), 4);
      CHECK(buffer2.Empty());
    }

    SUBCASE("Merge into empty") {
      TypedBufferArray buffer1;
      TypedBufferArray buffer2;
      buffer2.PushBack(10);
      buffer2.PushBack(20);

      buffer1.Merge(std::move(buffer2));

      CHECK_EQ(buffer1.Size(), 2);
      CHECK_EQ(buffer1.At<int>(0), 10);
      CHECK(buffer2.Empty());
    }

    SUBCASE("Merge empty into non-empty") {
      TypedBufferArray buffer1;
      buffer1.PushBack(1);
      TypedBufferArray buffer2;

      buffer1.Merge(std::move(buffer2));

      CHECK_EQ(buffer1.Size(), 1);
      CHECK_EQ(buffer1.At<int>(0), 1);
    }

    SUBCASE("Merge non-trivial types") {
      TypedBufferArray buffer1;
      buffer1.EmplaceBack<NonTrivial>("hello", 1);

      TypedBufferArray buffer2;
      buffer2.EmplaceBack<NonTrivial>("world", 2);
      buffer2.EmplaceBack<NonTrivial>("!", 3);

      buffer1.Merge(std::move(buffer2));

      CHECK_EQ(buffer1.Size(), 3);
      CHECK_EQ(buffer1.At<NonTrivial>(0).data, "hello");
      CHECK_EQ(buffer1.At<NonTrivial>(1).data, "world");
      CHECK_EQ(buffer1.At<NonTrivial>(2).data, "!");
      CHECK(buffer2.Empty());
    }
  }

  TEST_CASE("container::TypedBufferArray::Swap: swapping contents") {
    SUBCASE("Swap with another buffer") {
      TypedBufferArray buffer1;
      buffer1.PushBack(1);
      buffer1.PushBack(2);

      TypedBufferArray buffer2;
      buffer2.PushBack(10);

      buffer1.Swap(buffer2);

      CHECK_EQ(buffer1.Size(), 1);
      CHECK_EQ(buffer1.At<int>(0), 10);
      CHECK_EQ(buffer2.Size(), 2);
      CHECK_EQ(buffer2.At<int>(0), 1);
    }

    SUBCASE("Swap two elements within buffer (trivial)") {
      TypedBufferArray buffer;
      buffer.PushBack(1);
      buffer.PushBack(2);
      buffer.PushBack(3);

      buffer.Swap(0, 2);
      CHECK_EQ(buffer.At<int>(0), 3);
      CHECK_EQ(buffer.At<int>(2), 1);
    }

    SUBCASE("Swap two elements (typed)") {
      TypedBufferArray buffer;
      buffer.PushBack(10);
      buffer.PushBack(20);

      buffer.Swap<int>(0, 1);
      CHECK_EQ(buffer.At<int>(0), 20);
      CHECK_EQ(buffer.At<int>(1), 10);
    }

    SUBCASE("Swap with custom allocator") {
      using TrackingBuffer = TypedBufferArray<TrackingAllocator<std::byte>>;
      TrackingAllocator<std::byte> alloc;
      TrackingBuffer buffer(alloc);
      buffer.PushBack(1);
      buffer.PushBack(2);

      const int count_before_swap =
          TrackingAllocator<std::byte>::allocation_count.load();
      (void)count_before_swap;

      TrackingBuffer buffer2(alloc);
      buffer2.PushBack(10);

      buffer.Swap(buffer2);

      CHECK_EQ(buffer.Size(), 1);
      CHECK_EQ(buffer.At<int>(0), 10);
    }
  }

  TEST_CASE("container::TypedBufferArray::type identity methods") {
    SUBCASE("StoredTypeId") {
      TypedBufferArray buffer;
      CHECK(buffer.StoredTypeId().Empty());

      buffer.PushBack(1);
      const auto stored_id = buffer.StoredTypeId();
      const auto int_id =
          TypedBufferArray<std::allocator<std::byte>>::TypeIndexOf<int>();
      CHECK_EQ(stored_id, int_id);
    }

    SUBCASE("TypeIndexOf is consistent") {
      TypedBufferArray buffer;
      buffer.PushBack(1);
      buffer.PushBack(2);
      auto span = buffer.Data<int>();
      CHECK_EQ(span.size(), 2);
      const auto& const_buffer = buffer;
      auto cspan = const_buffer.Data<int>();
      CHECK_EQ(cspan.size(), 2);
    }
  }

  TEST_CASE("container::TypedBufferArray::allocator construction") {
    TypedBufferArray buffer;
    buffer.PushBack(1);
    buffer.PushBack(2);
    buffer.PushBack(3);

    SUBCASE("Merge from different allocator") {
      using DefaultBuffer = TypedBufferArray<std::allocator<std::byte>>;
      using TrackingBuffer = TypedBufferArray<TrackingAllocator<std::byte>>;

      DefaultBuffer dest;
      dest.PushBack(1);
      dest.PushBack(2);

      TrackingBuffer src;
      src.PushBack(3);
      src.PushBack(4);

      dest.Merge(std::move(src));

      CHECK_EQ(dest.Size(), 4);
      CHECK_EQ(dest.At<int>(2), 3);
      CHECK_EQ(dest.At<int>(3), 4);
      CHECK(src.Empty());
    }
  }

  TEST_CASE("container::TypedBufferArray::Merge: cross-allocator merging") {
    using DefaultBuffer = TypedBufferArray<std::allocator<std::byte>>;
    using TrackingBuffer = TypedBufferArray<TrackingAllocator<std::byte>>;

    SUBCASE("Merge non-trivial types from different allocator") {
      DefaultBuffer dest;
      dest.EmplaceBack<NonTrivial>("a", 1);

      TrackingBuffer src;
      src.EmplaceBack<NonTrivial>("b", 2);
      src.EmplaceBack<NonTrivial>("c", 3);

      dest.Merge(std::move(src));

      CHECK_EQ(dest.Size(), 3);
      CHECK_EQ(dest.At<NonTrivial>(0).data, "a");
      CHECK_EQ(dest.At<NonTrivial>(1).data, "b");
      CHECK_EQ(dest.At<NonTrivial>(2).data, "c");
      CHECK(src.Empty());
    }

    SUBCASE("Merge const lvalue non-trivial types from different allocator") {
      DefaultBuffer dest;
      dest.EmplaceBack<NonTrivial>("a", 1);

      TrackingBuffer src_mut;
      src_mut.EmplaceBack<NonTrivial>("b", 2);
      src_mut.EmplaceBack<NonTrivial>("c", 3);
      const TrackingBuffer& src = src_mut;

      dest.Merge(src);

      CHECK_EQ(dest.Size(), 3);
      CHECK_EQ(dest.At<NonTrivial>(0).data, "a");
      CHECK_EQ(dest.At<NonTrivial>(1).data, "b");
      CHECK_EQ(dest.At<NonTrivial>(2).data, "c");
      CHECK_EQ(src.Size(), 2);
      CHECK_EQ(src.At<NonTrivial>(0).data, "b");
      CHECK_EQ(src.At<NonTrivial>(1).data, "c");
    }

    SUBCASE("Merge into untyped dest from different allocator") {
      DefaultBuffer dest;

      TrackingBuffer src;
      src.EmplaceBack<int>(42);
      src.EmplaceBack<int>(99);

      DefaultBuffer dest2;
      TrackingBuffer src2;
      src2.EmplaceBack<int>(42);
      src2.EmplaceBack<int>(99);

      dest2.Merge(std::move(src2));

      CHECK_EQ(dest2.Size(), 2);
      CHECK_EQ(dest2.At<int>(0), 42);
      CHECK_EQ(dest2.At<int>(1), 99);
    }

    SUBCASE("TrackingAllocator: no leaks after cross-allocator merge") {
      TrackingAllocator<std::byte>::ResetCount();
      {
        TrackingBuffer src;
        src.EmplaceBack<int>(42);
        src.EmplaceBack<int>(99);

        DefaultBuffer dest;
        dest.Merge(std::move(src));
      }
      CHECK_EQ(TrackingAllocator<std::byte>::allocation_count.load(), 0);
    }
  }

  TEST_CASE("container::PmrTypedBufferArray: works with memory_resource") {
    std::byte buffer[512];
    std::pmr::monotonic_buffer_resource resource(buffer, sizeof(buffer));

    PmrTypedBufferArray typed_buffer_array(&resource);
    typed_buffer_array.EmplaceBack<int>(1);
    typed_buffer_array.EmplaceBack<int>(2);

    CHECK_EQ(typed_buffer_array.Size(), 2);
    CHECK_EQ(typed_buffer_array.At<int>(0), 1);
    CHECK_EQ(typed_buffer_array.At<int>(1), 2);
  }
}  // TEST_SUITE("container::TypedBufferArray")
