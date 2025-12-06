#include <doctest/doctest.h>

#include <helios/core/memory/frame_allocator.hpp>
#include <helios/core/memory/free_list_allocator.hpp>
#include <helios/core/memory/pool_allocator.hpp>
#include <helios/core/memory/stack_allocator.hpp>
#include <helios/core/memory/stl_allocator_adapter.hpp>

#include <algorithm>
#include <deque>
#include <list>
#include <map>
#include <numeric>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

using namespace helios::memory;

TEST_SUITE("memory::STLAllocatorAdapter") {
  TEST_CASE("Basic adapter construction") {
    SUBCASE("FrameAllocator adapter") {
      FrameAllocator allocator(4096);
      STLFrameAllocator<int> adapter(allocator);

      CHECK_EQ(adapter.get_allocator(), &allocator);
      CHECK_GT(adapter.max_size(), 0);
    }

    SUBCASE("PoolAllocator adapter") {
      PoolAllocator allocator(128, 100);
      STLPoolAllocator<int> adapter(allocator);

      CHECK_EQ(adapter.get_allocator(), &allocator);
    }

    SUBCASE("StackAllocator adapter") {
      StackAllocator allocator(4096);
      STLStackAllocator<int> adapter(allocator);

      CHECK_EQ(adapter.get_allocator(), &allocator);
    }

    SUBCASE("FreeListAllocator adapter") {
      FreeListAllocator allocator(4096);
      STLFreeListAllocator<int> adapter(allocator);

      CHECK_EQ(adapter.get_allocator(), &allocator);
    }
  }

  TEST_CASE("Adapter rebind") {
    SUBCASE("Rebind to different type") {
      FrameAllocator allocator(4096);
      STLFrameAllocator<int> int_adapter(allocator);

      using FloatAdapter = STLFrameAllocator<int>::rebind<float>::other;
      FloatAdapter float_adapter(int_adapter);

      CHECK_EQ(int_adapter.get_allocator(), float_adapter.get_allocator());
    }
  }

  TEST_CASE("Vector with FrameAllocator") {
    SUBCASE("Basic push_back") {
      FrameAllocator allocator(4096);
      std::vector<int, STLFrameAllocator<int>> vec{STLFrameAllocator<int>(allocator)};

      vec.push_back(1);
      vec.push_back(2);
      vec.push_back(3);

      CHECK_EQ(vec.size(), 3);
      CHECK_EQ(vec[0], 1);
      CHECK_EQ(vec[1], 2);
      CHECK_EQ(vec[2], 3);
    }

    SUBCASE("Reserve and resize") {
      FrameAllocator allocator(4096);
      std::vector<int, STLFrameAllocator<int>> vec{STLFrameAllocator<int>(allocator)};

      vec.reserve(100);
      CHECK_GE(vec.capacity(), 100);

      vec.resize(50, 42);
      CHECK_EQ(vec.size(), 50);
      CHECK_EQ(vec[49], 42);
    }

    SUBCASE("Range insertion") {
      FrameAllocator allocator(4096);
      std::vector<int, STLFrameAllocator<int>> vec{STLFrameAllocator<int>(allocator)};

      std::vector<int> source = {1, 2, 3, 4, 5};
      vec.insert(vec.end(), source.begin(), source.end());

      CHECK_EQ(vec.size(), 5);
      CHECK(std::equal(vec.begin(), vec.end(), source.begin()));
    }

    SUBCASE("Emplace back") {
      FrameAllocator allocator(4096);

      struct Complex {
        int x = 0;
        float y = 0.0F;
        Complex(int x_, float y_) : x{x_}, y{y_} {}
      };

      std::vector<Complex, STLFrameAllocator<Complex>> vec{STLFrameAllocator<Complex>(allocator)};
      vec.emplace_back(10, 3.14F);
      vec.emplace_back(20, 2.71F);

      CHECK_EQ(vec.size(), 2);
      CHECK_EQ(vec[0].x, 10);
      CHECK_EQ(vec[0].y, doctest::Approx(3.14F));
      CHECK_EQ(vec[1].x, 20);
    }

    SUBCASE("String vector") {
      FrameAllocator allocator(8192);
      std::vector<std::string, STLFrameAllocator<std::string>> vec{STLFrameAllocator<std::string>(allocator)};

      vec.push_back("Hello");
      vec.push_back("World");
      vec.emplace_back("From");
      vec.emplace_back("Custom");
      vec.emplace_back("Allocator");

      CHECK_EQ(vec.size(), 5);
      CHECK_EQ(vec[0], "Hello");
      CHECK_EQ(vec[4], "Allocator");
    }
  }

  TEST_CASE("List with FreeListAllocator") {
    SUBCASE("Push and pop") {
      FreeListAllocator allocator(8192);  // Use FreeListAllocator for variable-sized list nodes
      std::list<int, STLFreeListAllocator<int>> list{STLFreeListAllocator<int>(allocator)};

      list.push_back(1);
      list.push_back(2);
      list.push_front(0);

      CHECK_EQ(list.size(), 3);
      CHECK_EQ(list.front(), 0);
      CHECK_EQ(list.back(), 2);

      list.pop_front();
      CHECK_EQ(list.front(), 1);
    }

    SUBCASE("Insertion in middle") {
      FreeListAllocator allocator(8192);  // Use FreeListAllocator for variable-sized list nodes
      std::list<int, STLFreeListAllocator<int>> list{STLFreeListAllocator<int>(allocator)};

      list.push_back(1);
      list.push_back(3);

      auto it = list.begin();
      ++it;
      list.insert(it, 2);

      CHECK_EQ(list.size(), 3);
      auto vals = std::vector<int>(list.begin(), list.end());
      CHECK_EQ(vals[0], 1);
      CHECK_EQ(vals[1], 2);
      CHECK_EQ(vals[2], 3);
    }

    SUBCASE("Remove elements") {
      FreeListAllocator allocator(8192);  // Use FreeListAllocator for variable-sized list nodes
      std::list<int, STLFreeListAllocator<int>> list{STLFreeListAllocator<int>(allocator)};

      for (int i = 0; i < 10; ++i) {
        list.push_back(i);
      }

      list.remove_if([](int x) { return x % 2 == 0; });

      CHECK_EQ(list.size(), 5);
      for (int val : list) {
        CHECK_EQ(val % 2, 1);
      }
    }
  }

  TEST_CASE("Map with FreeListAllocator") {
    SUBCASE("Insert and find") {
      FreeListAllocator allocator(8192);
      using Alloc = STLAllocatorAdapter<std::pair<const int, std::string>, FreeListAllocator>;
      std::map<int, std::string, std::less<int>, Alloc> map{std::less<int>{}, Alloc(allocator)};

      map[1] = "one";
      map[2] = "two";
      map[3] = "three";

      CHECK_EQ(map.size(), 3);
      CHECK_EQ(map[1], "one");
      CHECK_EQ(map[2], "two");
      CHECK_NE(map.find(3), map.end());
      CHECK_EQ(map.find(4), map.end());
    }

    SUBCASE("Iteration") {
      FreeListAllocator allocator{8192};
      using Alloc = STLAllocatorAdapter<std::pair<const int, int>, FreeListAllocator>;
      std::map<int, int, std::less<int>, Alloc> map{std::less<int>{}, Alloc(allocator)};

      for (int i = 0; i < 10; ++i) {
        map[i] = i * 10;
      }

      int sum = 0;
      for (const auto& [key, value] : map) {
        sum += value;
      }

      CHECK_EQ(sum, 450);  // 0+10+20+...+90
    }

    SUBCASE("Erase elements") {
      FreeListAllocator allocator(8192);
      using Alloc = STLAllocatorAdapter<std::pair<const std::string, int>, FreeListAllocator>;
      std::map<std::string, int, std::less<std::string>, Alloc> map{std::less<std::string>{}, Alloc(allocator)};

      map["a"] = 1;
      map["b"] = 2;
      map["c"] = 3;

      map.erase("b");

      CHECK_EQ(map.size(), 2);
      CHECK_EQ(map.find("b"), map.end());
      CHECK_EQ(map["a"], 1);
      CHECK_EQ(map["c"], 3);
    }
  }

  TEST_CASE("UnorderedMap with FreeListAllocator") {
    SUBCASE("Basic operations") {
      FreeListAllocator allocator(8192);
      using Alloc = STLAllocatorAdapter<std::pair<const std::string, int>, FreeListAllocator>;
      std::unordered_map<std::string, int, std::hash<std::string>, std::equal_to<std::string>, Alloc> map{
          10, std::hash<std::string>{}, std::equal_to<std::string>{}, Alloc(allocator)};

      map["key1"] = 100;
      map["key2"] = 200;
      map["key3"] = 300;

      CHECK_EQ(map.size(), 3);
      CHECK_EQ(map["key1"], 100);
      CHECK_EQ(map.count("key2"), 1);
      CHECK_EQ(map.count("key4"), 0);
    }

    SUBCASE("Collision handling") {
      FreeListAllocator allocator(8192);
      using Alloc = STLAllocatorAdapter<std::pair<const int, int>, FreeListAllocator>;
      std::unordered_map<int, int, std::hash<int>, std::equal_to<int>, Alloc> map{
          10, std::hash<int>{}, std::equal_to<int>{}, Alloc(allocator)};

      // Insert many elements to test bucket allocation
      for (int i = 0; i < 100; ++i) {
        map[i] = i * 2;
      }

      CHECK_EQ(map.size(), 100);
      CHECK_EQ(map[50], 100);
      CHECK_GT(map.bucket_count(), 0);
    }
  }

  // NOTE: std::set/map with StackAllocator violates LIFO order due to tree rebalancing
  // Skip these tests as they trigger assertions
  TEST_CASE("Set with FreeListAllocator (StackAllocator violates LIFO)") {
    SUBCASE("Insert and find") {
      FreeListAllocator allocator(8192);
      using Alloc = STLAllocatorAdapter<int, FreeListAllocator>;
      std::set<int, std::less<int>, Alloc> set{std::less<int>{}, Alloc(allocator)};

      set.insert(5);
      set.insert(2);
      set.insert(8);
      set.insert(2);  // Duplicate

      CHECK_EQ(set.size(), 3);
      CHECK_EQ(set.count(2), 1);
      CHECK_EQ(set.count(5), 1);
      CHECK_EQ(set.count(10), 0);
    }

    SUBCASE("Ordered iteration") {
      FreeListAllocator allocator(8192);
      using Alloc = STLAllocatorAdapter<int, FreeListAllocator>;
      std::set<int, std::less<int>, Alloc> set{std::less<int>{}, Alloc(allocator)};

      set.insert(3);
      set.insert(1);
      set.insert(4);
      set.insert(1);
      set.insert(5);

      std::vector<int> values{set.begin(), set.end()};
      CHECK_EQ(values.size(), 4);
      CHECK(std::ranges::is_sorted(values));
      CHECK_EQ(values[0], 1);
      CHECK_EQ(values[3], 5);
    }
  }

  TEST_CASE("UnorderedSet with FrameAllocator") {
    SUBCASE("Basic operations") {
      FrameAllocator allocator(8192);
      using Alloc = STLAllocatorAdapter<std::string, FrameAllocator>;
      std::unordered_set<std::string, std::hash<std::string>, std::equal_to<std::string>, Alloc> set{
          10, std::hash<std::string>{}, std::equal_to<std::string>{}, Alloc(allocator)};

      set.insert("apple");
      set.insert("banana");
      set.insert("cherry");
      set.insert("apple");  // Duplicate

      CHECK_EQ(set.size(), 3);
      CHECK_EQ(set.count("apple"), 1);
      CHECK_EQ(set.count("grape"), 0);
    }
  }

  TEST_CASE("Deque with FreeListAllocator") {
    SUBCASE("Push front and back") {
      FreeListAllocator allocator(8192);
      std::deque<int, STLFreeListAllocator<int>> deque{STLFreeListAllocator<int>(allocator)};

      deque.push_back(2);
      deque.push_back(3);
      deque.push_front(1);
      deque.push_front(0);

      CHECK_EQ(deque.size(), 4);
      CHECK_EQ(deque[0], 0);
      CHECK_EQ(deque[1], 1);
      CHECK_EQ(deque[2], 2);
      CHECK_EQ(deque[3], 3);
    }

    SUBCASE("Random access") {
      FreeListAllocator allocator(8192);
      std::deque<int, STLFreeListAllocator<int>> deque{STLFreeListAllocator<int>(allocator)};

      for (int i = 0; i < 20; ++i) {
        deque.push_back(i);
      }

      CHECK_EQ(deque.size(), 20);
      CHECK_EQ(deque[10], 10);
      CHECK_EQ(deque.at(15), 15);
    }
  }

  TEST_CASE("Nested containers") {
    SUBCASE("Vector of vectors") {
      FrameAllocator allocator(16384);
      std::vector<std::vector<int>, STLFrameAllocator<std::vector<int>>> outer{
          STLFrameAllocator<std::vector<int>>(allocator)};

      outer.push_back({1, 2, 3});
      outer.push_back({4, 5});
      outer.push_back({6, 7, 8, 9});

      CHECK_EQ(outer.size(), 3);
      CHECK_EQ(outer[0].size(), 3);
      CHECK_EQ(outer[1].size(), 2);
      CHECK_EQ(outer[2].size(), 4);
      CHECK_EQ(outer[0][1], 2);
      CHECK_EQ(outer[2][3], 9);
    }

    SUBCASE("Map of vectors") {
      FreeListAllocator allocator(16384);

      using ValueType = std::vector<int>;
      using Alloc = STLAllocatorAdapter<std::pair<const std::string, ValueType>, FreeListAllocator>;
      std::map<std::string, ValueType, std::less<std::string>, Alloc> map{std::less<std::string>{}, Alloc(allocator)};

      map["nums1"].push_back(1);
      map["nums1"].push_back(2);
      map["nums2"].push_back(10);
      map["nums2"].push_back(20);
      map["nums2"].push_back(30);

      CHECK_EQ(map.size(), 2);
      CHECK_EQ(map["nums1"].size(), 2);
      CHECK_EQ(map["nums2"].size(), 3);
      CHECK_EQ(map["nums2"][1], 20);
    }
  }

  TEST_CASE("Complex data types") {
    SUBCASE("Struct with custom constructor") {
      struct Entity {
        int id = 0;
        float x = 0.0F;
        float y = 0.0F;
        float z = 0.0F;
        std::string name;
      };

      FrameAllocator allocator(8192);
      std::vector<Entity, STLFrameAllocator<Entity>> vec{STLFrameAllocator<Entity>(allocator)};

      vec.emplace_back(1, 1.0F, 2.0F, 3.0F, "Player");
      vec.emplace_back(2, 4.0F, 5.0F, 6.0F, "Enemy");

      CHECK_EQ(vec.size(), 2);
      CHECK_EQ(vec[0].id, 1);
      CHECK_EQ(vec[0].name, "Player");
      CHECK_EQ(vec[1].x, doctest::Approx(4.0F));
    }
  }

  TEST_CASE("Algorithm compatibility") {
    SUBCASE("std::sort") {
      FrameAllocator allocator(4096);
      std::vector<int, STLFrameAllocator<int>> vec{STLFrameAllocator<int>(allocator)};

      vec = {5, 2, 8, 1, 9, 3};
      std::ranges::sort(vec);

      CHECK_EQ(vec[0], 1);
      CHECK_EQ(vec[5], 9);
      CHECK(std::ranges::is_sorted(vec));
    }

    SUBCASE("std::find_if") {
      FrameAllocator allocator(4096);
      std::vector<int, STLFrameAllocator<int>> vec{STLFrameAllocator<int>(allocator)};

      for (int i = 0; i < 10; ++i) {
        vec.push_back(i);
      }

      auto it = std::ranges::find_if(vec, [](int x) { return x > 5; });
      CHECK_NE(it, vec.end());
      CHECK_EQ(*it, 6);
    }

    SUBCASE("std::ranges::transform") {
      FrameAllocator allocator(8192);
      std::vector<int, STLFrameAllocator<int>> src{STLFrameAllocator<int>(allocator)};
      std::vector<int, STLFrameAllocator<int>> dst{STLFrameAllocator<int>(allocator)};

      src = {1, 2, 3, 4, 5};
      dst.resize(src.size());

      std::ranges::transform(src, dst.begin(), [](int x) { return x * 2; });

      CHECK_EQ(dst[0], 2);
      CHECK_EQ(dst[4], 10);
    }

    SUBCASE("std::accumulate") {
      FreeListAllocator allocator(4096);
      std::vector<int, STLFreeListAllocator<int>> vec{STLFreeListAllocator<int>(allocator)};

      for (int i = 1; i <= 10; ++i) {
        vec.push_back(i);
      }

      const int sum = std::accumulate(vec.begin(), vec.end(), 0);
      CHECK_EQ(sum, 55);
    }
  }

  TEST_CASE("Move semantics") {
    SUBCASE("Move vector") {
      FrameAllocator allocator(4096);
      std::vector<int, STLFrameAllocator<int>> vec1{STLFrameAllocator<int>(allocator)};

      vec1.push_back(1);
      vec1.push_back(2);
      vec1.push_back(3);

      auto vec2 = std::move(vec1);

      CHECK_EQ(vec2.size(), 3);
      CHECK_EQ(vec2[1], 2);
    }
  }

  TEST_CASE("Allocator statistics tracking") {
    SUBCASE("Monitor allocations") {
      FrameAllocator allocator(8192);

      {
        std::vector<int, STLFrameAllocator<int>> vec{STLFrameAllocator<int>(allocator)};
        vec.reserve(100);

        const auto stats = allocator.Stats();
        CHECK_GT(stats.total_allocations, 0);
        CHECK_GT(stats.total_allocated, 0);
      }

      // After reset
      allocator.Reset();
      const auto stats = allocator.Stats();
      CHECK_EQ(stats.total_allocated, 0);
    }
  }

  TEST_CASE("Multiple containers sharing allocator") {
    SUBCASE("Frame allocator shared") {
      FrameAllocator allocator(16384);

      std::vector<int, STLFrameAllocator<int>> vec{STLFrameAllocator<int>(allocator)};
      std::list<float, STLFrameAllocator<float>> list{STLFrameAllocator<float>(allocator)};
      using MapAlloc = STLAllocatorAdapter<std::pair<const int, std::string>, FrameAllocator>;
      std::map<int, std::string, std::less<int>, MapAlloc> map{std::less<int>{}, MapAlloc(allocator)};

      vec.push_back(42);
      list.push_back(3.14F);
      map[1] = "one";

      CHECK_EQ(vec.size(), 1);
      CHECK_EQ(list.size(), 1);
      CHECK_EQ(map.size(), 1);

      const auto stats = allocator.Stats();
      CHECK_GE(stats.total_allocations, 3);

      // Reset frees all
      allocator.Reset();

      const auto stats2 = allocator.Stats();
      CHECK_EQ(stats2.total_allocated, 0);
    }
  }

  TEST_CASE("Large allocations") {
    SUBCASE("Large vector") {
      FrameAllocator allocator(1024 * 1024);  // 1MB
      std::vector<int, STLFrameAllocator<int>> vec{STLFrameAllocator<int>(allocator)};

      vec.resize(10000);

      for (size_t i = 0; i < vec.size(); ++i) {
        vec[i] = static_cast<int>(i);
      }

      CHECK_EQ(vec.size(), 10000);
      CHECK_EQ(vec[5000], 5000);
    }
  }

  TEST_CASE("Explicit adapter creation") {
    SUBCASE("Direct adapter usage") {
      FrameAllocator allocator(4096);
      STLFrameAllocator<int> adapter(allocator);

      std::vector<int, STLFrameAllocator<int>> vec(adapter);
      vec.push_back(100);

      CHECK_EQ(vec.size(), 1);
      CHECK_EQ(vec[0], 100);
    }
  }

  TEST_CASE("Boundary conditions") {
    SUBCASE("Empty container") {
      FrameAllocator allocator(4096);
      std::vector<int, STLFrameAllocator<int>> vec{STLFrameAllocator<int>(allocator)};

      CHECK(vec.empty());
      CHECK_EQ(vec.size(), 0);
    }

    SUBCASE("Single element") {
      FreeListAllocator allocator(4096);  // Use FreeListAllocator for variable-sized allocations
      std::vector<int, STLFreeListAllocator<int>> vec{STLFreeListAllocator<int>(allocator)};

      vec.push_back(42);

      CHECK_EQ(vec.size(), 1);
      CHECK_EQ(vec.front(), 42);
      CHECK_EQ(vec.back(), 42);
    }
  }
}

}  // namespace
