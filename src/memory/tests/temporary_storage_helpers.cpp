#include <doctest/doctest.h>

#include <helios/memory/temporary_storage.hpp>
#include <helios/memory/temporary_storage_helpers.hpp>

#include <memory_resource>
#include <ostream>
#include <thread>

using namespace helios;

TEST_SUITE("helios::utils::TempFormat") {
  TEST_CASE("helios::utils::TempFormat") {
    SUBCASE("Formats a simple string with arguments") {
      auto result = utils::TempFormat("{} + {} = {}", 1, 2, 3);
      CHECK_EQ(result, "1 + 2 = 3");
    }

    SUBCASE("Formats an empty format string") {
      auto result = utils::TempFormat("");
      CHECK(result.empty());
    }

    SUBCASE("Allocates from the calling thread's TemporaryStorage") {
      auto& storage = mem::TemporaryStorage::Instance();
      auto result = utils::TempFormat("{}", 42);
      CHECK_EQ(result.get_allocator().resource(), &storage);
    }

    SUBCASE("Allocating again after ResetTemporaryStorage works") {
      // TempFormat's result is backed by TemporaryStorage: per
      // TemporaryStorage::Reset()'s documented @warning, it must not
      // outlive a reset of this thread's storage. Confine it to its own
      // scope so it's destroyed before ResetTemporaryStorage() runs,
      // rather than kept alive across the reset (which previously caused
      // a use-after-free in this string's own destructor).
      {
        auto result = utils::TempFormat("{}", 123);
        CHECK_EQ(result, "123");
      }

      mem::ResetTemporaryStorage();

      auto& storage = mem::TemporaryStorage::Instance();
      void* ptr = storage.allocate(100);
      CHECK_NE(ptr, nullptr);
    }

    SUBCASE("Uses a different TemporaryStorage instance per thread") {
      auto result = utils::TempFormat("main-{}", 1);
      CHECK_EQ(result, "main-1");

      std::pmr::memory_resource* thread_resource = nullptr;
      std::thread thread([&thread_resource]() {
        auto thread_result = utils::TempFormat("thread-{}", 2);
        CHECK_EQ(thread_result, "thread-2");
        thread_resource = &mem::GetTemporaryStorage();
      });
      thread.join();

      REQUIRE_NE(thread_resource, nullptr);
      CHECK_NE(thread_resource, &mem::GetTemporaryStorage());
    }
  }
}
