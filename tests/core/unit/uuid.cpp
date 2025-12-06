#include <doctest/doctest.h>
#include <helios/core/uuid.hpp>

#include <array>
#include <cstring>
#include <random>
#include <string>

TEST_SUITE("helios::Uuid") {
  TEST_CASE("Uuid::ctor: Default construction is invalid") {
    helios::Uuid uuid;
    CHECK_FALSE(uuid.Valid());
    CHECK(uuid.ToString().empty());
    CHECK(uuid.AsBytes().empty());
  }

  TEST_CASE("Uuid::ctor: Construction from string and roundtrip") {
    const auto uuid1 = helios::Uuid::Generate();
    const std::string str = uuid1.ToString();
    helios::Uuid uuid2(str);

    CHECK(uuid1.Valid());
    CHECK(uuid2.Valid());
    CHECK_EQ(uuid1, uuid2);
    CHECK_EQ(str, uuid2.ToString());
  }

  TEST_CASE("Uuid::ctor: Construction from bytes") {
    const auto uuid1 = helios::Uuid::Generate();
    const auto bytes = uuid1.AsBytes();
    CHECK_EQ(bytes.size(), 16);

    std::array<std::byte, 16> arr;
    std::memcpy(arr.data(), bytes.data(), 16);

    helios::Uuid uuid2{std::span(arr)};
    CHECK_EQ(uuid1, uuid2);
    CHECK(uuid2.Valid());
  }

  TEST_CASE("Uuid::swap") {
    auto a = helios::Uuid::Generate();
    auto b = helios::Uuid::Generate();
    const auto a_str = a.ToString();
    const auto b_str = b.ToString();
    swap(a, b);
    CHECK_EQ(a.ToString(), b_str);
    CHECK_EQ(b.ToString(), a_str);
  }

  TEST_CASE("Uuid::operator==: Equality and inequality") {
    const auto a = helios::Uuid::Generate();
    const auto b = helios::Uuid::Generate();
    const helios::Uuid c(a);

    CHECK_NE(a, b);
    CHECK_EQ(a, c);
  }

  TEST_CASE("Uuid::Hash") {
    const auto uuid = helios::Uuid::Generate();
    std::hash<helios::Uuid> hasher;
    helios::Uuid uuid2(uuid.ToString());

    CHECK_NE(hasher(uuid), 0);
    CHECK_EQ(hasher(uuid), hasher(uuid2));
  }

  TEST_CASE("UuidGenerator::Generate: Custom random engine") {
    std::mt19937_64 engine(42);
    helios::UuidGenerator gen(std::move(engine));
    const auto uuid1 = gen.Generate();
    const auto uuid2 = gen.Generate();

    CHECK_NE(uuid1, uuid2);
    CHECK(uuid1.Valid());
    CHECK(uuid2.Valid());
  }

  TEST_CASE("Uuid::ctor: Invalid string yields nil UUID") {
    helios::Uuid uuid("not-a-uuid");
    CHECK_FALSE(uuid.Valid());
    CHECK(uuid.ToString().empty());
    CHECK(uuid.AsBytes().empty());
  }

  TEST_CASE("Uuid::AsBytes: Returns correct size for valid UUID") {
    const auto uuid = helios::Uuid::Generate();
    const auto bytes = uuid.AsBytes();
    CHECK_EQ(bytes.size(), 16);
  }

  TEST_CASE("Uuid::AsBytes: Returns empty span for invalid UUID") {
    helios::Uuid uuid;
    const auto bytes = uuid.AsBytes();
    CHECK(bytes.empty());
  }

  TEST_CASE("Uuid::ctor: Copy and move semantics") {
    const auto uuid1 = helios::Uuid::Generate();
    helios::Uuid uuid2(uuid1);
    helios::Uuid uuid3(std::move(uuid2));
    CHECK_EQ(uuid1, uuid3);

    helios::Uuid uuid4;
    uuid4 = uuid1;
    CHECK_EQ(uuid1, uuid4);

    helios::Uuid uuid5;
    uuid5 = std::move(uuid4);
    CHECK_EQ(uuid1, uuid5);
  }

}  // TEST_SUITE
