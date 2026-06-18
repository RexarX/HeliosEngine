#include <doctest/doctest.h>

#include <helios/ecs/component/component.hpp>

#include <string>
#include <string_view>

using namespace helios::ecs;

namespace {

struct Position {
  static constexpr std::string_view kName = "Position";

  float x = 0.0F;
  float y = 0.0F;
};

struct Velocity {
  float x = 0.0F;
  float y = 0.0F;
};

struct Tag {};

struct NamedTag {
  static constexpr std::string_view kName = "NamedTag";
};

struct ArchetypeTag {
  static constexpr auto kStorageType = ComponentStorageType::kArchetype;
};

struct Sparse {
  static constexpr auto kStorageType = ComponentStorageType::kSparseSet;

  int value = 0;
};

struct NamedSparse {
  static constexpr std::string_view kName = "NamedSparse";
  static constexpr auto kStorageType = ComponentStorageType::kSparseSet;

  double data = 0.0;
};

struct ArchetypeComponent {
  static constexpr auto kStorageType = ComponentStorageType::kArchetype;

  int value = 0;
};

struct NamedArchetypeComponent {
  static constexpr std::string_view kName = "NamedArchetypeComponent";
  static constexpr auto kStorageType = ComponentStorageType::kArchetype;

  double data = 0.0;
};

struct LargeComponent {
  double data[100] = {};
};

struct AlignedComponent {
  alignas(16) float data[4] = {};
};

class PolymorphicBase {
public:
  virtual ~PolymorphicBase() = default;
};

struct NonCopyableNonMovable {
  NonCopyableNonMovable() = default;
  NonCopyableNonMovable(const NonCopyableNonMovable&) = delete;
  NonCopyableNonMovable(NonCopyableNonMovable&&) = delete;

  NonCopyableNonMovable& operator=(const NonCopyableNonMovable&) = delete;
  NonCopyableNonMovable& operator=(NonCopyableNonMovable&&) = delete;
};

}  // namespace

TEST_SUITE("helios::ecs::ComponentTrait") {
  TEST_CASE("ecs::ComponentTrait::concept") {
    SUBCASE("Regular component types satisfy ComponentTrait") {
      CHECK(ComponentTrait<Position>);
      CHECK(ComponentTrait<Velocity>);
      CHECK(ComponentTrait<Tag>);
      CHECK(ComponentTrait<Sparse>);
      CHECK(ComponentTrait<LargeComponent>);
      CHECK(ComponentTrait<AlignedComponent>);
    }

    SUBCASE("Fundamental types satisfy ComponentTrait") {
      CHECK(ComponentTrait<int>);
      CHECK(ComponentTrait<float>);
      CHECK(ComponentTrait<double>);
      CHECK(ComponentTrait<std::string>);
    }

    SUBCASE("Polymorphic types do not satisfy ComponentTrait") {
      CHECK_FALSE(ComponentTrait<PolymorphicBase>);
    }

    SUBCASE(
        "Non-copyable and non-movable types do not satisfy ComponentTrait") {
      CHECK_FALSE(ComponentTrait<NonCopyableNonMovable>);
    }
  }
}

TEST_SUITE("helios::ecs::TagComponentTrait") {
  TEST_CASE("ecs::TagComponentTrait::concept") {
    SUBCASE("Empty types satisfy TagComponentTrait") {
      CHECK(TagComponentTrait<Tag>);
      CHECK(TagComponentTrait<NamedTag>);
    }

    SUBCASE("Non-empty types do not satisfy TagComponentTrait") {
      CHECK_FALSE(TagComponentTrait<Position>);
      CHECK_FALSE(TagComponentTrait<Velocity>);
      CHECK_FALSE(TagComponentTrait<Sparse>);
    }

    SUBCASE("Tag components also satisfy ComponentTrait") {
      CHECK(ComponentTrait<Tag>);
      CHECK(TagComponentTrait<Tag>);
    }
  }
}

TEST_SUITE("helios::ecs::ArchetypeComponentTrait") {
  TEST_CASE("ecs::ArchetypeComponentTrait::concept") {
    SUBCASE(
        "Archetype storage is used by default, unless sparse storage is "
        "specified or type is tag") {
      CHECK(ArchetypeComponentTrait<Position>);
      CHECK(ArchetypeComponentTrait<Velocity>);
      CHECK(ArchetypeComponentTrait<LargeComponent>);

      CHECK_FALSE(ArchetypeComponentTrait<Sparse>);
      CHECK_FALSE(ArchetypeComponentTrait<NamedSparse>);
      CHECK_FALSE(ArchetypeComponentTrait<Tag>);
    }

    SUBCASE(
        "Components with explicit archetype storage satisfy "
        "ArchetypeComponentTrait") {
      CHECK(ArchetypeComponentTrait<ArchetypeTag>);
      CHECK(ArchetypeComponentTrait<ArchetypeComponent>);
      CHECK(ArchetypeComponentTrait<NamedArchetypeComponent>);
    }
  }
}

TEST_SUITE("helios::ecs::SparseComponentTrait") {
  TEST_CASE("ecs::SparseComponentTrait::concept") {
    SUBCASE(
        "Sparse storage used by default for tag components, other must "
        "explicitly specify sparse storage") {
      CHECK(SparseComponentTrait<Sparse>);
      CHECK(SparseComponentTrait<NamedSparse>);
      CHECK(SparseComponentTrait<Tag>);

      CHECK_FALSE(SparseComponentTrait<Position>);
      CHECK_FALSE(SparseComponentTrait<Velocity>);
      CHECK_FALSE(SparseComponentTrait<LargeComponent>);
    }
  }
}

TEST_SUITE("helios::ecs::ComponentWithNameTrait") {
  TEST_CASE("ecs::ComponentWithNameTrait::concept") {
    SUBCASE("Components with kName satisfy ComponentWithNameTrait") {
      CHECK(ComponentWithNameTrait<Position>);
      CHECK(ComponentWithNameTrait<NamedTag>);
      CHECK(ComponentWithNameTrait<NamedSparse>);
    }

    SUBCASE("Components without kName do not satisfy ComponentWithNameTrait") {
      CHECK_FALSE(ComponentWithNameTrait<Velocity>);
      CHECK_FALSE(ComponentWithNameTrait<Tag>);
      CHECK_FALSE(ComponentWithNameTrait<Sparse>);
    }
  }
}

TEST_SUITE("helios::ecs::ComponentWithStorageTypeTrait") {
  TEST_CASE("ecs::ComponentWithStorageTypeTrait::concept") {
    SUBCASE(
        "Components with kStorageType satisfy ComponentWithStorageTypeTrait") {
      CHECK(ComponentWithStorageTypeTrait<Sparse>);
      CHECK(ComponentWithStorageTypeTrait<NamedSparse>);
    }

    SUBCASE(
        "Components without kStorageType do not satisfy "
        "ComponentWithStorageTypeTrait") {
      CHECK_FALSE(ComponentWithStorageTypeTrait<Position>);
      CHECK_FALSE(ComponentWithStorageTypeTrait<Velocity>);
      CHECK_FALSE(ComponentWithStorageTypeTrait<Tag>);
    }
  }
}

TEST_SUITE("helios::ecs::ComponentNameOf") {
  TEST_CASE("ecs::ComponentNameOf::basic") {
    SUBCASE("Component with custom name returns custom name") {
      constexpr auto name = ComponentNameOf<Position>();
      CHECK_EQ(name, "Position");
    }

    SUBCASE("Component with custom name (NamedTag)") {
      constexpr auto name = ComponentNameOf<NamedTag>();
      CHECK_EQ(name, "NamedTag");
    }

    SUBCASE("Component with custom name (NamedSparse)") {
      constexpr auto name = ComponentNameOf<NamedSparse>();
      CHECK_EQ(name, "NamedSparse");
    }

    SUBCASE("Component without custom name returns type name") {
      constexpr auto name = ComponentNameOf<Velocity>();
      CHECK_FALSE(name.empty());
    }

    SUBCASE("Component name with instance parameter") {
      constexpr Position pos{};
      constexpr auto name_from_type = ComponentNameOf<Position>();
      constexpr auto name_from_instance = ComponentNameOf(pos);

      CHECK_EQ(name_from_type, name_from_instance);
    }

    SUBCASE("Tag without custom name returns type name") {
      constexpr auto name = ComponentNameOf<Tag>();
      CHECK_FALSE(name.empty());
    }
  }
}

TEST_SUITE("helios::ecs::ComponentStorageTypeOf") {
  TEST_CASE("ecs::ComponentStorageTypeOf::basic") {
    SUBCASE("Component with custom storage type returns custom type") {
      constexpr auto storage = ComponentStorageTypeOf<Sparse>();
      CHECK_EQ(storage, ComponentStorageType::kSparseSet);
    }

    SUBCASE("Regular component defaults to archetype") {
      constexpr auto storage = ComponentStorageTypeOf<Position>();
      CHECK_EQ(storage, ComponentStorageType::kArchetype);
    }

    SUBCASE("Tag component type defaults to sparse set") {
      constexpr auto storage = ComponentStorageTypeOf<Tag>();
      CHECK_EQ(storage, ComponentStorageType::kSparseSet);
    }

    SUBCASE("Tag component with custom storage type") {
      constexpr auto storage = ComponentStorageTypeOf<ArchetypeTag>();
      CHECK_EQ(storage, ComponentStorageType::kArchetype);
    }

    SUBCASE("Storage type with instance parameter") {
      constexpr Sparse sparse{};
      constexpr auto storage_from_type = ComponentStorageTypeOf<Sparse>();
      constexpr auto storage_from_instance = ComponentStorageTypeOf(sparse);

      CHECK_EQ(storage_from_type, storage_from_instance);
    }
  }
}

TEST_SUITE("helios::ecs::ComponentTypeInfo") {
  TEST_CASE("ecs::ComponentTypeInfo::From") {
    SUBCASE("From regular component type") {
      constexpr auto info = ComponentTypeInfo::From<Position>();

      CHECK_EQ(info.Size(), sizeof(Position));
      CHECK_EQ(info.Alignment(), alignof(Position));
      CHECK_EQ(info.StorageType(), ComponentStorageType::kArchetype);
      CHECK_FALSE(info.IsTag());
    }

    SUBCASE("From tag component type") {
      constexpr auto info = ComponentTypeInfo::From<Tag>();

      CHECK_EQ(info.Size(), sizeof(Tag));
      CHECK_EQ(info.Alignment(), alignof(Tag));
      CHECK_EQ(info.StorageType(), ComponentStorageType::kSparseSet);
      CHECK(info.IsTag());
    }

    SUBCASE("From sparse storage component type") {
      constexpr auto info = ComponentTypeInfo::From<Sparse>();

      CHECK_EQ(info.Size(), sizeof(Sparse));
      CHECK_EQ(info.Alignment(), alignof(Sparse));
      CHECK_EQ(info.StorageType(), ComponentStorageType::kSparseSet);
      CHECK_FALSE(info.IsTag());
    }

    SUBCASE("From component with custom name") {
      constexpr auto info = ComponentTypeInfo::From<Position>();
      CHECK_EQ(info.Size(), sizeof(Position));
      CHECK_EQ(info.Alignment(), alignof(Position));
    }

    SUBCASE("From large component") {
      constexpr auto info = ComponentTypeInfo::From<LargeComponent>();

      CHECK_EQ(info.Size(), sizeof(LargeComponent));
      CHECK_EQ(info.Alignment(), alignof(LargeComponent));
      CHECK_FALSE(info.IsTag());
    }

    SUBCASE("From aligned component") {
      constexpr auto info = ComponentTypeInfo::From<AlignedComponent>();

      CHECK_EQ(info.Size(), sizeof(AlignedComponent));
      CHECK_EQ(info.Alignment(), alignof(AlignedComponent));
      CHECK_FALSE(info.IsTag());
    }

    SUBCASE("From fundamental type") {
      constexpr auto info = ComponentTypeInfo::From<int>();

      CHECK_EQ(info.Size(), sizeof(int));
      CHECK_EQ(info.Alignment(), alignof(int));
      CHECK_EQ(info.StorageType(), ComponentStorageType::kArchetype);
      CHECK_FALSE(info.IsTag());
    }
  }

  TEST_CASE("ecs::ComponentTypeInfo::ctors") {
    SUBCASE("Copy ctor") {
      constexpr auto original = ComponentTypeInfo::From<Position>();
      constexpr auto copy = original;

      CHECK_EQ(copy, original);
      CHECK_EQ(copy.Size(), original.Size());
      CHECK_EQ(copy.Alignment(), original.Alignment());
      CHECK_EQ(copy.StorageType(), original.StorageType());
      CHECK_EQ(copy.IsTag(), original.IsTag());
    }

    SUBCASE("Move ctor") {
      auto original = ComponentTypeInfo::From<Position>();
      const auto original_size = original.Size();
      const auto original_alignment = original.Alignment();

      const auto moved = std::move(original);

      CHECK_EQ(moved.Size(), original_size);
      CHECK_EQ(moved.Alignment(), original_alignment);
    }
  }

  TEST_CASE("ecs::ComponentTypeInfo::assignment") {
    SUBCASE("Copy assignment") {
      constexpr auto original = ComponentTypeInfo::From<Position>();
      auto assigned = ComponentTypeInfo::From<Velocity>();

      assigned = original;

      CHECK_EQ(assigned, original);
      CHECK_EQ(assigned.Size(), original.Size());
      CHECK_EQ(assigned.Alignment(), original.Alignment());
    }

    SUBCASE("Move assignment") {
      auto original = ComponentTypeInfo::From<Position>();
      const auto original_size = original.Size();
      auto assigned = ComponentTypeInfo::From<Velocity>();

      assigned = std::move(original);

      CHECK_EQ(assigned.Size(), original_size);
    }
  }

  TEST_CASE("ecs::ComponentTypeInfo::operator==") {
    SUBCASE("Same type") {
      constexpr auto info1 = ComponentTypeInfo::From<Position>();
      constexpr auto info2 = ComponentTypeInfo::From<Position>();
      CHECK(info1 == info2);
    }

    SUBCASE("Different types") {
      constexpr auto info1 = ComponentTypeInfo::From<Position>();
      constexpr auto info2 = ComponentTypeInfo::From<Velocity>();
      CHECK_FALSE(info1 == info2);
    }
  }

  TEST_CASE("ecs::ComponentTypeInfo::operator!=") {
    SUBCASE("Same type") {
      constexpr auto info1 = ComponentTypeInfo::From<Position>();
      constexpr auto info2 = ComponentTypeInfo::From<Position>();
      CHECK_FALSE(info1 != info2);
    }

    SUBCASE("Different types") {
      constexpr auto info1 = ComponentTypeInfo::From<Position>();
      constexpr auto info2 = ComponentTypeInfo::From<Velocity>();
      CHECK(info1 != info2);
    }
  }

  TEST_CASE("ecs::ComponentTypeInfo::operator<") {
    SUBCASE("Consistent ordering") {
      constexpr auto info1 = ComponentTypeInfo::From<Position>();
      constexpr auto info2 = ComponentTypeInfo::From<Velocity>();
      CHECK_EQ(info1 < info2, !(info2 < info1));
    }
  }

  TEST_CASE("ecs::ComponentTypeInfo::TypeIndex") {
    SUBCASE("TypeIndex matches ComponentTypeIndex") {
      constexpr auto info = ComponentTypeInfo::From<Position>();
      constexpr auto type_index = ComponentTypeIndex::From<Position>();
      CHECK_EQ(info.TypeIndex(), type_index);
    }

    SUBCASE("TypeIndex is unique for different types") {
      constexpr auto info1 = ComponentTypeInfo::From<Position>();
      constexpr auto info2 = ComponentTypeInfo::From<Velocity>();
      CHECK_NE(info1.TypeIndex(), info2.TypeIndex());
    }
  }

  TEST_CASE("ecs::ComponentTypeInfo::Size") {
    SUBCASE("Size matches sizeof for regular component") {
      constexpr auto info = ComponentTypeInfo::From<Position>();
      CHECK_EQ(info.Size(), sizeof(Position));
    }

    SUBCASE("Size matches sizeof for tag component") {
      constexpr auto info = ComponentTypeInfo::From<Tag>();
      CHECK_EQ(info.Size(), sizeof(Tag));
    }

    SUBCASE("Size matches sizeof for large component") {
      constexpr auto info = ComponentTypeInfo::From<LargeComponent>();
      CHECK_EQ(info.Size(), sizeof(LargeComponent));
    }

    SUBCASE("Size for fundamental type") {
      constexpr auto info = ComponentTypeInfo::From<int>();
      CHECK_EQ(info.Size(), sizeof(int));
    }
  }

  TEST_CASE("ecs::ComponentTypeInfo::Alignment") {
    SUBCASE("Alignment matches alignof for regular component") {
      constexpr auto info = ComponentTypeInfo::From<Position>();
      CHECK_EQ(info.Alignment(), alignof(Position));
    }

    SUBCASE("Alignment matches alignof for aligned component") {
      constexpr auto info = ComponentTypeInfo::From<AlignedComponent>();
      CHECK_EQ(info.Alignment(), alignof(AlignedComponent));
      CHECK_GE(info.Alignment(), 16);
    }

    SUBCASE("Alignment for fundamental type") {
      constexpr auto info = ComponentTypeInfo::From<int>();
      CHECK_EQ(info.Alignment(), alignof(int));
    }
  }

  TEST_CASE("ecs::ComponentTypeInfo::StorageType") {
    SUBCASE("StorageType for archetype component") {
      constexpr auto info = ComponentTypeInfo::From<Position>();
      CHECK_EQ(info.StorageType(), ComponentStorageType::kArchetype);
    }

    SUBCASE("StorageType for sparse set component") {
      constexpr auto info = ComponentTypeInfo::From<Sparse>();
      CHECK_EQ(info.StorageType(), ComponentStorageType::kSparseSet);
    }

    SUBCASE("StorageType for tag component defaults to archetype") {
      constexpr auto info = ComponentTypeInfo::From<Tag>();
      CHECK_EQ(info.StorageType(), ComponentStorageType::kSparseSet);
    }
  }

  TEST_CASE("ecs::ComponentTypeInfo::IsTag") {
    SUBCASE("IsTag returns true for tag component") {
      constexpr auto info = ComponentTypeInfo::From<Tag>();
      CHECK(info.IsTag());
    }

    SUBCASE("IsTag returns true for named tag component") {
      constexpr auto info = ComponentTypeInfo::From<NamedTag>();
      CHECK(info.IsTag());
    }

    SUBCASE("IsTag returns false for regular component") {
      constexpr auto info = ComponentTypeInfo::From<Position>();
      CHECK_FALSE(info.IsTag());
    }

    SUBCASE("IsTag returns false for sparse component") {
      constexpr auto info = ComponentTypeInfo::From<Sparse>();
      CHECK_FALSE(info.IsTag());
    }

    SUBCASE("IsTag returns false for large component") {
      constexpr auto info = ComponentTypeInfo::From<LargeComponent>();
      CHECK_FALSE(info.IsTag());
    }

    SUBCASE("IsTag returns false for fundamental type") {
      constexpr auto info = ComponentTypeInfo::From<int>();
      CHECK_FALSE(info.IsTag());
    }
  }
}

TEST_SUITE("std::hash<ecs::ComponentTypeInfo>") {
  TEST_CASE("std::hash<ecs::ComponentTypeInfo>::basic") {
    SUBCASE("std::hash produces same value as TypeIndex hash") {
      constexpr auto info = ComponentTypeInfo::From<Position>();
      CHECK_EQ(std::hash<ComponentTypeInfo>{}(info), info.TypeIndex().Hash());
    }

    SUBCASE("std::hash consistency") {
      constexpr auto info = ComponentTypeInfo::From<Position>();
      constexpr auto hash1 = std::hash<ComponentTypeInfo>{}(info);
      constexpr auto hash2 = std::hash<ComponentTypeInfo>{}(info);

      CHECK_EQ(hash1, hash2);
    }

    SUBCASE("std::hash uniqueness") {
      constexpr auto info1 = ComponentTypeInfo::From<Position>();
      constexpr auto info2 = ComponentTypeInfo::From<Velocity>();
      CHECK_NE(std::hash<ComponentTypeInfo>{}(info1),
               std::hash<ComponentTypeInfo>{}(info2));
    }
  }
}
