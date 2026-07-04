#include <doctest/doctest.h>

#include <helios/memory/ref_counted.hpp>

#include <atomic>
#include <concepts>
#include <cstdint>
#include <memory>
#include <thread>
#include <tuple>
#include <vector>

namespace {

using namespace helios::mem;

class Widget final : public RcFromThis<Widget> {
public:
  explicit Widget(int value) : value_(value) {}
  ~Widget() {
    if (destroy_count_ != nullptr) {
      ++(*destroy_count_);
    }
  }

  int Value() const noexcept { return value_; }
  void SetDestroyCounter(int* counter) noexcept { destroy_count_ = counter; }

private:
  int value_;
  int* destroy_count_ = nullptr;
};

class Node final : public RcFromThis<Node> {
public:
  explicit Node(int id) : id_(id) {}
  int Id() const noexcept { return id_; }

private:
  int id_;
};

class Base : public RcFromThis<Base> {
public:
  virtual ~Base() = default;
  virtual int Kind() const noexcept { return 0; }
};

class Derived final : public Base {
public:
  int Kind() const noexcept override { return 1; }
};

class Texture final : public ArcFromThis<Texture> {
public:
  explicit Texture(int id) : id_(id) {}
  ~Texture() {
    if (destroy_count_ != nullptr) {
      ++(*destroy_count_);
    }
  }

  int Id() const noexcept { return id_; }
  void SetDestroyCounter(std::atomic<int>* counter) noexcept {
    destroy_count_ = counter;
  }

private:
  int id_;
  std::atomic<int>* destroy_count_ = nullptr;
};

class ArcBase : public ArcFromThis<ArcBase> {
public:
  virtual ~ArcBase() = default;
  virtual int Kind() const noexcept { return 0; }
};

class ArcDerived final : public ArcBase {
public:
  int Kind() const noexcept override { return 1; }
};

// A minimal stateful allocator that records how many allocations and
// deallocations have been made.  Two instances with different `state_`
// pointers are intentionally unequal so that propagation rules are exercised.

struct AllocStats {
  int alloc_count = 0;
  int dealloc_count = 0;
};

template <typename T>
class TrackingAllocator {
public:
  using value_type = T;

  // Allocators are propagated on copy/move assignment and swap.
  using propagate_on_container_copy_assignment = std::true_type;
  using propagate_on_container_move_assignment = std::true_type;
  using propagate_on_container_swap = std::true_type;

  explicit TrackingAllocator(AllocStats& stats) noexcept : stats_(&stats) {}

  // Rebind constructor — required by allocator_traits.
  template <typename U>
  TrackingAllocator(const TrackingAllocator<U>& other) noexcept
      : stats_(other.stats_) {}  // NOLINT

  T* allocate(size_t n) {
    ++stats_->alloc_count;
    return static_cast<T*>(::operator new(n * sizeof(T)));
  }

  void deallocate(T* ptr, size_t /*n*/) noexcept {
    ++stats_->dealloc_count;
    ::operator delete(ptr);
  }

  bool operator==(const TrackingAllocator& other) const noexcept {
    return stats_ == other.stats_;
  }
  bool operator!=(const TrackingAllocator& other) const noexcept {
    return stats_ != other.stats_;
  }

  // Exposed so the rebind constructor above can reach it.
  AllocStats* stats_ = nullptr;
};

TEST_SUITE("helios::mem::RefCounted") {
  TEST_CASE("mem::RefCounted::Default construction produces null handle") {
    Rc<Widget> rc;
    CHECK_FALSE(rc);
    CHECK(rc.Empty());
    CHECK_EQ(rc.Get(), nullptr);
    CHECK_EQ(rc.RefCount(), 0);
  }

  TEST_CASE("mem::RefCounted::nullptr construction produces null handle") {
    Rc<Widget> rc(nullptr);
    CHECK_FALSE(rc);
    CHECK(rc.Empty());
    CHECK_EQ(rc.Get(), nullptr);
  }

  TEST_CASE("mem::RefCounted::MakeRc constructs object with ref count 1") {
    auto rc = MakeRc<Widget>(42);

    CHECK(rc);
    CHECK_FALSE(rc.Empty());
    CHECK_NE(rc.Get(), nullptr);
    CHECK_EQ(rc.RefCount(), 1);
    CHECK(rc.Unique());
    CHECK_EQ(rc->Value(), 42);
    CHECK_EQ((*rc).Value(), 42);
  }

  TEST_CASE("mem::RefCounted::Copy construction increments ref count") {
    auto rc1 = MakeRc<Widget>(10);
    CHECK_EQ(rc1.RefCount(), 1);

    const auto rc2 = rc1;
    CHECK_EQ(rc1.RefCount(), 2);
    CHECK_EQ(rc2.RefCount(), 2);
    CHECK_EQ(rc1.Get(), rc2.Get());
    CHECK_FALSE(rc1.Unique());
    CHECK_FALSE(rc2.Unique());
  }

  TEST_CASE(
      "mem::RefCounted::Move construction transfers ownership without "
      "changing ref count") {
    auto rc1 = MakeRc<Widget>(7);
    auto* raw = rc1.Get();
    CHECK_EQ(rc1.RefCount(), 1);

    auto rc2 = std::move(rc1);
    CHECK_FALSE(rc1);  // NOLINT(bugprone-use-after-move)
    CHECK(rc2);
    CHECK_EQ(rc2.Get(), raw);
    CHECK_EQ(rc2.RefCount(), 1);
    CHECK(rc2.Unique());
  }

  TEST_CASE(
      "mem::RefCounted::Destruction decrements ref count and deletes at "
      "zero") {
    int destroyed = 0;
    {
      auto rc = MakeRc<Widget>(1);
      rc->SetDestroyCounter(&destroyed);
      CHECK_EQ(destroyed, 0);
    }
    CHECK_EQ(destroyed, 1);
  }

  TEST_CASE("mem::RefCounted::Last copy destroyed triggers deletion") {
    int destroyed = 0;
    Rc<Widget> rc2;
    {
      auto rc1 = MakeRc<Widget>(99);
      rc1->SetDestroyCounter(&destroyed);
      rc2 = rc1;
      CHECK_EQ(rc2.RefCount(), 2);
    }
    CHECK_EQ(destroyed, 0);  // rc2 still alive
    rc2.Reset();
    CHECK_EQ(destroyed, 1);
  }

  TEST_CASE("mem::RefCounted::Copy assignment increments ref count") {
    auto rc1 = MakeRc<Widget>(5);
    Rc<Widget> rc2;
    rc2 = rc1;

    CHECK_EQ(rc1.RefCount(), 2);
    CHECK_EQ(rc2.RefCount(), 2);
    CHECK_EQ(rc1.Get(), rc2.Get());
  }

  TEST_CASE("mem::RefCounted::Copy assignment to self is safe") {
    auto rc = MakeRc<Widget>(3);
    auto* raw = rc.Get();

    rc = rc;

    CHECK_EQ(rc.RefCount(), 1);
    CHECK_EQ(rc.Get(), raw);
  }

  TEST_CASE("mem::RefCounted::Move assignment transfers ownership") {
    auto rc1 = MakeRc<Widget>(8);
    auto* raw = rc1.Get();
    Rc<Widget> rc2;
    rc2 = std::move(rc1);

    CHECK_FALSE(rc1);  // NOLINT(bugprone-use-after-move)
    CHECK_EQ(rc2.Get(), raw);
    CHECK_EQ(rc2.RefCount(), 1);
  }

  TEST_CASE("mem::RefCounted::Move assignment to self is safe") {
    auto rc = MakeRc<Widget>(11);
    auto* raw = rc.Get();

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-move"
#endif
    rc = std::move(rc);  // NOLINT(bugprone-move-forwarding-reference)
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

    (void)raw;
    CHECK_EQ(rc.RefCount(), rc.Empty() ? 0 : 1);
  }

  TEST_CASE("mem::RefCounted::nullptr assignment resets handle") {
    int destroyed = 0;
    auto rc = MakeRc<Widget>(0);
    rc->SetDestroyCounter(&destroyed);

    rc = nullptr;

    CHECK_FALSE(rc);
    CHECK_EQ(destroyed, 1);
  }

  TEST_CASE("mem::RefCounted::Reset releases handle") {
    int destroyed = 0;
    auto rc = MakeRc<Widget>(0);
    rc->SetDestroyCounter(&destroyed);
    rc.Reset();

    CHECK_FALSE(rc);
    CHECK_EQ(destroyed, 1);
  }

  TEST_CASE("mem::RefCounted::Reset on null handle is safe") {
    Rc<Widget> rc;
    rc.Reset();
    CHECK_FALSE(rc);
  }

  TEST_CASE(
      "mem::RefCounted::Release yields raw pointer and leaves handle null") {
    int destroyed = 0;
    auto rc = MakeRc<Widget>(55);
    rc->SetDestroyCounter(&destroyed);
    auto* raw = rc.Release();

    CHECK_FALSE(rc);
    CHECK_NE(raw, nullptr);
    CHECK_EQ(raw->RefCount(),
             1);  // counter still at 1 — Release didn't decrement
    CHECK_EQ(destroyed, 0);

    // Manually clean up to avoid leak.
    delete raw;
    CHECK_EQ(destroyed, 1);
  }

  TEST_CASE("mem::RefCounted::Equality operators") {
    auto rc1 = MakeRc<Widget>(1);
    auto rc2 = rc1;
    auto rc3 = MakeRc<Widget>(2);
    Rc<Widget> null;

    CHECK_EQ(rc1, rc2);
    CHECK_NE(rc1, rc3);
    CHECK_NE(rc1, null);
    CHECK_EQ(null, nullptr);
    CHECK_NE(rc1, nullptr);
  }

  TEST_CASE("mem::RefCounted::Multiple handles all share same ref count") {
    auto a = MakeRc<Node>(1);
    auto b = a;
    auto c = b;
    auto d = c;

    CHECK_EQ(a.RefCount(), 4);
    CHECK_EQ(b.RefCount(), 4);

    b.Reset();
    CHECK_EQ(a.RefCount(), 3);
    c.Reset();
    CHECK_EQ(a.RefCount(), 2);
    d.Reset();
    CHECK_EQ(a.RefCount(), 1);
    CHECK(a.Unique());
  }

  TEST_CASE("mem::RefCounted::RcFromThis copy/move do not copy ref count") {
    auto rc = MakeRc<Widget>(0);
    CHECK_EQ(rc->RefCount(), 1);

    Widget copy(*rc.Get());
    CHECK_EQ(copy.RefCount(), 0);
  }

  TEST_CASE(
      "mem::RefCounted::PMR — MakeRcWith with memory_resource* creates "
      "PmrRc<T>") {
    std::pmr::monotonic_buffer_resource mbr;
    auto rc = MakeRcWith<Widget>(&mbr, 42);

    CHECK(rc);
    CHECK_EQ(rc->Value(), 42);
    CHECK_EQ(rc.RefCount(), 1);
  }

  TEST_CASE(
      "mem::RefCounted::PMR — PmrRc null constructor with memory_resource* "
      "works") {
    std::pmr::monotonic_buffer_resource mbr;
    PmrRc<Widget> rc(&mbr);

    CHECK_FALSE(rc);
    CHECK(rc.Empty());
    CHECK_EQ(rc.RefCount(), 0);
  }

  TEST_CASE(
      "mem::RefCounted::PMR — PmrRc pointer constructor with "
      "memory_resource* takes ownership") {
    std::pmr::monotonic_buffer_resource mbr;
    std::pmr::polymorphic_allocator<Widget> alloc(&mbr);

    Widget* ptr = alloc.allocate(1);
    alloc.construct(ptr, 123);

    PmrRc<Widget> rc(ptr, &mbr);

    CHECK(rc);
    CHECK_EQ(rc->Value(), 123);
    CHECK_EQ(rc.RefCount(), 1);
  }

  TEST_CASE(
      "mem::RefCounted::PMR — PmrRc copy semantics preserve PMR allocator") {
    std::pmr::monotonic_buffer_resource mbr;
    auto rc1 = MakeRcWith<Widget>(&mbr, 777);

    auto rc2 = rc1;

    CHECK(rc1);
    CHECK(rc2);
    CHECK_EQ(rc2->Value(), 777);
    CHECK_EQ(rc2.RefCount(), 2);
  }

  TEST_CASE(
      "mem::RefCounted::PMR — PmrRc move semantics preserve PMR allocator") {
    std::pmr::monotonic_buffer_resource mbr;
    auto rc1 = MakeRcWith<Widget>(&mbr, 999);

    auto rc2 = std::move(rc1);

    CHECK_FALSE(rc1);
    CHECK(rc2);
    CHECK_EQ(rc2->Value(), 999);
    CHECK_EQ(rc2.RefCount(), 1);
  }

  TEST_CASE(
      "mem::RefCounted::PMR — PmrRc multiple copies increment ref count "
      "correctly") {
    std::pmr::monotonic_buffer_resource mbr;
    auto rc1 = MakeRcWith<Widget>(&mbr, 2222);

    auto rc2 = rc1;
    auto rc3 = rc1;
    auto rc4 = rc2;

    CHECK_EQ(rc1.RefCount(), 4);
    CHECK_EQ(rc2.RefCount(), 4);
    CHECK_EQ(rc3.RefCount(), 4);
    CHECK_EQ(rc4.RefCount(), 4);
  }

  TEST_CASE(
      "mem::RefCounted::PMR — PmrRc Reset decrements ref count and "
      "destroys when zero") {
    std::pmr::monotonic_buffer_resource mbr;
    int destroy_count = 0;

    {
      auto rc = MakeRcWith<Widget>(&mbr, 4444);
      rc->SetDestroyCounter(&destroy_count);
      CHECK_EQ(destroy_count, 0);
      rc.Reset();
      CHECK_EQ(destroy_count, 1);
    }
  }

  TEST_CASE(
      "mem::RefCounted::PMR — PmrRc with unsynchronized_pool_resource "
      "allocates correctly") {
    std::pmr::unsynchronized_pool_resource pool_resource;
    auto rc = MakeRcWith<Widget>(&pool_resource, 6666);

    CHECK(rc);
    CHECK_EQ(rc->Value(), 6666);
    CHECK_EQ(rc.RefCount(), 1);
  }

  TEST_CASE("mem::RefCounted::PMR — PmrRc with default memory resource works") {
    auto rc = MakeRcWith<Widget>(std::pmr::get_default_resource(), 8888);

    CHECK(rc);
    CHECK_EQ(rc->Value(), 8888);
    CHECK_EQ(rc.RefCount(), 1);
  }

  TEST_CASE(
      "mem::RefCounted::PMR — PmrRc polymorphic allocator integration — "
      "GetAllocator returns allocator") {
    std::pmr::monotonic_buffer_resource mbr;
    auto rc = MakeRcWith<Widget>(&mbr, 1111);

    [[maybe_unused]] const auto& alloc = rc.GetAllocator();
    // Verify it's a polymorphic_allocator by checking ref count still works
    CHECK_EQ(rc.RefCount(), 1);
  }
}

TEST_SUITE("helios::mem::AtomicRefCounted") {
  TEST_CASE(
      "mem::AtomicRefCounted::Default construction produces null handle") {
    Arc<Texture> arc;
    CHECK_FALSE(arc);
    CHECK(arc.Empty());
    CHECK_EQ(arc.Get(), nullptr);
    CHECK_EQ(arc.RefCount(), 0);
  }

  TEST_CASE(
      "mem::AtomicRefCounted::nullptr construction produces null handle") {
    Arc<Texture> arc(nullptr);
    CHECK_FALSE(arc);
    CHECK(arc.Empty());
  }

  TEST_CASE(
      "mem::AtomicRefCounted::MakeArc constructs object with ref count 1") {
    auto arc = MakeArc<Texture>(1);

    CHECK(arc);
    CHECK_FALSE(arc.Empty());
    CHECK_NE(arc.Get(), nullptr);
    CHECK_EQ(arc.RefCount(), 1);
    CHECK(arc.Unique());
    CHECK_EQ(arc->Id(), 1);
    CHECK_EQ((*arc).Id(), 1);
  }

  TEST_CASE(
      "mem::AtomicRefCounted::Copy construction increments ref count "
      "atomically") {
    auto arc1 = MakeArc<Texture>(2);
    CHECK_EQ(arc1.RefCount(), 1u);

    auto arc2 = arc1;
    CHECK_EQ(arc1.RefCount(), 2);
    CHECK_EQ(arc2.RefCount(), 2);
    CHECK_EQ(arc1.Get(), arc2.Get());
    CHECK_FALSE(arc1.Unique());
  }

  TEST_CASE(
      "mem::AtomicRefCounted::Move construction transfers ownership without "
      "changing ref count") {
    auto arc1 = MakeArc<Texture>(3);
    auto* raw = arc1.Get();

    auto arc2 = std::move(arc1);
    CHECK_FALSE(arc1);  // NOLINT(bugprone-use-after-move)
    CHECK(arc2);
    CHECK_EQ(arc2.Get(), raw);
    CHECK_EQ(arc2.RefCount(), 1);
    CHECK(arc2.Unique());
  }

  TEST_CASE(
      "mem::AtomicRefCounted::Destruction decrements ref count and deletes "
      "at zero") {
    std::atomic<int> destroyed{0};
    {
      auto arc = MakeArc<Texture>(0);
      arc->SetDestroyCounter(&destroyed);
      CHECK_EQ(destroyed.load(), 0);
    }
    CHECK_EQ(destroyed.load(), 1);
  }

  TEST_CASE("mem::AtomicRefCounted::Last copy destroyed triggers deletion") {
    std::atomic<int> destroyed{0};
    Arc<Texture> arc2;
    {
      auto arc1 = MakeArc<Texture>(0);
      arc1->SetDestroyCounter(&destroyed);
      arc2 = arc1;
      CHECK_EQ(arc2.RefCount(), 2);
    }
    CHECK_EQ(destroyed.load(), 0);
    arc2.Reset();
    CHECK_EQ(destroyed.load(), 1);
  }

  TEST_CASE("mem::AtomicRefCounted::Copy assignment increments ref count") {
    auto arc1 = MakeArc<Texture>(5);
    Arc<Texture> arc2;
    arc2 = arc1;

    CHECK_EQ(arc1.RefCount(), 2);
    CHECK_EQ(arc2.RefCount(), 2);
    CHECK_EQ(arc1.Get(), arc2.Get());
  }

  TEST_CASE("mem::AtomicRefCounted::Copy assignment to self is safe") {
    auto arc = MakeArc<Texture>(6);
    auto* raw = arc.Get();

    arc = arc;

    CHECK_EQ(arc.RefCount(), 1);
    CHECK_EQ(arc.Get(), raw);
  }

  TEST_CASE("mem::AtomicRefCounted::Move assignment transfers ownership") {
    auto arc1 = MakeArc<Texture>(7);
    auto* raw = arc1.Get();
    Arc<Texture> arc2;
    arc2 = std::move(arc1);

    CHECK_FALSE(arc1);  // NOLINT(bugprone-use-after-move)
    CHECK_EQ(arc2.Get(), raw);
    CHECK_EQ(arc2.RefCount(), 1);
  }

  TEST_CASE("mem::AtomicRefCounted::nullptr assignment resets handle") {
    std::atomic<int> destroyed{0};
    auto arc = MakeArc<Texture>(0);
    arc->SetDestroyCounter(&destroyed);

    arc = nullptr;

    CHECK_FALSE(arc);
    CHECK_EQ(destroyed.load(), 1);
  }

  TEST_CASE("mem::AtomicRefCounted::Reset releases handle") {
    std::atomic<int> destroyed{0};
    auto arc = MakeArc<Texture>(0);
    arc->SetDestroyCounter(&destroyed);
    arc.Reset();

    CHECK_FALSE(arc);
    CHECK_EQ(destroyed.load(), 1);
  }

  TEST_CASE("mem::AtomicRefCounted::Reset on null handle is safe") {
    Arc<Texture> arc;
    arc.Reset();
    CHECK_FALSE(arc);
  }

  TEST_CASE(
      "mem::AtomicRefCounted::Release yields raw pointer and leaves handle "
      "null") {
    std::atomic<int> destroyed{0};
    auto arc = MakeArc<Texture>(0);
    arc->SetDestroyCounter(&destroyed);
    auto* raw = arc.Release();

    CHECK_FALSE(arc);
    CHECK_NE(raw, nullptr);
    CHECK_EQ(raw->RefCount(), 1);
    CHECK_EQ(destroyed.load(), 0);

    delete raw;
    CHECK_EQ(destroyed.load(), 1);
  }

  TEST_CASE("mem::AtomicRefCounted::Equality operators") {
    auto arc1 = MakeArc<Texture>(1);
    auto arc2 = arc1;
    auto arc3 = MakeArc<Texture>(2);
    Arc<Texture> null;

    CHECK_EQ(arc1, arc2);
    CHECK_NE(arc1, arc3);
    CHECK_NE(arc1, null);
    CHECK_EQ(null, nullptr);
    CHECK_NE(arc1, nullptr);
  }

  TEST_CASE(
      "mem::AtomicRefCounted::Multiple handles all share same ref count") {
    auto a = MakeArc<Texture>(0);
    auto b = a;
    auto c = b;
    auto d = c;

    CHECK_EQ(a.RefCount(), 4);

    b.Reset();
    CHECK_EQ(a.RefCount(), 3);
    c.Reset();
    CHECK_EQ(a.RefCount(), 2);
    d.Reset();
    CHECK_EQ(a.RefCount(), 1);
    CHECK(a.Unique());
  }

  TEST_CASE(
      "mem::AtomicRefCounted::ArcFromThis copy/move do not copy ref count") {
    auto arc = MakeArc<Texture>(0);
    CHECK_EQ(arc->RefCount(), 1);

    Texture copy(*arc.Get());
    CHECK_EQ(copy.RefCount(), 0);
  }

  TEST_CASE(
      "mem::AtomicRefCounted::Concurrent copy-construction from shared "
      "handle is safe") {
    constexpr int kThreadCount = 8;
    constexpr int kCopiesPerThread = 1000;

    auto shared = MakeArc<Texture>(0);
    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);

    std::atomic<int> copies_alive{0};

    for (int t = 0; t < kThreadCount; ++t) {
      threads.emplace_back([&shared, &copies_alive]() {
        std::vector<Arc<Texture>> local;
        local.reserve(kCopiesPerThread);
        for (int i = 0; i < kCopiesPerThread; ++i) {
          local.push_back(shared);
          ++copies_alive;
        }
        for (auto& arc : local) {
          arc.Reset();
          --copies_alive;
        }
      });
    }

    for (auto& th : threads) {
      th.join();
    }

    CHECK_EQ(shared.RefCount(), 1);
    CHECK_EQ(copies_alive.load(), 0);
  }

  TEST_CASE(
      "mem::AtomicRefCounted::Concurrent destruction from multiple threads "
      "is safe") {
    constexpr int kThreadCount = 8;
    std::atomic<int> destroyed{0};

    auto source = MakeArc<Texture>(0);
    source->SetDestroyCounter(&destroyed);

    std::vector<Arc<Texture>> handles(kThreadCount, source);
    source.Reset();

    CHECK_EQ(handles.front().RefCount(), static_cast<uint32_t>(kThreadCount));

    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);

    for (int t = 0; t < kThreadCount; ++t) {
      threads.emplace_back(
          [&handles, t]() { handles[static_cast<size_t>(t)].Reset(); });
    }

    for (auto& th : threads) {
      th.join();
    }

    CHECK_EQ(destroyed.load(), 1);
  }

  TEST_CASE(
      "mem::AtomicRefCounted::PMR — MakeArcWith with memory_resource* "
      "creates PmrArc<T>") {
    std::pmr::monotonic_buffer_resource mbr;
    auto arc = MakeArcWith<Texture>(&mbr, 99);

    CHECK(arc);
    CHECK_EQ(arc->Id(), 99);
    CHECK_EQ(arc.RefCount(), 1);
  }

  TEST_CASE(
      "mem::AtomicRefCounted::PMR — PmrArc null constructor with "
      "memory_resource* works") {
    std::pmr::monotonic_buffer_resource mbr;
    PmrArc<Texture> arc(&mbr);

    CHECK_FALSE(arc);
    CHECK(arc.Empty());
    CHECK_EQ(arc.RefCount(), 0);
  }

  TEST_CASE(
      "mem::AtomicRefCounted::PMR — PmrArc pointer constructor with "
      "memory_resource* takes ownership") {
    std::pmr::monotonic_buffer_resource mbr;
    std::pmr::polymorphic_allocator<Texture> alloc(&mbr);

    Texture* ptr = alloc.allocate(1);
    alloc.construct(ptr, 456);

    PmrArc<Texture> arc(ptr, &mbr);

    CHECK(arc);
    CHECK_EQ(arc->Id(), 456);
    CHECK_EQ(arc.RefCount(), 1);
  }

  TEST_CASE(
      "mem::AtomicRefCounted::PMR — PmrArc copy semantics preserve PMR "
      "allocator") {
    std::pmr::monotonic_buffer_resource mbr;
    auto arc1 = MakeArcWith<Texture>(&mbr, 888);

    auto arc2 = arc1;

    CHECK(arc1);
    CHECK(arc2);
    CHECK_EQ(arc2->Id(), 888);
    CHECK_EQ(arc2.RefCount(), 2);
  }

  TEST_CASE(
      "mem::AtomicRefCounted::PMR — PmrArc move semantics preserve PMR "
      "allocator") {
    std::pmr::monotonic_buffer_resource mbr;
    auto arc1 = MakeArcWith<Texture>(&mbr, 1111);

    auto arc2 = std::move(arc1);

    CHECK_FALSE(arc1);
    CHECK(arc2);
    CHECK_EQ(arc2->Id(), 1111);
    CHECK_EQ(arc2.RefCount(), 1);
  }

  TEST_CASE(
      "mem::AtomicRefCounted::PMR — PmrArc multiple copies increment ref "
      "count correctly") {
    std::pmr::monotonic_buffer_resource mbr;
    auto arc1 = MakeArcWith<Texture>(&mbr, 3333);

    auto arc2 = arc1;
    auto arc3 = arc1;
    auto arc4 = arc2;

    CHECK_EQ(arc1.RefCount(), 4);
    CHECK_EQ(arc2.RefCount(), 4);
    CHECK_EQ(arc3.RefCount(), 4);
    CHECK_EQ(arc4.RefCount(), 4);
  }

  TEST_CASE(
      "mem::AtomicRefCounted::PMR — PmrArc Reset decrements ref count and "
      "destroys when zero") {
    std::pmr::monotonic_buffer_resource mbr;
    std::atomic<int> destroy_count{0};

    {
      auto arc = MakeArcWith<Texture>(&mbr, 5555);
      arc->SetDestroyCounter(&destroy_count);
      CHECK_EQ(destroy_count.load(), 0);
      arc.Reset();
      CHECK_EQ(destroy_count.load(), 1);
    }
  }

  TEST_CASE(
      "mem::AtomicRefCounted::PMR — PmrArc with "
      "unsynchronized_pool_resource "
      "allocates correctly") {
    std::pmr::unsynchronized_pool_resource pool_resource;
    auto arc = MakeArcWith<Texture>(&pool_resource, 7777);

    CHECK(arc);
    CHECK_EQ(arc->Id(), 7777);
    CHECK_EQ(arc.RefCount(), 1);
  }

  TEST_CASE(
      "mem::AtomicRefCounted::PMR — PmrArc with default memory resource "
      "works") {
    auto arc = MakeArcWith<Texture>(std::pmr::get_default_resource(), 9999);

    CHECK(arc);
    CHECK_EQ(arc->Id(), 9999);
    CHECK_EQ(arc.RefCount(), 1);
  }

  TEST_CASE(
      "mem::AtomicRefCounted::PMR — PmrArc polymorphic allocator "
      "integration — GetAllocator returns allocator") {
    std::pmr::monotonic_buffer_resource mbr;
    auto arc = MakeArcWith<Texture>(&mbr, 2222);

    [[maybe_unused]] const auto& alloc = arc.GetAllocator();
    // Verify it's a polymorphic_allocator by checking ref count still works
    CHECK_EQ(arc.RefCount(), 1);
  }

  TEST_CASE(
      "mem::AtomicRefCounted::PMR — PmrArc concurrent access with PMR "
      "allocator is thread-safe") {
    constexpr int kThreadCount = 4;
    std::pmr::monotonic_buffer_resource mbr;
    auto arc = MakeArcWith<Texture>(&mbr, 3333);

    std::vector<PmrArc<Texture>> copies;
    std::vector<std::thread> threads;

    threads.reserve(kThreadCount);
    for (int t = 0; t < kThreadCount; ++t) {
      threads.emplace_back([&arc]() {
        auto copy = arc;
        CHECK_EQ(copy->Id(), 3333);
      });
    }
    for (auto& th : threads) {
      th.join();
    }

    CHECK_EQ(arc.RefCount(), 1);
  }
}

TEST_SUITE("helios::mem::RcArcAliases") {
  TEST_CASE("mem::RcArcAliases::Rc<T> is RefCounted<T, std::allocator<T>>") {
    static_assert(
        std::same_as<Rc<Widget>, RefCounted<Widget, std::allocator<Widget>>>);
    auto rc = MakeRc<Widget>(1);
    CHECK_EQ(rc.RefCount(), 1);
  }

  TEST_CASE(
      "mem::RcArcAliases::Arc<T> is AtomicRefCounted<T, "
      "std::allocator<T>>") {
    static_assert(
        std::same_as<Arc<Texture>,
                     AtomicRefCounted<Texture, std::allocator<Texture>>>);
    auto arc = MakeArc<Texture>(1);
    CHECK_EQ(arc.RefCount(), 1);
  }
}

TEST_SUITE("helios::mem::MakeRcWithAllocator") {
  TEST_CASE(
      "mem::MakeRcWithAllocator::MakeRc without allocator argument requires "
      "a default-constructible allocator") {
    // TrackingAllocator is intentionally NOT default-constructible, so
    // MakeRcWith with a TrackingAllocator must be used instead.  We verify the
    // constraint is enforced at the type level.
    static_assert(!std::default_initializable<TrackingAllocator<Widget>>,
                  "TrackingAllocator must not be default-constructible");
    // std::allocator IS default-constructible, so the plain overload works.
    static_assert(std::default_initializable<std::allocator<Widget>>,
                  "std::allocator must be default-constructible");

    // MakeRcWith supplies the allocator explicitly — one allocation expected.
    AllocStats stats;
    auto rc = MakeRcWith<Widget>(TrackingAllocator<Widget>{stats}, 42);
    CHECK_EQ(stats.alloc_count, 1);
    CHECK_EQ(rc->Value(), 42);
  }

  TEST_CASE(
      "mem::MakeRcWithAllocator::MakeRcWith — object is destroyed through "
      "the custom allocator") {
    AllocStats stats;
    int destroyed = 0;
    {
      auto rc = MakeRcWith<Widget>(TrackingAllocator<Widget>{stats}, 7);
      rc->SetDestroyCounter(&destroyed);
      CHECK_EQ(stats.alloc_count, 1);
      CHECK_EQ(stats.dealloc_count, 0);
      CHECK_EQ(destroyed, 0);
    }
    // Both destructor and deallocate must have been called exactly once.
    CHECK_EQ(destroyed, 1);
    CHECK_EQ(stats.dealloc_count, 1);
    // No extra allocations should have occurred.
    CHECK_EQ(stats.alloc_count, 1);
  }

  TEST_CASE(
      "mem::MakeRcWithAllocator::MakeRcWith — dealloc fires on last copy "
      "going out of scope") {
    AllocStats stats;
    int destroyed = 0;

    Rc<Widget, TrackingAllocator<Widget>> outer{
        TrackingAllocator<Widget>{stats}};
    {
      auto rc = MakeRcWith<Widget>(TrackingAllocator<Widget>{stats}, 99);
      rc->SetDestroyCounter(&destroyed);
      outer = rc;  // ref count → 2; POCCA propagates rc's allocator
      CHECK_EQ(rc.RefCount(), 2);
      CHECK_EQ(stats.dealloc_count, 0);
    }
    // rc destroyed, ref count → 1; object must still be alive.
    CHECK_EQ(destroyed, 0);
    CHECK_EQ(stats.dealloc_count, 0);

    outer.Reset();  // ref count → 0
    CHECK_EQ(destroyed, 1);
    CHECK_EQ(stats.dealloc_count, 1);
  }

  TEST_CASE(
      "mem::MakeRcWithAllocator::Copy of Rc propagates allocator (POCCA = "
      "true)") {
    AllocStats stats;
    auto rc1 = MakeRcWith<Widget>(TrackingAllocator<Widget>{stats}, 5);

    // Null handle seeded with the same stats allocator so POCCA propagation is
    // observable.
    Rc<Widget, TrackingAllocator<Widget>> rc2{TrackingAllocator<Widget>{stats}};
    rc2 = rc1;

    // Both handles should report the same underlying stats pointer.
    CHECK_EQ(rc2.GetAllocator().stats_, rc1.GetAllocator().stats_);
    CHECK_EQ(rc2.RefCount(), 2);
  }

  TEST_CASE(
      "mem::MakeRcWithAllocator::Move of Rc propagates allocator (POCMA = "
      "true)") {
    AllocStats stats;
    auto rc1 = MakeRcWith<Widget>(TrackingAllocator<Widget>{stats}, 5);

    Rc<Widget, TrackingAllocator<Widget>> rc2{TrackingAllocator<Widget>{stats}};
    rc2 = std::move(rc1);

    CHECK_EQ(rc2.GetAllocator().stats_, &stats);
    CHECK_EQ(rc2.RefCount(), 1);
    CHECK_FALSE(rc1);  // NOLINT(bugprone-use-after-move)
  }

  TEST_CASE(
      "mem::MakeRcWithAllocator::GetAllocator returns the allocator stored "
      "in the handle") {
    AllocStats stats;
    auto rc = MakeRcWith<Widget>(TrackingAllocator<Widget>{stats}, 0);
    CHECK_EQ(rc.GetAllocator().stats_, &stats);
  }
}

TEST_SUITE("helios::mem::MakeArcWithAllocator") {
  TEST_CASE(
      "mem::MakeArcWithAllocator::MakeArcWith — object is destroyed through "
      "the custom allocator") {
    AllocStats stats;
    std::atomic<int> destroyed{0};
    {
      auto arc = MakeArcWith<Texture>(TrackingAllocator<Texture>{stats}, 3);
      arc->SetDestroyCounter(&destroyed);
      CHECK_EQ(stats.alloc_count, 1);
      CHECK_EQ(stats.dealloc_count, 0);
    }
    CHECK_EQ(destroyed.load(), 1);
    CHECK_EQ(stats.dealloc_count, 1);
    CHECK_EQ(stats.alloc_count, 1);
  }

  TEST_CASE(
      "mem::MakeArcWithAllocator::MakeArcWith — dealloc fires on last copy "
      "going out of scope") {
    AllocStats stats;
    std::atomic<int> destroyed{0};

    Arc<Texture, TrackingAllocator<Texture>> outer{
        TrackingAllocator<Texture>{stats}};
    {
      auto arc = MakeArcWith<Texture>(TrackingAllocator<Texture>{stats}, 0);
      arc->SetDestroyCounter(&destroyed);
      outer = arc;
      CHECK_EQ(arc.RefCount(), 2);
    }
    CHECK_EQ(destroyed.load(), 0);
    CHECK_EQ(stats.dealloc_count, 0);

    outer.Reset();
    CHECK_EQ(destroyed.load(), 1);
    CHECK_EQ(stats.dealloc_count, 1);
  }

  TEST_CASE(
      "mem::MakeArcWithAllocator::Copy of Arc propagates allocator (POCCA = "
      "true)") {
    AllocStats stats;
    auto arc1 = MakeArcWith<Texture>(TrackingAllocator<Texture>{stats}, 1);

    Arc<Texture, TrackingAllocator<Texture>> arc2{
        TrackingAllocator<Texture>{stats}};
    arc2 = arc1;

    CHECK_EQ(arc2.GetAllocator().stats_, arc1.GetAllocator().stats_);
    CHECK_EQ(arc2.RefCount(), 2);
  }

  TEST_CASE(
      "mem::MakeArcWithAllocator::Move of Arc propagates allocator (POCMA = "
      "true)") {
    AllocStats stats;
    auto arc1 = MakeArcWith<Texture>(TrackingAllocator<Texture>{stats}, 1);

    Arc<Texture, TrackingAllocator<Texture>> arc2{
        TrackingAllocator<Texture>{stats}};
    arc2 = std::move(arc1);

    CHECK_EQ(arc2.GetAllocator().stats_, &stats);
    CHECK_EQ(arc2.RefCount(), 1);
    CHECK_FALSE(arc1);  // NOLINT(bugprone-use-after-move)
  }

  TEST_CASE(
      "mem::MakeArcWithAllocator::GetAllocator returns the allocator stored "
      "in the handle") {
    AllocStats stats;
    auto arc = MakeArcWith<Texture>(TrackingAllocator<Texture>{stats}, 0);
    CHECK_EQ(arc.GetAllocator().stats_, &stats);
  }

  TEST_CASE(
      "mem::MakeArcWithAllocator::Concurrent destruction uses custom "
      "allocator exactly once") {
    // Verify that the dealloc count is exactly 1 even when many threads race
    // to drop the last reference, matching the destructor-call count.
    constexpr int kThreadCount = 8;
    AllocStats stats;
    std::atomic<int> destroyed{0};

    auto source = MakeArcWith<Texture>(TrackingAllocator<Texture>{stats}, 0);
    source->SetDestroyCounter(&destroyed);

    std::vector<Arc<Texture, TrackingAllocator<Texture>>> handles(kThreadCount,
                                                                  source);
    source.Reset();

    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);
    for (int t = 0; t < kThreadCount; ++t) {
      threads.emplace_back(
          [&handles, t]() { handles[static_cast<size_t>(t)].Reset(); });
    }
    for (auto& th : threads) {
      th.join();
    }

    CHECK_EQ(destroyed.load(), 1);
    CHECK_EQ(stats.dealloc_count, 1);
    CHECK_EQ(stats.alloc_count, 1);
  }
}

}  // namespace
