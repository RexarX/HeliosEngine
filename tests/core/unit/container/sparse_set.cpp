#include <doctest/doctest.h>

#include <helios/core/container/sparse_set.hpp>

#include <random>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

struct TestValue {
  int x = 0;
  float y = 0;

  constexpr bool operator==(const TestValue& other) const noexcept = default;
};

}  // namespace

TEST_SUITE("container::SparseSet") {
  TEST_CASE("SparseSet::ctor: default construction") {
    helios::container::SparseSet<int> set;
    const auto& const_set = set;

    CHECK(const_set.Empty());
    CHECK_EQ(const_set.Size(), 0);
    CHECK_EQ(const_set.Capacity(), 0);
    CHECK_EQ(const_set.SparseCapacity(), 0);
    CHECK_EQ(const_set.begin(), const_set.end());
    CHECK_EQ(const_set.cbegin(), const_set.cend());
    CHECK_EQ(const_set.rbegin(), const_set.rend());
    CHECK_EQ(const_set.crbegin(), const_set.crend());
  }

  TEST_CASE("SparseSet::ctor: allocator construction") {
    std::allocator<int> alloc;
    helios::container::SparseSet<int> set(alloc);

    CHECK(set.Empty());
    CHECK_EQ(set.Size(), 0);
    CHECK(set.GetAllocator() == alloc);
  }

  TEST_CASE("SparseSet::ctor: copy semantics") {
    helios::container::SparseSet<int> original;

    // Insert some values
    original.Insert(2, 100);
    original.Insert(5, 200);
    original.Insert(10, 300);

    // Copy construct
    helios::container::SparseSet<int> copy(original);

    CHECK_EQ(copy.Size(), 3);
    CHECK(copy.Contains(2));
    CHECK(copy.Contains(5));
    CHECK(copy.Contains(10));
    CHECK_EQ(copy.Get(2), 100);
    CHECK_EQ(copy.Get(5), 200);
    CHECK_EQ(copy.Get(10), 300);

    // Original should still be intact
    CHECK_EQ(original.Size(), 3);
    CHECK_EQ(original.Get(2), 100);

    // Copy assignment
    helios::container::SparseSet<int> assigned;
    assigned.Insert(99, 999);
    assigned = original;

    CHECK_EQ(assigned.Size(), 3);
    CHECK(assigned.Contains(2));
    CHECK_FALSE(assigned.Contains(99));
    CHECK_EQ(assigned.Get(2), 100);
  }

  TEST_CASE("SparseSet::ctor: move semantics") {
    helios::container::SparseSet<int> first_set;

    // Insert some values
    const auto dense_idx_0 = first_set.Insert(2, 100);
    const auto dense_idx_1 = first_set.Insert(5, 200);
    const auto dense_idx_2 = first_set.Insert(10, 300);

    CHECK_EQ(dense_idx_0, 0);
    CHECK_EQ(dense_idx_1, 1);
    CHECK_EQ(dense_idx_2, 2);
    CHECK_EQ(first_set.Size(), 3);
    CHECK(first_set.Contains(2));
    CHECK(first_set.Contains(5));
    CHECK(first_set.Contains(10));
    CHECK_EQ(first_set.Get(2), 100);
    CHECK_EQ(first_set.Get(5), 200);
    CHECK_EQ(first_set.Get(10), 300);

    // Move construct
    helios::container::SparseSet<int> second_set(std::move(first_set));

    // first_set should be empty after move
    CHECK(first_set.Empty());
    CHECK_EQ(first_set.Size(), 0);
    CHECK_FALSE(first_set.Contains(2));
    CHECK_FALSE(first_set.Contains(5));
    CHECK_FALSE(first_set.Contains(10));

    // second_set should have all elements
    CHECK_EQ(second_set.Size(), 3);
    CHECK(second_set.Contains(2));
    CHECK(second_set.Contains(5));
    CHECK(second_set.Contains(10));
    CHECK_EQ(second_set.Get(2), 100);
    CHECK_EQ(second_set.Get(5), 200);
    CHECK_EQ(second_set.Get(10), 300);

    // Test move assignment
    helios::container::SparseSet<int> third_set;
    third_set.Insert(100, 1000);
    CHECK_EQ(third_set.Size(), 1);

    third_set = std::move(second_set);

    // second_set should be empty after move
    CHECK(second_set.Empty());
    CHECK_EQ(second_set.Size(), 0);

    // third_set should have the moved elements
    CHECK_EQ(third_set.Size(), 3);
    CHECK(third_set.Contains(2));
    CHECK(third_set.Contains(5));
    CHECK(third_set.Contains(10));
    CHECK_FALSE(third_set.Contains(100));  // Old element should be gone
  }

  TEST_CASE("SparseSet::Insert: basic insert and contains") {
    helios::container::SparseSet<int> set;

    // Test basic insertion
    const auto dense_idx = set.Insert(42, 420);
    CHECK_EQ(dense_idx, 0);
    CHECK_EQ(set.Size(), 1);
    CHECK(set.Contains(42));
    CHECK_EQ(set.Get(42), 420);
    CHECK_FALSE(set.Empty());

    // Test multiple insertions
    set.Insert(0, 10);
    set.Insert(1000, 2000);
    set.Insert(5, 50);

    CHECK_EQ(set.Size(), 4);
    CHECK(set.Contains(0));
    CHECK(set.Contains(5));
    CHECK(set.Contains(42));
    CHECK(set.Contains(1000));
    CHECK_EQ(set.Get(0), 10);
    CHECK_EQ(set.Get(5), 50);
    CHECK_EQ(set.Get(42), 420);
    CHECK_EQ(set.Get(1000), 2000);
    CHECK_FALSE(set.Contains(999));
    CHECK_FALSE(set.Contains(43));
  }

  TEST_CASE("SparseSet::Insert: duplicate insertion") {
    helios::container::SparseSet<int> set;

    // Insert element first time
    const auto dense_idx_1 = set.Insert(42, 100);
    CHECK_EQ(dense_idx_1, 0);
    CHECK_EQ(set.Size(), 1);
    CHECK(set.Contains(42));
    CHECK_EQ(set.Get(42), 100);

    // Insert same element again - should replace the value
    const auto dense_idx_2 = set.Insert(42, 200);
    CHECK_EQ(dense_idx_2, 0);  // Should return the same dense index
    CHECK_EQ(set.Size(), 1);   // Size should not change
    CHECK(set.Contains(42));
    CHECK_EQ(set.Get(42), 200);  // Value should be updated

    // Insert other elements
    set.Insert(10, 300);
    set.Insert(20, 400);
    CHECK_EQ(set.Size(), 3);

    // Try duplicate insertion again
    const auto dense_idx_3 = set.Insert(10, 500);
    CHECK_EQ(dense_idx_3, 1);    // Dense index of element 10
    CHECK_EQ(set.Size(), 3);     // Size should remain same
    CHECK_EQ(set.Get(10), 500);  // Value should be updated

    // Verify all elements are still there
    CHECK(set.Contains(42));
    CHECK(set.Contains(10));
    CHECK(set.Contains(20));
    CHECK_EQ(set.Get(42), 200);
    CHECK_EQ(set.Get(10), 500);
    CHECK_EQ(set.Get(20), 400);
  }

  TEST_CASE("SparseSet::Insert: large sparse indices") {
    helios::container::SparseSet<int> set;

    // Insert indices with large gaps
    set.Insert(0, 100);
    set.Insert(1000000, 200);
    set.Insert(5, 300);

    CHECK_EQ(set.Size(), 3);
    CHECK(set.Contains(0));
    CHECK(set.Contains(5));
    CHECK(set.Contains(1000000));
    CHECK_EQ(set.Get(0), 100);
    CHECK_EQ(set.Get(5), 300);
    CHECK_EQ(set.Get(1000000), 200);

    // Sparse array should be large enough
    CHECK_GE(set.SparseCapacity(), 1000001);

    // But dense array should only contain 3 elements
    CHECK_EQ(set.Size(), 3);
  }

  TEST_CASE("SparseSet::Remove: basic removal") {
    helios::container::SparseSet<int> set;

    // Insert several elements
    set.Insert(10, 100);
    set.Insert(20, 200);
    set.Insert(30, 300);
    set.Insert(40, 400);
    set.Insert(50, 500);

    CHECK_EQ(set.Size(), 5);

    // Remove middle element
    set.Remove(30);
    CHECK_EQ(set.Size(), 4);
    CHECK_FALSE(set.Contains(30));
    CHECK(set.Contains(10));
    CHECK(set.Contains(20));
    CHECK(set.Contains(40));
    CHECK(set.Contains(50));

    // Remove first element
    set.Remove(10);
    CHECK_EQ(set.Size(), 3);
    CHECK_FALSE(set.Contains(10));

    // Remove last element (in dense array)
    set.Remove(50);
    CHECK_EQ(set.Size(), 2);
    CHECK_FALSE(set.Contains(50));

    // Remaining elements should still be there
    CHECK(set.Contains(20));
    CHECK(set.Contains(40));
    CHECK_EQ(set.Get(20), 200);
    CHECK_EQ(set.Get(40), 400);
  }

  TEST_CASE("SparseSet::GetDenseIndex: returns correct dense index") {
    helios::container::SparseSet<int> set;

    // Insert elements in non-sequential order
    const auto dense_idx_a = set.Insert(100, 1000);
    const auto dense_idx_b = set.Insert(5, 50);
    const auto dense_idx_c = set.Insert(1000, 10000);

    CHECK_EQ(dense_idx_a, 0);
    CHECK_EQ(dense_idx_b, 1);
    CHECK_EQ(dense_idx_c, 2);

    // Test dense index lookups
    CHECK_EQ(set.GetDenseIndex(100), 0);
    CHECK_EQ(set.GetDenseIndex(5), 1);
    CHECK_EQ(set.GetDenseIndex(1000), 2);

    // Test access by dense index
    CHECK_EQ(set.GetByDenseIndex(0), 1000);
    CHECK_EQ(set.GetByDenseIndex(1), 50);
    CHECK_EQ(set.GetByDenseIndex(2), 10000);

    // Remove middle element and test gap filling
    set.Remove(5);  // This should move the last element (1000 -> 10000) to position 1

    CHECK_EQ(set.Size(), 2);
    CHECK_EQ(set.GetDenseIndex(100), 0);
    CHECK_EQ(set.GetDenseIndex(1000), 1);  // Should now be at position 1

    CHECK_EQ(set.GetByDenseIndex(0), 1000);
    CHECK_EQ(set.GetByDenseIndex(1), 10000);  // Last element moved here
  }

  TEST_CASE("SparseSet::Insert: move semantics") {
    helios::container::SparseSet<std::string> set;

    std::string value1 = "Hello";
    std::string value2 = "World";

    const auto dense_idx1 = set.Insert(10, std::move(value1));
    const auto dense_idx2 = set.Insert(20, std::move(value2));

    CHECK_EQ(dense_idx1, 0);
    CHECK_EQ(dense_idx2, 1);
    CHECK_EQ(set.Size(), 2);
    CHECK(set.Contains(10));
    CHECK(set.Contains(20));
    CHECK_EQ(set.Get(10), "Hello");
    CHECK_EQ(set.Get(20), "World");

    // Original strings should be moved from (implementation detail)
    CHECK(value1.empty());
    CHECK(value2.empty());
  }

  TEST_CASE("SparseSet::Emplace: constructs in place") {
    helios::container::SparseSet<TestValue> set;

    const auto dense_idx1 = set.Emplace(10, 42, 3.14f);
    const auto dense_idx2 = set.Emplace(20, 84, 2.71f);

    CHECK_EQ(dense_idx1, 0);
    CHECK_EQ(dense_idx2, 1);
    CHECK_EQ(set.Size(), 2);
    CHECK(set.Contains(10));
    CHECK(set.Contains(20));

    const auto& val1 = set.Get(10);
    const auto& val2 = set.Get(20);

    CHECK_EQ(val1.x, 42);
    CHECK_EQ(val1.y, 3.14f);
    CHECK_EQ(val2.x, 84);
    CHECK_EQ(val2.y, 2.71f);
  }

  TEST_CASE("SparseSet::begin: forward and reverse iteration") {
    helios::container::SparseSet<int> set;

    set.Insert(10, 100);
    set.Insert(5, 50);
    set.Insert(15, 150);

    // Test forward iteration
    std::vector<int> values;
    for (const auto& value : set) {
      values.push_back(value);
    }

    // Values should be in insertion order
    CHECK_EQ(values.size(), 3);
    CHECK_EQ(values[0], 100);
    CHECK_EQ(values[1], 50);
    CHECK_EQ(values[2], 150);

    // Test reverse iteration
    values.clear();
    for (auto it = set.rbegin(); it != set.rend(); ++it) {
      values.push_back(*it);
    }

    CHECK_EQ(values.size(), 3);
    CHECK_EQ(values[0], 150);
    CHECK_EQ(values[1], 50);
    CHECK_EQ(values[2], 100);
  }

  TEST_CASE("SparseSet::Data: returns span to dense array") {
    helios::container::SparseSet<int> set;

    set.Insert(10, 100);
    set.Insert(20, 200);
    set.Insert(30, 300);

    auto data_span = set.Data();
    CHECK_EQ(data_span.size(), 3);
    CHECK_EQ(data_span[0], 100);
    CHECK_EQ(data_span[1], 200);
    CHECK_EQ(data_span[2], 300);

    // Test const version
    const auto& const_set = set;
    auto const_data_span = const_set.Data();
    CHECK_EQ(const_data_span.size(), 3);
    CHECK_EQ(const_data_span[0], 100);

    // Modify through non-const span
    data_span[0] = 999;
    CHECK_EQ(set.Get(10), 999);
  }

  TEST_CASE("SparseSet::operator==: equality comparison") {
    helios::container::SparseSet<int> set1, set2;

    // Empty sets should be equal
    CHECK_EQ(set1, set2);

    // Add same elements in same order
    set1.Insert(10, 100);
    set1.Insert(20, 200);
    set2.Insert(10, 100);
    set2.Insert(20, 200);
    CHECK_EQ(set1, set2);

    // Add same elements in different order
    helios::container::SparseSet<int> set3;
    set3.Insert(20, 200);
    set3.Insert(10, 100);
    CHECK_EQ(set1, set3);

    // Different values should not be equal
    helios::container::SparseSet<int> set4;
    set4.Insert(10, 999);
    set4.Insert(20, 200);
    CHECK_FALSE(set1 == set4);

    // Different sizes should not be equal
    set1.Insert(30, 300);
    CHECK_FALSE(set1 == set2);
  }

  TEST_CASE("SparseSet::Reserve: reserves capacity") {
    helios::container::SparseSet<int> set;

    CHECK_EQ(set.Capacity(), 0);
    CHECK_EQ(set.SparseCapacity(), 0);

    // Reserve dense capacity
    set.Reserve(100);
    CHECK_GE(set.Capacity(), 100);

    // Reserve sparse capacity
    set.ReserveSparse(1000);
    CHECK_GE(set.SparseCapacity(), 1001);  // +1 because we need space for index 1000

    // Add some elements
    set.Insert(500, 5000);
    set.Insert(999, 9990);

    CHECK_EQ(set.Size(), 2);
    CHECK(set.Contains(500));
    CHECK(set.Contains(999));
  }

  TEST_CASE("SparseSet::ShrinkToFit: reduces capacity") {
    helios::container::SparseSet<int> set;

    // Insert elements requiring large sparse array
    set.Insert(1000, 1);
    set.Insert(2000, 2);
    set.Insert(3000, 3);

    CHECK_GE(set.SparseCapacity(), 3001);

    // Remove some elements
    set.Remove(2000);
    set.Remove(3000);

    // Shrink to fit - sparse should resize to accommodate max index (1000)
    set.ShrinkToFit();

    CHECK_EQ(set.Size(), 1);
    CHECK(set.Contains(1000));
    CHECK_EQ(set.Get(1000), 1);

    // Sparse capacity should be much smaller now
    CHECK_LT(set.SparseCapacity(), 3001);
    CHECK_GE(set.SparseCapacity(), 1001);
  }

  TEST_CASE("SparseSet::Swap: swaps contents") {
    helios::container::SparseSet<int> set1, set2;

    set1.Insert(10, 100);
    set1.Insert(20, 200);

    set2.Insert(30, 300);
    set2.Insert(40, 400);
    set2.Insert(50, 500);

    set1.Swap(set2);

    // set1 should now have set2's original elements
    CHECK_EQ(set1.Size(), 3);
    CHECK(set1.Contains(30));
    CHECK(set1.Contains(40));
    CHECK(set1.Contains(50));
    CHECK_EQ(set1.Get(30), 300);

    // set2 should now have set1's original elements
    CHECK_EQ(set2.Size(), 2);
    CHECK(set2.Contains(10));
    CHECK(set2.Contains(20));
    CHECK_EQ(set2.Get(10), 100);

    // Test non-member swap
    swap(set1, set2);

    // Should be swapped back
    CHECK_EQ(set1.Size(), 2);
    CHECK(set1.Contains(10));
    CHECK_EQ(set2.Size(), 3);
    CHECK(set2.Contains(30));
  }

  TEST_CASE("SparseSet::Clear: removes all elements") {
    helios::container::SparseSet<int> set;

    set.Insert(10, 100);
    set.Insert(20, 200);
    set.Insert(30, 300);

    CHECK_EQ(set.Size(), 3);
    CHECK_FALSE(set.Empty());

    set.Clear();

    CHECK_EQ(set.Size(), 0);
    CHECK(set.Empty());
    CHECK_FALSE(set.Contains(10));
    CHECK_FALSE(set.Contains(20));
    CHECK_FALSE(set.Contains(30));

    // Should be able to insert again
    set.Insert(40, 400);
    CHECK_EQ(set.Size(), 1);
    CHECK(set.Contains(40));
    CHECK_EQ(set.Get(40), 400);
  }

  TEST_CASE("SparseSet: stress test with random operations") {
    helios::container::SparseSet<int> set;
    std::unordered_set<std::size_t> indices;
    std::mt19937 gen(42);
    std::uniform_int_distribution<std::size_t> dis(0, 10000);

    // Insert random indices
    for (int i = 0; i < 1000; ++i) {
      const auto index = dis(gen);
      const auto value = static_cast<int>(index * 2);

      set.Insert(index, value);
      indices.insert(index);

      CHECK(set.Contains(index));
      CHECK_EQ(set.Get(index), value);
    }

    CHECK_EQ(set.Size(), indices.size());

    // Verify all indices are present
    for (const auto& index : indices) {
      CHECK(set.Contains(index));
      CHECK_EQ(set.Get(index), static_cast<int>(index * 2));
    }

    // Remove half the elements
    auto it = indices.begin();
    std::advance(it, indices.size() / 2);
    for (auto remove_it = indices.begin(); remove_it != it; ++remove_it) {
      set.Remove(*remove_it);
      CHECK_FALSE(set.Contains(*remove_it));
    }

    CHECK_EQ(set.Size(), indices.size() - indices.size() / 2);

    // Verify remaining elements are still there
    for (; it != indices.end(); ++it) {
      CHECK(set.Contains(*it));
      CHECK_EQ(set.Get(*it), static_cast<int>(*it * 2));
    }
  }

  // Test with custom allocator
  template <typename T>
  class TestAllocator {
  public:
    using value_type = T;

    TestAllocator() noexcept = default;
    template <typename U>
    TestAllocator(const TestAllocator<U>&) noexcept {}

    T* allocate(std::size_t n) { return static_cast<T*>(std::malloc(n * sizeof(T))); }

    void deallocate(T* p, std::size_t) { std::free(p); }

    template <typename U>
    bool operator==(const TestAllocator<U>&) const noexcept {
      return true;
    }
  };

  TEST_CASE("SparseSet::TryGet") {
    helios::container::SparseSet<int> set;

    // Test with empty set
    CHECK_EQ(set.TryGet(42), nullptr);

    // Insert some values
    set.Insert(10, 100);
    set.Insert(20, 200);
    set.Insert(30, 300);

    // Test successful TryGet
    auto* ptr10 = set.TryGet(10);
    auto* ptr20 = set.TryGet(20);
    auto* ptr30 = set.TryGet(30);

    REQUIRE(ptr10 != nullptr);
    REQUIRE(ptr20 != nullptr);
    REQUIRE(ptr30 != nullptr);

    CHECK_EQ(*ptr10, 100);
    CHECK_EQ(*ptr20, 200);
    CHECK_EQ(*ptr30, 300);

    // Test failed TryGet (non-existent indices)
    CHECK_EQ(set.TryGet(0), nullptr);
    CHECK_EQ(set.TryGet(5), nullptr);
    CHECK_EQ(set.TryGet(15), nullptr);
    CHECK_EQ(set.TryGet(25), nullptr);
    CHECK_EQ(set.TryGet(100), nullptr);

    // Test const version
    const auto& const_set = set;
    const auto* const_ptr10 = const_set.TryGet(10);
    const auto* const_ptr_invalid = const_set.TryGet(999);

    REQUIRE(const_ptr10 != nullptr);
    CHECK_EQ(*const_ptr10, 100);
    CHECK_EQ(const_ptr_invalid, nullptr);

    // Test modification through returned pointer
    *ptr10 = 999;
    CHECK_EQ(set.Get(10), 999);
    CHECK_EQ(*set.TryGet(10), 999);

    // Test after removal
    set.Remove(20);
    CHECK_EQ(set.TryGet(20), nullptr);

    // Other elements should still be accessible
    auto* ptr10_after_removal = set.TryGet(10);
    auto* ptr30_after_removal = set.TryGet(30);

    REQUIRE(ptr10_after_removal != nullptr);
    REQUIRE(ptr30_after_removal != nullptr);
    CHECK_EQ(*ptr10_after_removal, 999);
    CHECK_EQ(*ptr30_after_removal, 300);
  }

  TEST_CASE("SparseSet::TryGet: large indices") {
    helios::container::SparseSet<int> set;

    // Test with large sparse indices
    set.Insert(1000000, 42);

    auto* ptr = set.TryGet(1000000);
    REQUIRE(ptr != nullptr);
    CHECK_EQ(*ptr, 42);

    // Test nearby indices that don't exist
    CHECK_EQ(set.TryGet(999999), nullptr);
    CHECK_EQ(set.TryGet(1000001), nullptr);
  }

  TEST_CASE("SparseSet::TryGet: custom types") {
    helios::container::SparseSet<TestValue> set;

    // Insert test values
    set.Emplace(1, 42, 3.14f);
    set.Emplace(2, 84, 2.71f);

    // Test successful TryGet
    auto* ptr1 = set.TryGet(1);
    auto* ptr2 = set.TryGet(2);

    REQUIRE(ptr1 != nullptr);
    REQUIRE(ptr2 != nullptr);

    CHECK_EQ(ptr1->x, 42);
    CHECK_EQ(ptr1->y, 3.14f);
    CHECK_EQ(ptr2->x, 84);
    CHECK_EQ(ptr2->y, 2.71f);

    // Test failed TryGet
    CHECK_EQ(set.TryGet(0), nullptr);
    CHECK_EQ(set.TryGet(3), nullptr);

    // Test modification through pointer
    ptr1->x = 999;
    ptr1->y = 1.23f;

    CHECK_EQ(set.Get(1).x, 999);
    CHECK_EQ(set.Get(1).y, 1.23f);
  }

  TEST_CASE("SparseSet: custom allocator") {
    helios::container::SparseSet<int, std::size_t, TestAllocator<int>> set;

    set.Insert(10, 100);
    set.Insert(20, 200);

    CHECK_EQ(set.Size(), 2);
    CHECK(set.Contains(10));
    CHECK(set.Contains(20));
    CHECK_EQ(set.Get(10), 100);
    CHECK_EQ(set.Get(20), 200);

    // Test TryGet with custom allocator
    auto* ptr = set.TryGet(10);
    REQUIRE(ptr != nullptr);
    CHECK_EQ(*ptr, 100);
    CHECK_EQ(set.TryGet(999), nullptr);
  }

}  // TEST_SUITE
