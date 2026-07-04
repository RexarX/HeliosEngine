#include <doctest/doctest.h>

#include <helios/utils/hash.hpp>

#include <string>
#include <string_view>

using namespace helios::utils;

TEST_SUITE("helios::utils::Hash") {
  TEST_CASE("utils::Hash::Fnv1aHash: constexpr evaluation with string_view") {
    constexpr auto hash = Fnv1aHash("hello");
    CHECK_NE(hash, 0);
    CHECK_NE(hash, kFnvBasis);
  }

  TEST_CASE("utils::Hash::Fnv1aHash: consistency for same input") {
    constexpr auto hash1 = Fnv1aHash("test_string");
    constexpr auto hash2 = Fnv1aHash("test_string");
    CHECK_EQ(hash1, hash2);
  }

  TEST_CASE(
      "utils::Hash::Fnv1aHash: different inputs produce different hashes") {
    constexpr auto hash_a = Fnv1aHash("alpha");
    constexpr auto hash_b = Fnv1aHash("beta");
    constexpr auto hash_c = Fnv1aHash("gamma");
    constexpr auto hash_d = Fnv1aHash("delta");

    CHECK_NE(hash_a, hash_b);
    CHECK_NE(hash_a, hash_c);
    CHECK_NE(hash_a, hash_d);
    CHECK_NE(hash_b, hash_c);
    CHECK_NE(hash_b, hash_d);
    CHECK_NE(hash_c, hash_d);
  }

  TEST_CASE("utils::Hash::Fnv1aHash: empty string returns basis") {
    constexpr auto hash = Fnv1aHash(std::string_view{});
    CHECK_EQ(hash, kFnvBasis);
  }

  TEST_CASE("utils::Hash::Fnv1aHash: single character") {
    constexpr auto hash_a = Fnv1aHash("a");
    constexpr auto hash_b = Fnv1aHash("b");

    CHECK_NE(hash_a, kFnvBasis);
    CHECK_NE(hash_b, kFnvBasis);
    CHECK_NE(hash_a, hash_b);
  }

  TEST_CASE(
      "utils::Hash::Fnv1aHash: string_view overload matches char array "
      "overload") {
    constexpr auto hash_sv = Fnv1aHash(std::string_view{"hello"});
    constexpr auto hash_arr = Fnv1aHash("hello");
    CHECK_EQ(hash_sv, hash_arr);
  }

  TEST_CASE("utils::Hash::Fnv1aHash: custom initial hash") {
    constexpr auto hash_default = Fnv1aHash("test");
    constexpr auto hash_custom = Fnv1aHash(std::string_view{"test"}, 42);

    CHECK_NE(hash_default, hash_custom);
  }

  TEST_CASE("utils::Hash::Fnv1aHash: order sensitivity") {
    constexpr auto hash_ab = Fnv1aHash("ab");
    constexpr auto hash_ba = Fnv1aHash("ba");
    CHECK_NE(hash_ab, hash_ba);
  }

  TEST_CASE("utils::Hash::Fnv1aHash: case sensitivity") {
    constexpr auto hash_lower = Fnv1aHash("hello");
    constexpr auto hash_upper = Fnv1aHash("HELLO");
    constexpr auto hash_mixed = Fnv1aHash("Hello");

    CHECK_NE(hash_lower, hash_upper);
    CHECK_NE(hash_lower, hash_mixed);
    CHECK_NE(hash_upper, hash_mixed);
  }

  TEST_CASE(
      "utils::Hash::Fnv1aHash: similar strings produce different hashes") {
    constexpr auto hash1 = Fnv1aHash("test1");
    constexpr auto hash2 = Fnv1aHash("test2");
    constexpr auto hash3 = Fnv1aHash("test3");

    CHECK_NE(hash1, hash2);
    CHECK_NE(hash2, hash3);
    CHECK_NE(hash1, hash3);
  }

  TEST_CASE("utils::Hash::Fnv1aHash: long strings") {
    constexpr auto hash_long = Fnv1aHash(
        "this is a reasonably long string to test the hash function with more "
        "data");
    CHECK_NE(hash_long, 0);
    CHECK_NE(hash_long, kFnvBasis);
  }

  TEST_CASE("utils::Hash::Fnv1aHash: runtime string_view matches constexpr") {
    constexpr auto compile_time_hash = Fnv1aHash("runtime_test");

    std::string runtime_str = "runtime_test";
    auto runtime_hash = Fnv1aHash(std::string_view{runtime_str});

    CHECK_EQ(compile_time_hash, runtime_hash);
  }

  TEST_CASE("utils::Hash::Fnv1aHash: known FNV-1a values") {
    // FNV-1a hash of empty string should equal the basis value
    constexpr auto empty_hash = Fnv1aHash(std::string_view{""});
    CHECK_EQ(empty_hash, kFnvBasis);

#if SIZE_MAX == UINT64_MAX
    // Known FNV-1a 64-bit values for common test vectors
    CHECK_EQ(Fnv1aHash(std::string_view{"a"}), 0xAF63DC4C8601EC8CULL);
    CHECK_EQ(Fnv1aHash(std::string_view{"foobar"}), 0x85944171F73967E8ULL);
#endif
  }

  TEST_CASE("utils::Hash::Fnv1aHash: constants are valid") {
    CHECK_NE(kFnvBasis, 0);
    CHECK_NE(kFnvPrime, 0);
    CHECK_NE(kFnvBasis, kFnvPrime);
  }

  TEST_CASE("utils::Hash::Fnv1aHash: prefix and suffix strings differ") {
    constexpr auto hash_prefix = Fnv1aHash("prefix_common");
    constexpr auto hash_suffix = Fnv1aHash("common_suffix");
    CHECK_NE(hash_prefix, hash_suffix);
  }

  TEST_CASE("utils::Hash::Fnv1aHash: whitespace matters") {
    constexpr auto hash_no_space = Fnv1aHash("helloworld");
    constexpr auto hash_space = Fnv1aHash("hello world");
    constexpr auto hash_tab = Fnv1aHash("hello\tworld");

    CHECK_NE(hash_no_space, hash_space);
    CHECK_NE(hash_space, hash_tab);
    CHECK_NE(hash_no_space, hash_tab);
  }
}
