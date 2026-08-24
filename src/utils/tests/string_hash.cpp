#include <doctest/doctest.h>

#include <helios/utils/string_hash.hpp>

#include <algorithm>
#include <string>
#include <string_view>
#include <unordered_map>

using namespace helios::utils;

TEST_SUITE("helios::utils::StringHash") {
  TEST_CASE("helios::utils::StringHash::operator(): basic hashing") {
    StringHash hasher;

    SUBCASE("Hash std::string") {
      const std::string str = "hello";
      const size_t hash_value = hasher(str);
      CHECK_GT(hash_value, 0);
    }

    SUBCASE("Hash std::string_view") {
      constexpr std::string_view str_view = "hello";
      const size_t hash_value = hasher(str_view);
      CHECK_GT(hash_value, 0);
    }

    SUBCASE("Hash C-string") {
      const char* cstr = "hello";
      const size_t hash_value = hasher(cstr);
      CHECK_GT(hash_value, 0);
    }
  }

  TEST_CASE(
      "helios::utils::StringHash::operator(): consistent hashing across "
      "types") {
    StringHash hasher;
    const std::string str = "test_string";
    const std::string_view str_view = str;
    const char* cstr = str.c_str();

    const size_t hash_str = hasher(str);
    const size_t hash_view = hasher(str_view);
    const size_t hash_cstr = hasher(cstr);

    CHECK_EQ(hash_str, hash_view);
    CHECK_EQ(hash_str, hash_cstr);
    CHECK_EQ(hash_view, hash_cstr);
  }

  TEST_CASE(
      "utils::StringHash::operator(): different strings have different "
      "hashes") {
    StringHash hasher;

    const size_t hash1 = hasher("string1");
    const size_t hash2 = hasher("string2");
    const size_t hash3 = hasher("different");

    CHECK_NE(hash1, hash2);
    CHECK_NE(hash1, hash3);
    CHECK_NE(hash2, hash3);
  }

  TEST_CASE("helios::utils::StringHash::operator(): empty string") {
    StringHash hasher;

    const std::string empty_str;
    constexpr std::string_view empty_view;
    const char* empty_cstr = "";

    const size_t hash_str = hasher(empty_str);
    const size_t hash_view = hasher(empty_view);
    const size_t hash_cstr = hasher(empty_cstr);

    CHECK_EQ(hash_str, hash_view);
    CHECK_EQ(hash_str, hash_cstr);
  }

  TEST_CASE("helios::utils::StringHash::operator(): special characters") {
    StringHash hasher;

    SUBCASE("Newlines and tabs") {
      const std::string str_with_newline = "hello\nworld";
      const std::string str_with_tab = "hello\tworld";

      const size_t hash_newline = hasher(str_with_newline);
      const size_t hash_tab = hasher(str_with_tab);

      CHECK_NE(hash_newline, hash_tab);
    }

    SUBCASE("Unicode characters") {
      const std::string unicode_str = "héllo wörld";
      const size_t hash_value = hasher(unicode_str);
      CHECK_GT(hash_value, 0);
    }
  }

  TEST_CASE("helios::utils::StringHash::operator(): collision resistance") {
    StringHash hasher;
    std::unordered_map<size_t, std::string> hash_map;

    // Insert many strings and check for excessive collisions
    const std::vector<std::string> test_strings = {
        "string1", "string2", "string3", "test",  "hello", "world", "foo",
        "bar",     "baz",     "qux",     "alpha", "beta",  "gamma", "delta"};

    int collision_count = 0;
    for (const auto& str : test_strings) {
      const size_t hash_value = hasher(str);
      if (hash_map.contains(hash_value)) {
        ++collision_count;
      }
      hash_map[hash_value] = str;
    }

    // We expect very few or no collisions for this small set
    CHECK_LT(collision_count, 3);
  }

  TEST_CASE(
      "helios::utils::StringHash::operator(): performance characteristics") {
    StringHash hasher;
    StringEqual equal;

    SUBCASE("Long strings") {
      const std::string long_str(1000, 'x');
      const std::string long_str2(1000, 'x');
      const std::string long_str_diff(1000, 'y');

      const size_t hash1 = hasher(long_str);
      const size_t hash2 = hasher(long_str2);
      const size_t hash3 = hasher(long_str_diff);

      CHECK_EQ(hash1, hash2);
      CHECK_NE(hash1, hash3);
      CHECK(equal(long_str, long_str2));
      CHECK_FALSE(equal(long_str, long_str_diff));
    }

    SUBCASE("Single character difference") {
      const std::string str1 = "almost_identical";
      const std::string str2 = "almost_identicaL";

      CHECK_NE(hasher(str1), hasher(str2));
      CHECK_FALSE(equal(str1, str2));
    }
  }

  TEST_CASE(
      "utils::StringHash::unordered_map:  unordered_map usage with "
      "heterogeneous lookup") {
    std::unordered_map<std::string, int, StringHash, StringEqual> map;

    SUBCASE("Insert and retrieve with std::string") {
      map["key1"] = 10;
      map["key2"] = 20;
      map["key3"] = 30;

      CHECK_EQ(map["key1"], 10);
      CHECK_EQ(map["key2"], 20);
      CHECK_EQ(map["key3"], 30);
      CHECK_EQ(map.size(), 3);
    }

    SUBCASE("Heterogeneous lookup with string_view") {
      map["test_key"] = 42;

      constexpr std::string_view view = "test_key";
      const auto iter = map.find(view);

      CHECK_NE(iter, map.end());
      CHECK_EQ(iter->second, 42);
    }

    SUBCASE("Heterogeneous lookup with C-string") {
      map["another_key"] = 99;

      const char* cstr = "another_key";
      const auto iter = map.find(cstr);

      CHECK_NE(iter, map.end());
      CHECK_EQ(iter->second, 99);
    }

    SUBCASE("Non-existent key") {
      map["exists"] = 123;

      constexpr std::string_view view = "does_not_exist";
      const auto iter = map.find(view);

      CHECK_EQ(iter, map.end());
    }
  }
}

TEST_SUITE("helios::utils::StringEqual") {
  TEST_CASE(
      "helios::utils::StringEqual::operator(): basic equality comparison") {
    StringEqual equal;

    SUBCASE("string_view comparison") {
      constexpr std::string_view view1 = "test";
      constexpr std::string_view view2 = "test";
      constexpr std::string_view view3 = "different";

      CHECK(equal(view1, view2));
      CHECK_FALSE(equal(view1, view3));
    }

    SUBCASE("std::string comparison") {
      const std::string str1 = "test";
      const std::string str2 = "test";
      const std::string str3 = "different";

      CHECK(equal(str1, str2));
      CHECK_FALSE(equal(str1, str3));
    }

    SUBCASE("C-string comparison") {
      const char* cstr1 = "test";
      const char* cstr2 = "different";
      constexpr std::string_view view = "test";

      CHECK(equal(cstr1, view));
      CHECK_FALSE(equal(cstr2, view));
    }
  }

  TEST_CASE(
      "helios::utils::StringEqual::operator(): heterogeneous comparison") {
    StringEqual equal;
    const std::string str = "hello";
    const std::string_view view = str;
    const char* cstr = "hello";

    SUBCASE("string vs string_view") {
      CHECK(equal(str, view));
      CHECK(equal(view, str));
    }

    SUBCASE("string_view vs C-string") {
      CHECK(equal(view, cstr));
      CHECK(equal(cstr, view));
    }

    SUBCASE("Different values") {
      const std::string different_str = "world";
      CHECK_FALSE(equal(str, different_str));
      CHECK_FALSE(equal(view, "world"));
    }
  }

  TEST_CASE("helios::utils::StringEqual::operator(): empty strings") {
    StringEqual equal;

    const std::string empty_str;
    constexpr std::string_view empty_view;
    const char* empty_cstr = "";

    CHECK(equal(empty_str, empty_view));
    CHECK(equal(empty_view, empty_cstr));
    CHECK(equal(empty_str, empty_cstr));
  }

  TEST_CASE("helios::utils::StringEqual::operator(): case sensitivity") {
    StringEqual equal;

    const std::string lower = "hello";
    const std::string upper = "HELLO";
    const std::string mixed = "HeLLo";

    CHECK_FALSE(equal(lower, upper));
    CHECK_FALSE(equal(lower, mixed));
    CHECK_FALSE(equal(upper, mixed));
  }

  TEST_CASE(
      "utils::StringEqual::operator(): reflexivity and symmetry properties") {
    StringEqual equal;
    const std::string str = "reflexive";
    const std::string_view view = str;

    SUBCASE("Reflexivity") {
      CHECK(equal(str, str));
      CHECK(equal(view, view));
    }

    SUBCASE("Symmetry") {
      const std::string str2 = "reflexive";
      CHECK(equal(str, str2));
      CHECK(equal(str2, str));
    }
  }
}

TEST_SUITE("helios::utils::StringLess") {
  TEST_CASE("helios::utils::StringLess::operator(): basic comparison") {
    StringLess less;

    SUBCASE("string_view comparison") {
      constexpr std::string_view view1 = "apple";
      constexpr std::string_view view2 = "banana";
      constexpr std::string_view view3 = "apple";

      CHECK(less(view1, view2));        // "apple" < "banana"
      CHECK_FALSE(less(view1, view3));  // "apple" is not < "apple"
      CHECK_FALSE(less(view2, view1));  // "banana" is not < "apple"
    }

    SUBCASE("std::string comparison") {
      const std::string str1 = "apple";
      const std::string str2 = "banana";
      const std::string str3 = "apple";

      CHECK(less(str1, str2));
      CHECK_FALSE(less(str1, str3));
      CHECK_FALSE(less(str2, str1));
    }

    SUBCASE("C-string comparison") {
      const char* cstr1 = "apple";
      const char* cstr2 = "banana";
      constexpr std::string_view view = "cherry";

      CHECK(less(cstr1, view));        // "apple" < "cherry"
      CHECK(less(cstr2, view));        // "banana" < "cherry"
      CHECK_FALSE(less(view, cstr1));  // "cherry" is not < "apple"
    }
  }

  TEST_CASE("helios::utils::StringLess::operator(): heterogeneous comparison") {
    StringLess less;
    const std::string str = "hello";
    const std::string_view view = str;
    const char* cstr = "hello";

    SUBCASE("string vs string_view") {
      CHECK_FALSE(less(str, view));  // equal, so not less
      CHECK_FALSE(less(view, str));  // equal, so not less
    }

    SUBCASE("string_view vs C-string") {
      CHECK_FALSE(less(view, cstr));  // equal, so not less
      CHECK_FALSE(less(cstr, view));  // equal, so not less
    }

    SUBCASE("Different values") {
      const std::string greater_str = "world";
      const std::string lesser_str = "goodbye";

      CHECK(less(str, greater_str));       // "hello" < "world"
      CHECK_FALSE(less(str, lesser_str));  // "hello" is not < "goodbye"
      CHECK(less(view, "world"));          // "hello" < "world"
      CHECK_FALSE(less(view, "goodbye"));  // "hello" is not < "goodbye"
    }
  }

  TEST_CASE("helios::utils::StringLess::operator(): empty strings") {
    StringLess less;

    const std::string empty_str;
    constexpr std::string_view empty_view;
    const char* empty_cstr = "";
    const std::string non_empty = "a";

    CHECK_FALSE(less(empty_str, empty_view));   // equal, so not less
    CHECK_FALSE(less(empty_view, empty_cstr));  // equal, so not less
    CHECK_FALSE(less(empty_str, empty_cstr));   // equal, so not less
    CHECK(less(empty_str, non_empty));          // "" < "a"
    CHECK_FALSE(less(non_empty, empty_str));    // "a" is not < ""
  }

  TEST_CASE("helios::utils::StringLess::operator(): case sensitivity") {
    StringLess less;

    const std::string lower = "hello";
    const std::string upper = "HELLO";
    const std::string mixed = "HeLLo";
    const std::string after_lower = "world";

    // ASCII: uppercase letters (65-90) come before lowercase (97-122)
    CHECK(less(upper, lower));        // "HELLO" < "hello" in ASCII
    CHECK(less(mixed, lower));        // "HeLLo" < "hello" in ASCII
    CHECK(less(lower, after_lower));  // "hello" < "world"
    CHECK_FALSE(less(lower, upper));  // "hello" is not < "HELLO"
  }

  TEST_CASE(
      "utils::StringLess::operator(): irreflexivity and asymmetry properties") {
    StringLess less;
    const std::string str = "reflexive";
    const std::string_view view = str;

    SUBCASE("Irreflexivity") {
      CHECK_FALSE(less(str, str));    // not less than itself
      CHECK_FALSE(less(view, view));  // not less than itself
    }

    SUBCASE("Asymmetry") {
      const std::string str2 = "reflexive";
      const std::string lesser_str = "apple";
      const std::string greater_str = "zebra";

      CHECK_FALSE(less(str, str2));  // equal, so not less
      CHECK_FALSE(less(str2, str));  // equal, so not less

      CHECK(less(lesser_str, greater_str));        // "apple" < "zebra"
      CHECK_FALSE(less(greater_str, lesser_str));  // "zebra" is not < "apple"
    }

    SUBCASE("Transitivity") {
      const std::string str1 = "apple";
      const std::string str2 = "banana";
      const std::string str3 = "cherry";

      CHECK(less(str1, str2));  // "apple" < "banana"
      CHECK(less(str2, str3));  // "banana" < "cherry"
      CHECK(less(str1, str3));  // "apple" < "cherry" (transitive)
    }
  }

  TEST_CASE(
      "helios::utils::StringLess::operator(): use with std::ranges::sort") {
    StringLess less;

    SUBCASE("Sort vector of strings") {
      std::vector<std::string> strings = {"banana", "apple", "cherry", "date"};
      const std::vector<std::string> expected = {"apple", "banana", "cherry",
                                                 "date"};
      std::ranges::sort(strings, less);

      CHECK_EQ(strings, expected);
    }

    SUBCASE("Sort vector of string_views") {
      const std::string s1 = "zebra";
      const std::string s2 = "apple";
      const std::string s3 = "mango";
      const std::string s4 = "banana";

      std::vector<std::string_view> views = {s1, s2, s3, s4};
      const std::vector<std::string_view> expected = {s2, s4, s3, s1};
      std::ranges::sort(views, less);

      CHECK_EQ(views, expected);
    }

    SUBCASE("Sort vector of C-strings") {
      std::vector<const char*> cstrings = {"delta", "alpha", "charlie",
                                           "bravo"};
      const std::vector<const char*> expected = {"alpha", "bravo", "charlie",
                                                 "delta"};
      std::ranges::sort(cstrings, less);

      const bool equal = std::ranges::equal(
          cstrings, expected, [](const char* a, const char* b) {
            return std::string_view(a) == std::string_view(b);
          });
      CHECK(equal);
    }

    SUBCASE("Sort empty vector") {
      std::vector<std::string> strings;
      std::ranges::sort(strings, less);
      CHECK(strings.empty());
    }

    SUBCASE("Sort vector with duplicate values") {
      std::vector<std::string> strings = {"apple", "banana", "apple", "cherry",
                                          "banana"};
      const std::vector<std::string> expected = {"apple", "apple", "banana",
                                                 "banana", "cherry"};

      std::ranges::sort(strings, less);

      CHECK_EQ(strings, expected);
    }

    SUBCASE("Sort vector with mixed case") {
      std::vector<std::string> strings = {"banana", "Apple", "cherry",
                                          "Banana"};
      const std::vector<std::string> expected = {"Apple", "Banana", "banana",
                                                 "cherry"};
      std::ranges::sort(strings, less);

      CHECK_EQ(strings, expected);
    }

    SUBCASE("Sort already sorted vector") {
      std::vector<std::string> strings = {"apple", "banana", "cherry", "date"};
      const std::vector<std::string> expected = {"apple", "banana", "cherry",
                                                 "date"};
      std::ranges::sort(strings, less);

      CHECK_EQ(strings, expected);
    }

    SUBCASE("Sort reverse sorted vector") {
      std::vector<std::string> strings = {"date", "cherry", "banana", "apple"};
      const std::vector<std::string> expected = {"apple", "banana", "cherry",
                                                 "date"};
      std::ranges::sort(strings, less);

      CHECK_EQ(strings, expected);
    }
  }
}
