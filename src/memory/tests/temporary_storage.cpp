#include <doctest/doctest.h>

#include <helios/memory/temporary_storage.hpp>

#include <cstddef>
#include <cstring>
#include <latch>
#include <thread>
#include <vector>

using namespace helios::mem;

TEST_SUITE("helios::mem::TemporaryStorage") {
  TEST_CASE("helios::mem::TemporaryStorage::Instance") {
    SUBCASE("Returns same instance on repeated calls") {
      auto& first = TemporaryStorage::Instance();
      auto& second = TemporaryStorage::Instance();

      CHECK_EQ(&first, &second);
    }

    SUBCASE("Returns different instances for different threads") {
      auto* main_instance = &TemporaryStorage::Instance();
      TemporaryStorage* thread_instance = nullptr;

      std::thread thread([&thread_instance]() {
        thread_instance = &TemporaryStorage::Instance();
      });
      thread.join();

      REQUIRE_NE(thread_instance, nullptr);
      CHECK_NE(main_instance, thread_instance);
    }
  }

  TEST_CASE("helios::mem::TemporaryStorage::Reset") {
    SUBCASE("Allocates after reset without error") {
      auto& storage = TemporaryStorage::Instance();

      void* ptr1 = storage.allocate(100);
      CHECK_NE(ptr1, nullptr);
      std::memset(ptr1, 0xAB, 100);

      storage.Reset();

      void* ptr2 = storage.allocate(100);
      CHECK_NE(ptr2, nullptr);
      std::memset(ptr2, 0xCD, 100);
      CHECK_EQ(static_cast<unsigned char*>(ptr2)[0], 0xCD);
      CHECK_EQ(static_cast<unsigned char*>(ptr2)[99], 0xCD);
    }

    SUBCASE("Can handle large allocations before reset") {
      auto& storage = TemporaryStorage::Instance();

      // Allocate more than initial block size to force multiple blocks
      std::vector<void*> allocations;
      for (int i = 0; i < 10; ++i) {
        void* ptr = storage.allocate(TemporaryStorage::kInitialBlockSize / 2);
        CHECK_NE(ptr, nullptr);
        allocations.push_back(ptr);
      }

      // Should not crash
      storage.Reset();
    }
  }

  TEST_CASE("helios::mem::TemporaryStorage::ResetAll") {
    SUBCASE("Resets all registered instances without crashing") {
      // Ensure main thread instance exists
      auto& main_storage = TemporaryStorage::Instance();

      // Create instances in other threads
      std::vector<std::thread> threads;
      for (int i = 0; i < 3; ++i) {
        threads.emplace_back([]() {
          auto& storage = TemporaryStorage::Instance();
          // Allocate something to ensure the resource is used
          void* ptr = storage.allocate(100);
          CHECK_NE(ptr, nullptr);
        });
      }

      for (auto& thread : threads) {
        thread.join();
      }

      // Should reset all without crashing
      TemporaryStorage::ResetAll();

      // Main thread should still be able to allocate
      void* ptr = main_storage.allocate(100);
      CHECK_NE(ptr, nullptr);
    }

    SUBCASE("ResetAll is safe while other threads exit") {
      auto& main_storage = TemporaryStorage::Instance();

      for (int round = 0; round < 32; ++round) {
        constexpr int kThreadCount = 8;
        std::latch allocated{kThreadCount};
        std::latch may_exit{1};

        std::vector<std::thread> threads;
        threads.reserve(static_cast<size_t>(kThreadCount));
        for (int i = 0; i < kThreadCount; ++i) {
          threads.emplace_back([&allocated, &may_exit]() {
            auto& storage = TemporaryStorage::Instance();
            void* ptr = storage.allocate(128);
            CHECK_NE(ptr, nullptr);
            allocated.count_down();
            may_exit.wait();
          });
        }

        allocated.wait();
        TemporaryStorage::ResetAll();

        may_exit.count_down();
        TemporaryStorage::ResetAll();

        for (auto& thread : threads) {
          thread.join();
        }

        TemporaryStorage::ResetAll();
      }

      void* ptr = main_storage.allocate(100);
      CHECK_NE(ptr, nullptr);
    }
  }

  TEST_CASE("helios::mem::TemporaryStorage::Resource") {
    SUBCASE("Returns valid memory resource") {
      auto& storage = TemporaryStorage::Instance();
      auto& resource = storage.Resource();

      // Should be able to allocate through the resource
      void* ptr = resource.allocate(100);
      CHECK_NE(ptr, nullptr);
      resource.deallocate(ptr, 100, alignof(std::max_align_t));
    }
  }

  TEST_CASE("helios::mem::TemporaryStorage::allocate") {
    SUBCASE("Allocates with proper alignment") {
      auto& storage = TemporaryStorage::Instance();

      // Test various alignments
      void* ptr1 = storage.allocate(100, 16);
      CHECK_NE(ptr1, nullptr);
      CHECK_EQ(reinterpret_cast<uintptr_t>(ptr1) % 16, 0);

      void* ptr2 = storage.allocate(100, 32);
      CHECK_NE(ptr2, nullptr);
      CHECK_EQ(reinterpret_cast<uintptr_t>(ptr2) % 32, 0);

      void* ptr3 = storage.allocate(100, 64);
      CHECK_NE(ptr3, nullptr);
      CHECK_EQ(reinterpret_cast<uintptr_t>(ptr3) % 64, 0);
    }

    SUBCASE("Allocates zero bytes") {
      auto& storage = TemporaryStorage::Instance();
      void* ptr = storage.allocate(0);
      CHECK_NE(ptr, nullptr);
    }
  }

  TEST_CASE("helios::mem::TemporaryStorage::deallocate") {
    SUBCASE("Deallocate is no-op but doesn't crash") {
      auto& storage = TemporaryStorage::Instance();
      void* ptr = storage.allocate(100);
      CHECK_NE(ptr, nullptr);

      // Should not crash
      storage.deallocate(ptr, 100, alignof(std::max_align_t));
    }
  }

  TEST_CASE("helios::mem::TemporaryStorage::is_equal") {
    SUBCASE("Returns true for same instance") {
      auto& storage = TemporaryStorage::Instance();
      CHECK(storage.is_equal(storage));
    }

    SUBCASE("Returns false for different instances") {
      auto& storage1 = TemporaryStorage::Instance();

      TemporaryStorage* storage2 = nullptr;
      std::thread thread(
          [&storage2]() { storage2 = &TemporaryStorage::Instance(); });
      thread.join();

      REQUIRE_NE(storage2, nullptr);
      CHECK_FALSE(storage1.is_equal(*storage2));
    }
  }
}

TEST_SUITE("helios::mem::GetTemporaryStorage") {
  TEST_CASE("helios::mem::GetTemporaryStorage") {
    SUBCASE("Returns same instance as TemporaryStorage::Instance") {
      auto& storage = GetTemporaryStorage();
      CHECK_EQ(&storage, &TemporaryStorage::Instance());
    }
  }
}

TEST_SUITE("helios::mem::GetTemporaryAllocator") {
  TEST_CASE("helios::mem::GetTemporaryAllocator") {
    SUBCASE("Allocator uses temporary storage resource") {
      auto allocator = GetTemporaryAllocator<int>();
      auto& storage = TemporaryStorage::Instance();

      CHECK_EQ(allocator.resource(), &storage);
    }

    SUBCASE("Can allocate and construct objects") {
      auto allocator = GetTemporaryAllocator<int>();

      int* ptr = allocator.allocate(1);
      REQUIRE_NE(ptr, nullptr);

      allocator.construct(ptr, 42);
      CHECK_EQ(*ptr, 42);

      allocator.destroy(ptr);
      // No need to deallocate with monotonic resource, but should not crash
      allocator.deallocate(ptr, 1);
    }
  }
}

TEST_SUITE("helios::mem::ResetTemporaryStorage") {
  TEST_CASE("helios::mem::ResetTemporaryStorage") {
    SUBCASE("Can allocate after reset") {
      auto& storage = TemporaryStorage::Instance();

      void* ptr1 = storage.allocate(100);
      CHECK_NE(ptr1, nullptr);

      ResetTemporaryStorage();

      void* ptr2 = storage.allocate(100);
      CHECK_NE(ptr2, nullptr);
    }
  }
}

TEST_SUITE("helios::mem::ResetAllTemporaryStorage") {
  TEST_CASE("helios::mem::ResetAllTemporaryStorage") {
    SUBCASE("Can allocate after global reset") {
      auto& storage = TemporaryStorage::Instance();

      void* ptr1 = storage.allocate(100);
      CHECK_NE(ptr1, nullptr);

      ResetAllTemporaryStorage();

      void* ptr2 = storage.allocate(100);
      CHECK_NE(ptr2, nullptr);
    }
  }
}
