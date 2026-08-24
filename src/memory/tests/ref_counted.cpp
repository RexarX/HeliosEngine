#include <doctest/doctest.h>

#include <helios/memory/ref_counted.hpp>

#include <atomic>
#include <concepts>
#include <cstddef>
#include <memory_resource>
#include <thread>
#include <utility>
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

  void SetDestroyCounter(int* counter) noexcept { destroy_count_ = counter; }

  [[nodiscard]] int Value() const noexcept { return value_; }

private:
  int value_;
  int* destroy_count_ = nullptr;
};

class Texture final : public ArcFromThis<Texture> {
public:
  explicit Texture(int id) : id_(id) {}
  ~Texture() {
    if (destroy_count_ != nullptr) {
      ++(*destroy_count_);
    }
  }

  void SetDestroyCounter(std::atomic<int>* counter) noexcept {
    destroy_count_ = counter;
  }

  [[nodiscard]] int Id() const noexcept { return id_; }

private:
  int id_;
  std::atomic<int>* destroy_count_ = nullptr;
};

class TrackingMemoryResource final : public std::pmr::memory_resource {
public:
  [[nodiscard]] int AllocCount() const noexcept { return alloc_count_; }
  [[nodiscard]] int DeallocCount() const noexcept { return dealloc_count_; }

private:
  void* do_allocate(size_t bytes, size_t alignment) override {
    ++alloc_count_;
    return std::pmr::new_delete_resource()->allocate(bytes, alignment);
  }

  void do_deallocate(void* ptr, size_t bytes, size_t alignment) override {
    ++dealloc_count_;
    std::pmr::new_delete_resource()->deallocate(ptr, bytes, alignment);
  }

  [[nodiscard]] bool do_is_equal(
      const std::pmr::memory_resource& other) const noexcept override {
    return this == &other;
  }

  int alloc_count_ = 0;
  int dealloc_count_ = 0;
};

class DefaultResourceGuard final {
public:
  explicit DefaultResourceGuard(std::pmr::memory_resource* resource) noexcept
      : previous_(std::pmr::set_default_resource(resource)) {}

  ~DefaultResourceGuard() { std::pmr::set_default_resource(previous_); }

private:
  std::pmr::memory_resource* previous_;
};

TEST_SUITE("helios::mem::RefCounted") {
  TEST_CASE("RefCounted::null construction") {
    Rc<Widget> default_constructed;
    Rc<Widget> nullptr_constructed(nullptr);

    CHECK_FALSE(default_constructed);
    CHECK(default_constructed.Empty());
    CHECK_EQ(default_constructed.Get(), nullptr);
    CHECK_EQ(default_constructed.RefCount(), 0);
    CHECK_FALSE(nullptr_constructed);
  }

  TEST_CASE("RefCounted::MakeRc uses default resource") {
    TrackingMemoryResource resource;
    {
      DefaultResourceGuard guard(&resource);
      auto rc = MakeRc<Widget>(42);

      static_assert(std::same_as<decltype(rc), Rc<Widget>>);
      static_assert(std::same_as<Rc<Widget>, RefCounted<Widget>>);
      CHECK_EQ(resource.AllocCount(), 1);
      CHECK_EQ(rc.GetMemoryResource(), &resource);
      CHECK_EQ(rc.GetAllocator().resource(), &resource);
      CHECK_EQ(rc->Value(), 42);
      CHECK_EQ((*rc).Value(), 42);
      CHECK_EQ(rc.RefCount(), 1);
      CHECK(rc.Unique());
    }
    CHECK_EQ(resource.DeallocCount(), 1);
  }

  TEST_CASE("RefCounted::copy and move construction propagate resource") {
    TrackingMemoryResource resource;
    auto source = MakeRcWith<Widget>(&resource, 7);
    auto copy = source;
    auto moved = std::move(copy);

    CHECK_EQ(source.RefCount(), 2);
    CHECK_EQ(moved.RefCount(), 2);
    CHECK_EQ(moved.GetMemoryResource(), &resource);
    CHECK_FALSE(copy);  // NOLINT(bugprone-use-after-move)
  }

  TEST_CASE("RefCounted::copy assignment propagates source resource") {
    TrackingMemoryResource old_resource;
    TrackingMemoryResource source_resource;
    auto target = MakeRcWith<Widget>(&old_resource, 1);
    auto source = MakeRcWith<Widget>(&source_resource, 2);

    target = source;

    CHECK_EQ(old_resource.DeallocCount(), 1);
    CHECK_EQ(target.GetMemoryResource(), &source_resource);
    CHECK_EQ(target.Get(), source.Get());
    CHECK_EQ(target.RefCount(), 2);

    source.Reset();
    CHECK_EQ(source_resource.DeallocCount(), 0);
    target.Reset();
    CHECK_EQ(source_resource.DeallocCount(), 1);
  }

  TEST_CASE("RefCounted::move assignment propagates source resource") {
    TrackingMemoryResource old_resource;
    TrackingMemoryResource source_resource;
    auto target = MakeRcWith<Widget>(&old_resource, 1);
    auto source = MakeRcWith<Widget>(&source_resource, 2);

    target = std::move(source);

    CHECK_EQ(old_resource.DeallocCount(), 1);
    CHECK_EQ(target.GetMemoryResource(), &source_resource);
    CHECK_EQ(target->Value(), 2);
    CHECK_FALSE(source);  // NOLINT(bugprone-use-after-move)

    target.Reset();
    CHECK_EQ(source_resource.DeallocCount(), 1);
  }

  TEST_CASE("RefCounted::self assignment is safe") {
    auto rc = MakeRc<Widget>(3);
    auto* ptr = rc.Get();

    rc = rc;

    CHECK_EQ(rc.Get(), ptr);
    CHECK_EQ(rc.RefCount(), 1);
  }

  TEST_CASE("RefCounted::reset and nullptr assignment destroy last owner") {
    TrackingMemoryResource resource;
    int destroy_count = 0;
    auto rc = MakeRcWith<Widget>(&resource, 4);
    rc->SetDestroyCounter(&destroy_count);
    auto copy = rc;

    rc.Reset();
    CHECK_EQ(destroy_count, 0);
    CHECK_EQ(copy.RefCount(), 1);

    copy = nullptr;
    CHECK_EQ(destroy_count, 1);
    CHECK_EQ(resource.DeallocCount(), 1);
  }

  TEST_CASE("RefCounted::release yields the owned pointer") {
    TrackingMemoryResource resource;
    auto rc = MakeRcWith<Widget>(&resource, 5);
    auto allocator = rc.GetAllocator();
    auto* ptr = rc.Release();

    CHECK_FALSE(rc);
    REQUIRE_NE(ptr, nullptr);
    CHECK_EQ(ptr->RefCount(), 1);

    std::allocator_traits<decltype(allocator)>::destroy(allocator, ptr);
    std::allocator_traits<decltype(allocator)>::deallocate(allocator, ptr, 1);
    CHECK_EQ(resource.DeallocCount(), 1);
  }

  TEST_CASE("RefCounted::pointer construction retains its resource") {
    TrackingMemoryResource resource;
    std::pmr::polymorphic_allocator<Widget> allocator(&resource);
    auto* ptr = allocator.allocate(1);
    allocator.construct(ptr, 6);

    Rc<Widget> rc(ptr, &resource);

    CHECK_EQ(rc.Get(), ptr);
    CHECK_EQ(rc.GetMemoryResource(), &resource);
    CHECK_EQ(rc.RefCount(), 1);
  }

  TEST_CASE("RefCounted::identity comparison") {
    auto first = MakeRc<Widget>(1);
    auto copy = first;
    auto second = MakeRc<Widget>(2);
    Rc<Widget> null;

    CHECK_EQ(first, copy);
    CHECK_NE(first, second);
    CHECK_NE(first, null);
    CHECK_EQ(null, nullptr);
  }

  TEST_CASE("RcFromThis::copy and move start unowned") {
    auto rc = MakeRc<Widget>(1);
    Widget copy(*rc);
    Widget moved(std::move(copy));

    CHECK_EQ(copy.RefCount(), 0);
    CHECK_EQ(moved.RefCount(), 0);
  }
}

TEST_SUITE("helios::mem::AtomicRefCounted") {
  TEST_CASE("AtomicRefCounted::null construction") {
    Arc<Texture> default_constructed;
    Arc<Texture> nullptr_constructed(nullptr);

    CHECK_FALSE(default_constructed);
    CHECK(default_constructed.Empty());
    CHECK_EQ(default_constructed.Get(), nullptr);
    CHECK_EQ(default_constructed.RefCount(), 0);
    CHECK_FALSE(nullptr_constructed);
  }

  TEST_CASE("AtomicRefCounted::MakeArc uses default resource") {
    TrackingMemoryResource resource;
    {
      DefaultResourceGuard guard(&resource);
      auto arc = MakeArc<Texture>(42);

      static_assert(std::same_as<decltype(arc), Arc<Texture>>);
      static_assert(std::same_as<Arc<Texture>, AtomicRefCounted<Texture>>);
      CHECK_EQ(resource.AllocCount(), 1);
      CHECK_EQ(arc.GetMemoryResource(), &resource);
      CHECK_EQ(arc.GetAllocator().resource(), &resource);
      CHECK_EQ(arc->Id(), 42);
      CHECK_EQ((*arc).Id(), 42);
      CHECK_EQ(arc.RefCount(), 1);
      CHECK(arc.Unique());
    }
    CHECK_EQ(resource.DeallocCount(), 1);
  }

  TEST_CASE("AtomicRefCounted::copy and move construction propagate resource") {
    TrackingMemoryResource resource;
    auto source = MakeArcWith<Texture>(&resource, 7);
    auto copy = source;
    auto moved = std::move(copy);

    CHECK_EQ(source.RefCount(), 2);
    CHECK_EQ(moved.RefCount(), 2);
    CHECK_EQ(moved.GetMemoryResource(), &resource);
    CHECK_FALSE(copy);  // NOLINT(bugprone-use-after-move)
  }

  TEST_CASE("AtomicRefCounted::copy assignment propagates source resource") {
    TrackingMemoryResource old_resource;
    TrackingMemoryResource source_resource;
    auto target = MakeArcWith<Texture>(&old_resource, 1);
    auto source = MakeArcWith<Texture>(&source_resource, 2);

    target = source;

    CHECK_EQ(old_resource.DeallocCount(), 1);
    CHECK_EQ(target.GetMemoryResource(), &source_resource);
    CHECK_EQ(target.Get(), source.Get());
    CHECK_EQ(target.RefCount(), 2);

    source.Reset();
    CHECK_EQ(source_resource.DeallocCount(), 0);
    target.Reset();
    CHECK_EQ(source_resource.DeallocCount(), 1);
  }

  TEST_CASE("AtomicRefCounted::move assignment propagates source resource") {
    TrackingMemoryResource old_resource;
    TrackingMemoryResource source_resource;
    auto target = MakeArcWith<Texture>(&old_resource, 1);
    auto source = MakeArcWith<Texture>(&source_resource, 2);

    target = std::move(source);

    CHECK_EQ(old_resource.DeallocCount(), 1);
    CHECK_EQ(target.GetMemoryResource(), &source_resource);
    CHECK_EQ(target->Id(), 2);
    CHECK_FALSE(source);  // NOLINT(bugprone-use-after-move)

    target.Reset();
    CHECK_EQ(source_resource.DeallocCount(), 1);
  }

  TEST_CASE("AtomicRefCounted::reset destroys the last owner") {
    TrackingMemoryResource resource;
    std::atomic<int> destroy_count{0};
    auto arc = MakeArcWith<Texture>(&resource, 3);
    arc->SetDestroyCounter(&destroy_count);
    auto copy = arc;

    arc.Reset();
    CHECK_EQ(destroy_count.load(), 0);
    copy = nullptr;
    CHECK_EQ(destroy_count.load(), 1);
    CHECK_EQ(resource.DeallocCount(), 1);
  }

  TEST_CASE("AtomicRefCounted::release yields the owned pointer") {
    TrackingMemoryResource resource;
    auto arc = MakeArcWith<Texture>(&resource, 4);
    auto allocator = arc.GetAllocator();
    auto* ptr = arc.Release();

    CHECK_FALSE(arc);
    REQUIRE_NE(ptr, nullptr);
    CHECK_EQ(ptr->RefCount(), 1);

    std::allocator_traits<decltype(allocator)>::destroy(allocator, ptr);
    std::allocator_traits<decltype(allocator)>::deallocate(allocator, ptr, 1);
    CHECK_EQ(resource.DeallocCount(), 1);
  }

  TEST_CASE("AtomicRefCounted::pointer construction retains its resource") {
    TrackingMemoryResource resource;
    std::pmr::polymorphic_allocator<Texture> allocator(&resource);
    auto* ptr = allocator.allocate(1);
    allocator.construct(ptr, 5);

    Arc<Texture> arc(ptr, &resource);

    CHECK_EQ(arc.Get(), ptr);
    CHECK_EQ(arc.GetMemoryResource(), &resource);
    CHECK_EQ(arc.RefCount(), 1);
  }

  TEST_CASE("AtomicRefCounted::separate handles release concurrently") {
    constexpr int kThreadCount = 8;
    TrackingMemoryResource resource;
    std::atomic<int> destroy_count{0};
    auto source = MakeArcWith<Texture>(&resource, 6);
    source->SetDestroyCounter(&destroy_count);
    std::vector<Arc<Texture>> handles(kThreadCount, source);
    source.Reset();

    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);
    for (int i = 0; i < kThreadCount; ++i) {
      threads.emplace_back(
          [&handles, i]() { handles[static_cast<size_t>(i)].Reset(); });
    }
    for (auto& thread : threads) {
      thread.join();
    }

    CHECK_EQ(destroy_count.load(), 1);
    CHECK_EQ(resource.DeallocCount(), 1);
  }

  TEST_CASE("ArcFromThis::copy and move start unowned") {
    auto arc = MakeArc<Texture>(1);
    Texture copy(*arc);
    Texture moved(std::move(copy));

    CHECK_EQ(copy.RefCount(), 0);
    CHECK_EQ(moved.RefCount(), 0);
  }
}

}  // namespace
