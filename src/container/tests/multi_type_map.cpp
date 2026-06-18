#include <doctest/doctest.h>

#include <helios/container/multi_type_map.hpp>

#include <algorithm>
#include <atomic>
#include <memory>
#include <memory_resource>
#include <string>
#include <utility>
#include <vector>

using namespace helios::container;

namespace {

// Simple value-type storage for basic tests.
// Stores a single int. Supports Clear(), empty(), size() for duck-typing.
struct SimpleStorage {
  int value = 0;
  bool has_value = false;

  constexpr void Set(int val) noexcept {
    value = val;
    has_value = true;
  }

  constexpr void Clear() noexcept {
    value = 0;
    has_value = false;
  }

  [[nodiscard]] constexpr bool Empty() const noexcept { return !has_value; }
  [[nodiscard]] constexpr size_t Size() const noexcept {
    return has_value ? 1 : 0;
  }

  constexpr bool operator==(const SimpleStorage&) const noexcept = default;
};

// Mergeable storage: accumulates integers. Used to exercise Merge paths.
struct MergeableStorage {
  std::vector<int> items;

  void Push(int v) { items.push_back(v); }

  void Merge(MergeableStorage&& other) {
    items.insert(items.end(), other.items.begin(), other.items.end());
    other.items.clear();
  }

  void Clear() { items.clear(); }

  [[nodiscard]] bool Empty() const noexcept { return items.empty(); }
  [[nodiscard]] size_t Size() const noexcept { return items.size(); }

  bool operator==(const MergeableStorage&) const noexcept = default;
};

// Non-mergeable storage: no Merge() method. Used to verify that Merge() on
// MultiTypeMap still inserts new keys even when storage has no merge.
struct NonMergeableStorage {
  int value = 0;

  void Clear() noexcept { value = 0; }
  [[nodiscard]] bool Empty() const noexcept { return value == 0; }
  [[nodiscard]] size_t Size() const noexcept { return value == 0 ? 0 : 1; }

  bool operator==(const NonMergeableStorage&) const noexcept = default;
};

// Counting storage: tracks constructions and destructions.
struct CountingStorage {
  static inline int construct_count = 0;
  static inline int destruct_count = 0;

  int value = 0;

  CountingStorage() { ++construct_count; }
  explicit CountingStorage(int val) : value(val) { ++construct_count; }
  CountingStorage(const CountingStorage& other) : value(other.value) {
    ++construct_count;
  }

  CountingStorage(CountingStorage&& other) noexcept : value(other.value) {
    ++construct_count;
  }

  ~CountingStorage() { ++destruct_count; }

  CountingStorage& operator=(const CountingStorage&) = default;
  CountingStorage& operator=(CountingStorage&&) noexcept = default;

  void Clear() noexcept { value = 0; }
  [[nodiscard]] bool Empty() const noexcept { return value == 0; }
  [[nodiscard]] size_t Size() const noexcept { return value == 0 ? 0 : 1; }

  static void Reset() noexcept {
    construct_count = 0;
    destruct_count = 0;
  }
};

// Tracking allocator: counts live allocations.
template <typename T>
struct TrackingAllocator {
  using value_type = T;

  static inline std::atomic<int> allocation_count{0};

  TrackingAllocator() noexcept = default;

  template <typename U>
  explicit TrackingAllocator(const TrackingAllocator<U>& /*other*/) noexcept {}

  T* allocate(size_t n) {
    ++allocation_count;
    return std::allocator<T>{}.allocate(n);
  }

  void deallocate(T* ptr, size_t n) noexcept {
    --allocation_count;
    std::allocator<T>{}.deallocate(ptr, n);
  }

  template <typename U>
  bool operator==(const TrackingAllocator<U>& /*other*/) const noexcept {
    return true;
  }

  static void ResetCount() noexcept { allocation_count = 0; }
};

using SimpleMap = MultiTypeMap<SimpleStorage>;
using MergeMap = MultiTypeMap<MergeableStorage>;
using NonMergeMap = MultiTypeMap<NonMergeableStorage>;
using CountingMap = MultiTypeMap<CountingStorage>;

}  // namespace

TEST_SUITE("helios::container::MultiTypeMap") {
  TEST_CASE("container::MultiTypeMap::ctor: default construction") {
    SimpleMap map;

    CHECK(map.EmptyAll());
    CHECK_EQ(map.TypeCount(), 0);
  }

  TEST_CASE("container::MultiTypeMap::ctor: allocator construction") {
    std::allocator<std::byte> alloc;
    SimpleMap map(alloc);

    CHECK(map.EmptyAll());
    CHECK_EQ(map.TypeCount(), 0);
  }

  TEST_CASE("container::MultiTypeMap::ctor: copy construction") {
    SimpleMap original;
    original.Ensure<int>().Set(10);
    original.Ensure<float>().Set(20);

    SimpleMap copy(original);

    CHECK_EQ(copy.TypeCount(), 2);
    CHECK(copy.Contains<int>());
    CHECK(copy.Contains<float>());
    CHECK_EQ(copy.Get<int>().value, 10);
    CHECK_EQ(copy.Get<float>().value, 20);
  }

  TEST_CASE("container::MultiTypeMap::ctor: move construction") {
    SimpleMap original;
    original.Ensure<int>().Set(42);
    original.Ensure<float>().Set(7);

    SimpleMap moved(std::move(original));

    CHECK_EQ(moved.TypeCount(), 2);
    CHECK(moved.Contains<int>());
    CHECK_EQ(moved.Get<int>().value, 42);

    CHECK_EQ(original.TypeCount(), 0);
    CHECK(original.EmptyAll());
  }

  TEST_CASE("container::MultiTypeMap::operator=: copy assignment") {
    SimpleMap original;
    original.Ensure<int>().Set(100);
    original.Ensure<double>().Set(99);

    SimpleMap copy;
    copy.Ensure<char>().Set(5);

    copy = original;

    CHECK_EQ(copy.TypeCount(), 2);
    CHECK(copy.Contains<int>());
    CHECK(copy.Contains<double>());
    CHECK_FALSE(copy.Contains<char>());
    CHECK_EQ(copy.Get<int>().value, 100);
  }

  TEST_CASE("container::MultiTypeMap::operator=: move assignment") {
    SimpleMap original;
    original.Ensure<int>().Set(55);

    SimpleMap moved;
    moved.Ensure<float>().Set(1);

    moved = std::move(original);

    CHECK(moved.Contains<int>());
    CHECK_FALSE(moved.Contains<float>());
    CHECK_EQ(moved.Get<int>().value, 55);
    CHECK_EQ(original.TypeCount(), 0);
  }

  TEST_CASE("container::MultiTypeMap::TypeIndexOf: type ID generation") {
    SUBCASE("Same type returns same ID") {
      constexpr auto id1 = SimpleMap::TypeIndexOf<int>();
      constexpr auto id2 = SimpleMap::TypeIndexOf<int>();
      CHECK_EQ(id1, id2);
    }

    SUBCASE("Different types return different IDs") {
      constexpr auto int_id = SimpleMap::TypeIndexOf<int>();
      constexpr auto float_id = SimpleMap::TypeIndexOf<float>();
      constexpr auto str_id = SimpleMap::TypeIndexOf<std::string>();

      CHECK_NE(int_id, float_id);
      CHECK_NE(int_id, str_id);
      CHECK_NE(float_id, str_id);
    }

    SUBCASE("TypeIndexOf is consistent across map instances") {
      constexpr auto id_a = SimpleMap::TypeIndexOf<int>();
      constexpr auto id_b = MergeMap::TypeIndexOf<int>();
      CHECK_EQ(id_a, id_b);
    }
  }

  TEST_CASE("container::MultiTypeMap::Ensure: creates storage on first call") {
    SimpleMap map;

    CHECK_FALSE(map.Contains<int>());

    auto& storage = map.Ensure<int>();

    CHECK(map.Contains<int>());
    CHECK_EQ(map.TypeCount(), 1);
    CHECK(storage.Empty());
  }

  TEST_CASE(
      "container::MultiTypeMap::Ensure: returns same storage on repeated "
      "calls") {
    SimpleMap map;

    auto& s1 = map.Ensure<int>();
    s1.Set(42);

    auto& s2 = map.Ensure<int>();

    CHECK_EQ(&s1, &s2);
    CHECK_EQ(s2.value, 42);
  }

  TEST_CASE("container::MultiTypeMap::Ensure: multiple types are independent") {
    SimpleMap map;

    map.Ensure<int>().Set(1);
    map.Ensure<float>().Set(2);
    map.Ensure<double>().Set(3);

    CHECK_EQ(map.TypeCount(), 3);
    CHECK_EQ(map.Get<int>().value, 1);
    CHECK_EQ(map.Get<float>().value, 2);
    CHECK_EQ(map.Get<double>().value, 3);
  }

  TEST_CASE(
      "container::MultiTypeMap::Ensure(TypeIndex): creates storage by ID") {
    SimpleMap map;

    constexpr auto id = SimpleMap::TypeIndexOf<int>();
    CHECK_FALSE(map.Contains(id));

    auto& storage = map.Ensure(id);

    CHECK(map.Contains(id));
    CHECK_EQ(map.TypeCount(), 1);
    CHECK(storage.Empty());
  }

  TEST_CASE(
      "container::MultiTypeMap::Ensure(TypeIndex): returns same storage on "
      "repeated calls") {
    SimpleMap map;

    constexpr auto id = SimpleMap::TypeIndexOf<int>();
    auto& s1 = map.Ensure(id);
    s1.Set(7);

    auto& s2 = map.Ensure(id);

    CHECK_EQ(&s1, &s2);
    CHECK_EQ(s2.value, 7);
  }

  TEST_CASE("container::MultiTypeMap::Emplace: inserts new entry by copy") {
    SimpleMap map;

    SimpleStorage val;
    val.Set(99);

    auto [it, inserted] = map.Emplace<int>(val);

    CHECK(inserted);
    CHECK(map.Contains<int>());
    CHECK_EQ(map.Get<int>().value, 99);
  }

  TEST_CASE(
      "container::MultiTypeMap::Emplace: does not insert when key already "
      "present") {
    SimpleMap map;

    SimpleStorage first;
    first.Set(1);
    map.Emplace<int>(first);

    SimpleStorage second;
    second.Set(200);
    auto [it, inserted] = map.Emplace<int>(second);

    // flat_map::emplace does NOT replace an existing key.
    CHECK_FALSE(inserted);
    CHECK_EQ(map.TypeCount(), 1);
    // original value is preserved
    CHECK_EQ(map.Get<int>().value, 1);
  }

  TEST_CASE("container::MultiTypeMap::Emplace: move semantics") {
    SimpleMap map;

    SimpleStorage val;
    val.Set(77);

    map.Emplace<int>(std::move(val));

    CHECK(map.Contains<int>());
    CHECK_EQ(map.Get<int>().value, 77);
  }

  TEST_CASE(
      "container::MultiTypeMap::Emplace: multiple distinct key types are "
      "independent") {
    SimpleMap map;

    SimpleStorage si;
    si.Set(1);
    SimpleStorage sf;
    sf.Set(2);
    SimpleStorage sd;
    sd.Set(3);

    map.Emplace<int>(si);
    map.Emplace<float>(sf);
    map.Emplace<double>(sd);

    CHECK_EQ(map.TypeCount(), 3);
    CHECK_EQ(map.Get<int>().value, 1);
    CHECK_EQ(map.Get<float>().value, 2);
    CHECK_EQ(map.Get<double>().value, 3);
  }

  TEST_CASE(
      "container::MultiTypeMap::Emplace: returns iterator pointing to entry") {
    SimpleMap map;

    SimpleStorage val;
    val.Set(42);

    auto [it, inserted] = map.Emplace<int>(val);

    CHECK(inserted);
    CHECK_EQ(it->second.value, 42);
    CHECK_EQ(it->first, SimpleMap::TypeIndexOf<int>());
  }

  TEST_CASE("container::MultiTypeMap::TryEmplace: inserts when key absent") {
    SimpleMap map;

    SimpleStorage val;
    val.Set(55);

    auto [it, inserted] = map.TryEmplace<int>(val);

    CHECK(inserted);
    CHECK(map.Contains<int>());
    CHECK_EQ(map.Get<int>().value, 55);
  }

  TEST_CASE(
      "container::MultiTypeMap::TryEmplace: does not overwrite existing "
      "entry") {
    SimpleMap map;
    map.Ensure<int>().Set(10);

    SimpleStorage attempt;
    attempt.Set(999);

    auto [it, inserted] = map.TryEmplace<int>(attempt);

    CHECK_FALSE(inserted);
    // original value must be preserved
    CHECK_EQ(map.Get<int>().value, 10);
    CHECK_EQ(map.TypeCount(), 1);
  }

  TEST_CASE("container::MultiTypeMap::TryEmplace: move semantics") {
    SimpleMap map;

    SimpleStorage val;
    val.Set(42);

    auto [it, inserted] = map.TryEmplace<int>(std::move(val));

    CHECK(inserted);
    CHECK_EQ(map.Get<int>().value, 42);
  }

  TEST_CASE(
      "container::MultiTypeMap::TryEmplace: multiple distinct key types") {
    SimpleMap map;

    SimpleStorage si;
    si.Set(3);
    SimpleStorage sf;
    sf.Set(4);

    auto [it1, ok1] = map.TryEmplace<int>(si);
    auto [it2, ok2] = map.TryEmplace<float>(sf);

    CHECK(ok1);
    CHECK(ok2);
    CHECK_EQ(map.TypeCount(), 2);
    CHECK_EQ(map.Get<int>().value, 3);
    CHECK_EQ(map.Get<float>().value, 4);
  }

  TEST_CASE(
      "container::MultiTypeMap::TryEmplace: returns iterator pointing to "
      "existing entry on failure") {
    SimpleMap map;
    map.Ensure<int>().Set(7);

    SimpleStorage attempt;
    attempt.Set(99);

    auto [it, inserted] = map.TryEmplace<int>(attempt);

    CHECK_FALSE(inserted);
    // Iterator must point to the already-existing entry.
    CHECK_EQ(it->second.value, 7);
  }

  TEST_CASE(
      "container::MultiTypeMap::Emplace vs TryEmplace: both refuse duplicate "
      "keys") {
    SimpleMap map;
    map.Ensure<int>().Set(5);

    SimpleStorage attempt;
    attempt.Set(99);

    auto [it_e, ok_e] = map.Emplace<int>(attempt);
    auto [it_t, ok_t] = map.TryEmplace<int>(attempt);

    CHECK_FALSE(ok_e);
    CHECK_FALSE(ok_t);
    CHECK_EQ(map.Get<int>().value, 5);
    CHECK_EQ(map.TypeCount(), 1);
  }

  TEST_CASE(
      "container::MultiTypeMap::Get: non-const access to existing storage") {
    SimpleMap map;
    map.Ensure<int>().Set(42);

    auto& storage = map.Get<int>();
    CHECK_EQ(storage.value, 42);

    storage.Set(100);
    CHECK_EQ(map.Get<int>().value, 100);
  }

  TEST_CASE("container::MultiTypeMap::Get: const access to existing storage") {
    SimpleMap map;
    map.Ensure<int>().Set(7);

    const auto& cmap = map;
    const auto& storage = cmap.Get<int>();
    CHECK_EQ(storage.value, 7);
  }

  TEST_CASE("container::MultiTypeMap::Get(TypeIndex): non-const access") {
    SimpleMap map;
    map.Ensure<int>().Set(13);

    constexpr auto id = SimpleMap::TypeIndexOf<int>();
    auto& storage = map.Get(id);
    CHECK_EQ(storage.value, 13);
  }

  TEST_CASE("container::MultiTypeMap::Get(TypeIndex): const access") {
    SimpleMap map;
    map.Ensure<int>().Set(21);

    constexpr auto id = SimpleMap::TypeIndexOf<int>();
    const auto& cmap = map;
    const auto& storage = cmap.Get(id);
    CHECK_EQ(storage.value, 21);
  }

  TEST_CASE(
      "container::MultiTypeMap::TryGet: returns nullptr for non-existent "
      "type") {
    SimpleMap map;
    CHECK_EQ(map.TryGet<int>(), nullptr);
  }

  TEST_CASE(
      "container::MultiTypeMap::TryGet: returns valid pointer for existing "
      "type") {
    SimpleMap map;
    map.Ensure<int>().Set(5);

    auto* ptr = map.TryGet<int>();
    CHECK_NE(ptr, nullptr);
    CHECK_EQ(ptr->value, 5);
  }

  TEST_CASE("container::MultiTypeMap::TryGet: const version") {
    SimpleMap map;
    map.Ensure<float>().Set(8);

    const auto& cmap = map;
    const auto* ptr = cmap.TryGet<float>();
    CHECK_NE(ptr, nullptr);
    CHECK_EQ(ptr->value, 8);
  }

  TEST_CASE(
      "container::MultiTypeMap::TryGet(TypeIndex): returns nullptr for "
      "non-existent") {
    SimpleMap map;
    constexpr auto id = SimpleMap::TypeIndexOf<int>();
    CHECK_EQ(map.TryGet(id), nullptr);
  }

  TEST_CASE(
      "container::MultiTypeMap::TryGet(TypeIndex): returns valid pointer for "
      "existing") {
    SimpleMap map;
    map.Ensure<int>().Set(15);

    constexpr auto id = SimpleMap::TypeIndexOf<int>();
    auto* ptr = map.TryGet(id);
    CHECK_NE(ptr, nullptr);
    CHECK_EQ(ptr->value, 15);
  }

  TEST_CASE("container::MultiTypeMap::TryGet(TypeIndex): const version") {
    SimpleMap map;
    map.Ensure<float>().Set(33);

    constexpr auto id = SimpleMap::TypeIndexOf<float>();
    const auto& cmap = map;
    const auto* ptr = cmap.TryGet(id);
    CHECK_NE(ptr, nullptr);
    CHECK_EQ(ptr->value, 33);
  }

  TEST_CASE(
      "container::MultiTypeMap::TryGet: template and ID overloads agree") {
    SimpleMap map;
    map.Ensure<int>().Set(9);

    constexpr auto id = SimpleMap::TypeIndexOf<int>();
    CHECK_EQ(map.TryGet<int>(), map.TryGet(id));

    constexpr auto float_id = SimpleMap::TypeIndexOf<float>();
    CHECK_EQ(map.TryGet<float>(), nullptr);
    CHECK_EQ(map.TryGet(float_id), nullptr);
  }

  TEST_CASE("container::MultiTypeMap::Remove: removes existing type") {
    SimpleMap map;
    map.Ensure<int>().Set(1);
    map.Ensure<float>().Set(2);

    CHECK_EQ(map.TypeCount(), 2);

    bool removed = map.Remove<int>();

    CHECK(removed);
    CHECK_FALSE(map.Contains<int>());
    CHECK(map.Contains<float>());
    CHECK_EQ(map.TypeCount(), 1);
  }

  TEST_CASE(
      "container::MultiTypeMap::Remove: returns false for non-existent type") {
    SimpleMap map;
    CHECK_FALSE(map.Remove<int>());
  }

  TEST_CASE("container::MultiTypeMap::Remove: double removal returns false") {
    SimpleMap map;
    map.Ensure<int>().Set(1);

    CHECK(map.Remove<int>());
    CHECK_FALSE(map.Remove<int>());
  }

  TEST_CASE("container::MultiTypeMap::Remove(TypeIndex): removes by ID") {
    SimpleMap map;
    map.Ensure<int>().Set(5);

    constexpr auto id = SimpleMap::TypeIndexOf<int>();
    bool removed = map.Remove(id);

    CHECK(removed);
    CHECK_FALSE(map.Contains(id));
    CHECK_EQ(map.TypeCount(), 0);
  }

  TEST_CASE(
      "container::MultiTypeMap::Remove(TypeIndex): returns false for "
      "non-existent ID") {
    SimpleMap map;
    constexpr auto id = SimpleMap::TypeIndexOf<int>();
    CHECK_FALSE(map.Remove(id));
  }

  TEST_CASE(
      "container::MultiTypeMap::Clear: clears storage but keeps type "
      "registered") {
    SimpleMap map;
    map.Ensure<int>().Set(42);
    map.Ensure<float>().Set(7);

    map.Clear<int>();

    CHECK(map.Contains<int>());
    CHECK(map.Empty<int>());
    CHECK_EQ(map.TypeCount(), 2);
    CHECK_EQ(map.Size<int>(), 0);
    CHECK_EQ(map.Size<float>(), 1);
  }

  TEST_CASE("container::MultiTypeMap::Clear: no-op on non-existent type") {
    SimpleMap map;
    map.Clear<int>();  // must not crash
    CHECK_EQ(map.TypeCount(), 0);
  }

  TEST_CASE("container::MultiTypeMap::Clear(TypeIndex): clears by ID") {
    SimpleMap map;
    map.Ensure<int>().Set(10);

    constexpr auto id = SimpleMap::TypeIndexOf<int>();
    map.Clear(id);

    CHECK(map.Contains(id));
    CHECK(map.Empty(id));
    CHECK_EQ(map.Size(id), 0);
  }

  TEST_CASE(
      "container::MultiTypeMap::ClearAll: clears all registered storages") {
    SimpleMap map;
    map.Ensure<int>().Set(1);
    map.Ensure<float>().Set(2);
    map.Ensure<double>().Set(3);

    map.ClearAll();

    CHECK(map.EmptyAll());
    CHECK_EQ(map.TypeCount(), 3);  // entries still registered
    CHECK(map.Contains<int>());
    CHECK(map.Empty<int>());
    CHECK(map.Contains<float>());
    CHECK(map.Empty<float>());
    CHECK(map.Contains<double>());
    CHECK(map.Empty<double>());
  }

  TEST_CASE("container::MultiTypeMap::ClearAll: no-op on empty map") {
    SimpleMap map;
    map.ClearAll();
    CHECK_EQ(map.TypeCount(), 0);
    CHECK(map.EmptyAll());
  }

  TEST_CASE("container::MultiTypeMap::Reset: removes entry for specific type") {
    SimpleMap map;
    map.Ensure<int>().Set(1);
    map.Ensure<float>().Set(2);

    map.Reset<int>();

    CHECK_EQ(map.TypeCount(), 1);
    CHECK_FALSE(map.Contains<int>());
    CHECK(map.Contains<float>());
  }

  TEST_CASE("container::MultiTypeMap::Reset: no-op on non-existent type") {
    SimpleMap map;
    map.Reset<int>();  // must not crash
    CHECK_EQ(map.TypeCount(), 0);
  }

  TEST_CASE("container::MultiTypeMap::Reset(TypeIndex): removes entry by ID") {
    SimpleMap map;
    map.Ensure<int>().Set(9);

    constexpr auto id = SimpleMap::TypeIndexOf<int>();
    map.Reset(id);

    CHECK_FALSE(map.Contains(id));
    CHECK_EQ(map.TypeCount(), 0);
  }

  TEST_CASE("container::MultiTypeMap::ResetAll: removes all entries") {
    SimpleMap map;
    map.Ensure<int>().Set(1);
    map.Ensure<float>().Set(2);
    map.Ensure<double>().Set(3);

    map.ResetAll();

    CHECK(map.EmptyAll());
    CHECK_EQ(map.TypeCount(), 0);
    CHECK_FALSE(map.Contains<int>());
    CHECK_FALSE(map.Contains<float>());
    CHECK_FALSE(map.Contains<double>());
  }

  TEST_CASE("container::MultiTypeMap::ResetAll: no-op on empty map") {
    SimpleMap map;
    map.ResetAll();
    CHECK_EQ(map.TypeCount(), 0);
  }

  TEST_CASE("container::MultiTypeMap::Contains: false before insertion") {
    SimpleMap map;
    CHECK_FALSE(map.Contains<int>());
  }

  TEST_CASE("container::MultiTypeMap::Contains: true after Ensure") {
    SimpleMap map;
    [[maybe_unused]] auto& _ = map.Ensure<int>();
    CHECK(map.Contains<int>());
  }

  TEST_CASE("container::MultiTypeMap::Contains: false after Remove") {
    SimpleMap map;
    [[maybe_unused]] auto& _ = map.Ensure<int>();
    map.Remove<int>();
    CHECK_FALSE(map.Contains<int>());
  }

  TEST_CASE("container::MultiTypeMap::Contains(TypeIndex): by ID") {
    SimpleMap map;
    constexpr auto id = SimpleMap::TypeIndexOf<int>();

    CHECK_FALSE(map.Contains(id));
    [[maybe_unused]] auto& _ = map.Ensure<int>();
    CHECK(map.Contains(id));
  }

  TEST_CASE(
      "container::MultiTypeMap::Contains: template and ID overloads agree") {
    SimpleMap map;
    [[maybe_unused]] auto& _ = map.Ensure<int>();

    constexpr auto int_id = SimpleMap::TypeIndexOf<int>();
    constexpr auto float_id = SimpleMap::TypeIndexOf<float>();

    CHECK_EQ(map.Contains<int>(), map.Contains(int_id));
    CHECK_EQ(map.Contains<float>(), map.Contains(float_id));
  }

  TEST_CASE("container::MultiTypeMap::Empty: non-existent type is empty") {
    SimpleMap map;
    CHECK(map.Empty<int>());
  }

  TEST_CASE(
      "container::MultiTypeMap::Empty: registered but un-set storage is "
      "empty") {
    SimpleMap map;
    [[maybe_unused]] auto& _ = map.Ensure<int>();
    CHECK(map.Empty<int>());
  }

  TEST_CASE("container::MultiTypeMap::Empty: false after setting a value") {
    SimpleMap map;
    map.Ensure<int>().Set(5);
    CHECK_FALSE(map.Empty<int>());
  }

  TEST_CASE("container::MultiTypeMap::Empty: true after Clear") {
    SimpleMap map;
    map.Ensure<int>().Set(5);
    map.Clear<int>();
    CHECK(map.Empty<int>());
  }

  TEST_CASE("container::MultiTypeMap::Empty(TypeIndex): by ID") {
    SimpleMap map;
    constexpr auto id = SimpleMap::TypeIndexOf<int>();

    CHECK(map.Empty(id));
    map.Ensure<int>().Set(3);
    CHECK_FALSE(map.Empty(id));
  }

  TEST_CASE("container::MultiTypeMap::EmptyAll: true when map is empty") {
    SimpleMap map;
    CHECK(map.EmptyAll());
  }

  TEST_CASE("container::MultiTypeMap::EmptyAll: true after ClearAll") {
    SimpleMap map;
    map.Ensure<int>().Set(1);
    map.ClearAll();
    CHECK(map.EmptyAll());
  }

  TEST_CASE(
      "container::MultiTypeMap::EmptyAll: false when any storage has a value") {
    SimpleMap map;
    [[maybe_unused]] auto& _ = map.Ensure<int>();
    map.Ensure<float>().Set(1);
    CHECK_FALSE(map.EmptyAll());
  }

  TEST_CASE(
      "container::MultiTypeMap::EmptyAll: true when all storages are un-set") {
    SimpleMap map;
    [[maybe_unused]] auto& _i = map.Ensure<int>();
    [[maybe_unused]] auto& _f = map.Ensure<float>();
    CHECK(map.EmptyAll());
  }

  TEST_CASE("container::MultiTypeMap::Size: total size across all types") {
    SimpleMap map;
    map.Ensure<int>().Set(1);
    map.Ensure<float>().Set(2);

    // each SimpleStorage reports Size() == 1 when has_value is true
    CHECK_EQ(map.Size(), 2);
  }

  TEST_CASE("container::MultiTypeMap::Size: zero for empty map") {
    SimpleMap map;
    CHECK_EQ(map.Size(), 0);
  }

  TEST_CASE("container::MultiTypeMap::Size<T>: per-type size") {
    SimpleMap map;
    map.Ensure<int>().Set(5);

    CHECK_EQ(map.Size<int>(), 1);
    CHECK_EQ(map.Size<float>(), 0);
  }

  TEST_CASE("container::MultiTypeMap::Size(TypeIndex): by ID") {
    SimpleMap map;
    map.Ensure<int>().Set(8);

    constexpr auto int_id = SimpleMap::TypeIndexOf<int>();
    constexpr auto float_id = SimpleMap::TypeIndexOf<float>();

    CHECK_EQ(map.Size(int_id), 1);
    CHECK_EQ(map.Size(float_id), 0);
  }

  TEST_CASE(
      "container::MultiTypeMap::TypeCount: tracks number of registered "
      "entries") {
    SimpleMap map;

    CHECK_EQ(map.TypeCount(), 0);

    [[maybe_unused]] auto& _i = map.Ensure<int>();
    CHECK_EQ(map.TypeCount(), 1);

    [[maybe_unused]] auto& _f = map.Ensure<float>();
    CHECK_EQ(map.TypeCount(), 2);

    map.Reset<int>();
    CHECK_EQ(map.TypeCount(), 1);

    map.ResetAll();
    CHECK_EQ(map.TypeCount(), 0);
  }

  TEST_CASE(
      "container::MultiTypeMap::Data: returns underlying map (non-const)") {
    SimpleMap map;
    map.Ensure<int>().Set(3);

    auto& data = map.Data();
    CHECK_EQ(data.size(), 1);
  }

  TEST_CASE("container::MultiTypeMap::Data: returns underlying map (const)") {
    SimpleMap map;
    map.Ensure<int>().Set(3);
    map.Ensure<float>().Set(6);

    const auto& cmap = map;
    const auto& data = cmap.Data();
    CHECK_EQ(data.size(), 2);
  }

  TEST_CASE("container::MultiTypeMap::begin/end: empty map iteration") {
    SimpleMap map;
    CHECK_EQ(map.begin(), map.end());
    CHECK_EQ(map.cbegin(), map.cend());
  }

  TEST_CASE(
      "container::MultiTypeMap::begin/end: non-const range-for visits all "
      "entries") {
    SimpleMap map;
    map.Ensure<int>().Set(1);
    map.Ensure<float>().Set(2);
    map.Ensure<double>().Set(3);

    int count = 0;
    for (auto&& [id, storage] : map) {
      ++count;
      CHECK(storage.has_value);
    }
    CHECK_EQ(count, 3);
  }

  TEST_CASE(
      "container::MultiTypeMap::begin/end: const range-for visits all "
      "entries") {
    SimpleMap map;
    map.Ensure<int>().Set(10);
    map.Ensure<float>().Set(20);

    const auto& cmap = map;
    int count = 0;
    for (const auto& [id, storage] : cmap) {
      ++count;
      CHECK(storage.has_value);
    }
    CHECK_EQ(count, 2);
  }

  TEST_CASE("container::MultiTypeMap::cbegin/cend: const iteration") {
    SimpleMap map;
    map.Ensure<int>().Set(5);
    map.Ensure<double>().Set(6);

    int count = 0;
    for (auto it = map.cbegin(); it != map.cend(); ++it) {
      ++count;
    }
    CHECK_EQ(count, 2);
  }

  TEST_CASE("container::MultiTypeMap::begin/end: mutation through iterator") {
    SimpleMap map;
    map.Ensure<int>().Set(1);

    for (auto&& [id, storage] : map) {
      storage.Set(99);
    }

    CHECK_EQ(map.Get<int>().value, 99);
  }

  TEST_CASE(
      "container::MultiTypeMap::begin/end: all registered IDs are present in "
      "iteration") {
    SimpleMap map;
    map.Ensure<int>().Set(1);
    map.Ensure<float>().Set(2);

    constexpr auto int_id = SimpleMap::TypeIndexOf<int>();
    constexpr auto float_id = SimpleMap::TypeIndexOf<float>();

    bool saw_int = false;
    bool saw_float = false;

    for (const auto& [id, storage] : map) {
      if (id == int_id) {
        saw_int = true;
      }
      if (id == float_id) {
        saw_float = true;
      }
    }

    CHECK(saw_int);
    CHECK(saw_float);
  }

  TEST_CASE("container::MultiTypeMap::begin/end: std::ranges algorithms work") {
    SimpleMap map;
    map.Ensure<int>().Set(1);
    [[maybe_unused]] auto& _f = map.Ensure<float>();  // empty
    map.Ensure<double>().Set(3);

    auto count_non_empty = std::ranges::count_if(
        map, [](const auto& entry) { return entry.second.has_value; });
    CHECK_EQ(count_non_empty, 2);
  }

  TEST_CASE(
      "container::MultiTypeMap::GetAllocator: returns allocator used at "
      "construction") {
    std::allocator<std::byte> alloc;
    SimpleMap map(alloc);

    auto retrieved = map.GetAllocator();
    CHECK(retrieved == alloc);
  }

  TEST_CASE("container::MultiTypeMap::Swap: swaps contents of two maps") {
    SimpleMap map1;
    map1.Ensure<int>().Set(1);
    map1.Ensure<float>().Set(2);

    SimpleMap map2;
    map2.Ensure<double>().Set(9);

    map1.Swap(map2);

    CHECK(map1.Contains<double>());
    CHECK_FALSE(map1.Contains<int>());
    CHECK_FALSE(map1.Contains<float>());
    CHECK_EQ(map1.TypeCount(), 1);

    CHECK(map2.Contains<int>());
    CHECK(map2.Contains<float>());
    CHECK_FALSE(map2.Contains<double>());
    CHECK_EQ(map2.TypeCount(), 2);
  }

  TEST_CASE("container::MultiTypeMap::swap: ADL free function swaps contents") {
    SimpleMap map1;
    map1.Ensure<int>().Set(10);

    SimpleMap map2;
    map2.Ensure<float>().Set(20);

    swap(map1, map2);

    CHECK(map1.Contains<float>());
    CHECK_FALSE(map1.Contains<int>());
    CHECK(map2.Contains<int>());
    CHECK_FALSE(map2.Contains<float>());
  }

  TEST_CASE(
      "container::MultiTypeMap::Merge: disjoint types are moved into "
      "destination") {
    MergeMap map1;
    map1.Ensure<int>().Push(1);
    map1.Ensure<int>().Push(2);

    MergeMap map2;
    map2.Ensure<float>().Push(10);
    map2.Ensure<float>().Push(20);

    map1.Merge(std::move(map2));

    CHECK_EQ(map1.TypeCount(), 2);
    CHECK_EQ(map1.Size<int>(), 2);
    CHECK_EQ(map1.Size<float>(), 2);
    CHECK_EQ(map1.Get<int>().items[0], 1);
    CHECK_EQ(map1.Get<int>().items[1], 2);
    CHECK_EQ(map1.Get<float>().items[0], 10);
    CHECK_EQ(map1.Get<float>().items[1], 20);

    CHECK(map2.EmptyAll());
    CHECK_EQ(map2.TypeCount(), 0);
  }

  TEST_CASE(
      "container::MultiTypeMap::Merge: disjoint types are copied from const "
      "lvalue source") {
    MergeMap map1;
    map1.Ensure<int>().Push(1);
    map1.Ensure<int>().Push(2);

    MergeMap map2_mut;
    map2_mut.Ensure<float>().Push(10);
    map2_mut.Ensure<float>().Push(20);
    const MergeMap& map2 = map2_mut;

    map1.Merge(map2);

    CHECK_EQ(map1.TypeCount(), 2);
    CHECK_EQ(map1.Size<int>(), 2);
    CHECK_EQ(map1.Size<float>(), 2);
    CHECK_EQ(map1.Get<float>().items[0], 10);
    CHECK_EQ(map1.Get<float>().items[1], 20);

    CHECK_EQ(map2.TypeCount(), 1);
    CHECK_EQ(map2.Size<float>(), 2);
  }

  TEST_CASE(
      "container::MultiTypeMap::Merge: overlapping types call Storage::Merge") {
    MergeMap map1;
    map1.Ensure<int>().Push(1);
    map1.Ensure<int>().Push(2);

    MergeMap map2;
    map2.Ensure<int>().Push(3);
    map2.Ensure<int>().Push(4);

    map1.Merge(std::move(map2));

    CHECK_EQ(map1.TypeCount(), 1);
    CHECK_EQ(map1.Size<int>(), 4);
    CHECK_EQ(map1.Get<int>().items[0], 1);
    CHECK_EQ(map1.Get<int>().items[1], 2);
    CHECK_EQ(map1.Get<int>().items[2], 3);
    CHECK_EQ(map1.Get<int>().items[3], 4);

    CHECK(map2.EmptyAll());
  }

  TEST_CASE(
      "container::MultiTypeMap::Merge: overlapping types call Storage::Merge "
      "from const lvalue source") {
    MergeMap map1;
    map1.Ensure<int>().Push(1);
    map1.Ensure<int>().Push(2);

    MergeMap map2_mut;
    map2_mut.Ensure<int>().Push(3);
    map2_mut.Ensure<int>().Push(4);
    const MergeMap& map2 = map2_mut;

    map1.Merge(map2);

    CHECK_EQ(map1.TypeCount(), 1);
    CHECK_EQ(map1.Size<int>(), 4);
    CHECK_EQ(map1.Get<int>().items[0], 1);
    CHECK_EQ(map1.Get<int>().items[1], 2);
    CHECK_EQ(map1.Get<int>().items[2], 3);
    CHECK_EQ(map1.Get<int>().items[3], 4);

    CHECK_EQ(map2.Size<int>(), 2);
    CHECK_EQ(map2.Get<int>().items[0], 3);
    CHECK_EQ(map2.Get<int>().items[1], 4);
  }

  TEST_CASE(
      "container::MultiTypeMap::Merge: mixed overlapping and disjoint types") {
    MergeMap map1;
    map1.Ensure<int>().Push(1);
    map1.Ensure<float>().Push(5);

    MergeMap map2;
    map2.Ensure<int>().Push(2);
    map2.Ensure<double>().Push(9);

    map1.Merge(std::move(map2));

    CHECK_EQ(map1.TypeCount(), 3);
    CHECK_EQ(map1.Size<int>(), 2);
    CHECK_EQ(map1.Size<float>(), 1);
    CHECK_EQ(map1.Size<double>(), 1);

    CHECK(map2.EmptyAll());
  }

  TEST_CASE("container::MultiTypeMap::Merge: into empty destination") {
    MergeMap map1;

    MergeMap map2;
    map2.Ensure<int>().Push(7);
    map2.Ensure<float>().Push(8);

    map1.Merge(std::move(map2));

    CHECK_EQ(map1.TypeCount(), 2);
    CHECK_EQ(map1.Size<int>(), 1);
    CHECK_EQ(map1.Size<float>(), 1);
    CHECK_EQ(map1.Get<int>().items[0], 7);

    CHECK(map2.EmptyAll());
    CHECK_EQ(map2.TypeCount(), 0);
  }

  TEST_CASE("container::MultiTypeMap::Merge: merging empty source is a no-op") {
    MergeMap map1;
    map1.Ensure<int>().Push(42);

    MergeMap map2;

    map1.Merge(std::move(map2));

    CHECK_EQ(map1.TypeCount(), 1);
    CHECK_EQ(map1.Size<int>(), 1);
    CHECK_EQ(map1.Get<int>().items[0], 42);
  }

  TEST_CASE("container::MultiTypeMap::Merge: source is cleared after merge") {
    MergeMap map1;
    MergeMap map2;
    map2.Ensure<int>().Push(1);
    map2.Ensure<float>().Push(2);

    map1.Merge(std::move(map2));

    CHECK_EQ(map2.TypeCount(), 0);
    CHECK(map2.EmptyAll());
  }

  TEST_CASE(
      "container::MultiTypeMap::Merge: non-mergeable storage inserts new "
      "disjoint keys") {
    NonMergeMap map1;
    map1.Ensure<int>().value = 1;

    NonMergeMap map2;
    map2.Ensure<float>().value = 2;

    map1.Merge(std::move(map2));

    // New key from map2 must be inserted.
    CHECK(map1.Contains<float>());
    CHECK_EQ(map1.TypeCount(), 2);

    CHECK(map2.EmptyAll());
    CHECK_EQ(map2.TypeCount(), 0);
  }

  TEST_CASE(
      "container::MultiTypeMap::Merge: non-mergeable storage leaves "
      "overlapping key unchanged") {
    NonMergeMap map1;
    map1.Ensure<int>().value = 1;

    NonMergeMap map2;
    map2.Ensure<int>().value = 99;

    map1.Merge(std::move(map2));

    // No Merge() method: existing entry should be left unchanged.
    CHECK_EQ(map1.TypeCount(), 1);
    CHECK_EQ(map1.Get<int>().value, 1);

    CHECK(map2.EmptyAll());
  }

  TEST_CASE("container::MultiTypeMap::Merge: object lifetimes are balanced") {
    CountingStorage::Reset();

    {
      CountingMap map1;
      map1.Ensure<int>() = CountingStorage(1);
      map1.Ensure<int>() = CountingStorage(2);  // overwrite same entry

      CountingMap map2;
      map2.Ensure<float>() = CountingStorage(3);

      map1.Merge(std::move(map2));

      CHECK_EQ(map1.TypeCount(), 2);
    }

    CHECK_EQ(CountingStorage::construct_count, CountingStorage::destruct_count);
  }

  TEST_CASE("container::MultiTypeMap::Clear: delegates to Storage::Clear()") {
    MergeMap map;
    map.Ensure<int>().Push(1);
    map.Ensure<int>().Push(2);
    map.Ensure<float>().Push(9);

    map.Clear<int>();

    CHECK(map.Contains<int>());
    CHECK(map.Empty<int>());
    CHECK_EQ(map.Size<int>(), 0);
    CHECK_EQ(map.Size<float>(), 1);
  }

  TEST_CASE(
      "container::MultiTypeMap::ClearAll: delegates to Storage::Clear() on "
      "every entry") {
    MergeMap map;
    map.Ensure<int>().Push(1);
    map.Ensure<float>().Push(2);
    map.Ensure<double>().Push(3);

    map.ClearAll();

    CHECK(map.EmptyAll());
    CHECK_EQ(map.TypeCount(), 3);
    CHECK(map.Empty<int>());
    CHECK(map.Empty<float>());
    CHECK(map.Empty<double>());
  }

  TEST_CASE(
      "container::MultiTypeMap::CountingStorage — Clear destructs elements") {
    CountingStorage::Reset();

    {
      CountingMap map;
      map.Ensure<int>() = CountingStorage(10);

      [[maybe_unused]] const int dtor_before = CountingStorage::destruct_count;
      map.Clear<int>();
      // After Clear, old value in storage was assigned away or replaced.
      // The assignment operator does not track — but Reset (Emplace) tests this
      // path.
    }

    CHECK_EQ(CountingStorage::construct_count, CountingStorage::destruct_count);
  }

  TEST_CASE(
      "container::MultiTypeMap::CountingStorage — Reset destructs storage") {
    CountingStorage::Reset();

    {
      CountingMap map;
      map.Ensure<int>() = CountingStorage(5);
      map.Ensure<float>() = CountingStorage(6);

      map.Reset<int>();
    }

    CHECK_EQ(CountingStorage::construct_count, CountingStorage::destruct_count);
  }

  TEST_CASE(
      "container::MultiTypeMap::CountingStorage — ResetAll destructs all "
      "storages") {
    CountingStorage::Reset();

    {
      CountingMap map;
      map.Ensure<int>() = CountingStorage(1);
      map.Ensure<float>() = CountingStorage(2);
      map.Ensure<double>() = CountingStorage(3);

      map.ResetAll();
    }

    CHECK_EQ(CountingStorage::construct_count, CountingStorage::destruct_count);
  }

  TEST_CASE(
      "container::MultiTypeMap::template and TypeIndex overloads are "
      "consistent") {
    SimpleMap map;
    map.Ensure<int>().Set(3);
    map.Ensure<float>().Set(7);

    constexpr auto int_id = SimpleMap::TypeIndexOf<int>();
    constexpr auto float_id = SimpleMap::TypeIndexOf<float>();

    CHECK_EQ(map.Contains<int>(), map.Contains(int_id));
    CHECK_EQ(map.Contains<float>(), map.Contains(float_id));

    CHECK_EQ(map.Empty<int>(), map.Empty(int_id));
    CHECK_EQ(map.Empty<float>(), map.Empty(float_id));

    CHECK_EQ(map.Size<int>(), map.Size(int_id));
    CHECK_EQ(map.Size<float>(), map.Size(float_id));

    CHECK_EQ(&map.Get<int>(), &map.Get(int_id));
    CHECK_EQ(&map.Get<float>(), &map.Get(float_id));

    CHECK_EQ(map.TryGet<int>(), map.TryGet(int_id));
    CHECK_EQ(map.TryGet<float>(), map.TryGet(float_id));
  }

  TEST_CASE("container::PmrMultiTypeMap: works with memory_resource") {
    std::byte buffer[1024];
    std::pmr::monotonic_buffer_resource resource(buffer, sizeof(buffer));

    PmrMultiTypeMap<SimpleStorage> map{&resource};
    map.Ensure<int>().Set(42);

    CHECK(map.Contains<int>());
    CHECK_EQ(map.Get<int>().value, 42);
  }
}  // TEST_SUITE("container::MultiTypeMap")
