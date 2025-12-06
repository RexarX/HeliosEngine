#include <doctest/doctest.h>

#include <helios/core/utils/string_hash.hpp>

#include <string>
#include <string_view>
#include <unordered_map>

using namespace helios::utils;

TEST_SUITE("utils::StringHash") {
  TEST_CASE("StringHash::operator(): basic hashing") {
    StringHash hasher;

    SUBCASE("Hash std::string") {
      std::string str = "hello";
      size_t hash_value = hasher(str);
      CHECK_GT(hash_value, 0);
    }

    SUBCASE("Hash std::string_view") {
      std::string_view str_view = "hello";
      size_t hash_value = hasher(str_view);
      CHECK_GT(hash_value, 0);
    }

    SUBCASE("Hash C-string") {
      const char* cstr = "hello";
      size_t hash_value = hasher(cstr);
      CHECK_GT(hash_value, 0);
    }
  }

  TEST_CASE("StringHash::operator(): consistent hashing across types") {
    StringHash hasher;
    std::string str = "test_string";
    std::string_view str_view = str;
    const char* cstr = str.c_str();

    size_t hash_str = hasher(str);
    size_t hash_view = hasher(str_view);
    size_t hash_cstr = hasher(cstr);

    CHECK_EQ(hash_str, hash_view);
    CHECK_EQ(hash_str, hash_cstr);
    CHECK_EQ(hash_view, hash_cstr);
  }

  TEST_CASE("StringHash::operator(): different strings have different hashes") {
    StringHash hasher;

    size_t hash1 = hasher("string1");
    size_t hash2 = hasher("string2");
    size_t hash3 = hasher("different");

    CHECK_NE(hash1, hash2);
    CHECK_NE(hash1, hash3);
    CHECK_NE(hash2, hash3);
  }

  TEST_CASE("StringHash::operator(): empty string") {
    StringHash hasher;

    std::string empty_str;
    std::string_view empty_view;
    const char* empty_cstr = "";

    size_t hash_str = hasher(empty_str);
    size_t hash_view = hasher(empty_view);
    size_t hash_cstr = hasher(empty_cstr);

    CHECK_EQ(hash_str, hash_view);
    CHECK_EQ(hash_str, hash_cstr);
  }

  TEST_CASE("StringHash::operator(): special characters") {
    StringHash hasher;

    SUBCASE("Newlines and tabs") {
      std::string str_with_newline = "hello\nworld";
      std::string str_with_tab = "hello\tworld";

      size_t hash_newline = hasher(str_with_newline);
      size_t hash_tab = hasher(str_with_tab);

      CHECK_NE(hash_newline, hash_tab);
    }

    SUBCASE("Unicode characters") {
      std::string unicode_str = "héllo wörld";
      size_t hash_value = hasher(unicode_str);
      CHECK_GT(hash_value, 0);
    }
  }

  TEST_CASE("StringEqual::operator(): basic equality comparison") {
    StringEqual equal;

    SUBCASE("string_view comparison") {
      std::string_view view1 = "test";
      std::string_view view2 = "test";
      std::string_view view3 = "different";

      CHECK(equal(view1, view2));
      CHECK_FALSE(equal(view1, view3));
    }

    SUBCASE("std::string comparison") {
      std::string str1 = "test";
      std::string str2 = "test";
      std::string str3 = "different";

      CHECK(equal(str1, str2));
      CHECK_FALSE(equal(str1, str3));
    }

    SUBCASE("C-string comparison") {
      const char* cstr1 = "test";
      const char* cstr2 = "different";
      std::string_view view = "test";

      CHECK(equal(cstr1, view));
      CHECK_FALSE(equal(cstr2, view));
    }
  }

  TEST_CASE("StringEqual::operator(): heterogeneous comparison") {
    StringEqual equal;
    std::string str = "hello";
    std::string_view view = str;
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
      std::string different_str = "world";
      CHECK_FALSE(equal(str, different_str));
      CHECK_FALSE(equal(view, "world"));
    }
  }

  TEST_CASE("StringEqual::operator(): empty strings") {
    StringEqual equal;

    std::string empty_str;
    std::string_view empty_view;
    const char* empty_cstr = "";

    CHECK(equal(empty_str, empty_view));
    CHECK(equal(empty_view, empty_cstr));
    CHECK(equal(empty_str, empty_cstr));
  }

  TEST_CASE("StringEqual::operator(): case sensitivity") {
    StringEqual equal;

    std::string lower = "hello";
    std::string upper = "HELLO";
    std::string mixed = "HeLLo";

    CHECK_FALSE(equal(lower, upper));
    CHECK_FALSE(equal(lower, mixed));
    CHECK_FALSE(equal(upper, mixed));
  }

  TEST_CASE("StringHash: unordered_map usage with heterogeneous lookup") {
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

      std::string_view view = "test_key";
      auto iter = map.find(view);

      CHECK_NE(iter, map.end());
      CHECK_EQ(iter->second, 42);
    }

    SUBCASE("Heterogeneous lookup with C-string") {
      map["another_key"] = 99;

      const char* cstr = "another_key";
      auto iter = map.find(cstr);

      CHECK_NE(iter, map.end());
      CHECK_EQ(iter->second, 99);
    }

    SUBCASE("Non-existent key") {
      map["exists"] = 123;

      std::string_view view = "does_not_exist";
      auto iter = map.find(view);

      CHECK_EQ(iter, map.end());
    }
  }

  TEST_CASE("StringHash::operator(): deterministic") {
    StringHash hasher;
    std::string test_str = "deterministic";

    size_t hash1 = hasher(test_str);
    size_t hash2 = hasher(test_str);
    size_t hash3 = hasher(test_str);

    CHECK_EQ(hash1, hash2);
    CHECK_EQ(hash2, hash3);
  }

  TEST_CASE("StringEqual::operator(): reflexivity and symmetry properties") {
    StringEqual equal;
    std::string str = "reflexive";
    std::string_view view = str;

    SUBCASE("Reflexivity") {
      CHECK(equal(str, str));
      CHECK(equal(view, view));
    }

    SUBCASE("Symmetry") {
      std::string str2 = "reflexive";
      CHECK(equal(str, str2));
      CHECK(equal(str2, str));
    }
  }

  TEST_CASE("StringHash::operator(): performance characteristics") {
    StringHash hasher;
    StringEqual equal;

    SUBCASE("Long strings") {
      std::string long_str(1000, 'x');
      std::string long_str2(1000, 'x');
      std::string long_str_diff(1000, 'y');

      size_t hash1 = hasher(long_str);
      size_t hash2 = hasher(long_str2);
      size_t hash3 = hasher(long_str_diff);

      CHECK_EQ(hash1, hash2);
      CHECK_NE(hash1, hash3);
      CHECK(equal(long_str, long_str2));
      CHECK_FALSE(equal(long_str, long_str_diff));
    }

    SUBCASE("Single character difference") {
      std::string str1 = "almost_identical";
      std::string str2 = "almost_identicaL";

      CHECK_NE(hasher(str1), hasher(str2));
      CHECK_FALSE(equal(str1, str2));
    }
  }

  TEST_CASE("StringHash::operator(): collision resistance") {
    StringHash hasher;
    std::unordered_map<size_t, std::string> hash_map;

    // Insert many strings and check for excessive collisions
    std::vector<std::string> test_strings = {"string1", "string2", "string3", "test",  "hello", "world", "foo",
                                             "bar",     "baz",     "qux",     "alpha", "beta",  "gamma", "delta"};

    int collision_count = 0;
    for (const auto& str : test_strings) {
      size_t hash_value = hasher(str);
      if (hash_map.find(hash_value) != hash_map.end()) {
        ++collision_count;
      }
      hash_map[hash_value] = str;
    }

    // We expect very few or no collisions for this small set
    CHECK_LT(collision_count, 3);
  }
}  // TEST_SUITE
