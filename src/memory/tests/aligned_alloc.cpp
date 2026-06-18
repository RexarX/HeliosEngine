#include <doctest/doctest.h>

#include <helios/memory/aligned_alloc.hpp>

#include <cstdint>

using namespace helios::mem;

TEST_SUITE("AlignedAlloc") {
  TEST_CASE("AlignedAlloc") {
    SUBCASE("Returns non-null for valid inputs") {
      void* const ptr = AlignedAlloc(16, 64);
      CHECK_NE(ptr, nullptr);
      AlignedFree(ptr);
    }

    SUBCASE("Returned pointer satisfies alignment of 1") {
      void* const ptr = AlignedAlloc(1, 64);
      CHECK_NE(ptr, nullptr);
      CHECK_EQ(reinterpret_cast<uintptr_t>(ptr) % 1, static_cast<uintptr_t>(0));
      AlignedFree(ptr);
    }

    SUBCASE("Returned pointer satisfies small power-of-two alignments") {
      for (size_t alignment = 2; alignment <= 64; alignment *= 2) {
        void* const ptr = AlignedAlloc(alignment, 64);
        CHECK_NE(ptr, nullptr);
        CHECK_EQ(reinterpret_cast<uintptr_t>(ptr) % alignment,
                 static_cast<uintptr_t>(0));
        AlignedFree(ptr);
      }
    }

    SUBCASE("Returned pointer satisfies large power-of-two alignment") {
      constexpr size_t kAlignment = 4096;
      void* const ptr = AlignedAlloc(kAlignment, 4096);
      CHECK_NE(ptr, nullptr);
      CHECK_EQ(reinterpret_cast<uintptr_t>(ptr) % kAlignment,
               static_cast<uintptr_t>(0));
      AlignedFree(ptr);
    }

    SUBCASE("Size smaller than alignment is rounded up correctly") {
      // On non-MSVC, AlignUp(size, alignment) must produce a multiple of
      // alignment; verify the returned pointer is still valid and aligned.
      constexpr size_t kAlignment = 64;
      constexpr size_t kSize = 1;
      void* const ptr = AlignedAlloc(kAlignment, kSize);
      CHECK_NE(ptr, nullptr);
      CHECK_EQ(reinterpret_cast<uintptr_t>(ptr) % kAlignment,
               static_cast<uintptr_t>(0));
      AlignedFree(ptr);
    }

    SUBCASE("Size equal to alignment") {
      constexpr size_t kAlignment = 32;
      void* const ptr = AlignedAlloc(kAlignment, kAlignment);
      CHECK_NE(ptr, nullptr);
      CHECK_EQ(reinterpret_cast<uintptr_t>(ptr) % kAlignment,
               static_cast<uintptr_t>(0));
      AlignedFree(ptr);
    }

    SUBCASE("Size larger than alignment") {
      constexpr size_t kAlignment = 16;
      constexpr size_t kSize = 1024;
      void* const ptr = AlignedAlloc(kAlignment, kSize);
      CHECK_NE(ptr, nullptr);
      CHECK_EQ(reinterpret_cast<uintptr_t>(ptr) % kAlignment,
               static_cast<uintptr_t>(0));
      AlignedFree(ptr);
    }

    SUBCASE("Size not a multiple of alignment is rounded up correctly") {
      // E.g. alignment=32, size=50 → AlignUp produces 64, which is a valid
      // multiple. The allocation must succeed and the pointer must be aligned.
      constexpr size_t kAlignment = 32;
      constexpr size_t kSize = 50;
      void* const ptr = AlignedAlloc(kAlignment, kSize);
      CHECK_NE(ptr, nullptr);
      CHECK_EQ(reinterpret_cast<uintptr_t>(ptr) % kAlignment,
               static_cast<uintptr_t>(0));
      AlignedFree(ptr);
    }

    SUBCASE("Allocated memory is readable and writable") {
      constexpr size_t kAlignment = 16;
      constexpr size_t kSize = 256;
      auto* const ptr = static_cast<uint8_t*>(AlignedAlloc(kAlignment, kSize));
      CHECK_NE(ptr, nullptr);

      for (size_t i = 0; i < kSize; ++i) {
        ptr[i] = static_cast<uint8_t>(i & 0xFF);
      }
      for (size_t i = 0; i < kSize; ++i) {
        CHECK_EQ(ptr[i], static_cast<uint8_t>(i & 0xFF));
      }

      AlignedFree(ptr);
    }

    SUBCASE("Successive allocations return distinct non-null pointers") {
      constexpr size_t kAlignment = 16;
      constexpr size_t kSize = 64;
      void* const ptr1 = AlignedAlloc(kAlignment, kSize);
      void* const ptr2 = AlignedAlloc(kAlignment, kSize);
      CHECK_NE(ptr1, nullptr);
      CHECK_NE(ptr2, nullptr);
      CHECK_NE(ptr1, ptr2);
      AlignedFree(ptr1);
      AlignedFree(ptr2);
    }
  }
}

TEST_SUITE("AlignedFree") {
  TEST_CASE("AlignedFree") {
    SUBCASE("No-op on nullptr") {
      // Must not crash, assert, or invoke undefined behaviour.
      AlignedFree(nullptr);
    }

    SUBCASE("Frees a valid pointer without crash") {
      void* const ptr = AlignedAlloc(32, 128);
      CHECK_NE(ptr, nullptr);
      AlignedFree(ptr);
    }

    SUBCASE("Allocator remains usable after free") {
      // A fresh allocation after freeing must succeed and be correctly aligned,
      // confirming the allocator is not left in a broken state.
      constexpr size_t kAlignment = 64;
      constexpr size_t kSize = 256;

      void* const ptr1 = AlignedAlloc(kAlignment, kSize);
      CHECK_NE(ptr1, nullptr);
      AlignedFree(ptr1);

      void* const ptr2 = AlignedAlloc(kAlignment, kSize);
      CHECK_NE(ptr2, nullptr);
      CHECK_EQ(reinterpret_cast<uintptr_t>(ptr2) % kAlignment,
               static_cast<uintptr_t>(0));
      AlignedFree(ptr2);
    }
  }
}
