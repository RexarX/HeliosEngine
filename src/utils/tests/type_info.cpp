#include <doctest/doctest.h>

#include <helios/utils/type_info.hpp>

#include <string_view>

using namespace helios::utils;

namespace {

struct Foo {};
struct Bar {};

namespace outer {

struct Baz {};

namespace inner {

struct Qux {};

}  // namespace inner

}  // namespace outer

}  // namespace

TEST_SUITE("helios::utils::TypeIndex") {
  TEST_CASE("utils::TypeIndex::ctor") {
    SUBCASE("Default ctor produces empty TypeIndex") {
      constexpr TypeIndex idx;
      CHECK(idx.Empty());
      CHECK_EQ(idx.Hash(), 0);
    }

    SUBCASE("Copy ctor preserves value") {
      constexpr auto idx = TypeIndex::From<Foo>();

      constexpr TypeIndex copy = idx;

      CHECK_EQ(copy, idx);
      CHECK_FALSE(copy.Empty());
    }

    SUBCASE("Move ctor preserves value") {
      constexpr auto idx = TypeIndex::From<Foo>();

      constexpr TypeIndex moved = std::move(idx);

      CHECK_FALSE(moved.Empty());
      CHECK_EQ(moved, TypeIndex::From<Foo>());
    }

    SUBCASE("Construct from TypeId") {
      constexpr auto type_id = TypeId::From<Foo>();
      constexpr TypeIndex idx(type_id);
      CHECK_EQ(idx, TypeIndex::From<Foo>());
    }
  }

  TEST_CASE("utils::TypeIndex::operator=") {
    SUBCASE("Copy assignment preserves value") {
      constexpr auto index = TypeIndex::From<Foo>();

      TypeIndex other;
      other = index;

      CHECK_EQ(other, index);
    }

    SUBCASE("Move assignment preserves value") {
      auto index = TypeIndex::From<Foo>();

      TypeIndex other;
      other = std::move(index);

      CHECK_EQ(other, TypeIndex::From<Foo>());
    }
  }

  TEST_CASE("utils::TypeIndex::From") {
    SUBCASE("From<T>() produces non-empty TypeIndex") {
      constexpr auto idx = TypeIndex::From<Foo>();
      CHECK_FALSE(idx.Empty());
    }

    SUBCASE("Same type produces identical TypeIndex") {
      constexpr auto index1 = TypeIndex::From<Foo>();
      constexpr auto index2 = TypeIndex::From<Foo>();
      CHECK_EQ(index1, index2);
    }

    SUBCASE("Different types produce different TypeIndex values") {
      constexpr auto index1 = TypeIndex::From<Foo>();
      constexpr auto index2 = TypeIndex::From<Bar>();
      CHECK_NE(index1, index2);
    }

    SUBCASE("From instance deduces type correctly") {
      const Foo instance{};
      const auto idx = TypeIndex::From(instance);
      CHECK_EQ(idx, TypeIndex::From<Foo>());
    }

    SUBCASE("Deeply nested type produces distinct TypeIndex") {
      constexpr auto index1 = TypeIndex::From<outer::inner::Qux>();
      constexpr auto index2 = TypeIndex::From<Foo>();

      CHECK_NE(index1, index2);
    }
  }

  TEST_CASE("utils::TypeIndex::FromHash") {
    SUBCASE("FromHash reproduces TypeIndex from an existing hash") {
      constexpr auto expected = TypeIndex::From<Foo>();
      const auto idx = TypeIndex::FromHash(expected.Hash());
      CHECK_EQ(idx, expected);
    }

    SUBCASE("FromHash with zero produces empty TypeIndex") {
      const auto idx = TypeIndex::FromHash(0);
      CHECK(idx.Empty());
    }

    SUBCASE("Different hashes produce different TypeIndex values") {
      constexpr auto foo_idx = TypeIndex::From<Foo>();
      constexpr auto bar_idx = TypeIndex::From<Bar>();
      const auto from_foo = TypeIndex::FromHash(foo_idx.Hash());
      const auto from_bar = TypeIndex::FromHash(bar_idx.Hash());
      CHECK_EQ(from_foo, foo_idx);
      CHECK_EQ(from_bar, bar_idx);
      CHECK_NE(from_foo, from_bar);
    }
  }

  TEST_CASE("utils::TypeIndex::Empty") {
    SUBCASE("Default-constructed TypeIndex is empty") {
      constexpr TypeIndex idx;
      CHECK(idx.Empty());
    }

    SUBCASE("From<T>() TypeIndex is not empty") {
      constexpr auto idx = TypeIndex::From<Foo>();
      CHECK_FALSE(idx.Empty());
    }
  }

  TEST_CASE("utils::TypeIndex::Hash") {
    SUBCASE("Default-constructed TypeIndex has hash 0") {
      constexpr TypeIndex idx;
      CHECK_EQ(idx.Hash(), 0);
    }

    SUBCASE("Non-empty TypeIndex has non-zero hash") {
      constexpr auto idx = TypeIndex::From<Foo>();
      CHECK_NE(idx.Hash(), 0);
    }

    SUBCASE("Hash is consistent across calls") {
      constexpr auto idx = TypeIndex::From<Foo>();
      CHECK_EQ(idx.Hash(), idx.Hash());
    }

    SUBCASE("Different types produce different hashes") {
      constexpr auto index1 = TypeIndex::From<Foo>();
      constexpr auto index2 = TypeIndex::From<Bar>();
      CHECK_NE(index1.Hash(), index2.Hash());
    }
  }

  TEST_CASE("utils::TypeIndex::operator size_t") {
    SUBCASE("Explicit conversion to size_t equals Hash()") {
      constexpr auto idx = TypeIndex::From<Foo>();
      CHECK_EQ(static_cast<size_t>(idx), idx.Hash());
    }
  }

  TEST_CASE("utils::TypeIndex::operator<=>") {
    SUBCASE("TypeIndex is equal to itself") {
      constexpr auto idx = TypeIndex::From<Foo>();
      CHECK_EQ(idx, idx);
    }

    SUBCASE("Two TypeIndex values from the same type are equal") {
      constexpr auto lhs = TypeIndex::From<Foo>();
      constexpr auto rhs = TypeIndex::From<Foo>();
      CHECK_EQ(lhs, rhs);
    }

    SUBCASE("Two TypeIndex values from different types are not equal") {
      constexpr auto lhs = TypeIndex::From<Foo>();
      constexpr auto rhs = TypeIndex::From<Bar>();
      CHECK_NE(lhs, rhs);
    }

    SUBCASE("Ordering is consistent: a < b implies !(b < a)") {
      constexpr auto index1 = TypeIndex::From<Foo>();
      constexpr auto index2 = TypeIndex::From<Bar>();

      if (index1 < index2) {
        CHECK_FALSE(index2 < index1);
      } else {
        CHECK_FALSE(index1 < index2);
      }
    }
  }
}

TEST_SUITE("helios::utils::TypeId") {
  TEST_CASE("utils::TypeId::ctor") {
    SUBCASE("Default ctor produces empty TypeId") {
      constexpr TypeId id;

      CHECK(id.Empty());
      CHECK(id.Name().empty());
      CHECK(id.QualifiedName().empty());
    }

    SUBCASE("Copy ctor preserves value") {
      constexpr auto id = TypeId::From<Foo>();

      constexpr TypeId copy = id;

      CHECK_EQ(copy, id);
      CHECK_FALSE(copy.Empty());
    }

    SUBCASE("Move ctor preserves value") {
      constexpr auto id = TypeId::From<Foo>();

      constexpr TypeId moved = std::move(id);

      CHECK_FALSE(moved.Empty());
      CHECK_EQ(moved, TypeId::From<Foo>());
    }
  }

  TEST_CASE("utils::TypeId::operator=") {
    SUBCASE("Copy assignment preserves value") {
      constexpr auto id1 = TypeId::From<Foo>();

      TypeId id2;
      id2 = id1;

      CHECK_EQ(id2, id1);
    }

    SUBCASE("Move assignment preserves value") {
      auto id1 = TypeId::From<Foo>();

      TypeId id2;
      id2 = std::move(id1);

      CHECK_EQ(id2, TypeId::From<Foo>());
    }
  }

  TEST_CASE("utils::TypeId::From") {
    SUBCASE("From<T>() produces non-empty TypeId") {
      constexpr auto id = TypeId::From<Foo>();
      CHECK_FALSE(id.Empty());
    }

    SUBCASE("Same type produces identical TypeId") {
      constexpr auto id1 = TypeId::From<Foo>();
      constexpr auto id2 = TypeId::From<Foo>();
      CHECK_EQ(id1, id2);
    }

    SUBCASE("Different types produce different TypeId values") {
      constexpr auto id1 = TypeId::From<Foo>();
      constexpr auto id2 = TypeId::From<Bar>();
      CHECK_NE(id1, id2);
    }

    SUBCASE("From instance deduces type correctly") {
      const Foo instance{};
      const auto id = TypeId::From(instance);
      CHECK_EQ(id, TypeId::From<Foo>());
    }
  }

  TEST_CASE("utils::TypeId::FromExported") {
    SUBCASE("FromExported round-trips hash and qualified name") {
      constexpr auto original = TypeId::From<Foo>();
      const auto restored = TypeId::FromExported(original.Index().Hash(),
                                                 original.QualifiedName());
      CHECK_EQ(restored, original);
      CHECK_EQ(restored.Name(), "Foo");
      CHECK_EQ(restored.Index(), original.Index());
    }

    SUBCASE("FromExported with empty name yields empty Name()") {
      constexpr auto original = TypeId::From<Bar>();
      const auto restored = TypeId::FromExported(original.Index().Hash());
      CHECK_EQ(restored.Index(), original.Index());
      CHECK(restored.Name().empty());
    }

    SUBCASE("FromExported hash-only matches TypeIndex::FromHash") {
      constexpr auto original = TypeId::From<outer::inner::Qux>();
      const auto restored = TypeId::FromExported(original.Index().Hash());
      CHECK_EQ(restored.Index(), TypeIndex::FromHash(original.Index().Hash()));
    }
  }

  TEST_CASE("utils::TypeId::Empty") {
    SUBCASE("Default-constructed TypeId is empty") {
      constexpr TypeId id;
      CHECK(id.Empty());
    }

    SUBCASE("From<T>() TypeId is not empty") {
      constexpr auto id = TypeId::From<Foo>();
      CHECK_FALSE(id.Empty());
    }
  }

  TEST_CASE("utils::TypeId::Name") {
    SUBCASE("Returns unqualified type name") {
      constexpr auto id = TypeId::From<Foo>();
      CHECK_EQ(id.Name(), "Foo");
    }

    SUBCASE("Returns unqualified name for nested type") {
      constexpr auto id = TypeId::From<outer::inner::Qux>();
      CHECK_EQ(id.Name(), "Qux");
    }

    SUBCASE("Returns empty string for default-constructed TypeId") {
      constexpr TypeId id;
      CHECK(id.Name().empty());
    }
  }

  TEST_CASE("utils::TypeId::QualifiedName") {
    SUBCASE("Contains type name") {
      constexpr auto id = TypeId::From<Foo>();
      CHECK_FALSE(id.QualifiedName().empty());
      CHECK_NE(id.QualifiedName().find("Foo"), std::string_view::npos);
    }

    SUBCASE("Contains full qualifier for nested type") {
      constexpr auto id = TypeId::From<outer::inner::Qux>();

      CHECK_NE(id.QualifiedName().find("Qux"), std::string_view::npos);
      CHECK_NE(id.QualifiedName().find("outer"), std::string_view::npos);
      CHECK_NE(id.QualifiedName().find("inner"), std::string_view::npos);
    }

    SUBCASE("Returns empty string for default-constructed TypeId") {
      constexpr TypeId id;
      CHECK(id.QualifiedName().empty());
    }
  }

  TEST_CASE("utils::TypeId::Index") {
    SUBCASE("Index matches TypeIndex::From<T>()") {
      constexpr auto id = TypeId::From<Foo>();
      CHECK_EQ(id.Index(), TypeIndex::From<Foo>());
    }

    SUBCASE("Default-constructed TypeId has empty Index") {
      constexpr TypeId id;
      CHECK(id.Index().Empty());
    }
  }

  TEST_CASE("utils::TypeId::operator<=>") {
    SUBCASE("TypeId is equal to itself") {
      constexpr auto id = TypeId::From<Foo>();
      CHECK_EQ(id, id);
    }

    SUBCASE("Two TypeId values from the same type are equal") {
      constexpr auto lhs = TypeId::From<Foo>();
      constexpr auto rhs = TypeId::From<Foo>();
      CHECK_EQ(lhs, rhs);
    }

    SUBCASE("Two TypeId values from different types are not equal") {
      constexpr auto lhs = TypeId::From<Foo>();
      constexpr auto rhs = TypeId::From<Bar>();
      CHECK_NE(lhs, rhs);
    }

    SUBCASE("Ordering is consistent: a < b implies !(b < a)") {
      constexpr auto id1 = TypeId::From<Foo>();
      constexpr auto id2 = TypeId::From<Bar>();

      if (id1 < id2) {
        CHECK_FALSE(id2 < id1);
      } else {
        CHECK_FALSE(id1 < id2);
      }
    }
  }
}

TEST_SUITE(
    "helios::utils::TypeId and helios::utils::TypeIndex: cross-type "
    "comparisons") {
  TEST_CASE("operator==(TypeId, TypeIndex)") {
    SUBCASE("TypeId equals TypeIndex from the same type") {
      constexpr auto id = TypeId::From<Foo>();
      constexpr auto idx = TypeIndex::From<Foo>();

      CHECK(id == idx);
      CHECK(idx == id);
    }

    SUBCASE("TypeId does not equal TypeIndex from different type") {
      constexpr auto id = TypeId::From<Foo>();
      constexpr auto idx = TypeIndex::From<Bar>();

      CHECK_FALSE(id == idx);
      CHECK_FALSE(idx == id);
    }
  }

  TEST_CASE("operator<=>(TypeId, TypeIndex)") {
    SUBCASE(
        "Ordering between TypeId and TypeIndex from the same type is equal") {
      constexpr auto id = TypeId::From<Foo>();
      constexpr auto idx = TypeIndex::From<Foo>();

      CHECK_EQ((id <=> idx), std::strong_ordering::equal);
      CHECK_EQ((idx <=> id), std::strong_ordering::equal);
    }
  }
}

TEST_SUITE("helios::utils::TypeNameOf") {
  TEST_CASE("utils::TypeNameOf (template)") {
    SUBCASE("Returns unqualified name for simple type") {
      constexpr std::string_view name = TypeNameOf<Foo>();
      CHECK_EQ(name, "Foo");
    }

    SUBCASE("Returns unqualified name for nested type") {
      constexpr std::string_view name = TypeNameOf<outer::inner::Qux>();
      CHECK_EQ(name, "Qux");
    }

    SUBCASE("Returns unqualified name for outer namespace type") {
      constexpr std::string_view name = TypeNameOf<outer::Baz>();
      CHECK_EQ(name, "Baz");
    }
  }

  TEST_CASE("utils::TypeNameOf (instance overload)") {
    SUBCASE("Returns unqualified name from instance") {
      constexpr std::string_view name = TypeNameOf(Foo{});
      CHECK_EQ(name, "Foo");
    }
  }
}

TEST_SUITE("helios::utils::QualifiedTypeNameOf") {
  TEST_CASE("utils::QualifiedTypeNameOf (template)") {
    SUBCASE("Contains type name for simple type") {
      constexpr std::string_view name = QualifiedTypeNameOf<Foo>();
      CHECK_NE(name.find("Foo"), std::string_view::npos);
    }

    SUBCASE("Contains full qualifier for nested type") {
      constexpr std::string_view name =
          QualifiedTypeNameOf<outer::inner::Qux>();

      CHECK_NE(name.find("outer"), std::string_view::npos);
      CHECK_NE(name.find("inner"), std::string_view::npos);
      CHECK_NE(name.find("Qux"), std::string_view::npos);
    }

    SUBCASE("Qualified name is longer than or equal to unqualified name") {
      constexpr std::string_view qualified =
          QualifiedTypeNameOf<outer::inner::Qux>();
      constexpr std::string_view unqualified = TypeNameOf<outer::inner::Qux>();
      CHECK_GE(qualified.size(), unqualified.size());
    }
  }

  TEST_CASE("utils::QualifiedTypeNameOf (instance overload)") {
    SUBCASE("Returns qualified name from instance") {
      constexpr std::string_view name = QualifiedTypeNameOf(outer::Baz{});
      CHECK_NE(name.find("Baz"), std::string_view::npos);
      CHECK_NE(name.find("outer"), std::string_view::npos);
    }
  }
}

TEST_SUITE("std::hash specializations") {
  TEST_CASE("std::hash<TypeId>") {
    SUBCASE("Hash of TypeId equals TypeIndex hash") {
      const TypeId id = TypeId::From<Foo>();
      const size_t hash = std::hash<TypeId>{}(id);
      CHECK_EQ(hash, id.Index().Hash());
    }

    SUBCASE("Same type produces same hash") {
      const size_t hash1 = std::hash<TypeId>{}(TypeId::From<Foo>());
      const size_t hash2 = std::hash<TypeId>{}(TypeId::From<Foo>());
      CHECK_EQ(hash1, hash2);
    }

    SUBCASE("Different types produce different hashes") {
      const size_t hash1 = std::hash<TypeId>{}(TypeId::From<Foo>());
      const size_t hash2 = std::hash<TypeId>{}(TypeId::From<Bar>());
      CHECK_NE(hash1, hash2);
    }
  }

  TEST_CASE("std::hash<TypeIndex>") {
    SUBCASE("Hash of TypeIndex equals TypeIndex::Hash()") {
      const TypeIndex idx = TypeIndex::From<Foo>();
      const size_t hash = std::hash<TypeIndex>{}(idx);
      CHECK_EQ(hash, idx.Hash());
    }

    SUBCASE("Same type produces same hash") {
      const size_t hash1 = std::hash<TypeIndex>{}(TypeIndex::From<Foo>());
      const size_t hash2 = std::hash<TypeIndex>{}(TypeIndex::From<Foo>());
      CHECK_EQ(hash1, hash2);
    }

    SUBCASE("Different types produce different hashes") {
      const size_t hash1 = std::hash<TypeIndex>{}(TypeIndex::From<Foo>());
      const size_t hash2 = std::hash<TypeIndex>{}(TypeIndex::From<Bar>());
      CHECK_NE(hash1, hash2);
    }
  }
}
