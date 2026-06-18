#include <doctest/doctest.h>

#include <helios/container/typed_buffer.hpp>

#include <atomic>
#include <memory>
#include <memory_resource>
#include <string>

using namespace helios::container;

namespace {

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

TEST_SUITE("helios::container::TypedBuffer") {
  TEST_CASE("container::TypedBuffer::ctor: default construction") {
    TypedBuffer buf;

    CHECK(buf.Empty());
    CHECK_FALSE(buf.HasType());
    CHECK_EQ(buf.ElementSize(), 0);
    CHECK(buf.Bytes().empty());
  }

  TEST_CASE("container::TypedBuffer::ctor: allocator construction") {
    std::allocator<std::byte> alloc;
    TypedBuffer buf(alloc);

    CHECK(buf.Empty());
    CHECK_FALSE(buf.HasType());
  }

  TEST_CASE("container::TypedBuffer::ctor: in_place construction (trivial)") {
    TypedBuffer buf(std::in_place_type<int>, 42);

    CHECK_FALSE(buf.Empty());
    CHECK(buf.HasType());
    CHECK(buf.IsType<int>());
    CHECK_EQ(buf.Value<int>(), 42);
    CHECK_EQ(buf.ElementSize(), sizeof(int));
  }

  TEST_CASE(
      "container::TypedBuffer::ctor: in_place construction (non-trivial)") {
    TypedBuffer buf(std::in_place_type<NonTrivial>, "hello", 7);

    CHECK_FALSE(buf.Empty());
    CHECK(buf.IsType<NonTrivial>());
    CHECK_EQ(buf.Value<NonTrivial>().data, "hello");
    CHECK_EQ(buf.Value<NonTrivial>().value, 7);
  }

  TEST_CASE("container::TypedBuffer::ctor: in_place with allocator") {
    std::allocator<std::byte> alloc;
    TypedBuffer buf(std::in_place_type<int>, alloc, 99);

    CHECK_FALSE(buf.Empty());
    CHECK_EQ(buf.Value<int>(), 99);
  }

  TEST_CASE("container::TypedBuffer::ctor: copy construction (with value)") {
    TypedBuffer original(std::in_place_type<int>, 123);
    TypedBuffer copy(original);

    CHECK_FALSE(copy.Empty());
    CHECK(copy.IsType<int>());
    CHECK_EQ(copy.Value<int>(), 123);

    // Original unchanged
    CHECK_EQ(original.Value<int>(), 123);
  }

  TEST_CASE(
      "container::TypedBuffer::ctor: copy construction (empty with type)") {
    TypedBuffer original;
    original.ChangeType<int>();

    TypedBuffer copy(original);

    CHECK(copy.Empty());
    CHECK(copy.HasType());
    CHECK(copy.IsType<int>());
  }

  TEST_CASE("container::TypedBuffer::ctor: copy construction (no type)") {
    TypedBuffer original;
    TypedBuffer copy(original);

    CHECK(copy.Empty());
    CHECK_FALSE(copy.HasType());
  }

  TEST_CASE("container::TypedBuffer::ctor: move construction") {
    TypedBuffer original(std::in_place_type<int>, 42);
    TypedBuffer moved(std::move(original));

    CHECK_FALSE(moved.Empty());
    CHECK(moved.IsType<int>());
    CHECK_EQ(moved.Value<int>(), 42);

    // Original is now empty
    CHECK(original.Empty());
    CHECK_FALSE(original.HasType());
  }

  TEST_CASE("container::TypedBuffer::operator=: copy assignment") {
    TypedBuffer original(std::in_place_type<int>, 55);
    TypedBuffer copy;

    copy = original;

    CHECK_FALSE(copy.Empty());
    CHECK_EQ(copy.Value<int>(), 55);
    CHECK_EQ(original.Value<int>(), 55);
  }

  TEST_CASE("container::TypedBuffer::operator=: move assignment") {
    TypedBuffer original(std::in_place_type<int>, 77);
    TypedBuffer moved;

    moved = std::move(original);

    CHECK_FALSE(moved.Empty());
    CHECK_EQ(moved.Value<int>(), 77);
    CHECK(original.Empty());
  }

  TEST_CASE("container::TypedBuffer::ChangeType: sets type without value") {
    TypedBuffer buf;

    buf.ChangeType<int>();

    CHECK(buf.Empty());
    CHECK(buf.HasType());
    CHECK(buf.IsType<int>());
  }

  TEST_CASE("container::TypedBuffer::ChangeType: destroys existing value") {
    CountingType::Reset();

    TypedBuffer buf(std::in_place_type<CountingType>, 1);
    CHECK_EQ(CountingType::construct_count, 1);

    buf.ChangeType<float>();

    CHECK_EQ(CountingType::destruct_count, 1);
    CHECK(buf.Empty());
    CHECK(buf.IsType<float>());
  }

  TEST_CASE("container::TypedBuffer::Reset: clears value and type") {
    TypedBuffer buf(std::in_place_type<int>, 99);

    buf.Reset();

    CHECK(buf.Empty());
    CHECK_FALSE(buf.HasType());
  }

  TEST_CASE("container::TypedBuffer::Reset: destroys non-trivial value") {
    CountingType::Reset();

    {
      TypedBuffer buf(std::in_place_type<CountingType>, 5);
      CHECK_EQ(CountingType::construct_count, 1);

      buf.Reset();
      CHECK_EQ(CountingType::destruct_count, 1);
    }

    CHECK_EQ(CountingType::construct_count, CountingType::destruct_count);
  }

  TEST_CASE("container::TypedBuffer::Set: set new value") {
    TypedBuffer buf;

    auto& ref = buf.Set<int>(42);

    CHECK_EQ(ref, 42);
    CHECK_FALSE(buf.Empty());
    CHECK(buf.IsType<int>());
    CHECK_EQ(buf.Value<int>(), 42);
  }

  TEST_CASE("container::TypedBuffer::Set: replace existing value (same type)") {
    TypedBuffer buf(std::in_place_type<int>, 10);

    buf.Set<int>(20);

    CHECK_EQ(buf.Value<int>(), 20);
  }

  TEST_CASE(
      "container::TypedBuffer::Set: replace existing value (different type)") {
    TypedBuffer buf(std::in_place_type<int>, 42);

    buf.Set<float>(3.14f);

    CHECK(buf.IsType<float>());
    CHECK_EQ(buf.Value<float>(), doctest::Approx(3.14f));
  }

  TEST_CASE(
      "container::TypedBuffer::Set: destroys old value before constructing "
      "new") {
    CountingType::Reset();

    TypedBuffer buf(std::in_place_type<CountingType>, 1);
    CHECK_EQ(CountingType::construct_count, 1);

    buf.Set<CountingType>(2);

    CHECK_EQ(CountingType::destruct_count, 1);
    CHECK_EQ(CountingType::construct_count, 2);
    CHECK_EQ(buf.Value<CountingType>().value, 2);
  }

  TEST_CASE("container::TypedBuffer::Set: non-trivial type with arguments") {
    TypedBuffer buf;

    buf.Set<NonTrivial>("world", 42);

    CHECK(buf.IsType<NonTrivial>());
    CHECK_EQ(buf.Value<NonTrivial>().data, "world");
    CHECK_EQ(buf.Value<NonTrivial>().value, 42);
  }

  TEST_CASE("container::TypedBuffer::Swap: swaps two buffers") {
    TypedBuffer buf1(std::in_place_type<int>, 10);
    TypedBuffer buf2(std::in_place_type<int>, 20);

    buf1.Swap(buf2);

    CHECK_EQ(buf1.Value<int>(), 20);
    CHECK_EQ(buf2.Value<int>(), 10);
  }

  TEST_CASE("container::TypedBuffer::Swap: swap with empty") {
    TypedBuffer buf1(std::in_place_type<int>, 99);
    TypedBuffer buf2;

    buf1.Swap(buf2);

    CHECK(buf1.Empty());
    CHECK_FALSE(buf2.Empty());
    CHECK_EQ(buf2.Value<int>(), 99);
  }

  TEST_CASE("container::TypedBuffer::swap friend function") {
    TypedBuffer buf1(std::in_place_type<int>, 1);
    TypedBuffer buf2(std::in_place_type<int>, 2);

    swap(buf1, buf2);

    CHECK_EQ(buf1.Value<int>(), 2);
    CHECK_EQ(buf2.Value<int>(), 1);
  }

  TEST_CASE("container::TypedBuffer::Value: access stored value") {
    TypedBuffer buf(std::in_place_type<int>, 123);

    CHECK_EQ(buf.Value<int>(), 123);

    buf.Value<int>() = 456;
    CHECK_EQ(buf.Value<int>(), 456);
  }

  TEST_CASE("container::TypedBuffer::Value const: const access") {
    const TypedBuffer buf(std::in_place_type<int>, 77);

    CHECK_EQ(buf.Value<int>(), 77);
  }

  TEST_CASE("container::TypedBuffer::Empty: empty check") {
    TypedBuffer buf;
    CHECK(buf.Empty());

    buf.ChangeType<int>();
    CHECK(buf.Empty());

    buf.Set<int>(1);
    CHECK_FALSE(buf.Empty());

    buf.Reset();
    CHECK(buf.Empty());
  }

  TEST_CASE("container::TypedBuffer::HasType: type check") {
    TypedBuffer buf;
    CHECK_FALSE(buf.HasType());

    buf.ChangeType<int>();
    CHECK(buf.HasType());

    buf.Reset();
    CHECK_FALSE(buf.HasType());
  }

  TEST_CASE("container::TypedBuffer::IsType: type comparison") {
    TypedBuffer buf;
    CHECK(buf.IsType<int>());
    CHECK(buf.IsType<float>());

    buf.ChangeType<int>();
    CHECK(buf.IsType<int>());
    CHECK_FALSE(buf.IsType<float>());
  }

  TEST_CASE("container::TypedBuffer::StoredTypeId: returns type index") {
    TypedBuffer buf(std::in_place_type<int>, 0);

    const auto stored_id = buf.StoredTypeId();
    const auto int_id = TypedBuffer<>::TypeIndexOf<int>();

    CHECK_EQ(stored_id, int_id);
  }

  TEST_CASE("container::TypedBuffer::ElementSize: returns element size") {
    TypedBuffer buf;
    CHECK_EQ(buf.ElementSize(), 0);

    buf.ChangeType<int>();
    CHECK_EQ(buf.ElementSize(), sizeof(int));

    buf.ChangeType<double>();
    CHECK_EQ(buf.ElementSize(), sizeof(double));
  }

  TEST_CASE("container::TypedBuffer::Bytes: byte span") {
    TypedBuffer buf;
    CHECK(buf.Bytes().empty());

    buf.Set<int>(42);
    CHECK_EQ(buf.Bytes().size(), sizeof(int));
  }

  TEST_CASE("container::TypedBuffer::non-trivial type lifetime management") {
    CountingType::Reset();

    {
      TypedBuffer buf(std::in_place_type<CountingType>, 10);
      CHECK_EQ(CountingType::construct_count, 1);
    }

    CHECK_EQ(CountingType::destruct_count, 1);
  }

  TEST_CASE("container::TypedBuffer::copy constructor copies non-trivial") {
    CountingType::Reset();

    TypedBuffer original(std::in_place_type<CountingType>, 5);
    TypedBuffer copy(original);

    CHECK_EQ(CountingType::copy_count, 1);
    CHECK_EQ(copy.Value<CountingType>().value, 5);
    CHECK_EQ(original.Value<CountingType>().value, 5);
  }

  TEST_CASE("container::TypedBuffer::string storage") {
    TypedBuffer buf;
    buf.Set<std::string>("hello world");

    CHECK(buf.IsType<std::string>());
    CHECK_EQ(buf.Value<std::string>(), "hello world");

    buf.Set<std::string>("changed");
    CHECK_EQ(buf.Value<std::string>(), "changed");
  }

  TEST_CASE("container::TypedBuffer::custom allocator") {
    using TrackingBuffer = TypedBuffer<TrackingAllocator<std::byte>>;
    TrackingAllocator<std::byte>::ResetCount();

    {
      TrackingBuffer buf(std::in_place_type<int>, 42);
      CHECK_FALSE(buf.Empty());
      CHECK_EQ(buf.Value<int>(), 42);
    }

    CHECK_EQ(TrackingAllocator<std::byte>::allocation_count.load(), 0);
  }

  TEST_CASE("container::TypedBuffer::multiple Set calls reuse storage") {
    TypedBuffer buf;

    buf.Set<int>(1);
    buf.Set<int>(2);
    buf.Set<int>(3);

    CHECK_EQ(buf.Value<int>(), 3);
    CHECK_FALSE(buf.Empty());
  }

  TEST_CASE("container::TypedBuffer::ChangeType then Set") {
    TypedBuffer buf;

    buf.ChangeType<float>();
    CHECK(buf.Empty());
    CHECK(buf.IsType<float>());

    buf.Set<float>(1.5f);
    CHECK_FALSE(buf.Empty());
    CHECK_EQ(buf.Value<float>(), doctest::Approx(1.5f));
  }

  TEST_CASE("container::PmrTypedBuffer: works with memory_resource") {
    std::byte buffer[256];
    std::pmr::monotonic_buffer_resource resource(buffer, sizeof(buffer));

    PmrTypedBuffer typed_buffer{&resource};
    typed_buffer.Set<int>(42);

    CHECK_FALSE(typed_buffer.Empty());
    CHECK_EQ(typed_buffer.Value<int>(), 42);
  }
}  // TEST_SUITE("container::TypedBuffer")
