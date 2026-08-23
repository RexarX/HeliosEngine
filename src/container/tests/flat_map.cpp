#include <doctest/doctest.h>

#include <helios/container/flat_map.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory_resource>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace helios::container;

TEST_SUITE("helios::container::FlatMap") {
  TEST_CASE("helios::container::FlatMap::ctor: default construction") {
    FlatMap<int, int> map;

    CHECK(map.Empty());
    CHECK_EQ(map.Size(), 0);
    CHECK_EQ(map.Capacity(), 0);
  }

  TEST_CASE("helios::container::FlatMap::ctor: allocator construction") {
    std::pmr::monotonic_buffer_resource resource;
    FlatMap<int, int> map(&resource);

    CHECK(map.Empty());
    CHECK_EQ(map.GetMemoryResource(), &resource);
  }

  TEST_CASE("helios::container::FlatMap::ctor: copy construction") {
    FlatMap<int, int> original;
    original.Insert(1, 10);
    original.Insert(2, 20);

    FlatMap<int, int> copy(original);

    CHECK_EQ(copy.Size(), 2);
    CHECK_EQ(copy.At(1), 10);
    CHECK_EQ(copy.At(2), 20);
    CHECK_EQ(original.Size(), 2);
  }

  TEST_CASE("helios::container::FlatMap::ctor: move construction") {
    FlatMap<int, int> original;
    original.Insert(1, 10);

    FlatMap<int, int> moved(std::move(original));

    CHECK_EQ(moved.Size(), 1);
    CHECK_EQ(moved.At(1), 10);
    CHECK(original.Empty());
  }

  TEST_CASE("helios::container::FlatMap::operator=: copy assignment") {
    FlatMap<int, int> original;
    original.Insert(3, 30);

    FlatMap<int, int> copy;
    copy.Insert(1, 1);
    copy = original;

    CHECK_EQ(copy.Size(), 1);
    CHECK_EQ(copy.At(3), 30);
  }

  TEST_CASE("helios::container::FlatMap::operator=: move assignment") {
    FlatMap<int, int> original;
    original.Insert(4, 40);

    FlatMap<int, int> moved;
    moved.Insert(1, 1);
    moved = std::move(original);

    CHECK_EQ(moved.At(4), 40);
    CHECK(original.Empty());
  }

  TEST_CASE("helios::container::FlatMap::Clear: removes entries") {
    FlatMap<int, int> map;
    map.Insert(1, 1);
    map.Insert(2, 2);

    map.Clear();

    CHECK(map.Empty());
    CHECK_EQ(map.Size(), 0);
    CHECK_GE(map.Capacity(), 2);
  }

  TEST_CASE("helios::container::FlatMap::Reserve: reserves capacity") {
    FlatMap<int, int> map;
    map.Reserve(32);

    CHECK_GE(map.Capacity(), 32);
    CHECK(map.Empty());
  }

  TEST_CASE("helios::container::FlatMap::Insert: single entry") {
    SUBCASE("inserts new key") {
      FlatMap<int, int> map;
      const auto it = map.Insert(2, 20);

      CHECK_EQ(it->second, 20);
      CHECK_EQ(map.Size(), 1);
      CHECK_EQ(map.At(2), 20);
    }

    SUBCASE("does not overwrite existing key") {
      FlatMap<int, int> map;
      map.Insert(1, 10);
      const auto it = map.Insert(1, 99);

      CHECK_EQ(it->second, 10);
      CHECK_EQ(map.At(1), 10);
      CHECK_EQ(map.Size(), 1);
    }

    SUBCASE("keeps keys sorted") {
      FlatMap<int, int> map;
      map.Insert(3, 3);
      map.Insert(1, 1);
      map.Insert(2, 2);

      auto it = map.begin();
      CHECK_EQ(it->first, 1);
      ++it;
      CHECK_EQ(it->first, 2);
      ++it;
      CHECK_EQ(it->first, 3);
    }

    SUBCASE("pair overload") {
      FlatMap<int, int> map;
      map.Insert(std::pair<int, int>{5, 50});
      CHECK_EQ(map.At(5), 50);
    }
  }

  TEST_CASE("helios::container::FlatMap::Insert: range and iterators") {
    SUBCASE("range insert skips existing keys") {
      FlatMap<int, int> map;
      map.Insert(2, 20);

      std::vector<std::pair<int, int>> extra = {{1, 10}, {2, 99}, {3, 30}};
      map.Insert(extra);

      CHECK_EQ(map.Size(), 3);
      CHECK_EQ(map.At(1), 10);
      CHECK_EQ(map.At(2), 20);
      CHECK_EQ(map.At(3), 30);
    }

    SUBCASE("iterator insert from unsorted input") {
      FlatMap<int, int> map;
      std::vector<std::pair<int, int>> extra = {
          {4, 4}, {1, 1}, {4, 40}, {2, 2}};
      map.Insert(extra.begin(), extra.end());

      CHECK_EQ(map.Size(), 3);
      CHECK_EQ(map.At(1), 1);
      CHECK_EQ(map.At(2), 2);
      CHECK_EQ(map.At(4), 4);
    }

    SUBCASE("rvalue range moves values") {
      FlatMap<std::string, std::string> map;
      std::vector<std::pair<std::string, std::string>> extra;
      extra.emplace_back("a", "alpha");
      extra.emplace_back("b", "beta");

      map.Insert(std::move(extra));

      CHECK_EQ(map.At("a"), "alpha");
      CHECK_EQ(map.At(std::string_view{"b"}), "beta");
    }

    SUBCASE("insert into empty from unsorted range") {
      FlatMap<int, int> map;
      std::vector<std::pair<int, int>> extra = {{2, 2}, {1, 1}};
      map.Insert(extra);

      CHECK_EQ(map.Size(), 2);
      CHECK_EQ(map.begin()->first, 1);
    }

    SUBCASE("empty range is a no-op") {
      FlatMap<int, int> map;
      map.Insert(1, 1);
      std::vector<std::pair<int, int>> extra;
      map.Insert(extra);
      CHECK_EQ(map.Size(), 1);
    }

    SUBCASE("drops duplicate incoming keys") {
      FlatMap<int, int> map;
      std::vector<std::pair<int, int>> extra = {{1, 1}, {1, 2}, {1, 3}};
      map.Insert(extra);
      CHECK_EQ(map.Size(), 1);
    }
  }

  TEST_CASE("helios::container::FlatMap::InsertOrAssign: overwrites") {
    FlatMap<int, int> map;
    const auto inserted = map.InsertOrAssign(1, 10);
    CHECK_EQ(inserted->second, 10);
    CHECK_EQ(map.At(1), 10);

    const auto assigned = map.InsertOrAssign(1, 99);
    CHECK_EQ(assigned->second, 99);
    CHECK_EQ(map.At(1), 99);
    CHECK_EQ(map.Size(), 1);
  }

  TEST_CASE("helios::container::FlatMap::Emplace: constructs value") {
    FlatMap<int, std::string> map;
    const auto it = map.Emplace(1, std::size_t{3}, 'x');

    CHECK_EQ(it->second, "xxx");
    CHECK_EQ(map.At(1), "xxx");
    map.Emplace(1, std::size_t{1}, 'z');
    CHECK_EQ(map.At(1), "xxx");
  }

  TEST_CASE("helios::container::FlatMap::Emplace: moves key") {
    FlatMap<std::string, int> map;
    std::string key = "moved";
    const auto it = map.Emplace(std::move(key), 1);

    CHECK_EQ(it->first, "moved");
    CHECK_EQ(map.At(std::string_view{"moved"}), 1);
    CHECK(key.empty());
  }

  TEST_CASE("helios::container::FlatMap::EmplaceHint: inserts near hint") {
    SUBCASE("correct hint inserts without searching") {
      FlatMap<int, int> map;
      map.Insert(1, 1);
      map.Insert(3, 3);

      const auto it = map.EmplaceHint(map.end(), 4, 4);
      CHECK_EQ(it->second, 4);
      CHECK_EQ(map.Size(), 3);
      CHECK_EQ(std::prev(map.end())->first, 4);
    }

    SUBCASE("stale hint still inserts at the sorted position") {
      FlatMap<int, int> map;
      map.Insert(1, 1);
      map.Insert(4, 4);

      const auto it = map.EmplaceHint(map.end(), 2, 2);
      CHECK_EQ(it->first, 2);
      CHECK_EQ(it->second, 2);

      auto walk = map.begin();
      CHECK_EQ(walk->first, 1);
      ++walk;
      CHECK_EQ(walk->first, 2);
      ++walk;
      CHECK_EQ(walk->first, 4);
    }

    SUBCASE("existing key is not overwritten") {
      FlatMap<int, int> map;
      map.Insert(1, 10);

      const auto it = map.EmplaceHint(map.begin(), 1, 99);
      CHECK_EQ(it->second, 10);
      CHECK_EQ(map.Size(), 1);
    }

    SUBCASE("moves key") {
      FlatMap<std::string, int> map;
      std::string key = "hinted";
      const auto it = map.EmplaceHint(map.end(), std::move(key), 7);
      CHECK_EQ(it->second, 7);
      CHECK(key.empty());
    }
  }

  TEST_CASE(
      "helios::container::FlatMap::TryEmplace: constructs only if absent") {
    FlatMap<int, std::string> map;
    auto [it, inserted] = map.TryEmplace(1, "first");
    CHECK(inserted);
    CHECK_EQ(it->second, "first");

    auto [it2, inserted2] = map.TryEmplace(1, "second");
    CHECK_FALSE(inserted2);
    CHECK_EQ(it2->second, "first");
  }

  TEST_CASE("helios::container::FlatMap::TryEmplace: moves key") {
    FlatMap<std::string, int> map;
    std::string key = "try";
    auto [it, inserted] = map.TryEmplace(std::move(key), 1);
    CHECK(inserted);
    CHECK_EQ(it->second, 1);
    CHECK(key.empty());

    std::string again = "try";
    auto [it2, inserted2] = map.TryEmplace(std::move(again), 2);
    CHECK_FALSE(inserted2);
    CHECK_EQ(it2->second, 1);
    CHECK_EQ(again, "try");
  }

  TEST_CASE("helios::container::FlatMap::Erase: by key and iterator") {
    SUBCASE("erase existing key") {
      FlatMap<int, int> map;
      map.Insert(1, 1);
      map.Insert(2, 2);

      CHECK(map.Erase(1));
      CHECK_FALSE(map.Contains(1));
      CHECK_EQ(map.Size(), 1);
    }

    SUBCASE("erase missing key") {
      FlatMap<int, int> map;
      CHECK_FALSE(map.Erase(1));
    }

    SUBCASE("erase iterator") {
      FlatMap<int, int> map;
      map.Insert(1, 1);
      map.Insert(2, 2);

      const auto next = map.Erase(map.begin());
      CHECK_EQ(next->first, 2);
      CHECK_EQ(map.Size(), 1);
    }

    SUBCASE("erase iterator range") {
      FlatMap<int, int> map;
      map.Insert(1, 1);
      map.Insert(2, 2);
      map.Insert(3, 3);

      auto first = map.begin();
      ++first;
      map.Erase(first, map.end());

      CHECK_EQ(map.Size(), 1);
      CHECK_EQ(map.At(1), 1);
    }
  }

  TEST_CASE("helios::container::FlatMap::Merge: another FlatMap") {
    SUBCASE("disjoint keys are moved from rvalue source") {
      FlatMap<int, int> map;
      map.Insert(2, 20);

      FlatMap<int, int> other;
      other.Insert(3, 30);
      other.Insert(1, 10);

      map.Merge(std::move(other));

      CHECK_EQ(map.Size(), 3);
      CHECK_EQ(map.At(1), 10);
      CHECK_EQ(map.At(2), 20);
      CHECK_EQ(map.At(3), 30);
      CHECK(other.Empty());
    }

    SUBCASE("existing keys win; duplicates stay in rvalue source") {
      FlatMap<int, int> map;
      map.Insert(1, 10);

      FlatMap<int, int> other;
      other.Insert(1, 99);
      other.Insert(2, 20);

      map.Merge(std::move(other));

      CHECK_EQ(map.At(1), 10);
      CHECK_EQ(map.At(2), 20);
      CHECK_EQ(other.Size(), 1);
      CHECK_EQ(other.At(1), 99);
    }

    SUBCASE("const lvalue source is copied and left unchanged") {
      FlatMap<int, int> map;
      map.Insert(1, 10);

      FlatMap<int, int> other_mut;
      other_mut.Insert(1, 99);
      other_mut.Insert(2, 20);
      const FlatMap<int, int>& other = other_mut;

      map.Merge(other);

      CHECK_EQ(map.At(1), 10);
      CHECK_EQ(map.At(2), 20);
      CHECK_EQ(other.Size(), 2);
      CHECK_EQ(other.At(1), 99);
      CHECK_EQ(other.At(2), 20);
    }

    SUBCASE("merge into empty") {
      FlatMap<int, int> map;
      FlatMap<int, int> other;
      other.Insert(2, 2);
      other.Insert(1, 1);

      map.Merge(std::move(other));

      CHECK_EQ(map.Size(), 2);
      CHECK_EQ(map.begin()->first, 1);
      CHECK(other.Empty());
    }

    SUBCASE("merging empty source is a no-op") {
      FlatMap<int, int> map;
      map.Insert(1, 1);
      FlatMap<int, int> other;
      map.Merge(std::move(other));
      CHECK_EQ(map.Size(), 1);
    }

    SUBCASE("self-merge is a no-op") {
      FlatMap<int, int> map;
      map.Insert(1, 1);
      map.Insert(2, 2);

      map.Merge(map);
      CHECK_EQ(map.Size(), 2);

      map.Merge(std::move(map));
      CHECK_EQ(map.Size(), 2);
    }
  }

  TEST_CASE("helios::container::FlatMap::ShrinkToFit: reduces capacity") {
    FlatMap<int, int> map;
    map.Reserve(64);
    map.Insert(1, 1);

    CHECK_GE(map.Capacity(), 64);
    map.ShrinkToFit();
    CHECK_LE(map.Capacity(), 8);
    CHECK_EQ(map.At(1), 1);
  }

  TEST_CASE("helios::container::FlatMap::Swap: swaps contents") {
    std::pmr::monotonic_buffer_resource resource;
    FlatMap<int, int> map1(&resource);
    map1.Insert(1, 1);

    FlatMap<int, int> map2(&resource);
    map2.Insert(2, 2);

    map1.Swap(map2);

    CHECK(map1.Contains(2));
    CHECK(map2.Contains(1));

    swap(map1, map2);
    CHECK(map1.Contains(1));
    CHECK(map2.Contains(2));
  }

  TEST_CASE("helios::container::FlatMap::Find: lookup") {
    FlatMap<int, int> map;
    map.Insert(1, 10);

    const auto it = map.Find(1);
    CHECK_NE(it, map.end());
    CHECK_EQ(it->second, 10);
    CHECK_EQ(map.Find(2), map.end());

    const auto& cmap = map;
    CHECK_EQ(cmap.Find(1)->second, 10);
    CHECK_EQ(cmap.Find(2), cmap.end());
  }

  TEST_CASE("helios::container::FlatMap::LowerBound / UpperBound") {
    FlatMap<int, int> map;
    map.Insert(1, 10);
    map.Insert(3, 30);
    map.Insert(5, 50);

    CHECK_EQ(map.LowerBound(3)->first, 3);
    CHECK_EQ(map.UpperBound(3)->first, 5);
    CHECK_EQ(map.LowerBound(4)->first, 5);
    CHECK_EQ(map.UpperBound(5), map.end());

    const auto& cmap = map;
    CHECK_EQ(cmap.LowerBound(1)->second, 10);
    CHECK_EQ(cmap.UpperBound(1)->first, 3);
  }

  TEST_CASE("helios::container::FlatMap::At: access") {
    FlatMap<int, int> map;
    map.Insert(1, 10);
    map.At(1) = 11;
    CHECK_EQ(map.At(1), 11);

    const auto& cmap = map;
    CHECK_EQ(cmap.At(1), 11);
  }

  TEST_CASE("helios::container::FlatMap::operator[]: insert or access") {
    FlatMap<int, int> map;
    map[1] = 10;
    CHECK_EQ(map[1], 10);
    map[1] = 20;
    CHECK_EQ(map.Size(), 1);
    CHECK_EQ(map[1], 20);

    std::string key = "idx";
    FlatMap<std::string, int> strings;
    strings[std::move(key)] = 3;
    CHECK_EQ(strings.At(std::string_view{"idx"}), 3);
    CHECK(key.empty());
  }

  TEST_CASE("helios::container::FlatMap::operator== / <=>") {
    FlatMap<int, int> a;
    a.Insert(1, 1);
    FlatMap<int, int> b;
    b.Insert(1, 1);

    CHECK(a == b);
    b.Insert(2, 2);
    CHECK_NE(a, b);
    CHECK(a < b);
  }

  TEST_CASE("helios::container::FlatMap::Contains / Empty") {
    FlatMap<int, int> map;
    CHECK(map.Empty());
    CHECK_FALSE(map.Contains(1));

    map.Insert(1, 1);
    CHECK_FALSE(map.Empty());
    CHECK(map.Contains(1));
  }

  TEST_CASE("helios::container::FlatMap::iterators") {
    FlatMap<int, int> map;
    map.Insert(1, 10);
    map.Insert(2, 20);

    int sum = 0;
    for (const auto& [key, value] : map) {
      sum += value;
    }
    CHECK_EQ(sum, 30);
    CHECK_EQ(std::distance(map.rbegin(), map.rend()), 2);
    CHECK_EQ(map.cbegin()->first, 1);
  }

  TEST_CASE("helios::container::FlatMap::heterogeneous lookup") {
    SUBCASE("string keys accept string_view and C strings") {
      FlatMap<std::string, int> map;
      map.Insert("alpha", 1);
      map.Insert(std::string{"beta"}, 2);

      CHECK(map.Contains(std::string_view{"alpha"}));
      CHECK(map.Contains("beta"));
      CHECK_EQ(map.Find(std::string_view{"alpha"})->second, 1);
      CHECK_EQ(map.At("beta"), 2);
      CHECK(map.Erase(std::string_view{"alpha"}));
      CHECK_FALSE(map.Contains("alpha"));
    }

    SUBCASE("transparent std::less<> compares related integer types") {
      FlatMap<int, int> map;
      map.Insert(42, 1);

      const long key = 42;
      CHECK(map.Contains(key));
      CHECK_EQ(map.At(key), 1);
    }

    SUBCASE("non-transparent compare rejects unrelated lookup types") {
      static_assert(FlatMapLookupKey<std::less<int>, int, int>);
      static_assert(!FlatMapLookupKey<std::less<int>, int, std::string_view>);
      static_assert(FlatMapLookupKey<std::less<>, int, long>);
    }
  }

  TEST_CASE("helios::container::erase_if: removes matching entries") {
    FlatMap<int, int> map;
    map.Insert(1, 1);
    map.Insert(2, 2);
    map.Insert(3, 3);

    const auto removed =
        erase_if(map, [](const auto& entry) { return entry.first % 2 == 1; });

    CHECK_EQ(removed, 2);
    CHECK_EQ(map.Size(), 1);
    CHECK(map.Contains(2));
  }
}
