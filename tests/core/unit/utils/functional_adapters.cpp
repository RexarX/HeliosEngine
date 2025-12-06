#include <doctest/doctest.h>

#include <helios/core/utils/functional_adapters.hpp>

#include <algorithm>
#include <string>
#include <tuple>
#include <vector>

using namespace helios::utils;

namespace {

// Simple test iterator that wraps a vector iterator
template <typename T>
class VectorIterator {
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = T;
  using difference_type = ptrdiff_t;
  using pointer = value_type*;
  using reference = value_type&;

  constexpr VectorIterator() = default;
  constexpr explicit VectorIterator(typename std::vector<T>::const_iterator iter) : iter_(iter) {}

  constexpr VectorIterator& operator++() {
    ++iter_;
    return *this;
  }

  constexpr VectorIterator operator++(int) {
    auto temp = *this;
    ++iter_;
    return temp;
  }

  constexpr value_type operator*() const { return *iter_; }

  constexpr bool operator==(const VectorIterator& other) const { return iter_ == other.iter_; }
  constexpr bool operator!=(const VectorIterator& other) const { return iter_ != other.iter_; }

private:
  typename std::vector<T>::const_iterator iter_;
};

// Test iterator that returns tuples
template <typename... Types>
class TupleIterator {
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = std::tuple<Types...>;
  using difference_type = ptrdiff_t;
  using pointer = value_type*;
  using reference = value_type&;

  constexpr TupleIterator() = default;
  constexpr explicit TupleIterator(typename std::vector<value_type>::const_iterator iter) : iter_(iter) {}

  constexpr TupleIterator& operator++() {
    ++iter_;
    return *this;
  }

  constexpr TupleIterator operator++(int) {
    auto temp = *this;
    ++iter_;
    return temp;
  }

  constexpr value_type operator*() const { return *iter_; }

  constexpr bool operator==(const TupleIterator& other) const { return iter_ == other.iter_; }
  constexpr bool operator!=(const TupleIterator& other) const { return iter_ != other.iter_; }

private:
  typename std::vector<value_type>::const_iterator iter_;
};

}  // namespace

TEST_SUITE("utils::FunctionalAdapters") {
  TEST_CASE("FilterAdapter: basic filtering") {
    std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto begin_iter = VectorIterator<int>(data.begin());
    auto end_iter = VectorIterator<int>(data.end());

    SUBCASE("Filter even numbers") {
      auto filtered = FilterAdapter(begin_iter, end_iter, [](int value) { return value % 2 == 0; });

      std::vector<int> result;
      for (auto iter = filtered.begin(); iter != filtered.end(); ++iter) {
        result.push_back(*iter);
      }

      CHECK_EQ(result.size(), 5);
      CHECK_EQ(result, std::vector<int>{2, 4, 6, 8, 10});
    }

    SUBCASE("Range ctor: Filter even numbers") {
      auto filtered = FilterAdapterFromRange(data, [](int value) { return value % 2 == 0; });

      std::vector<int> result;
      for (auto value : filtered) {
        result.push_back(value);
      }

      CHECK_EQ(result.size(), 5);
      CHECK_EQ(result, std::vector<int>{2, 4, 6, 8, 10});
    }

    SUBCASE("Filter odd numbers") {
      auto filtered = FilterAdapter(begin_iter, end_iter, [](int value) { return value % 2 != 0; });

      std::vector<int> result;
      for (auto value : filtered) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{1, 3, 5, 7, 9});
    }

    SUBCASE("Filter greater than 5") {
      auto filtered = FilterAdapter(begin_iter, end_iter, [](int value) { return value > 5; });

      int count = 0;
      for ([[maybe_unused]] auto value : filtered) {
        ++count;
      }

      CHECK_EQ(count, 5);
    }

    SUBCASE("Empty result") {
      auto filtered = FilterAdapter(begin_iter, end_iter, [](int /*value*/) { return false; });

      int count = 0;
      for ([[maybe_unused]] auto value : filtered) {
        ++count;
      }

      CHECK_EQ(count, 0);
    }
  }

  TEST_CASE("FilterAdapter: chained filtering") {
    std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    auto begin_iter = VectorIterator<int>(data.begin());
    auto end_iter = VectorIterator<int>(data.end());

    auto filtered = FilterAdapter(begin_iter, end_iter, [](int value) { return value > 3; }).Filter([](int value) {
      return value % 2 == 0;
    });

    std::vector<int> result;
    for (auto value : filtered) {
      result.push_back(value);
    }

    CHECK_EQ(result, std::vector<int>{4, 6, 8, 10, 12});
  }

  TEST_CASE("MapAdapter: basic transformation") {
    std::vector<int> data = {1, 2, 3, 4, 5};
    auto begin_iter = VectorIterator<int>(data.begin());
    auto end_iter = VectorIterator<int>(data.end());

    SUBCASE("Double values") {
      auto mapped = MapAdapter(begin_iter, end_iter, [](int value) { return value * 2; });

      std::vector<int> result;
      for (auto value : mapped) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{2, 4, 6, 8, 10});
    }

    SUBCASE("Range ctor: Double values") {
      auto mapped = MapAdapterFromRange(data, [](int value) { return value * 2; });

      std::vector<int> result;
      for (auto value : mapped) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{2, 4, 6, 8, 10});
    }

    SUBCASE("Convert to string") {
      auto mapped = MapAdapter(begin_iter, end_iter, [](int value) { return std::to_string(value); });

      std::vector<std::string> result;
      for (auto value : mapped) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<std::string>{"1", "2", "3", "4", "5"});
    }

    SUBCASE("Range ctor: Convert to string") {
      auto mapped = MapAdapterFromRange(data, [](int value) { return std::to_string(value); });

      std::vector<std::string> result;
      for (auto value : mapped) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<std::string>{"1", "2", "3", "4", "5"});
    }
  }

  TEST_CASE("MapAdapter: tuple unpacking") {
    std::vector<std::tuple<int, int>> data = {{1, 10}, {2, 20}, {3, 30}};
    auto begin_iter = TupleIterator<int, int>(data.begin());
    auto end_iter = TupleIterator<int, int>(data.end());

    auto mapped = MapAdapter(begin_iter, end_iter, [](int first, int second) { return first + second; });

    std::vector<int> result;
    for (auto value : mapped) {
      result.push_back(value);
    }

    CHECK_EQ(result, std::vector<int>{11, 22, 33});
  }

  TEST_CASE("FilterAdapter: tuple unpacking") {
    std::vector<std::tuple<int, int>> data = {{1, 10}, {2, 20}, {3, 30}, {4, 40}, {5, 50}};
    auto begin_iter = TupleIterator<int, int>(data.begin());
    auto end_iter = TupleIterator<int, int>(data.end());

    SUBCASE("Filter with tuple unpacking") {
      auto filtered = FilterAdapter(begin_iter, end_iter, [](int first, int second) { return first + second > 25; });

      std::vector<std::tuple<int, int>> result;
      for (auto value : filtered) {
        result.push_back(value);
      }

      CHECK_EQ(result.size(), 3);
      CHECK_EQ(result, std::vector<std::tuple<int, int>>{{3, 30}, {4, 40}, {5, 50}});
    }

    SUBCASE("Filter with full tuple") {
      auto filtered =
          FilterAdapter(begin_iter, end_iter, [](const std::tuple<int, int>& t) { return std::get<0>(t) % 2 == 0; });

      std::vector<std::tuple<int, int>> result;
      for (auto value : filtered) {
        result.push_back(value);
      }

      CHECK_EQ(result.size(), 2);
      CHECK_EQ(result, std::vector<std::tuple<int, int>>{{2, 20}, {4, 40}});
    }
  }

  TEST_CASE("TakeWhileAdapter: tuple unpacking") {
    std::vector<std::tuple<int, int>> data = {{1, 10}, {2, 20}, {3, 30}, {4, 40}, {5, 50}};
    auto begin_iter = TupleIterator<int, int>(data.begin());
    auto end_iter = TupleIterator<int, int>(data.end());

    SUBCASE("TakeWhile with tuple unpacking") {
      auto taken = TakeWhileAdapter(begin_iter, end_iter, [](int first, int second) { return first + second < 35; });

      std::vector<std::tuple<int, int>> result;
      for (auto value : taken) {
        result.push_back(value);
      }

      CHECK_EQ(result.size(), 3);
      CHECK_EQ(result, std::vector<std::tuple<int, int>>{{1, 10}, {2, 20}, {3, 30}});
    }
  }

  TEST_CASE("SkipWhileAdapter: tuple unpacking") {
    std::vector<std::tuple<int, int>> data = {{1, 10}, {2, 20}, {3, 30}, {4, 40}, {5, 50}};
    auto begin_iter = TupleIterator<int, int>(data.begin());
    auto end_iter = TupleIterator<int, int>(data.end());

    SUBCASE("SkipWhile with tuple unpacking") {
      auto skipped = SkipWhileAdapter(begin_iter, end_iter, [](int first, int second) { return first + second < 35; });

      std::vector<std::tuple<int, int>> result;
      for (auto value : skipped) {
        result.push_back(value);
      }

      CHECK_EQ(result.size(), 2);
      CHECK_EQ(result, std::vector<std::tuple<int, int>>{{4, 40}, {5, 50}});
    }
  }

  TEST_CASE("InspectAdapter: tuple unpacking") {
    std::vector<std::tuple<int, int>> data = {{1, 10}, {2, 20}, {3, 30}};
    auto begin_iter = TupleIterator<int, int>(data.begin());
    auto end_iter = TupleIterator<int, int>(data.end());

    SUBCASE("Inspect with tuple unpacking") {
      int sum_first = 0;
      int sum_second = 0;
      auto inspected = InspectAdapter(begin_iter, end_iter, [&sum_first, &sum_second](int first, int second) {
        sum_first += first;
        sum_second += second;
      });

      std::vector<std::tuple<int, int>> result;
      for (auto value : inspected) {
        result.push_back(value);
      }

      CHECK_EQ(result, data);
      CHECK_EQ(sum_first, 6);
      CHECK_EQ(sum_second, 60);
    }
  }

  TEST_CASE("TakeAdapter: limit elements") {
    std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto begin_iter = VectorIterator<int>(data.begin());
    auto end_iter = VectorIterator<int>(data.end());

    SUBCASE("Take 5 elements") {
      auto taken = TakeAdapter(begin_iter, end_iter, 5);

      std::vector<int> result;
      for (auto value : taken) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{1, 2, 3, 4, 5});
    }

    SUBCASE("Range ctor: Take 5 elements") {
      auto taken = TakeAdapterFromRange(data, 5);

      std::vector<int> result;
      for (auto value : taken) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{1, 2, 3, 4, 5});
    }

    SUBCASE("Take more than available") {
      auto taken = TakeAdapter(begin_iter, end_iter, 20);

      std::vector<int> result;
      for (auto value : taken) {
        result.push_back(value);
      }

      CHECK_EQ(result.size(), 10);
    }

    SUBCASE("Range ctor: Take more than available") {
      auto taken = TakeAdapterFromRange(data, 20);

      std::vector<int> result;
      for (auto value : taken) {
        result.push_back(value);
      }

      CHECK_EQ(result.size(), 10);
    }

    SUBCASE("Take zero") {
      auto taken = TakeAdapter(begin_iter, end_iter, 0);

      int count = 0;
      for ([[maybe_unused]] auto value : taken) {
        ++count;
      }

      CHECK_EQ(count, 0);
    }

    SUBCASE("Range ctor: Take zero") {
      auto taken = TakeAdapterFromRange(data, 0);

      int count = 0;
      for ([[maybe_unused]] auto value : taken) {
        ++count;
      }

      CHECK_EQ(count, 0);
    }
  }

  TEST_CASE("SkipAdapter: skip elements") {
    std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto begin_iter = VectorIterator<int>(data.begin());
    auto end_iter = VectorIterator<int>(data.end());

    SUBCASE("Skip 3 elements") {
      auto skipped = SkipAdapter(begin_iter, end_iter, 3);

      std::vector<int> result;
      for (auto value : skipped) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{4, 5, 6, 7, 8, 9, 10});
    }

    SUBCASE("Range ctor: Skip 3 elements") {
      auto skipped = SkipAdapterFromRange(data, 3);

      std::vector<int> result;
      for (auto value : skipped) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{4, 5, 6, 7, 8, 9, 10});
    }

    SUBCASE("Skip all") {
      auto skipped = SkipAdapter(begin_iter, end_iter, 10);

      int count = 0;
      for ([[maybe_unused]] auto value : skipped) {
        ++count;
      }

      CHECK_EQ(count, 0);
    }

    SUBCASE("Range ctor: Skip all") {
      auto skipped = SkipAdapterFromRange(data, 10);

      int count = 0;
      for ([[maybe_unused]] auto value : skipped) {
        ++count;
      }

      CHECK_EQ(count, 0);
    }

    SUBCASE("Skip more than available") {
      auto skipped = SkipAdapter(begin_iter, end_iter, 20);

      int count = 0;
      for ([[maybe_unused]] auto value : skipped) {
        ++count;
      }

      CHECK_EQ(count, 0);
    }

    SUBCASE("Range ctor: Skip more than available") {
      auto skipped = SkipAdapterFromRange(data, 20);

      int count = 0;
      for ([[maybe_unused]] auto value : skipped) {
        ++count;
      }

      CHECK_EQ(count, 0);
    }
  }

  TEST_CASE("TakeWhileAdapter: conditional take") {
    std::vector<int> data = {1, 2, 3, 4, 5, 4, 3, 2, 1};
    auto begin_iter = VectorIterator<int>(data.begin());
    auto end_iter = VectorIterator<int>(data.end());

    SUBCASE("Take while less than 5") {
      auto taken = TakeWhileAdapter(begin_iter, end_iter, [](int value) { return value < 5; });

      std::vector<int> result;
      for (auto value : taken) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{1, 2, 3, 4});
    }

    SUBCASE("Range ctor: Take while less than 5") {
      auto taken = TakeWhileAdapterFromRange(data, [](int value) { return value < 5; });

      std::vector<int> result;
      for (auto value : taken) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{1, 2, 3, 4});
    }

    SUBCASE("Take while always false") {
      auto taken = TakeWhileAdapter(begin_iter, end_iter, [](int /*value*/) { return false; });

      int count = 0;
      for ([[maybe_unused]] auto value : taken) {
        ++count;
      }

      CHECK_EQ(count, 0);
    }

    SUBCASE("Range ctor: Take while always false") {
      auto taken = TakeWhileAdapterFromRange(data, [](int /*value*/) { return false; });

      int count = 0;
      for ([[maybe_unused]] auto value : taken) {
        ++count;
      }

      CHECK_EQ(count, 0);
    }
  }

  TEST_CASE("SkipWhileAdapter: conditional skip") {
    std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8};
    auto begin_iter = VectorIterator<int>(data.begin());
    auto end_iter = VectorIterator<int>(data.end());

    SUBCASE("Skip while less than 5") {
      auto skipped = SkipWhileAdapter(begin_iter, end_iter, [](int value) { return value < 5; });

      std::vector<int> result;
      for (auto value : skipped) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{5, 6, 7, 8});
    }

    SUBCASE("Range ctor: Skip while less than 5") {
      auto skipped = SkipWhileAdapterFromRange(data, [](int value) { return value < 5; });

      std::vector<int> result;
      for (auto value : skipped) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{5, 6, 7, 8});
    }

    SUBCASE("Skip while always true") {
      auto skipped = SkipWhileAdapter(begin_iter, end_iter, [](int /*value*/) { return true; });

      int count = 0;
      for ([[maybe_unused]] auto value : skipped) {
        ++count;
      }

      CHECK_EQ(count, 0);
    }

    SUBCASE("Range ctor: Skip while always true") {
      auto skipped = SkipWhileAdapterFromRange(data, [](int /*value*/) { return true; });

      int count = 0;
      for ([[maybe_unused]] auto value : skipped) {
        ++count;
      }

      CHECK_EQ(count, 0);
    }
  }

  TEST_CASE("EnumerateAdapter: add indices") {
    std::vector<std::string> data = {"a", "b", "c", "d"};
    auto begin_iter = VectorIterator<std::string>(data.begin());
    auto end_iter = VectorIterator<std::string>(data.end());

    auto enumerated = EnumerateAdapter(begin_iter, end_iter);

    std::vector<size_t> indices;
    std::vector<std::string> values;

    for (auto [index, value] : enumerated) {
      indices.push_back(index);
      values.push_back(value);
    }

    CHECK_EQ(indices, std::vector<size_t>{0, 1, 2, 3});
    CHECK_EQ(values, data);

    SUBCASE("Range ctor: Enumerate") {
      auto enumerated = EnumerateAdapterFromRange(data);

      std::vector<size_t> indices;
      std::vector<std::string> values;

      for (auto [index, value] : enumerated) {
        indices.push_back(index);
        values.push_back(value);
      }

      CHECK_EQ(indices, std::vector<size_t>{0, 1, 2, 3});
      CHECK_EQ(values, data);
    }
  }

  TEST_CASE("InspectAdapter: side effects") {
    std::vector<int> data = {1, 2, 3, 4, 5};
    auto begin_iter = VectorIterator<int>(data.begin());
    auto end_iter = VectorIterator<int>(data.end());

    int sum = 0;
    auto inspected = InspectAdapter(begin_iter, end_iter, [&sum](int value) { sum += value; });

    std::vector<int> result;
    for (auto value : inspected) {
      result.push_back(value);
    }

    CHECK_EQ(result, data);
    CHECK_EQ(sum, 15);

    SUBCASE("Range ctor: Inspect") {
      int sum2 = 0;
      auto inspected = InspectAdapterFromRange(data, [&sum2](int value) { sum2 += value; });

      std::vector<int> result;
      for (auto value : inspected) {
        result.push_back(value);
      }

      CHECK_EQ(result, data);
      CHECK_EQ(sum2, 15);
    }
  }

  TEST_CASE("StepByAdapter: sample elements") {
    std::vector<int> data = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto begin_iter = VectorIterator<int>(data.begin());
    auto end_iter = VectorIterator<int>(data.end());

    SUBCASE("Step by 2") {
      auto stepped = StepByAdapter(begin_iter, end_iter, 2);

      std::vector<int> result;
      for (auto value : stepped) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{0, 2, 4, 6, 8});
    }

    SUBCASE("Range ctor: Step by 2") {
      auto stepped = StepByAdapterFromRange(data, 2);

      std::vector<int> result;
      for (auto value : stepped) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{0, 2, 4, 6, 8});
    }

    SUBCASE("Step by 3") {
      auto stepped = StepByAdapter(begin_iter, end_iter, 3);

      std::vector<int> result;
      for (auto value : stepped) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{0, 3, 6, 9});
    }

    SUBCASE("Range ctor: Step by 3") {
      auto stepped = StepByAdapterFromRange(data, 3);

      std::vector<int> result;
      for (auto value : stepped) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{0, 3, 6, 9});
    }

    SUBCASE("Step by 1") {
      auto stepped = StepByAdapter(begin_iter, end_iter, 1);

      std::vector<int> result;
      for (auto value : stepped) {
        result.push_back(value);
      }

      CHECK_EQ(result, data);
    }

    SUBCASE("Range ctor: Step by 1") {
      auto stepped = StepByAdapterFromRange(data, 1);

      std::vector<int> result;
      for (auto value : stepped) {
        result.push_back(value);
      }

      CHECK_EQ(result, data);
    }
  }

  TEST_CASE("ChainAdapter: combine sequences") {
    std::vector<int> data1 = {1, 2, 3};
    std::vector<int> data2 = {4, 5, 6};

    auto begin1 = VectorIterator<int>(data1.begin());
    auto end1 = VectorIterator<int>(data1.end());
    auto begin2 = VectorIterator<int>(data2.begin());
    auto end2 = VectorIterator<int>(data2.end());

    auto chained = ChainAdapter(begin1, end1, begin2, end2);

    std::vector<int> result;
    for (auto value : chained) {
      result.push_back(value);
    }

    CHECK_EQ(result, std::vector<int>{1, 2, 3, 4, 5, 6});

    SUBCASE("Range ctor: ChainAdapter") {
      auto chained = ChainAdapterFromRange(data1, data2);

      std::vector<int> result;
      for (auto value : chained) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{1, 2, 3, 4, 5, 6});
    }
  }

  TEST_CASE("Complex adapter chains") {
    std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    auto begin_iter = VectorIterator<int>(data.begin());
    auto end_iter = VectorIterator<int>(data.end());

    SUBCASE("Filter + Map + Take") {
      auto chain = FilterAdapter(begin_iter, end_iter, [](int value) {
                     return value % 2 == 0;
                   }).Map([](int value) {
                       return value * 2;
                     }).Take(3);

      std::vector<int> result;
      for (auto value : chain) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{4, 8, 12});
    }

    SUBCASE("Skip + Filter + Enumerate") {
      auto chain = SkipAdapter(begin_iter, end_iter, 5).Filter([](int value) { return value % 3 == 0; }).Enumerate();

      std::vector<std::tuple<size_t, int>> result;
      for (auto [index, value] : chain) {
        result.push_back({index, value});
      }

      CHECK_EQ(result.size(), 4);
      CHECK_EQ(std::get<1>(result[0]), 6);
      CHECK_EQ(std::get<1>(result[1]), 9);
      CHECK_EQ(std::get<1>(result[2]), 12);
      CHECK_EQ(std::get<1>(result[3]), 15);
    }

    SUBCASE("TakeWhile + Map") {
      auto chain = TakeWhileAdapter(begin_iter, end_iter, [](int value) { return value <= 5; }).Map([](int value) {
        return value * 10;
      });

      std::vector<int> result;
      for (auto value : chain) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{10, 20, 30, 40, 50});
    }

    SUBCASE("Filter + Skip + Take") {
      auto chain = FilterAdapter(begin_iter, end_iter, [](int value) { return value % 2 != 0; }).Skip(2).Take(3);

      std::vector<int> result;
      for (auto value : chain) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{5, 7, 9});
    }
  }

  TEST_CASE("Adapter: iterator operations") {
    std::vector<int> data = {1, 2, 3, 4, 5};
    auto begin_iter = VectorIterator<int>(data.begin());
    auto end_iter = VectorIterator<int>(data.end());

    SUBCASE("Pre-increment") {
      auto filtered = FilterAdapter(begin_iter, end_iter, [](int value) { return value % 2 == 0; });
      auto iter = filtered.begin();
      CHECK_EQ(*iter, 2);
      ++iter;
      CHECK_EQ(*iter, 4);
    }

    SUBCASE("Post-increment") {
      auto filtered = FilterAdapter(begin_iter, end_iter, [](int value) { return value % 2 == 0; });
      auto iter = filtered.begin();
      auto old_iter = iter++;
      CHECK_EQ(*old_iter, 2);
      CHECK_EQ(*iter, 4);
    }

    SUBCASE("Equality comparison") {
      auto filtered = FilterAdapter(begin_iter, end_iter, [](int /*value*/) { return true; });
      auto begin = filtered.begin();
      auto end = filtered.end();
      CHECK_NE(begin, end);

      for (int idx = 0; idx < 5; ++idx) {
        ++begin;
      }
      CHECK_EQ(begin, end);
    }
  }

  TEST_CASE("Adapter: empty sequences") {
    std::vector<int> empty_data;
    auto begin_iter = VectorIterator<int>(empty_data.begin());
    auto end_iter = VectorIterator<int>(empty_data.end());

    SUBCASE("Filter empty") {
      auto filtered = FilterAdapter(begin_iter, end_iter, [](int /*value*/) { return true; });
      CHECK_EQ(filtered.begin(), filtered.end());
    }

    SUBCASE("Map empty") {
      auto mapped = MapAdapter(begin_iter, end_iter, [](int value) { return value * 2; });
      CHECK_EQ(mapped.begin(), mapped.end());
    }

    SUBCASE("Take from empty") {
      auto taken = TakeAdapter(begin_iter, end_iter, 5);
      CHECK_EQ(taken.begin(), taken.end());
    }

    SUBCASE("Skip from empty") {
      auto skipped = SkipAdapter(begin_iter, end_iter, 5);
      CHECK_EQ(skipped.begin(), skipped.end());
    }
  }

  TEST_CASE("Const iterator support: FilterAdapter") {
    const std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    SUBCASE("Filter even numbers from const range") {
      auto filtered = FilterAdapterFromRange(data, [](int value) { return value % 2 == 0; });

      std::vector<int> result;
      for (auto value : filtered) {
        result.push_back(value);
      }

      CHECK_EQ(result.size(), 5);
      CHECK_EQ(result, std::vector<int>{2, 4, 6, 8, 10});
    }

    SUBCASE("Filter odd numbers from const range") {
      auto filtered = FilterAdapterFromRange(data, [](int value) { return value % 2 != 0; });

      std::vector<int> result;
      for (auto value : filtered) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{1, 3, 5, 7, 9});
    }
  }

  TEST_CASE("Const iterator support: MapAdapter") {
    const std::vector<int> data = {1, 2, 3, 4, 5};

    SUBCASE("Double values from const range") {
      auto mapped = MapAdapterFromRange(data, [](int value) { return value * 2; });

      std::vector<int> result;
      for (auto value : mapped) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{2, 4, 6, 8, 10});
    }

    SUBCASE("Transform to string from const range") {
      auto mapped = MapAdapterFromRange(data, [](int value) { return std::to_string(value); });

      std::vector<std::string> result;
      for (auto value : mapped) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<std::string>{"1", "2", "3", "4", "5"});
    }
  }

  TEST_CASE("Const iterator support: TakeAdapter") {
    const std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    SUBCASE("Take first 5 from const range") {
      auto taken = TakeAdapterFromRange(data, 5);

      std::vector<int> result;
      for (auto value : taken) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{1, 2, 3, 4, 5});
    }

    SUBCASE("Take more than available from const range") {
      auto taken = TakeAdapterFromRange(data, 15);

      int count = 0;
      for ([[maybe_unused]] auto value : taken) {
        ++count;
      }

      CHECK_EQ(count, 10);
    }
  }

  TEST_CASE("Const iterator support: SkipAdapter") {
    const std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    SUBCASE("Skip first 5 from const range") {
      auto skipped = SkipAdapterFromRange(data, 5);

      std::vector<int> result;
      for (auto value : skipped) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{6, 7, 8, 9, 10});
    }

    SUBCASE("Skip all from const range") {
      auto skipped = SkipAdapterFromRange(data, 10);

      int count = 0;
      for ([[maybe_unused]] auto value : skipped) {
        ++count;
      }

      CHECK_EQ(count, 0);
    }
  }

  TEST_CASE("Const iterator support: TakeWhileAdapter") {
    const std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    SUBCASE("Take while less than 6 from const range") {
      auto taken = TakeWhileAdapterFromRange(data, [](int value) { return value < 6; });

      std::vector<int> result;
      for (auto value : taken) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{1, 2, 3, 4, 5});
    }

    SUBCASE("Take while even from const range") {
      const std::vector<int> even_data = {2, 4, 6, 8, 1, 3, 5};
      auto taken = TakeWhileAdapterFromRange(even_data, [](int value) { return value % 2 == 0; });

      std::vector<int> result;
      for (auto value : taken) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{2, 4, 6, 8});
    }
  }

  TEST_CASE("Const iterator support: SkipWhileAdapter") {
    const std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    SUBCASE("Skip while less than 6 from const range") {
      auto skipped = SkipWhileAdapterFromRange(data, [](int value) { return value < 6; });

      std::vector<int> result;
      for (auto value : skipped) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{6, 7, 8, 9, 10});
    }

    SUBCASE("Skip while odd from const range") {
      const std::vector<int> odd_data = {1, 3, 5, 2, 4, 6};
      auto skipped = SkipWhileAdapterFromRange(odd_data, [](int value) { return value % 2 != 0; });

      std::vector<int> result;
      for (auto value : skipped) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{2, 4, 6});
    }
  }

  TEST_CASE("Const iterator support: EnumerateAdapter") {
    const std::vector<std::string> data = {"a", "b", "c", "d"};

    SUBCASE("Enumerate const range") {
      auto enumerated = EnumerateAdapterFromRange(data);

      std::vector<std::pair<size_t, std::string>> result;
      for (auto [index, value] : enumerated) {
        result.push_back({index, value});
      }

      CHECK_EQ(result.size(), 4);
      CHECK_EQ(result[0], std::make_pair(0ul, std::string("a")));
      CHECK_EQ(result[1], std::make_pair(1ul, std::string("b")));
      CHECK_EQ(result[2], std::make_pair(2ul, std::string("c")));
      CHECK_EQ(result[3], std::make_pair(3ul, std::string("d")));
    }
  }

  TEST_CASE("Const iterator support: InspectAdapter") {
    const std::vector<int> data = {1, 2, 3, 4, 5};

    SUBCASE("Inspect each element from const range") {
      int sum = 0;
      auto inspected = InspectAdapterFromRange(data, [&sum](int value) { sum += value; });

      int count = 0;
      for ([[maybe_unused]] auto value : inspected) {
        ++count;
      }

      CHECK_EQ(count, 5);
      CHECK_EQ(sum, 15);
    }
  }

  TEST_CASE("Const iterator support: StepByAdapter") {
    const std::vector<int> data = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    SUBCASE("Step by 2 from const range") {
      auto stepped = StepByAdapterFromRange(data, 2);

      std::vector<int> result;
      for (auto value : stepped) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{0, 2, 4, 6, 8});
    }

    SUBCASE("Step by 3 from const range") {
      auto stepped = StepByAdapterFromRange(data, 3);

      std::vector<int> result;
      for (auto value : stepped) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{0, 3, 6, 9});
    }
  }

  TEST_CASE("Const iterator support: ChainAdapter") {
    const std::vector<int> data1 = {1, 2, 3};
    const std::vector<int> data2 = {4, 5, 6};

    SUBCASE("Chain two const ranges") {
      auto chained = ChainAdapterFromRange(data1, data2);

      std::vector<int> result;
      for (auto value : chained) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{1, 2, 3, 4, 5, 6});
    }

    SUBCASE("Chain const and non-const ranges") {
      std::vector<int> data3 = {7, 8, 9};
      auto chained = ChainAdapterFromRange(data1, data3);

      std::vector<int> result;
      for (auto value : chained) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{1, 2, 3, 7, 8, 9});
    }

    SUBCASE("Chain non-const and const ranges") {
      std::vector<int> data3 = {7, 8, 9};
      auto chained = ChainAdapterFromRange(data3, data1);

      std::vector<int> result;
      for (auto value : chained) {
        result.push_back(value);
      }

      CHECK_EQ(result, std::vector<int>{7, 8, 9, 1, 2, 3});
    }
  }

  TEST_CASE("Const iterator support: chained operations") {
    const std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    SUBCASE("Filter and map on const range") {
      auto result = FilterAdapterFromRange(data, [](int value) { return value % 2 == 0; }).Map([](int value) {
        return value * 3;
      });

      std::vector<int> collected;
      for (auto value : result) {
        collected.push_back(value);
      }

      CHECK_EQ(collected, std::vector<int>{6, 12, 18, 24, 30});
    }

    SUBCASE("Take and enumerate on const range") {
      auto result = TakeAdapterFromRange(data, 5).Enumerate();

      std::vector<std::pair<size_t, int>> collected;
      for (auto [index, value] : result) {
        collected.push_back({index, value});
      }

      CHECK_EQ(collected.size(), 5);
      CHECK_EQ(collected[0], std::make_pair(0ul, 1));
      CHECK_EQ(collected[4], std::make_pair(4ul, 5));
    }

    SUBCASE("Skip, filter, and map on const range") {
      auto result = SkipAdapterFromRange(data, 3).Filter([](int value) { return value % 2 != 0; }).Map([](int value) {
        return value * 2;
      });

      std::vector<int> collected;
      for (auto value : result) {
        collected.push_back(value);
      }

      CHECK_EQ(collected, std::vector<int>{10, 14, 18});
    }
  }

  TEST_CASE("Terminal operations: ForEach") {
    std::vector<int> data = {1, 2, 3, 4, 5};
    auto begin_iter = VectorIterator<int>(data.begin());
    auto end_iter = VectorIterator<int>(data.end());

    SUBCASE("ForEach accumulates values") {
      auto adapter = FilterAdapter(begin_iter, end_iter, [](int value) { return value % 2 == 0; });

      int sum = 0;
      adapter.ForEach([&sum](int value) { sum += value; });

      CHECK_EQ(sum, 6);  // 2 + 4
    }

    SUBCASE("ForEach with range adapter") {
      int product = 1;
      FilterAdapterFromRange(data, [](int value) { return value > 2; }).ForEach([&product](int value) {
        product *= value;
      });

      CHECK_EQ(product, 60);  // 3 * 4 * 5
    }

    SUBCASE("ForEach with tuple iterator") {
      std::vector<std::tuple<int, int>> tuple_data = {{1, 10}, {2, 20}, {3, 30}};
      auto tuple_begin = TupleIterator<int, int>(tuple_data.begin());
      auto tuple_end = TupleIterator<int, int>(tuple_data.end());

      auto adapter = FilterAdapter(tuple_begin, tuple_end, [](const auto& t) { return std::get<0>(t) > 1; });

      int sum = 0;
      adapter.ForEach([&sum](const auto& t) { sum += std::get<0>(t) + std::get<1>(t); });

      CHECK_EQ(sum, 55);  // (2 + 20) + (3 + 30)
    }
  }

  TEST_CASE("Terminal operations: Fold") {
    std::vector<int> data = {1, 2, 3, 4, 5};
    auto begin_iter = VectorIterator<int>(data.begin());
    auto end_iter = VectorIterator<int>(data.end());

    SUBCASE("Fold sums values") {
      auto adapter = FilterAdapter(begin_iter, end_iter, [](int /*value*/) { return true; });

      int result = adapter.Fold(0, [](int acc, int value) { return acc + value; });

      CHECK_EQ(result, 15);
    }

    SUBCASE("Fold with multiplication") {
      auto adapter = FilterAdapterFromRange(data, [](int value) { return value <= 4; });

      int result = adapter.Fold(1, [](int acc, int value) { return acc * value; });

      CHECK_EQ(result, 24);  // 1 * 2 * 3 * 4
    }

    SUBCASE("Fold builds string") {
      auto adapter = MapAdapterFromRange(data, [](int value) { return std::to_string(value); });

      std::string result = adapter.Fold(std::string(""), [](std::string acc, const std::string& value) {
        return acc.empty() ? value : acc + "," + value;
      });

      CHECK_EQ(result, "1,2,3,4,5");
    }

    SUBCASE("Fold with tuple iterator") {
      std::vector<std::tuple<int, int>> tuple_data = {{1, 10}, {2, 20}, {3, 30}};
      auto tuple_begin = TupleIterator<int, int>(tuple_data.begin());
      auto tuple_end = TupleIterator<int, int>(tuple_data.end());

      auto adapter = FilterAdapter(tuple_begin, tuple_end, [](const auto& /*t*/) { return true; });

      int result = adapter.Fold(0, [](int acc, const auto& t) { return acc + std::get<0>(t) + std::get<1>(t); });

      CHECK_EQ(result, 66);  // (1 + 10) + (2 + 20) + (3 + 30)
    }
  }

  TEST_CASE("Terminal operations: Any") {
    std::vector<int> data = {1, 2, 3, 4, 5};
    auto begin_iter = VectorIterator<int>(data.begin());
    auto end_iter = VectorIterator<int>(data.end());

    SUBCASE("Any finds matching element") {
      auto adapter = FilterAdapter(begin_iter, end_iter, [](int /*value*/) { return true; });

      bool result = adapter.Any([](int value) { return value > 3; });

      CHECK(result);
    }

    SUBCASE("Any returns false when no match") {
      auto adapter = FilterAdapterFromRange(data, [](int value) { return value < 3; });

      bool result = adapter.Any([](int value) { return value > 10; });

      CHECK_FALSE(result);
    }

    SUBCASE("Any on empty range") {
      auto adapter = FilterAdapterFromRange(data, [](int /*value*/) { return false; });

      bool result = adapter.Any([](int /*value*/) { return true; });

      CHECK_FALSE(result);
    }
  }

  TEST_CASE("Terminal operations: All") {
    std::vector<int> data = {2, 4, 6, 8, 10};
    auto begin_iter = VectorIterator<int>(data.begin());
    auto end_iter = VectorIterator<int>(data.end());

    SUBCASE("All returns true when all match") {
      auto adapter = FilterAdapter(begin_iter, end_iter, [](int /*value*/) { return true; });

      bool result = adapter.All([](int value) { return value % 2 == 0; });

      CHECK(result);
    }

    SUBCASE("All returns false when one doesn't match") {
      std::vector<int> mixed_data = {2, 4, 5, 8};
      auto adapter = FilterAdapterFromRange(mixed_data, [](int /*value*/) { return true; });

      bool result = adapter.All([](int value) { return value % 2 == 0; });

      CHECK_FALSE(result);
    }

    SUBCASE("All on empty range") {
      auto adapter = FilterAdapterFromRange(data, [](int /*value*/) { return false; });

      bool result = adapter.All([](int /*value*/) { return false; });

      CHECK(result);  // Vacuous truth
    }
  }

  TEST_CASE("Terminal operations: None") {
    std::vector<int> data = {1, 3, 5, 7, 9};
    auto begin_iter = VectorIterator<int>(data.begin());
    auto end_iter = VectorIterator<int>(data.end());

    SUBCASE("None returns true when none match") {
      auto adapter = FilterAdapter(begin_iter, end_iter, [](int /*value*/) { return true; });

      bool result = adapter.None([](int value) { return value % 2 == 0; });

      CHECK(result);
    }

    SUBCASE("None returns false when at least one matches") {
      std::vector<int> mixed_data = {1, 3, 4, 7};
      auto adapter = FilterAdapterFromRange(mixed_data, [](int /*value*/) { return true; });

      bool result = adapter.None([](int value) { return value % 2 == 0; });

      CHECK_FALSE(result);
    }

    SUBCASE("None on empty range") {
      auto adapter = FilterAdapterFromRange(data, [](int /*value*/) { return false; });

      bool result = adapter.None([](int /*value*/) { return true; });

      CHECK(result);
    }
  }

  TEST_CASE("Terminal operations: Find") {
    std::vector<int> data = {1, 2, 3, 4, 5};
    auto begin_iter = VectorIterator<int>(data.begin());
    auto end_iter = VectorIterator<int>(data.end());

    SUBCASE("Find returns first matching element") {
      auto adapter = FilterAdapter(begin_iter, end_iter, [](int /*value*/) { return true; });

      auto result = adapter.Find([](int value) { return value > 3; });

      REQUIRE(result.has_value());
      CHECK_EQ(result.value(), 4);
    }

    SUBCASE("Find returns nullopt when no match") {
      auto adapter = FilterAdapterFromRange(data, [](int value) { return value < 3; });

      auto result = adapter.Find([](int value) { return value > 10; });

      CHECK_FALSE(result.has_value());
    }

    SUBCASE("Find on empty range") {
      auto adapter = FilterAdapterFromRange(data, [](int /*value*/) { return false; });

      auto result = adapter.Find([](int /*value*/) { return true; });

      CHECK_FALSE(result.has_value());
    }

    SUBCASE("Find with tuple iterator") {
      std::vector<std::tuple<int, int>> tuple_data = {{1, 10}, {2, 20}, {3, 30}};
      auto tuple_begin = TupleIterator<int, int>(tuple_data.begin());
      auto tuple_end = TupleIterator<int, int>(tuple_data.end());

      auto adapter = FilterAdapter(tuple_begin, tuple_end, [](const auto& /*t*/) { return true; });

      auto result = adapter.Find([](const auto& t) { return std::get<0>(t) + std::get<1>(t) > 25; });

      REQUIRE(result.has_value());
      CHECK_EQ(std::get<0>(result.value()), 3);
      CHECK_EQ(std::get<1>(result.value()), 30);
    }
  }

  TEST_CASE("Terminal operations: CountIf") {
    std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto begin_iter = VectorIterator<int>(data.begin());
    auto end_iter = VectorIterator<int>(data.end());

    SUBCASE("CountIf counts matching elements") {
      auto adapter = FilterAdapter(begin_iter, end_iter, [](int /*value*/) { return true; });

      size_t result = adapter.CountIf([](int value) { return value % 2 == 0; });

      CHECK_EQ(result, 5);
    }

    SUBCASE("CountIf with no matches") {
      auto adapter = FilterAdapterFromRange(data, [](int value) { return value < 5; });

      size_t result = adapter.CountIf([](int value) { return value > 10; });

      CHECK_EQ(result, 0);
    }

    SUBCASE("CountIf on empty range") {
      auto adapter = FilterAdapterFromRange(data, [](int /*value*/) { return false; });

      size_t result = adapter.CountIf([](int /*value*/) { return true; });

      CHECK_EQ(result, 0);
    }

    SUBCASE("CountIf with chaining") {
      size_t result = FilterAdapterFromRange(data, [](int value) { return value > 3; }).Take(5).CountIf([](int value) {
        return value % 2 == 0;
      });

      CHECK_EQ(result, 3);  // 4, 6, and 8 from {4, 5, 6, 7, 8}
    }
  }

  TEST_CASE("Terminal operations: Collect") {
    std::vector<int> data = {1, 2, 3, 4, 5};
    auto begin_iter = VectorIterator<int>(data.begin());
    auto end_iter = VectorIterator<int>(data.end());

    SUBCASE("Collect gathers all elements") {
      auto adapter = FilterAdapter(begin_iter, end_iter, [](int value) { return value % 2 == 0; });

      auto result = adapter.Collect();

      CHECK_EQ(result, std::vector<int>{2, 4});
    }

    SUBCASE("Collect with transformation") {
      auto result = MapAdapterFromRange(data, [](int value) { return value * 2; }).Collect();

      CHECK_EQ(result, std::vector<int>{2, 4, 6, 8, 10});
    }

    SUBCASE("Collect empty range") {
      auto adapter = FilterAdapterFromRange(data, [](int /*value*/) { return false; });

      auto result = adapter.Collect();

      CHECK(result.empty());
    }

    SUBCASE("Collect with complex chaining") {
      auto result = FilterAdapterFromRange(data, [](int value) { return value > 2; })
                        .Map([](int value) { return value * 3; })
                        .Take(2)
                        .Collect();

      CHECK_EQ(result, std::vector<int>{9, 12});  // (3 * 3), (4 * 3)
    }

    SUBCASE("Collect tuple values") {
      std::vector<std::tuple<int, int>> tuple_data = {{1, 10}, {2, 20}, {3, 30}};
      auto tuple_begin = TupleIterator<int, int>(tuple_data.begin());
      auto tuple_end = TupleIterator<int, int>(tuple_data.end());

      auto adapter = FilterAdapter(tuple_begin, tuple_end, [](const auto& t) { return std::get<0>(t) > 1; });

      auto result = adapter.Collect();

      CHECK_EQ(result.size(), 2);
      CHECK_EQ(std::get<0>(result[0]), 2);
      CHECK_EQ(std::get<1>(result[0]), 20);
    }
  }

  TEST_CASE("Terminal operations: chained with adapters") {
    std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    SUBCASE("Filter -> Map -> Fold") {
      int result = FilterAdapterFromRange(data, [](int value) {
                     return value % 2 == 0;
                   }).Map([](int value) {
                       return value * 2;
                     }).Fold(0, [](int acc, int value) { return acc + value; });

      CHECK_EQ(result, 60);  // (2*2) + (4*2) + (6*2) + (8*2) + (10*2) = 4+8+12+16+20
    }

    SUBCASE("Take -> Any") {
      bool result = TakeAdapterFromRange(data, 3).Any([](int value) { return value > 2; });

      CHECK(result);
    }

    SUBCASE("Skip -> All") {
      bool result = SkipAdapterFromRange(data, 7).All([](int value) { return value > 7; });

      CHECK(result);
    }

    SUBCASE("Enumerate -> Find") {
      auto result = EnumerateAdapterFromRange(data).Find([](const auto& t) { return std::get<1>(t) == 5; });

      REQUIRE(result.has_value());
      CHECK_EQ(std::get<0>(result.value()), 4);  // index
      CHECK_EQ(std::get<1>(result.value()), 5);  // value
    }

    SUBCASE("StepBy -> CountIf") {
      size_t result = StepByAdapterFromRange(data, 2).CountIf([](int value) { return value % 3 == 0; });

      CHECK_EQ(result, 2);  // 3 and 9 from {1, 3, 5, 7, 9}
    }

    SUBCASE("Complex chain with terminal") {
      auto result = FilterAdapterFromRange(data, [](int value) { return value > 3; })
                        .Take(5)
                        .Map([](int value) { return value * 2; })
                        .Filter([](int value) { return value > 10; })
                        .Collect();

      CHECK_EQ(result, std::vector<int>{12, 14, 16});  // (6*2), (7*2), (8*2)
    }
  }

  TEST_CASE("ReverseAdapter") {
    SUBCASE("Basic reverse iteration") {
      std::vector<int> data = {1, 2, 3, 4, 5};
      auto reversed = ReverseAdapterFromRange(data);
      auto result = reversed.Collect();

      CHECK_EQ(result, std::vector<int>{5, 4, 3, 2, 1});
    }

    SUBCASE("Empty range") {
      std::vector<int> data;
      auto reversed = ReverseAdapterFromRange(data);
      auto result = reversed.Collect();

      CHECK(result.empty());
    }

    SUBCASE("Single element") {
      std::vector<int> data = {42};
      auto reversed = ReverseAdapterFromRange(data);
      auto result = reversed.Collect();

      CHECK_EQ(result, std::vector<int>{42});
    }

    SUBCASE("Reverse with Filter") {
      std::vector<int> data = {1, 2, 3, 4, 5, 6};
      auto result = ReverseAdapterFromRange(data).Filter([](int x) { return x % 2 == 0; }).Collect();

      CHECK_EQ(result, std::vector<int>{6, 4, 2});
    }

    SUBCASE("Reverse with Map") {
      std::vector<int> data = {1, 2, 3};
      auto result = ReverseAdapterFromRange(data).Map([](int x) { return x * 10; }).Collect();

      CHECK_EQ(result, std::vector<int>{30, 20, 10});
    }
  }

  TEST_CASE("JoinAdapter") {
    SUBCASE("Basic join") {
      std::vector<std::vector<int>> nested = {{1, 2}, {3, 4}, {5}};
      auto joined = JoinAdapterFromRange(nested);
      auto result = joined.Collect();

      CHECK_EQ(result, std::vector<int>{1, 2, 3, 4, 5});
    }

    SUBCASE("Join with empty inner vectors") {
      std::vector<std::vector<int>> nested = {{1, 2}, {}, {3, 4}, {}, {5}};
      auto joined = JoinAdapterFromRange(nested);
      auto result = joined.Collect();

      CHECK_EQ(result, std::vector<int>{1, 2, 3, 4, 5});
    }

    SUBCASE("Join empty outer vector") {
      std::vector<std::vector<int>> nested;
      auto joined = JoinAdapterFromRange(nested);
      auto result = joined.Collect();

      CHECK(result.empty());
    }

    SUBCASE("Join with Filter") {
      std::vector<std::vector<int>> nested = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
      auto result = JoinAdapterFromRange(nested).Filter([](int x) { return x % 2 == 0; }).Collect();

      CHECK_EQ(result, std::vector<int>{2, 4, 6, 8});
    }

    SUBCASE("Join with Map") {
      std::vector<std::vector<int>> nested = {{1, 2}, {3, 4}};
      auto result = JoinAdapterFromRange(nested).Map([](int x) { return x * x; }).Collect();

      CHECK_EQ(result, std::vector<int>{1, 4, 9, 16});
    }
  }

  TEST_CASE("SlideAdapter") {
    SUBCASE("Basic sliding window") {
      std::vector<int> data = {1, 2, 3, 4, 5};
      auto windows = SlideAdapterFromRange(data, 3);
      auto result = windows.Collect();

      CHECK_EQ(result.size(), 3);
      CHECK_EQ(result[0], std::vector<int>{1, 2, 3});
      CHECK_EQ(result[1], std::vector<int>{2, 3, 4});
      CHECK_EQ(result[2], std::vector<int>{3, 4, 5});
    }

    SUBCASE("Window size equals data size") {
      std::vector<int> data = {1, 2, 3};
      auto windows = SlideAdapterFromRange(data, 3);
      auto result = windows.Collect();

      CHECK_EQ(result.size(), 1);
      CHECK_EQ(result[0], std::vector<int>{1, 2, 3});
    }

    SUBCASE("Window size larger than data") {
      std::vector<int> data = {1, 2};
      auto windows = SlideAdapterFromRange(data, 3);
      auto result = windows.Collect();

      CHECK(result.empty());
    }

    SUBCASE("Window size 1") {
      std::vector<int> data = {1, 2, 3};
      auto windows = SlideAdapterFromRange(data, 1);
      auto result = windows.Collect();

      CHECK_EQ(result.size(), 3);
      CHECK_EQ(result[0], std::vector<int>{1});
      CHECK_EQ(result[1], std::vector<int>{2});
      CHECK_EQ(result[2], std::vector<int>{3});
    }

    SUBCASE("Slide with Filter") {
      std::vector<int> data = {1, 2, 3, 4, 5};
      auto result = SlideAdapterFromRange(data, 2)
                        .Filter([](const std::vector<int>& window) { return window[0] % 2 == 1; })
                        .Collect();

      CHECK_EQ(result.size(), 2);
      CHECK_EQ(result[0], std::vector<int>{1, 2});
      CHECK_EQ(result[1], std::vector<int>{3, 4});
    }
  }

  TEST_CASE("StrideAdapter") {
    SUBCASE("Basic stride") {
      std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9};
      auto strided = StrideAdapterFromRange(data, 3);
      auto result = strided.Collect();

      CHECK_EQ(result, std::vector<int>{1, 4, 7});
    }

    SUBCASE("Stride of 1") {
      std::vector<int> data = {1, 2, 3, 4, 5};
      auto strided = StrideAdapterFromRange(data, 1);
      auto result = strided.Collect();

      CHECK_EQ(result, std::vector<int>{1, 2, 3, 4, 5});
    }

    SUBCASE("Stride of 2") {
      std::vector<int> data = {1, 2, 3, 4, 5, 6};
      auto strided = StrideAdapterFromRange(data, 2);
      auto result = strided.Collect();

      CHECK_EQ(result, std::vector<int>{1, 3, 5});
    }

    SUBCASE("Stride larger than data") {
      std::vector<int> data = {1, 2, 3};
      auto strided = StrideAdapterFromRange(data, 10);
      auto result = strided.Collect();

      CHECK_EQ(result, std::vector<int>{1});
    }

    SUBCASE("Stride with Filter") {
      std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9};
      auto result = StrideAdapterFromRange(data, 2).Filter([](int x) { return x > 3; }).Collect();

      CHECK_EQ(result, std::vector<int>{5, 7, 9});
    }

    SUBCASE("Stride with Map") {
      std::vector<int> data = {1, 2, 3, 4, 5, 6};
      auto result = StrideAdapterFromRange(data, 2).Map([](int x) { return x * 10; }).Collect();

      CHECK_EQ(result, std::vector<int>{10, 30, 50});
    }
  }

  TEST_CASE("ZipAdapter") {
    SUBCASE("Basic zip") {
      std::vector<int> first = {1, 2, 3};
      std::vector<int> second = {10, 20, 30};
      auto zipped = ZipAdapterFromRange(first, second);
      auto result = zipped.Collect();

      CHECK_EQ(result.size(), 3);
      CHECK_EQ(std::get<0>(result[0]), 1);
      CHECK_EQ(std::get<1>(result[0]), 10);
      CHECK_EQ(std::get<0>(result[1]), 2);
      CHECK_EQ(std::get<1>(result[1]), 20);
      CHECK_EQ(std::get<0>(result[2]), 3);
      CHECK_EQ(std::get<1>(result[2]), 30);
    }

    SUBCASE("Zip with different lengths - first shorter") {
      std::vector<int> first = {1, 2};
      std::vector<int> second = {10, 20, 30, 40};
      auto zipped = ZipAdapterFromRange(first, second);
      auto result = zipped.Collect();

      CHECK_EQ(result.size(), 2);
      CHECK_EQ(std::get<0>(result[0]), 1);
      CHECK_EQ(std::get<1>(result[0]), 10);
      CHECK_EQ(std::get<0>(result[1]), 2);
      CHECK_EQ(std::get<1>(result[1]), 20);
    }

    SUBCASE("Zip with different lengths - second shorter") {
      std::vector<int> first = {1, 2, 3, 4};
      std::vector<int> second = {10, 20};
      auto zipped = ZipAdapterFromRange(first, second);
      auto result = zipped.Collect();

      CHECK_EQ(result.size(), 2);
      CHECK_EQ(std::get<0>(result[0]), 1);
      CHECK_EQ(std::get<1>(result[0]), 10);
      CHECK_EQ(std::get<0>(result[1]), 2);
      CHECK_EQ(std::get<1>(result[1]), 20);
    }

    SUBCASE("Zip with empty ranges") {
      std::vector<int> first;
      std::vector<int> second = {10, 20, 30};
      auto zipped = ZipAdapterFromRange(first, second);
      auto result = zipped.Collect();

      CHECK(result.empty());
    }

    SUBCASE("Zip with Filter") {
      std::vector<int> first = {1, 2, 3, 4, 5};
      std::vector<int> second = {10, 20, 30, 40, 50};
      auto result = ZipAdapterFromRange(first, second).Filter([](int a, int b) { return a % 2 == 0; }).Collect();

      CHECK_EQ(result.size(), 2);
      CHECK_EQ(std::get<0>(result[0]), 2);
      CHECK_EQ(std::get<1>(result[0]), 20);
      CHECK_EQ(std::get<0>(result[1]), 4);
      CHECK_EQ(std::get<1>(result[1]), 40);
    }

    SUBCASE("Zip with Map") {
      std::vector<int> first = {1, 2, 3};
      std::vector<int> second = {10, 20, 30};
      auto result = ZipAdapterFromRange(first, second).Map([](int a, int b) { return a + b; }).Collect();

      CHECK_EQ(result, std::vector<int>{11, 22, 33});
    }

    SUBCASE("Zip different types") {
      std::vector<int> ints = {1, 2, 3};
      std::vector<std::string> strings = {"a", "b", "c"};
      auto zipped = ZipAdapterFromRange(ints, strings);

      size_t count = 0;
      for (auto [num, str] : zipped) {
        if (count == 0) {
          CHECK_EQ(num, 1);
          CHECK_EQ(str, "a");
        }
        ++count;
      }
      CHECK_EQ(count, 3);
    }
  }

  TEST_CASE("Complex adapter chains with new adapters") {
    SUBCASE("Reverse -> Filter -> Map") {
      std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
      auto result = ReverseAdapterFromRange(data)
                        .Filter([](int x) { return x % 2 == 0; })
                        .Map([](int x) { return x * x; })
                        .Collect();

      CHECK_EQ(result, std::vector<int>{100, 64, 36, 16, 4});
    }

    SUBCASE("Stride -> Filter -> Take") {
      std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9};
      auto result = StrideAdapterFromRange(data, 2).Filter([](int x) { return x > 3; }).Take(2).Collect();

      CHECK_EQ(result, std::vector<int>{5, 7});
    }

    SUBCASE("Slide -> Map -> Filter") {
      std::vector<int> data = {1, 2, 3, 4, 5};
      auto result = SlideAdapterFromRange(data, 2)
                        .Map([](const std::vector<int>& window) { return window[0] + window[1]; })
                        .Filter([](int sum) { return sum > 4; })
                        .Collect();

      CHECK_EQ(result, std::vector<int>{5, 7, 9});
    }

    SUBCASE("Zip -> Filter -> Map") {
      std::vector<int> first = {1, 2, 3, 4, 5};
      std::vector<int> second = {5, 4, 3, 2, 1};
      auto result = ZipAdapterFromRange(first, second)
                        .Filter([](int a, int b) { return a < b; })
                        .Map([](int a, int b) { return a * b; })
                        .Collect();

      CHECK_EQ(result, std::vector<int>{5, 8});
    }
  }

}  // TEST_SUITE
