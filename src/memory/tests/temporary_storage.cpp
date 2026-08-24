#include <doctest/doctest.h>

#include <helios/memory/temporary_storage.hpp>

#include <atomic>
#include <cstddef>
#include <cstring>
#include <latch>
#include <memory_resource>
#include <thread>
#include <vector>

using namespace helios::mem;

TEST_SUITE("helios::mem::TemporaryStorage") {
  TEST_CASE("helios::mem::TemporaryStorage::Instance") {
    SUBCASE("Returns the same resource on repeated calls") {
      auto& first = TemporaryStorage::Instance();
      auto& second = TemporaryStorage::Instance();

      CHECK_EQ(&first, &second);
    }

    SUBCASE("Returns a different resource for a different thread") {
      auto* main_resource = &TemporaryStorage::Instance();
      std::pmr::memory_resource* thread_resource = nullptr;

      std::thread thread([&thread_resource]() {
        thread_resource = &TemporaryStorage::Instance();
      });
      thread.join();

      REQUIRE_NE(thread_resource, nullptr);
      CHECK_NE(main_resource, thread_resource);
    }

    SUBCASE("Reclaims a slot after the previous thread exits") {
      std::thread([]() {
        void* ptr = TemporaryStorage::Instance().allocate(64);
        CHECK_NE(ptr, nullptr);
      }).join();

      void* ptr = TemporaryStorage::Instance().allocate(64);
      CHECK_NE(ptr, nullptr);
    }

    SUBCASE("Allocates with the requested alignment") {
      auto& resource = TemporaryStorage::Instance();

      void* ptr1 = resource.allocate(100, 16);
      CHECK_NE(ptr1, nullptr);
      CHECK_EQ(reinterpret_cast<uintptr_t>(ptr1) % 16, 0);

      void* ptr2 = resource.allocate(100, 32);
      CHECK_NE(ptr2, nullptr);
      CHECK_EQ(reinterpret_cast<uintptr_t>(ptr2) % 32, 0);

      void* ptr3 = resource.allocate(100, 64);
      CHECK_NE(ptr3, nullptr);
      CHECK_EQ(reinterpret_cast<uintptr_t>(ptr3) % 64, 0);
    }

    SUBCASE("Allocates zero bytes") {
      void* ptr = TemporaryStorage::Instance().allocate(0);
      CHECK_NE(ptr, nullptr);
    }

    SUBCASE("deallocate is a no-op") {
      auto& resource = TemporaryStorage::Instance();
      void* ptr = resource.allocate(100);
      CHECK_NE(ptr, nullptr);
      resource.deallocate(ptr, 100, alignof(std::max_align_t));
    }

    SUBCASE("is_equal is true for the same thread resource") {
      auto& resource = TemporaryStorage::Instance();
      CHECK(resource.is_equal(resource));
    }

    SUBCASE("is_equal is false for a different thread's resource") {
      auto& main_resource = TemporaryStorage::Instance();
      std::pmr::memory_resource* thread_resource = nullptr;

      std::thread thread([&thread_resource]() {
        thread_resource = &TemporaryStorage::Instance();
      });
      thread.join();

      REQUIRE_NE(thread_resource, nullptr);
      CHECK_FALSE(main_resource.is_equal(*thread_resource));
    }
  }

  TEST_CASE("helios::mem::TemporaryStorage::Reset") {
    SUBCASE("Allocates after reset without error") {
      auto& resource = TemporaryStorage::Instance();

      void* ptr1 = resource.allocate(100);
      CHECK_NE(ptr1, nullptr);
      std::memset(ptr1, 0xAB, 100);

      TemporaryStorage::Reset();

      void* ptr2 = resource.allocate(100);
      CHECK_NE(ptr2, nullptr);
      std::memset(ptr2, 0xCD, 100);
      CHECK_EQ(static_cast<unsigned char*>(ptr2)[0], 0xCD);
      CHECK_EQ(static_cast<unsigned char*>(ptr2)[99], 0xCD);
    }

    SUBCASE("Can handle large allocations before reset") {
      auto& resource = TemporaryStorage::Instance();

      std::vector<void*> allocations;
      for (int i = 0; i < 10; ++i) {
        void* ptr = resource.allocate(TemporaryStorage::kInitialBlockSize / 2);
        CHECK_NE(ptr, nullptr);
        allocations.push_back(ptr);
      }

      TemporaryStorage::Reset();
    }
  }

  TEST_CASE("helios::mem::TemporaryStorage::ResetAll") {
    SUBCASE("Resets all live slots without crashing") {
      auto& main_resource = TemporaryStorage::Instance();

      std::vector<std::thread> threads;
      for (int i = 0; i < 3; ++i) {
        threads.emplace_back([]() {
          void* ptr = TemporaryStorage::Instance().allocate(100);
          CHECK_NE(ptr, nullptr);
        });
      }

      for (auto& thread : threads) {
        thread.join();
      }

      TemporaryStorage::ResetAll();

      void* ptr = main_resource.allocate(100);
      CHECK_NE(ptr, nullptr);
    }

    SUBCASE("ResetAll is safe while other threads exit") {
      auto& main_resource = TemporaryStorage::Instance();

      for (int round = 0; round < 32; ++round) {
        constexpr int kThreadCount = 8;
        std::latch allocated{kThreadCount};
        std::latch may_exit{1};

        std::vector<std::thread> threads;
        threads.reserve(static_cast<size_t>(kThreadCount));
        for (int i = 0; i < kThreadCount; ++i) {
          threads.emplace_back([&allocated, &may_exit]() {
            void* ptr = TemporaryStorage::Instance().allocate(128);
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

      void* ptr = main_resource.allocate(100);
      CHECK_NE(ptr, nullptr);
    }

    SUBCASE("ResetAll racing thread exit does not stick slots") {
      // ResetAll() must not run concurrently with allocate() on a live
      // slot — that is a data race on monotonic_buffer_resource. Race
      // only TLS teardown against ResetAll: workers allocate, then wait,
      // then exit while ResetAll loops.
      constexpr int kRounds = 256;
      constexpr int kThreadCount = 8;

      for (int round = 0; round < kRounds; ++round) {
        std::latch allocated{kThreadCount};
        std::latch may_exit{1};

        std::vector<std::thread> threads;
        threads.reserve(static_cast<size_t>(kThreadCount));
        for (int i = 0; i < kThreadCount; ++i) {
          threads.emplace_back([&allocated, &may_exit]() {
            void* ptr = TemporaryStorage::Instance().allocate(32);
            CHECK_NE(ptr, nullptr);
            allocated.count_down();
            may_exit.wait();
          });
        }

        allocated.wait();

        std::atomic<bool> stop{false};
        std::thread resetter([&stop]() {
          while (!stop.load(std::memory_order_relaxed)) {
            TemporaryStorage::ResetAll();
          }
        });

        may_exit.count_down();
        for (auto& thread : threads) {
          thread.join();
        }
        stop.store(true, std::memory_order_relaxed);
        resetter.join();
      }

      void* ptr = TemporaryStorage::Instance().allocate(32);
      CHECK_NE(ptr, nullptr);
    }
  }
}

TEST_SUITE("helios::mem::GetTemporaryStorage") {
  TEST_CASE("helios::mem::GetTemporaryStorage") {
    SUBCASE("Returns the same resource as TemporaryStorage::Instance") {
      auto& resource = GetTemporaryStorage();
      CHECK_EQ(&resource, &TemporaryStorage::Instance());
    }
  }
}

TEST_SUITE("helios::mem::GetTemporaryAllocator") {
  TEST_CASE("helios::mem::GetTemporaryAllocator") {
    SUBCASE("Allocator uses this thread's temporary resource") {
      auto allocator = GetTemporaryAllocator<int>();
      CHECK_EQ(allocator.resource(), &TemporaryStorage::Instance());
    }

    SUBCASE("Can allocate and construct objects") {
      auto allocator = GetTemporaryAllocator<int>();

      int* ptr = allocator.allocate(1);
      REQUIRE_NE(ptr, nullptr);

      allocator.construct(ptr, 42);
      CHECK_EQ(*ptr, 42);

      allocator.destroy(ptr);
      allocator.deallocate(ptr, 1);
    }
  }
}

TEST_SUITE("helios::mem::ResetTemporaryStorage") {
  TEST_CASE("helios::mem::ResetTemporaryStorage") {
    SUBCASE("Can allocate after reset") {
      auto& resource = TemporaryStorage::Instance();

      void* ptr1 = resource.allocate(100);
      CHECK_NE(ptr1, nullptr);

      ResetTemporaryStorage();

      void* ptr2 = resource.allocate(100);
      CHECK_NE(ptr2, nullptr);
    }
  }
}

TEST_SUITE("helios::mem::ResetAllTemporaryStorage") {
  TEST_CASE("helios::mem::ResetAllTemporaryStorage") {
    SUBCASE("Can allocate after global reset") {
      auto& resource = TemporaryStorage::Instance();

      void* ptr1 = resource.allocate(100);
      CHECK_NE(ptr1, nullptr);

      ResetAllTemporaryStorage();

      void* ptr2 = resource.allocate(100);
      CHECK_NE(ptr2, nullptr);
    }
  }
}
