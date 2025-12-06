#include <doctest/doctest.h>

#include <helios/core/memory/allocator_resources.hpp>
#include <helios/core/memory/allocator_traits.hpp>
#include <helios/core/memory/growable_allocator.hpp>
#include <helios/core/memory/stl_allocator_adapter.hpp>

#include <vector>

namespace {

using namespace helios::app;
using namespace helios::memory;

TEST_SUITE("memory::AllocatorResources") {
  TEST_CASE("FrameAllocatorResource") {
    SUBCASE("Default construction") {
      FrameAllocatorResource resource;

      auto& allocator = resource.Get();
      CHECK(allocator.Empty());
      CHECK(allocator.Capacity() > 0);
    }

    SUBCASE("Custom capacity") {
      constexpr size_t capacity = 1024 * 1024;
      FrameAllocatorResource resource{capacity};

      CHECK_EQ(resource.Capacity(), capacity);
    }

    SUBCASE("Allocate and reset") {
      FrameAllocatorResource resource{8192};

      auto& allocator = resource.Get();
      const auto result = allocator.Allocate(1024, 64);

      CHECK(result.Valid());
      CHECK_FALSE(resource.Empty());

      resource.Reset();
      CHECK(resource.Empty());
    }

    SUBCASE("Statistics") {
      FrameAllocatorResource resource{16384};

      auto& allocator = resource.Get();
      const auto result1 = allocator.Allocate(1024, 64);
      const auto result2 = allocator.Allocate(2048, 64);
      CHECK(result1.Valid());
      CHECK(result2.Valid());

      const auto stats = resource.Stats();
      CHECK_EQ(stats.total_allocations, 2);
      CHECK_GT(stats.total_allocated, 0);
    }

    SUBCASE("Resource name") {
      CHECK(FrameAllocatorResource::Name() == "FrameAllocatorResource");
    }

    SUBCASE("Move semantics") {
      FrameAllocatorResource resource1{4096};
      auto& allocator1 = resource1.Get();
      const auto result = allocator1.Allocate(512, 64);
      CHECK(result.Valid());

      FrameAllocatorResource resource2{std::move(resource1)};
      CHECK_FALSE(resource2.Empty());
    }
  }

  TEST_CASE("FreeListAllocatorResource") {
    SUBCASE("Default construction") {
      FreeListAllocatorResource resource;

      auto& allocator = resource.Get();
      CHECK(allocator.Empty());
      CHECK(allocator.Capacity() > 0);
    }

    SUBCASE("Custom capacity") {
      constexpr size_t capacity = 1024 * 1024;
      FreeListAllocatorResource resource{capacity};

      CHECK_EQ(resource.Capacity(), capacity);
    }

    SUBCASE("Allocate and deallocate") {
      FreeListAllocatorResource resource{16384};

      auto& allocator = resource.Get();
      const auto result1 = allocator.Allocate(1024, 64);
      const auto result2 = allocator.Allocate(2048, 64);

      CHECK(result1.Valid());
      CHECK(result2.Valid());

      allocator.Deallocate(result1.ptr);
      allocator.Deallocate(result2.ptr);
    }

    SUBCASE("Reset") {
      FreeListAllocatorResource resource{8192};

      auto& allocator = resource.Get();
      const auto result = allocator.Allocate(1024, 64);
      CHECK(result.Valid());

      CHECK_FALSE(allocator.Empty());

      resource.Reset();
      CHECK(allocator.Empty());
    }

    SUBCASE("Statistics") {
      FreeListAllocatorResource resource{32768};

      auto& allocator = resource.Get();
      const auto result = allocator.Allocate(1024, 64);
      CHECK(result.Valid());

      const auto stats = resource.Stats();
      CHECK_GT(stats.total_allocations, 0);
    }

    SUBCASE("Resource name") {
      CHECK(FreeListAllocatorResource::Name() == "FreeListAllocatorResource");
    }
  }

  TEST_CASE("PoolAllocatorResource") {
    SUBCASE("ForType static factory") {
      struct alignas(8) TestStruct {
        int a{};
        int b{};
        int c{};
      };

      auto resource = PoolAllocatorResource::ForType<TestStruct>(100);

      CHECK_GE(resource.BlockSize(), sizeof(TestStruct));
      CHECK_EQ(resource.BlockCount(), 100);
    }

    SUBCASE("Manual construction") {
      PoolAllocatorResource resource{128, 50, 64};

      CHECK_GE(resource.BlockSize(), 128);
      CHECK_EQ(resource.BlockCount(), 50);
    }

    SUBCASE("Allocate and deallocate") {
      auto resource = PoolAllocatorResource::ForType<std::int64_t>(100);

      auto& allocator = resource.Get();
      const auto result1 = allocator.Allocate(allocator.BlockSize());
      const auto result2 = allocator.Allocate(allocator.BlockSize());

      CHECK(result1.Valid());
      CHECK(result2.Valid());

      allocator.Deallocate(result1.ptr);
      allocator.Deallocate(result2.ptr);
    }

    SUBCASE("Reset") {
      auto resource = PoolAllocatorResource::ForType<std::int64_t>(10);

      auto& allocator = resource.Get();
      const auto result = allocator.Allocate(allocator.BlockSize());
      CHECK(result.Valid());

      CHECK_FALSE(allocator.Empty());

      resource.Reset();
      CHECK(allocator.Empty());
    }

    SUBCASE("Statistics") {
      auto resource = PoolAllocatorResource::ForType<std::int64_t>(20);

      auto& allocator = resource.Get();
      const auto result1 = allocator.Allocate(allocator.BlockSize());
      const auto result2 = allocator.Allocate(allocator.BlockSize());
      CHECK(result1.Valid());
      CHECK(result2.Valid());

      const auto stats = resource.Stats();
      CHECK_EQ(stats.total_allocations, 2);
      CHECK_EQ(stats.allocation_count, 2);
    }

    SUBCASE("Resource name") {
      CHECK(PoolAllocatorResource::Name() == "PoolAllocatorResource");
    }
  }

  TEST_CASE("StackAllocatorResource") {
    SUBCASE("Default construction") {
      StackAllocatorResource resource;

      auto& allocator = resource.Get();
      CHECK(allocator.Empty());
      CHECK(allocator.Capacity() > 0);
    }

    SUBCASE("Custom capacity") {
      constexpr size_t capacity = 1024 * 1024;
      StackAllocatorResource resource{capacity};

      CHECK_EQ(resource.Capacity(), capacity);
    }

    SUBCASE("LIFO allocation and deallocation") {
      StackAllocatorResource resource{8192};

      auto& allocator = resource.Get();
      const auto result1 = allocator.Allocate(1024, 64);
      const auto result2 = allocator.Allocate(512, 64);

      CHECK(result1.Valid());
      CHECK(result2.Valid());

      // LIFO order
      allocator.Deallocate(result2.ptr, result2.allocated_size);
      allocator.Deallocate(result1.ptr, result1.allocated_size);

      CHECK(allocator.Empty());
    }

    SUBCASE("Reset") {
      StackAllocatorResource resource{4096};

      auto& allocator = resource.Get();
      const auto result = allocator.Allocate(512, 64);
      CHECK(result.Valid());

      CHECK_FALSE(allocator.Empty());

      resource.Reset();
      CHECK(allocator.Empty());
    }

    SUBCASE("Statistics") {
      StackAllocatorResource resource{16384};

      auto& allocator = resource.Get();
      const auto result = allocator.Allocate(1024, 64);
      CHECK(result.Valid());

      const auto stats = resource.Stats();
      CHECK_GT(stats.total_allocations, 0);
    }

    SUBCASE("Resource name") {
      CHECK(StackAllocatorResource::Name() == "StackAllocatorResource");
    }
  }

  TEST_CASE("Integration patterns") {
    SUBCASE("Per-frame allocation pattern with FrameAllocatorResource") {
      FrameAllocatorResource resource{32768};

      // Simulate multiple frames
      for (int frame = 0; frame < 5; ++frame) {
        auto& allocator = resource.Get();

        STLFrameAllocator<int> stl_alloc{allocator};
        std::vector<int, STLFrameAllocator<int>> temp_data{stl_alloc};

        for (int i = 0; i < 100; ++i) {
          temp_data.push_back(i * frame);
        }

        CHECK_EQ(temp_data.size(), 100);
        CHECK_FALSE(resource.Empty());

        // End of frame
        resource.Reset();
        CHECK(resource.Empty());
      }
    }

    SUBCASE("Mixed allocator usage") {
      FrameAllocatorResource local_frame{16384};
      FreeListAllocatorResource freelist{32768};

      // Use local frame allocator
      {
        auto& allocator = local_frame.Get();
        const auto result = allocator.Allocate(512, 64);
        CHECK(result.Valid());
        CHECK_FALSE(local_frame.Empty());
      }

      // Use freelist allocator
      {
        auto& allocator = freelist.Get();
        const auto result = allocator.Allocate(2048, 64);
        CHECK(result.Valid());
        allocator.Deallocate(result.ptr);
      }

      // Reset frame allocator
      local_frame.Reset();

      CHECK(local_frame.Empty());
    }
  }

  TEST_CASE("Free function Allocate<T>: templated allocation helpers") {
    SUBCASE("Allocate single object") {
      FrameAllocatorResource resource{4096};
      auto& allocator = resource.Get();

      int* ptr = Allocate<int>(allocator);
      CHECK_NE(ptr, nullptr);
      CHECK(IsAligned(ptr, alignof(int)));

      new (ptr) int(42);
      CHECK_EQ(*ptr, 42);
    }

    SUBCASE("Allocate array") {
      FrameAllocatorResource resource{4096};
      auto& allocator = resource.Get();

      constexpr size_t count = 10;
      double* ptr = Allocate<double>(allocator, count);
      CHECK_NE(ptr, nullptr);
      CHECK(IsAligned(ptr, alignof(double)));

      for (size_t i = 0; i < count; ++i) {
        new (&ptr[i]) double(static_cast<double>(i) * 1.5);
      }

      for (size_t i = 0; i < count; ++i) {
        CHECK_EQ(ptr[i], doctest::Approx(static_cast<double>(i) * 1.5));
      }
    }

    SUBCASE("Allocate zero count returns nullptr") {
      FrameAllocatorResource resource{4096};
      auto& allocator = resource.Get();

      int* ptr = Allocate<int>(allocator, 0);
      CHECK_EQ(ptr, nullptr);
    }
  }

  TEST_CASE("Free function AllocateAndConstruct: allocation with construction") {
    SUBCASE("Construct int with value") {
      FrameAllocatorResource resource{4096};
      auto& allocator = resource.Get();

      int* ptr = AllocateAndConstruct<int>(allocator, 123);
      CHECK_NE(ptr, nullptr);
      CHECK_EQ(*ptr, 123);
    }

    SUBCASE("Construct struct with multiple args") {
      struct Vec3 {
        float x;
        float y;
        float z;

        Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
      };

      FrameAllocatorResource resource{4096};
      auto& allocator = resource.Get();

      Vec3* ptr = AllocateAndConstruct<Vec3>(allocator, 1.0F, 2.0F, 3.0F);
      CHECK_NE(ptr, nullptr);
      CHECK_EQ(ptr->x, doctest::Approx(1.0F));
      CHECK_EQ(ptr->y, doctest::Approx(2.0F));
      CHECK_EQ(ptr->z, doctest::Approx(3.0F));
    }

    SUBCASE("Construct with default constructor") {
      struct DefaultValue {
        int value = 999;
      };

      FrameAllocatorResource resource{4096};
      auto& allocator = resource.Get();

      DefaultValue* ptr = AllocateAndConstruct<DefaultValue>(allocator);
      CHECK_NE(ptr, nullptr);
      CHECK_EQ(ptr->value, 999);
    }
  }

  TEST_CASE("Free function AllocateAndConstructArray: array construction") {
    SUBCASE("Construct array of default-initialized ints") {
      FrameAllocatorResource resource{4096};
      auto& allocator = resource.Get();

      constexpr size_t count = 10;
      int* ptr = AllocateAndConstructArray<int>(allocator, count);
      CHECK_NE(ptr, nullptr);

      for (size_t i = 0; i < count; ++i) {
        CHECK_EQ(ptr[i], 0);
      }
    }

    SUBCASE("Construct array of structs with default values") {
      struct Item {
        int id = 77;
      };

      FrameAllocatorResource resource{4096};
      auto& allocator = resource.Get();

      constexpr size_t count = 5;
      Item* ptr = AllocateAndConstructArray<Item>(allocator, count);
      CHECK_NE(ptr, nullptr);

      for (size_t i = 0; i < count; ++i) {
        CHECK_EQ(ptr[i].id, 77);
      }
    }

    SUBCASE("Zero count returns nullptr") {
      FrameAllocatorResource resource{4096};
      auto& allocator = resource.Get();

      int* ptr = AllocateAndConstructArray<int>(allocator, 0);
      CHECK_EQ(ptr, nullptr);
    }
  }

  TEST_CASE("Templated allocate with different allocator types") {
    SUBCASE("With StackAllocator") {
      StackAllocatorResource resource{4096};
      auto& allocator = resource.Get();

      int* ptr = Allocate<int>(allocator);
      CHECK_NE(ptr, nullptr);

      new (ptr) int(42);
      CHECK_EQ(*ptr, 42);
    }

    SUBCASE("With FreeListAllocator") {
      FreeListAllocatorResource resource{4096};
      auto& allocator = resource.Get();

      double* ptr = AllocateAndConstruct<double>(allocator, 3.14159);
      CHECK_NE(ptr, nullptr);
      CHECK_EQ(*ptr, doctest::Approx(3.14159));

      allocator.Deallocate(ptr);
    }

    SUBCASE("With GrowableAllocator") {
      GrowableAllocator<FrameAllocator> allocator{1024};

      constexpr size_t count = 20;
      int* ptr = AllocateAndConstructArray<int>(allocator, count);
      CHECK_NE(ptr, nullptr);

      for (size_t i = 0; i < count; ++i) {
        CHECK_EQ(ptr[i], 0);
      }
    }
  }
}

}  // namespace
