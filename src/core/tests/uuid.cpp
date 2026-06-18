#include <doctest/doctest.h>

#include <helios/uuid.hpp>

#include <algorithm>
#include <array>
#include <set>
#include <string>
#include <unordered_set>

using namespace helios;

TEST_SUITE("helios::Uuid") {
  TEST_CASE("Uuid::ctor: default construction creates invalid UUID") {
    Uuid uuid;
    CHECK_FALSE(uuid.Valid());
    CHECK_EQ(uuid.ToString(), "");
  }

  TEST_CASE("Uuid::ctor: construction from valid string") {
    SUBCASE("lowercase string") {
      Uuid uuid("550e8400-e29b-41d4-a716-446655440000");
      CHECK(uuid.Valid());
      CHECK_EQ(uuid.ToString(), "550e8400-e29b-41d4-a716-446655440000");
    }

    SUBCASE("uppercase string") {
      Uuid uuid("550E8400-E29B-41D4-A716-446655440000");
      CHECK(uuid.Valid());
      // ToString should return lowercase
      CHECK_EQ(uuid.ToString(), "550e8400-e29b-41d4-a716-446655440000");
    }

    SUBCASE("mixed case string") {
      Uuid uuid("550E8400-e29b-41D4-a716-446655440000");
      CHECK(uuid.Valid());
      CHECK_EQ(uuid.ToString(), "550e8400-e29b-41d4-a716-446655440000");
    }
  }

  TEST_CASE("Uuid::ctor: construction from invalid string") {
    SUBCASE("empty string") {
      Uuid uuid("");
      CHECK_FALSE(uuid.Valid());
    }

    SUBCASE("too short string") {
      Uuid uuid("550e8400-e29b-41d4-a716");
      CHECK_FALSE(uuid.Valid());
    }

    SUBCASE("too long string") {
      Uuid uuid("550e8400-e29b-41d4-a716-446655440000-extra");
      CHECK_FALSE(uuid.Valid());
    }

    SUBCASE("missing dashes - stduuid accepts this format") {
      // Note: stduuid library accepts UUIDs without dashes
      Uuid uuid("550e8400e29b41d4a716446655440000");
      CHECK(uuid.Valid());
      CHECK_EQ(uuid.ToString(), "550e8400-e29b-41d4-a716-446655440000");
    }

    SUBCASE("invalid characters") {
      Uuid uuid("550e8400-e29b-41d4-a716-44665544000g");
      CHECK_FALSE(uuid.Valid());
    }

    SUBCASE("dashes in wrong positions - stduuid is flexible") {
      // Note: stduuid library is flexible with dash positions
      Uuid uuid("550e-8400-e29b41d4-a716-446655440000");
      CHECK(uuid.Valid());
    }
  }

  TEST_CASE("Uuid::ctor: construction from bytes") {
    SUBCASE("valid 16-byte span") {
      std::array<std::byte, 16> bytes = {
          std::byte{0x55}, std::byte{0x0e}, std::byte{0x84}, std::byte{0x00},
          std::byte{0xe2}, std::byte{0x9b}, std::byte{0x41}, std::byte{0xd4},
          std::byte{0xa7}, std::byte{0x16}, std::byte{0x44}, std::byte{0x66},
          std::byte{0x55}, std::byte{0x44}, std::byte{0x00}, std::byte{0x00}};
      Uuid uuid{bytes};
      CHECK(uuid.Valid());
      CHECK_EQ(uuid.ToString(), "550e8400-e29b-41d4-a716-446655440000");
    }

    SUBCASE("all zeros span creates invalid UUID") {
      std::array<std::byte, 16> zeros = {};
      Uuid uuid{zeros};
      CHECK_FALSE(uuid.Valid());
    }
  }

  TEST_CASE("Uuid::ctor: copy and move semantics") {
    SUBCASE("copy construction") {
      Uuid original("550e8400-e29b-41d4-a716-446655440000");
      Uuid copy(original);
      CHECK(copy.Valid());
      CHECK_EQ(copy, original);
      CHECK_EQ(copy.ToString(), original.ToString());
    }

    SUBCASE("copy assignment") {
      Uuid original("550e8400-e29b-41d4-a716-446655440000");
      Uuid copy;
      copy = original;
      CHECK(copy.Valid());
      CHECK_EQ(copy, original);
    }

    SUBCASE("move construction") {
      Uuid original("550e8400-e29b-41d4-a716-446655440000");
      const std::string original_str = original.ToString();
      Uuid moved(std::move(original));
      CHECK(moved.Valid());
      CHECK_EQ(moved.ToString(), original_str);
    }

    SUBCASE("move assignment") {
      Uuid original("550e8400-e29b-41d4-a716-446655440000");
      const std::string original_str = original.ToString();
      Uuid moved;
      moved = std::move(original);
      CHECK(moved.Valid());
      CHECK_EQ(moved.ToString(), original_str);
    }
  }

  TEST_CASE("Uuid::Generate: generates valid unique UUIDs") {
    SUBCASE("generated UUID is valid") {
      Uuid uuid = Uuid::Generate();
      CHECK(uuid.Valid());
      CHECK_FALSE(uuid.ToString().empty());
      CHECK_EQ(uuid.AsBytes().size(), 16);
    }

    SUBCASE("generated UUIDs are unique") {
      constexpr size_t kCount = 100;
      std::set<std::string> uuid_strings;

      for (size_t i = 0; i < kCount; ++i) {
        Uuid uuid = Uuid::Generate();
        CHECK(uuid.Valid());
        uuid_strings.insert(uuid.ToString());
      }

      CHECK_EQ(uuid_strings.size(), kCount);
    }

    SUBCASE("generated UUID has correct format") {
      Uuid uuid = Uuid::Generate();
      const std::string str = uuid.ToString();

      CHECK_EQ(str.length(), 36);
      CHECK_EQ(str[8], '-');
      CHECK_EQ(str[13], '-');
      CHECK_EQ(str[18], '-');
      CHECK_EQ(str[23], '-');
    }
  }

  TEST_CASE("Uuid::ToString: string representation") {
    SUBCASE("valid UUID returns correct format") {
      Uuid uuid("550e8400-e29b-41d4-a716-446655440000");
      const std::string str = uuid.ToString();

      CHECK_EQ(str.length(), 36);
      CHECK_EQ(str, "550e8400-e29b-41d4-a716-446655440000");
    }

    SUBCASE("invalid UUID returns empty string") {
      Uuid uuid;
      CHECK_EQ(uuid.ToString(), "");
    }
  }

  TEST_CASE("Uuid::AsBytes: byte representation") {
    SUBCASE("valid UUID returns 16 bytes") {
      Uuid uuid("550e8400-e29b-41d4-a716-446655440000");
      auto bytes = uuid.AsBytes();

      CHECK_EQ(bytes.size(), 16);
      CHECK_EQ(bytes[0], std::byte{0x55});
      CHECK_EQ(bytes[1], std::byte{0x0e});
      CHECK_EQ(bytes[2], std::byte{0x84});
      CHECK_EQ(bytes[3], std::byte{0x00});
    }

    SUBCASE("round-trip bytes to string") {
      Uuid original("a1b2c3d4-e5f6-7890-abcd-ef1234567890");
      auto bytes = original.AsBytes();
      Uuid reconstructed(bytes);
      CHECK_EQ(reconstructed.ToString(), original.ToString());
    }
  }

  TEST_CASE("Uuid::operator==: comparison operators") {
    SUBCASE("equal UUIDs") {
      Uuid uuid1("550e8400-e29b-41d4-a716-446655440000");
      Uuid uuid2("550e8400-e29b-41d4-a716-446655440000");
      CHECK_EQ(uuid1, uuid2);
      CHECK_FALSE(uuid1 != uuid2);
    }

    SUBCASE("different UUIDs") {
      Uuid uuid1("550e8400-e29b-41d4-a716-446655440000");
      Uuid uuid2("660e8400-e29b-41d4-a716-446655440000");
      CHECK_NE(uuid1, uuid2);
      CHECK_FALSE(uuid1 == uuid2);
    }

    SUBCASE("invalid UUIDs are equal") {
      Uuid uuid1;
      Uuid uuid2;
      CHECK_EQ(uuid1, uuid2);
    }

    SUBCASE("valid and invalid UUIDs are not equal") {
      Uuid valid("550e8400-e29b-41d4-a716-446655440000");
      Uuid invalid;
      CHECK_NE(valid, invalid);
    }
  }

  TEST_CASE("Uuid::operator<: ordering") {
    SUBCASE("less than comparison") {
      Uuid uuid1("110e8400-e29b-41d4-a716-446655440000");
      Uuid uuid2("550e8400-e29b-41d4-a716-446655440000");
      CHECK(uuid1 < uuid2);
      CHECK_FALSE(uuid2 < uuid1);
    }

    SUBCASE("equal UUIDs are not less than") {
      Uuid uuid1("550e8400-e29b-41d4-a716-446655440000");
      Uuid uuid2("550e8400-e29b-41d4-a716-446655440000");
      CHECK_FALSE(uuid1 < uuid2);
      CHECK_FALSE(uuid2 < uuid1);
    }

    SUBCASE("can be used in ordered containers") {
      std::set<Uuid> uuid_set;
      uuid_set.insert(Uuid::Generate());
      uuid_set.insert(Uuid::Generate());
      uuid_set.insert(Uuid::Generate());
      CHECK_EQ(uuid_set.size(), 3);
    }
  }

  TEST_CASE("Uuid::Hash: hashing") {
    SUBCASE("valid UUID has non-zero hash") {
      Uuid uuid("550e8400-e29b-41d4-a716-446655440000");
      CHECK_NE(uuid.Hash(), 0);
    }

    SUBCASE("equal UUIDs have equal hashes") {
      Uuid uuid1("550e8400-e29b-41d4-a716-446655440000");
      Uuid uuid2("550e8400-e29b-41d4-a716-446655440000");
      CHECK_EQ(uuid1.Hash(), uuid2.Hash());
    }

    SUBCASE("different UUIDs likely have different hashes") {
      Uuid uuid1("550e8400-e29b-41d4-a716-446655440000");
      Uuid uuid2("660e8400-e29b-41d4-a716-446655440000");
      // This isn't guaranteed but should be true for good hash functions
      CHECK_NE(uuid1.Hash(), uuid2.Hash());
    }

    SUBCASE("can be used in unordered containers") {
      std::unordered_set<Uuid> uuid_set;
      uuid_set.insert(Uuid::Generate());
      uuid_set.insert(Uuid::Generate());
      uuid_set.insert(Uuid::Generate());
      CHECK_EQ(uuid_set.size(), 3);
    }

    SUBCASE("std::hash specialization works") {
      std::hash<Uuid> hasher;
      Uuid uuid("550e8400-e29b-41d4-a716-446655440000");
      CHECK_EQ(hasher(uuid), uuid.Hash());
    }
  }

  TEST_CASE("Uuid::swap: swap functionality") {
    Uuid uuid1("110e8400-e29b-41d4-a716-446655440000");
    Uuid uuid2("550e8400-e29b-41d4-a716-446655440000");

    const std::string str1 = uuid1.ToString();
    const std::string str2 = uuid2.ToString();

    swap(uuid1, uuid2);

    CHECK_EQ(uuid1.ToString(), str2);
    CHECK_EQ(uuid2.ToString(), str1);
  }

  TEST_CASE("UuidGenerator: custom generator") {
    SUBCASE("default construction and generation") {
      UuidGenerator generator;
      Uuid uuid = generator.Generate();
      CHECK(uuid.Valid());
    }

    SUBCASE("custom engine construction") {
      std::mt19937_64 engine(12345);
      UuidGenerator generator(std::move(engine));
      Uuid uuid = generator.Generate();
      CHECK(uuid.Valid());
    }

    SUBCASE("generates unique UUIDs") {
      UuidGenerator generator;
      std::set<std::string> uuid_strings;

      for (size_t i = 0; i < 50; ++i) {
        Uuid uuid = generator.Generate();
        CHECK(uuid.Valid());
        uuid_strings.insert(uuid.ToString());
      }

      CHECK_EQ(uuid_strings.size(), 50);
    }

    SUBCASE("deterministic with same seed") {
      std::mt19937_64 engine1(42);
      std::mt19937_64 engine2(42);

      UuidGenerator generator1(std::move(engine1));
      UuidGenerator generator2(std::move(engine2));

      Uuid uuid1 = generator1.Generate();
      Uuid uuid2 = generator2.Generate();

      CHECK_EQ(uuid1, uuid2);
    }

    SUBCASE("move semantics") {
      UuidGenerator original;
      Uuid first_uuid = original.Generate();
      CHECK(first_uuid.Valid());

      UuidGenerator moved(std::move(original));
      Uuid second_uuid = moved.Generate();
      CHECK(second_uuid.Valid());
      CHECK_NE(first_uuid, second_uuid);
    }
  }
}  // TEST_SUITE
